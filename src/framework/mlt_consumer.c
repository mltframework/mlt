/**
 * \file mlt_consumer.c
 * \brief abstraction for all consumer services
 * \see mlt_consumer_s
 *
 * Copyright (C) 2003-2015 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mlt_consumer.h"
#include "mlt_factory.h"
#include "mlt_producer.h"
#include "mlt_frame.h"
#include "mlt_profile.h"
#include "mlt_log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

/** Define this if you want an automatic deinterlace (if necessary) when the
 * consumer's producer is not running at normal speed.
 */
#undef DEINTERLACE_ON_NOT_NORMAL_SPEED

/** This is not the ideal place for this, but it is needed by VDPAU as well.
 */
pthread_mutex_t mlt_sdl_mutex = PTHREAD_MUTEX_INITIALIZER;

/** \brief private members of mlt_consumer */

typedef struct
{
	int real_time;
	int ahead;
	int preroll;
	mlt_image_format image_format;
	mlt_audio_format audio_format;
	mlt_deque queue;
	void *ahead_thread;
	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_cond;
	pthread_mutex_t put_mutex;
	pthread_cond_t put_cond;
	mlt_frame put;
	int put_active;
	mlt_event event_listener;
	mlt_position position;
	int is_purge;
	int aud_counter;
	double fps;
	int channels;
	int frequency;

	/* additional fields added for the parallel work queue */
	mlt_deque worker_threads;
	pthread_mutex_t done_mutex;
	pthread_cond_t done_cond;
	int consecutive_dropped;
	int consecutive_rendered;
	int process_head;
	int started;
	pthread_t *threads; /**< used to deallocate all threads */
}
consumer_private;

typedef void* ( *thread_function_t )( void* );

static void mlt_consumer_frame_render( mlt_listener listener, mlt_properties owner, mlt_service self, void **args );
static void mlt_consumer_frame_show( mlt_listener listener, mlt_properties owner, mlt_service self, void **args );
static void mlt_consumer_property_changed( mlt_properties owner, mlt_consumer self, char *name );
static void apply_profile_properties( mlt_consumer self, mlt_profile profile, mlt_properties properties );
static void on_consumer_frame_show( mlt_properties owner, mlt_consumer self, mlt_frame frame );
static void transmit_thread_create( mlt_listener listener, mlt_properties owner, mlt_service self, void **args );
static void mlt_thread_create( mlt_consumer self, thread_function_t function );
static void transmit_thread_join( mlt_listener listener, mlt_properties owner, mlt_service self, void **args );
static void mlt_thread_join( mlt_consumer self );
static void consumer_read_ahead_start( mlt_consumer self );

/** Initialize a consumer service.
 *
 * \public \memberof mlt_consumer_s
 * \param self the consumer to initialize
 * \param child a pointer to the object for the subclass
 * \param profile the \p mlt_profile_s to use (optional but recommended,
 * uses the environment variable MLT if self is NULL)
 * \return true if there was an error
 */

int mlt_consumer_init( mlt_consumer self, void *child, mlt_profile profile )
{
	int error = 0;
	memset( self, 0, sizeof( struct mlt_consumer_s ) );
	self->child = child;
	consumer_private *priv = self->local = calloc( 1, sizeof( consumer_private ) );

	error = mlt_service_init( &self->parent, self );
	if ( error == 0 )
	{
		// Get the properties from the service
		mlt_properties properties = MLT_SERVICE_PROPERTIES( &self->parent );

		// Apply profile to properties
		if ( profile == NULL )
		{
			// Normally the application creates the profile and controls its lifetime
			// This is the fallback exception handling
			profile = mlt_profile_init( NULL );
			mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
			mlt_properties_set_data( properties, "_profile", profile, 0, (mlt_destructor)mlt_profile_close, NULL );
		}
		apply_profile_properties( self, profile, properties );

		// Default rescaler for all consumers
		mlt_properties_set( properties, "rescale", "bilinear" );

		// Default read ahead buffer size
		mlt_properties_set_int( properties, "buffer", 25 );
		mlt_properties_set_int( properties, "drop_max", 5 );

		// Default audio frequency and channels
		mlt_properties_set_int( properties, "frequency", 48000 );
		mlt_properties_set_int( properties, "channels", 2 );

		// Default of all consumers is real time
		mlt_properties_set_int( properties, "real_time", 1 );

		// Default to environment test card
		mlt_properties_set( properties, "test_card", mlt_environment( "MLT_TEST_CARD" ) );

		// Hmm - default all consumers to yuv422 with s16 :-/
		priv->image_format = mlt_image_yuv422;
		priv->audio_format = mlt_audio_s16;

		mlt_events_register( properties, "consumer-frame-show", ( mlt_transmitter )mlt_consumer_frame_show );
		mlt_events_register( properties, "consumer-frame-render", ( mlt_transmitter )mlt_consumer_frame_render );
		mlt_events_register( properties, "consumer-thread-started", NULL );
		mlt_events_register( properties, "consumer-thread-stopped", NULL );
		mlt_events_register( properties, "consumer-stopping", NULL );
		mlt_events_register( properties, "consumer-stopped", NULL );
		mlt_events_register( properties, "consumer-thread-create", ( mlt_transmitter )transmit_thread_create );
		mlt_events_register( properties, "consumer-thread-join", ( mlt_transmitter )transmit_thread_join );
		mlt_events_listen( properties, self, "consumer-frame-show", ( mlt_listener )on_consumer_frame_show );

		// Register a property-changed listener to handle the profile property -
		// subsequent properties can override the profile
		priv->event_listener = mlt_events_listen( properties, self, "property-changed", ( mlt_listener )mlt_consumer_property_changed );

		// Create the push mutex and condition
		pthread_mutex_init( &priv->put_mutex, NULL );
		pthread_cond_init( &priv->put_cond, NULL );

	}
	return error;
}

/** Convert the profile into properties on the consumer.
 *
 * \private \memberof mlt_consumer_s
 * \param self a consumer
 * \param profile a profile
 * \param properties a properties list (typically, the consumer's)
 */

static void apply_profile_properties( mlt_consumer self, mlt_profile profile, mlt_properties properties )
{
	consumer_private *priv = self->local;
	mlt_event_block( priv->event_listener );
	mlt_properties_set_double( properties, "fps", mlt_profile_fps( profile ) );
	mlt_properties_set_int( properties, "frame_rate_num", profile->frame_rate_num );
	mlt_properties_set_int( properties, "frame_rate_den", profile->frame_rate_den );
	mlt_properties_set_int( properties, "width", profile->width );
	mlt_properties_set_int( properties, "height", profile->height );
	mlt_properties_set_int( properties, "progressive", profile->progressive );
	mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile )  );
	mlt_properties_set_int( properties, "sample_aspect_num", profile->sample_aspect_num );
	mlt_properties_set_int( properties, "sample_aspect_den", profile->sample_aspect_den );
	mlt_properties_set_double( properties, "display_ratio", mlt_profile_dar( profile )  );
	mlt_properties_set_int( properties, "display_aspect_num", profile->display_aspect_num );
	mlt_properties_set_int( properties, "display_aspect_den", profile->display_aspect_den );
	mlt_properties_set_int( properties, "colorspace", profile->colorspace );
	mlt_event_unblock( priv->event_listener );
}

/** The property-changed event listener
 *
 * \private \memberof mlt_consumer_s
 * \param owner the events object
 * \param self the consumer
 * \param name the name of the property that changed
 */

static void mlt_consumer_property_changed( mlt_properties owner, mlt_consumer self, char *name )
{
	if ( !strcmp( name, "mlt_profile" ) )
	{
		// Get the properies
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );

		// Get the current profile
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );

		// Load the new profile
		mlt_profile new_profile = mlt_profile_init( mlt_properties_get( properties, name ) );

		if ( new_profile )
		{
			// Copy the profile
			if ( profile != NULL )
			{
				free( profile->description );
				memcpy( profile, new_profile, sizeof( struct mlt_profile_s ) );
				profile->description = strdup( new_profile->description );
			}
			else
			{
				profile = new_profile;
			}

			// Apply to properties
			apply_profile_properties( self, profile, properties );
			mlt_profile_close( new_profile );
		}
 	}
	else if ( !strcmp( name, "frame_rate_num" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );
		if ( profile )
		{
			profile->frame_rate_num = mlt_properties_get_int( properties, "frame_rate_num" );
			mlt_properties_set_double( properties, "fps", mlt_profile_fps( profile ) );
		}
	}
	else if ( !strcmp( name, "frame_rate_den" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );
		if ( profile )
		{
			profile->frame_rate_den = mlt_properties_get_int( properties, "frame_rate_den" );
			mlt_properties_set_double( properties, "fps", mlt_profile_fps( profile ) );
		}
	}
	else if ( !strcmp( name, "width" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );
		if ( profile )
			profile->width = mlt_properties_get_int( properties, "width" );
	}
	else if ( !strcmp( name, "height" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );
		if ( profile )
			profile->height = mlt_properties_get_int( properties, "height" );
	}
	else if ( !strcmp( name, "progressive" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );
		if ( profile )
			profile->progressive = mlt_properties_get_int( properties, "progressive" );
	}
	else if ( !strcmp( name, "sample_aspect_num" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );
		if ( profile )
		{
			profile->sample_aspect_num = mlt_properties_get_int( properties, "sample_aspect_num" );
			mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile )  );
		}
	}
	else if ( !strcmp( name, "sample_aspect_den" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );
		if ( profile )
		{
			profile->sample_aspect_den = mlt_properties_get_int( properties, "sample_aspect_den" );
			mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile )  );
		}
	}
	else if ( !strcmp( name, "display_aspect_num" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );
		if ( profile )
		{
			profile->display_aspect_num = mlt_properties_get_int( properties, "display_aspect_num" );
			mlt_properties_set_double( properties, "display_ratio", mlt_profile_dar( profile )  );
		}
	}
	else if ( !strcmp( name, "display_aspect_den" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );
		if ( profile )
		{
			profile->display_aspect_den = mlt_properties_get_int( properties, "display_aspect_den" );
			mlt_properties_set_double( properties, "display_ratio", mlt_profile_dar( profile )  );
		}
	}
	else if ( !strcmp( name, "colorspace" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );
		if ( profile )
			profile->colorspace = mlt_properties_get_int( properties, "colorspace" );
	}
}

/** The transmitter for the consumer-frame-show event
 *
 * Invokes the listener.
 *
 * \private \memberof mlt_consumer_s
 * \param listener a function pointer that will be invoked
 * \param owner the events object that will be passed to \p listener
 * \param self a service that will be passed to \p listener
 * \param args an array of pointers - the first entry is passed as a frame to \p listener
 */

static void mlt_consumer_frame_show( mlt_listener listener, mlt_properties owner, mlt_service self, void **args )
{
	if ( listener != NULL )
		listener( owner, self, ( mlt_frame )args[ 0 ] );
}

/** The transmitter for the consumer-frame-render event
 *
 * Invokes the listener.
 *
 * \private \memberof mlt_consumer_s
 * \param listener a function pointer that will be invoked
 * \param owner the events object that will be passed to \p listener
 * \param self a service that will be passed to \p listener
 * \param args an array of pointers - the first entry is passed as a frame to \p listener
 */

static void mlt_consumer_frame_render( mlt_listener listener, mlt_properties owner, mlt_service self, void **args )
{
	if ( listener != NULL )
		listener( owner, self, ( mlt_frame )args[ 0 ] );
}

/** A listener on the consumer-frame-show event
 *
 * Saves the position of the frame shown.
 *
 * \private \memberof mlt_consumer_s
 * \param owner the events object
 * \param consumer the consumer on which this event occurred
 * \param frame the frame that was shown
 */

static void on_consumer_frame_show( mlt_properties owner, mlt_consumer consumer, mlt_frame frame )
{
	if ( frame )
		( ( consumer_private*) consumer->local )->position = mlt_frame_get_position( frame );
}

/** Create a new consumer.
 *
 * \public \memberof mlt_consumer_s
 * \param profile a profile (optional, but recommended)
 * \return a new consumer
 */

mlt_consumer mlt_consumer_new( mlt_profile profile )
{
	// Create the memory for the structure
	mlt_consumer self = malloc( sizeof( struct mlt_consumer_s ) );

	// Initialise it
	if ( self != NULL && mlt_consumer_init( self, NULL, profile ) == 0 )
	{
		// Return it
		return self;
	}
	else
	{
		free(self);
		return NULL;
	}
}

/** Get the parent service object.
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 * \return the parent service class
 * \see MLT_CONSUMER_SERVICE
 */

mlt_service mlt_consumer_service( mlt_consumer self )
{
	return self != NULL ? &self->parent : NULL;
}

/** Get the consumer properties.
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 * \return the consumer's properties list
 * \see MLT_CONSUMER_PROPERTIES
 */

mlt_properties mlt_consumer_properties( mlt_consumer self )
{
	return self != NULL ? MLT_SERVICE_PROPERTIES( &self->parent ) : NULL;
}

/** Connect the consumer to the producer.
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 * \param producer a producer
 * \return > 0 warning, == 0 success, < 0 serious error,
 *         1 = this service does not accept input,
 *         2 = the producer is invalid,
 *         3 = the producer is already registered with this consumer
 */

int mlt_consumer_connect( mlt_consumer self, mlt_service producer )
{
	return mlt_service_connect_producer( &self->parent, producer, 0 );
}

/** Set the audio format to use in the render thread.
 *
 * \private \memberof mlt_consumer_s
 * \param self a consumer
 */

static void set_audio_format( mlt_consumer self )
{
	// Get the audio format to use for rendering threads.
	consumer_private *priv = self->local;
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
	const char *format = mlt_properties_get( properties, "mlt_audio_format" );
	if ( format )
	{
		if ( !strcmp( format, "none" ) )
			priv->audio_format = mlt_audio_none;
		else if ( !strcmp( format, "s32" ) )
			priv->audio_format = mlt_audio_s32;
		else if ( !strcmp( format, "s32le" ) )
			priv->audio_format = mlt_audio_s32le;
		else if ( !strcmp( format, "float" ) )
			priv->audio_format = mlt_audio_float;
		else if ( !strcmp( format, "f32le" ) )
			priv->audio_format = mlt_audio_f32le;
		else if ( !strcmp( format, "u8" ) )
			priv->audio_format = mlt_audio_u8;
	}
}

/** Set the image format to use in render threads.
 *
 * \private \memberof mlt_consumer_s
 * \param self a consumer
 */

static void set_image_format( mlt_consumer self )
{
	// Get the image format to use for rendering threads.
	consumer_private *priv = self->local;
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
	const char* format = mlt_properties_get( properties, "mlt_image_format" );
	if ( format )
	{
		priv->image_format = mlt_image_format_id( format );
		if ( mlt_image_invalid == priv->image_format )
			priv->image_format = mlt_image_yuv422;
		// mlt_image_glsl is for internal use only.
		// Remapping it glsl_texture prevents breaking existing apps
		// using the legacy "glsl" name.
		else if ( mlt_image_glsl == priv->image_format )
			priv->image_format = mlt_image_glsl_texture;
	}
}

/** Start the consumer.
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 * \return true if there was an error
 */

int mlt_consumer_start( mlt_consumer self )
{
	int error = 0;

	if ( !mlt_consumer_is_stopped( self ) )
		return error;

	consumer_private *priv = self->local;

	// Stop listening to the property-changed event
	mlt_event_block( priv->event_listener );

	// Get the properies
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );

	// Determine if there's a test card producer
	char *test_card = mlt_properties_get( properties, "test_card" );

	// Just to make sure nothing is hanging around...
	pthread_mutex_lock( &priv->put_mutex );
	priv->put = NULL;
	priv->put_active = 1;
	pthread_mutex_unlock( &priv->put_mutex );

	// Deal with it now.
	if ( test_card != NULL )
	{
		if ( mlt_properties_get_data( properties, "test_card_producer", NULL ) == NULL )
		{
			// Create a test card producer
			mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( self ) );
			mlt_producer producer = mlt_factory_producer( profile, NULL, test_card );

			// Do we have a producer
			if ( producer != NULL )
			{
				// Test card should loop I guess...
				mlt_properties_set( MLT_PRODUCER_PROPERTIES( producer ), "eof", "loop" );
				//mlt_producer_set_speed( producer, 0 );
				//mlt_producer_set_in_and_out( producer, 0, 0 );

				// Set the test card on the consumer
				mlt_properties_set_data( properties, "test_card_producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );
			}
		}
	}
	else
	{
		// Allow the hash table to speed things up
		mlt_properties_set_data( properties, "test_card_producer", NULL, 0, NULL, NULL );
	}

	// The profile could have changed between a stop and a restart.
	apply_profile_properties( self, mlt_service_profile( MLT_CONSUMER_SERVICE(self) ), properties );

	// Set the frame duration in microseconds for the frame-dropping heuristic
	int frame_rate_num = mlt_properties_get_int( properties, "frame_rate_num" );
	int frame_rate_den = mlt_properties_get_int( properties, "frame_rate_den" );
	int frame_duration = 0;

	if ( frame_rate_num && frame_rate_den )
	{
		frame_duration = 1000000 / frame_rate_num * frame_rate_den;
	}

	mlt_properties_set_int( properties, "frame_duration", frame_duration );
	mlt_properties_set_int( properties, "drop_count", 0 );

	// Check and run an ante command
	if ( mlt_properties_get( properties, "ante" ) )
		if ( system( mlt_properties_get( properties, "ante" ) ) == -1 )
			mlt_log( MLT_CONSUMER_SERVICE( self ), MLT_LOG_ERROR, "system(%s) failed!\n", mlt_properties_get( properties, "ante" ) );

	// Set the real_time preference
	priv->real_time = mlt_properties_get_int( properties, "real_time" );

	// For worker threads implementation, buffer must be at least # threads
	if ( abs( priv->real_time ) > 1 && mlt_properties_get_int( properties, "buffer" ) <= abs( priv->real_time ) )
		mlt_properties_set_int( properties, "_buffer", abs( priv->real_time ) + 1 );

	priv->preroll = 1;
#ifdef _WIN32
	if ( priv->real_time == 1 || priv->real_time == -1 )
		consumer_read_ahead_start( self );
#endif

	// Start the service
	if ( self->start != NULL )
		error = self->start( self );

	// Store the parameters for audio processing.
	priv->aud_counter = 0;
	priv->fps = mlt_properties_get_double( properties, "fps" );
	priv->channels = mlt_properties_get_int( properties, "channels" );
	priv->frequency = mlt_properties_get_int( properties, "frequency" );

	return error;
}

/** An alternative method to feed frames into the consumer.
 *
 * Only valid if the consumer itself is not connected.
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 * \param frame a frame
 * \return true (ignore self for now)
 */

int mlt_consumer_put_frame( mlt_consumer self, mlt_frame frame )
{
	int error = 1;

	// Get the service assoicated to the consumer
	mlt_service service = MLT_CONSUMER_SERVICE( self );

	if ( mlt_service_producer( service ) == NULL )
	{
		struct timeval now;
		struct timespec tm;
		consumer_private *priv = self->local;

		mlt_properties_set_int( MLT_CONSUMER_PROPERTIES(self), "put_pending", 1 );
		pthread_mutex_lock( &priv->put_mutex );
		while ( priv->put_active && priv->put != NULL )
		{
			gettimeofday( &now, NULL );
			tm.tv_sec = now.tv_sec + 1;
			tm.tv_nsec = now.tv_usec * 1000;
			pthread_cond_timedwait( &priv->put_cond, &priv->put_mutex, &tm );
		}
		mlt_properties_set_int( MLT_CONSUMER_PROPERTIES(self), "put_pending", 0 );
		if ( priv->put_active && priv->put == NULL )
			priv->put = frame;
		else
			mlt_frame_close( frame );
		pthread_cond_broadcast( &priv->put_cond );
		pthread_mutex_unlock( &priv->put_mutex );
	}
	else
	{
		mlt_frame_close( frame );
	}

	return error;
}

/** Protected method for consumer to get frames from connected service
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 * \return a frame
 */

mlt_frame mlt_consumer_get_frame( mlt_consumer self )
{
	// Frame to return
	mlt_frame frame = NULL;

	// Get the service assoicated to the consumer
	mlt_service service = MLT_CONSUMER_SERVICE( self );

	// Get the consumer properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );

	// Get the frame
	if ( mlt_service_producer( service ) == NULL && mlt_properties_get_int( properties, "put_mode" ) )
	{
		struct timeval now;
		struct timespec tm;
		consumer_private *priv = self->local;

		pthread_mutex_lock( &priv->put_mutex );
		while ( priv->put_active && priv->put == NULL )
		{
			gettimeofday( &now, NULL );
			tm.tv_sec = now.tv_sec + 1;
			tm.tv_nsec = now.tv_usec * 1000;
			pthread_cond_timedwait( &priv->put_cond, &priv->put_mutex, &tm );
		}
		frame = priv->put;
		priv->put = NULL;
		pthread_cond_broadcast( &priv->put_cond );
		pthread_mutex_unlock( &priv->put_mutex );
		if ( frame != NULL )
			mlt_service_apply_filters( service, frame, 0 );
	}
	else if ( mlt_service_producer( service ) != NULL )
	{
		mlt_service_get_frame( service, &frame, 0 );
	}
	else
	{
		frame = mlt_frame_init( service );
	}

	if ( frame != NULL )
	{
		// Get the frame properties
		mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

		// Get the test card producer
		mlt_producer test_card = mlt_properties_get_data( properties, "test_card_producer", NULL );

		// Attach the test frame producer to it.
		if ( test_card != NULL )
			mlt_properties_set_data( frame_properties, "test_card_producer", test_card, 0, NULL, NULL );

		// Pass along the interpolation and deinterlace options
		// TODO: get rid of consumer_deinterlace and use profile.progressive
		mlt_properties_set( frame_properties, "rescale.interp", mlt_properties_get( properties, "rescale" ) );
		mlt_properties_set_int( frame_properties, "consumer_deinterlace", mlt_properties_get_int( properties, "progressive" ) | mlt_properties_get_int( properties, "deinterlace" ) );
		mlt_properties_set( frame_properties, "deinterlace_method", mlt_properties_get( properties, "deinterlace_method" ) );
		mlt_properties_set_int( frame_properties, "consumer_tff", mlt_properties_get_int( properties, "top_field_first" ) );
		mlt_properties_set( frame_properties, "consumer_color_trc", mlt_properties_get( properties, "color_trc" ) );
	}

	// Return the frame
	return frame;
}

/** Compute the time difference between now and a time value.
 *
 * \private \memberof mlt_consumer_s
 * \param time1 a time value to be compared against now
 * \return the difference in microseconds
 */

static inline long time_difference( struct timeval *time1 )
{
	struct timeval time2;
	time2.tv_sec = time1->tv_sec;
	time2.tv_usec = time1->tv_usec;
	gettimeofday( time1, NULL );
	return time1->tv_sec * 1000000 + time1->tv_usec - time2.tv_sec * 1000000 - time2.tv_usec;
}

/** The thread procedure for asynchronously pulling frames through the service
 * network connected to a consumer.
 *
 * \private \memberof mlt_consumer_s
 * \param arg a consumer
 */

static void *consumer_read_ahead_thread( void *arg )
{
	// The argument is the consumer
	mlt_consumer self = arg;
	consumer_private *priv = self->local;

	// Get the properties of the consumer
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );

	// Get the width and height
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );

	// See if video is turned off
	int video_off = mlt_properties_get_int( properties, "video_off" );
	int preview_off = mlt_properties_get_int( properties, "preview_off" );
	int preview_format = mlt_properties_get_int( properties, "preview_format" );

	// Audio processing variables
	int samples = 0;
	void *audio = NULL;

	// See if audio is turned off
	int audio_off = mlt_properties_get_int( properties, "audio_off" );

	// Get the maximum size of the buffer
	int buffer = mlt_properties_get_int( properties, "buffer" ) + 1;

	// General frame variable
	mlt_frame frame = NULL;
	uint8_t *image = NULL;

	// Time structures
	struct timeval ante;

	// Average time for get_frame and get_image
	int count = 0;
	int skipped = 0;
	int64_t time_process = 0;
	int skip_next = 0;
	mlt_position pos = 0;
	mlt_position start_pos = 0;
	mlt_position last_pos = 0;
	int frame_duration = mlt_properties_get_int( properties, "frame_duration" );
	int drop_max = mlt_properties_get_int( properties, "drop_max" );

	if ( preview_off && preview_format != 0 )
		priv->image_format = preview_format;

	set_audio_format( self );
	set_image_format( self );

	mlt_events_fire( properties, "consumer-thread-started", NULL );

	// Get the first frame
	frame = mlt_consumer_get_frame( self );

	if ( frame )
	{
		// Get the audio of the first frame
		if ( !audio_off )
		{
			samples = mlt_sample_calculator( priv->fps, priv->frequency, priv->aud_counter++ );
			mlt_frame_get_audio( frame, &audio, &priv->audio_format, &priv->frequency, &priv->channels, &samples );
		}

		// Get the image of the first frame
		if ( !video_off )
		{
			mlt_events_fire( MLT_CONSUMER_PROPERTIES( self ), "consumer-frame-render", frame, NULL );
			mlt_frame_get_image( frame, &image, &priv->image_format, &width, &height, 0 );
		}

		// Mark as rendered
		mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "rendered", 1 );
		last_pos = start_pos = pos = mlt_frame_get_position( frame );
	}

	// Get the starting time (can ignore the times above)
	gettimeofday( &ante, NULL );

	// Continue to read ahead
	while ( priv->ahead )
	{
		// Put the current frame into the queue
		pthread_mutex_lock( &priv->queue_mutex );
		while( priv->ahead && mlt_deque_count( priv->queue ) >= buffer )
			pthread_cond_wait( &priv->queue_cond, &priv->queue_mutex );
		if ( priv->is_purge )
		{
			mlt_frame_close( frame );
			priv->is_purge = 0;
		}
		else
		{
			mlt_deque_push_back( priv->queue, frame );
		}
		pthread_cond_broadcast( &priv->queue_cond );
		pthread_mutex_unlock( &priv->queue_mutex );

		mlt_log_timings_begin();
		// Get the next frame
		frame = mlt_consumer_get_frame( self );
		mlt_log_timings_end( NULL, "mlt_consumer_get_frame" );

		// If there's no frame, we're probably stopped...
		if ( frame == NULL )
			continue;
		pos = mlt_frame_get_position( frame );

		// WebVfx uses this to setup a consumer-stopping event handler.
		mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "consumer", self, 0, NULL, NULL );

		// Increment the counter used for averaging processing cost
		count ++;

		// Always process audio
		if ( !audio_off )
		{
			samples = mlt_sample_calculator( priv->fps, priv->frequency, priv->aud_counter++ );
			mlt_frame_get_audio( frame, &audio, &priv->audio_format, &priv->frequency, &priv->channels, &samples );
		}

		// All non-normal playback frames should be shown
		if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "_speed" ) != 1 )
		{
#ifdef DEINTERLACE_ON_NOT_NORMAL_SPEED
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "consumer_deinterlace", 1 );
#endif
			// Indicate seeking or trick-play
			start_pos = pos;
		}

		// If skip flag not set or frame-dropping disabled
		if ( !skip_next || priv->real_time == -1 )
		{
			if ( !video_off )
			{
				// Reset width/height - could have been changed by previous mlt_frame_get_image
				width = mlt_properties_get_int( properties, "width" );
				height = mlt_properties_get_int( properties, "height" );

				// Get the image
				mlt_events_fire( MLT_CONSUMER_PROPERTIES( self ), "consumer-frame-render", frame, NULL );
				mlt_log_timings_begin();
				mlt_frame_get_image( frame, &image, &priv->image_format, &width, &height, 0 );
				mlt_log_timings_end( NULL, "mlt_frame_get_image" );
			}

			// Indicate the rendered image is available.
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "rendered", 1 );

			// Reset consecutively-skipped counter
			skipped = 0;
		}
		else // Skip image processing
		{
			// Increment the number of consecutively-skipped frames
			skipped++;

			// If too many (1 sec) consecutively-skipped frames
			if ( skipped > drop_max )
			{
				// Reset cost tracker
				time_process = 0;
				count = 1;
				mlt_log_verbose( self, "too many frames dropped - forcing next frame\n" );
			}
		}

		// Get the time to process this frame
		int64_t time_current = time_difference( &ante );

		// If the current time is not suddenly some large amount
		if ( time_current < time_process / count * 20 || !time_process || count < 5 )
		{
			// Accumulate the cost for processing this frame
			time_process += time_current;
		}
		else
		{
			mlt_log_debug( self, "current %"PRId64" threshold %"PRId64" count %d\n",
				time_current, (int64_t) (time_process / count * 20), count );
			// Ignore the cost of this frame's time
			count--;
		}

		// Determine if we started, resumed, or seeked
		if ( pos != last_pos + 1 )
			start_pos = pos;
		last_pos = pos;

		// Do not skip the first 20% of buffer at start, resume, or seek
		if ( pos - start_pos <= buffer / 5 + 1 )
		{
			// Reset cost tracker
			time_process = 0;
			count = 1;
		}

		// Reset skip flag
		skip_next = 0;

		// Only consider skipping if the buffer level is low (or really small)
		if ( mlt_deque_count( priv->queue ) <= buffer / 5 + 1 && count > 1 )
		{
			// Skip next frame if average cost exceeds frame duration.
			if ( time_process / count > frame_duration )
				skip_next = 1;
			if ( skip_next )
				mlt_log_debug( self, "avg usec %"PRId64" (%"PRId64"/%d) duration %d\n",
					time_process/count, time_process, count, frame_duration);
		}
	}

	// Remove the last frame
	mlt_frame_close( frame );

	// Wipe the queue
	pthread_mutex_lock( &priv->queue_mutex );
	while ( mlt_deque_count( priv->queue ) )
		mlt_frame_close( mlt_deque_pop_back( priv->queue ) );

	// Close the queue
	mlt_deque_close( priv->queue );
	priv->queue = NULL;
	pthread_mutex_unlock( &priv->queue_mutex );

	mlt_events_fire( MLT_CONSUMER_PROPERTIES(self), "consumer-thread-stopped", NULL );

	return NULL;
}

/** Locate the first unprocessed frame in the queue.
 *
 * When playing with realtime behavior, we do not use the true head, but
 * rather an adjusted process_head. The process_head is adjusted based on
 * the rate of frame-dropping or recovery from frame-dropping. The idea is
 * that as the level of frame-dropping increases to move the process_head
 * closer to the tail because the frames are not completing processing prior
 * to their playout! Then, as frames are not dropped the process_head moves
 * back closer to the head of the queue so that worker threads can work 
 * ahead of the playout point (queue head).
 *
 * \private \memberof mlt_consumer_s
 * \param self a consumer
 * \return an index into the queue
 */

static inline int first_unprocessed_frame( mlt_consumer self )
{
	consumer_private *priv = self->local;
	int index = priv->real_time <= 0 ? 0 : priv->process_head;
	while ( index < mlt_deque_count( priv->queue ) && MLT_FRAME( mlt_deque_peek( priv->queue, index ) )->is_processing )
		index++;
	return index;
}

/** The worker thread procedure for parallel processing frames.
 *
 * \private \memberof mlt_consumer_s
 * \param arg a consumer
 */

static void *consumer_worker_thread( void *arg )
{
	// The argument is the consumer
	mlt_consumer self = arg;
	consumer_private *priv = self->local;

	// Get the properties of the consumer
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );

	// Get the width and height
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );
	mlt_image_format format = priv->image_format;

	// See if video is turned off
	int video_off = mlt_properties_get_int( properties, "video_off" );
	int preview_off = mlt_properties_get_int( properties, "preview_off" );
	int preview_format = mlt_properties_get_int( properties, "preview_format" );

	// General frame variable
	mlt_frame frame = NULL;
	uint8_t *image = NULL;

	if ( preview_off && preview_format != 0 )
		format = preview_format;

	mlt_events_fire( properties, "consumer-thread-started", NULL );

	// Continue to read ahead
	while ( priv->ahead )
	{
		// Get the next unprocessed frame from the work queue
		pthread_mutex_lock( &priv->queue_mutex );
		int index = first_unprocessed_frame( self );
		while ( priv->ahead && index >= mlt_deque_count( priv->queue ) )
		{
			mlt_log_debug( MLT_CONSUMER_SERVICE(self), "waiting in worker index = %d queue count = %d\n",
				index, mlt_deque_count( priv->queue ) );
			pthread_cond_wait( &priv->queue_cond, &priv->queue_mutex );
			index = first_unprocessed_frame( self );
		}

		// Mark the frame for processing
		frame = mlt_deque_peek( priv->queue, index );
		if ( frame )
		{
			mlt_log_debug( MLT_CONSUMER_SERVICE(self), "worker processing index = %d frame " MLT_POSITION_FMT " queue count = %d\n",
				index, mlt_frame_get_position(frame), mlt_deque_count( priv->queue ) );
			frame->is_processing = 1;
			mlt_properties_inc_ref( MLT_FRAME_PROPERTIES( frame ) );
		}
		pthread_mutex_unlock( &priv->queue_mutex );

		// If there's no frame, we're probably stopped...
		if ( frame == NULL )
			continue;

		// WebVfx uses this to setup a consumer-stopping event handler.
		mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "consumer", self, 0, NULL, NULL );

#ifdef DEINTERLACE_ON_NOT_NORMAL_SPEED
		// All non normal playback frames should be shown
		if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "_speed" ) != 1 )
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "consumer_deinterlace", 1 );
#endif

		// Get the image
		if ( !video_off )
		{
			// Fetch width/height again
			width = mlt_properties_get_int( properties, "width" );
			height = mlt_properties_get_int( properties, "height" );
			mlt_events_fire( MLT_CONSUMER_PROPERTIES( self ), "consumer-frame-render", frame, NULL );
			mlt_frame_get_image( frame, &image, &format, &width, &height, 0 );
		}
		mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "rendered", 1 );
		mlt_frame_close( frame );

		// Tell a waiting thread (non-realtime main consumer thread) that we are done.
		pthread_mutex_lock( &priv->done_mutex );
		pthread_cond_broadcast( &priv->done_cond );
		pthread_mutex_unlock( &priv->done_mutex );
	}

	return NULL;
}

/** Start the read/render thread.
 *
 * \private \memberof mlt_consumer_s
 * \param self a consumer
 */

static void consumer_read_ahead_start( mlt_consumer self )
{
	consumer_private *priv = self->local;

	if ( priv->started )
		return;

	// We're running now
	priv->ahead = 1;

	// Create the frame queue
	priv->queue = mlt_deque_init( );

	// Create the queue mutex
	pthread_mutex_init( &priv->queue_mutex, NULL );

	// Create the condition
	pthread_cond_init( &priv->queue_cond, NULL );

	// Create the read ahead
	mlt_thread_create( self, (thread_function_t) consumer_read_ahead_thread );
	priv->started = 1;
}

/** Start the worker threads.
 *
 * \private \memberof mlt_consumer_s
 * \param self a consumer
 */

static void consumer_work_start( mlt_consumer self )
{
	consumer_private *priv = self->local;
	int n = abs( priv->real_time );
	pthread_t *thread;

	if ( priv->started )
		return;

	thread = calloc( 1, sizeof( pthread_t ) * n );

	// We're running now
	priv->ahead = 1;
	priv->threads = thread;
	
	// These keep track of the accelleration of frame dropping or recovery.
	priv->consecutive_dropped = 0;
	priv->consecutive_rendered = 0;
	
	// This is the position in the queue from which to look for a frame to process.
	// If we always start from the head, then we may likely not complete processing
	// before the frame is played out.
	priv->process_head = 0;

	// Create the queues
	priv->queue = mlt_deque_init();
	priv->worker_threads = mlt_deque_init();

	// Create the mutexes
	pthread_mutex_init( &priv->queue_mutex, NULL );
	pthread_mutex_init( &priv->done_mutex, NULL );

	// Create the conditions
	pthread_cond_init( &priv->queue_cond, NULL );
	pthread_cond_init( &priv->done_cond, NULL );

	// Create the read ahead
	if ( mlt_properties_get( MLT_CONSUMER_PROPERTIES( self ), "priority" ) )
	{

		struct sched_param priority;
		pthread_attr_t thread_attributes;

		priority.sched_priority = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( self ), "priority" );
		pthread_attr_init( &thread_attributes );
		pthread_attr_setschedpolicy( &thread_attributes, SCHED_OTHER );
		pthread_attr_setschedparam( &thread_attributes, &priority );
		pthread_attr_setinheritsched( &thread_attributes, PTHREAD_EXPLICIT_SCHED );
		pthread_attr_setscope( &thread_attributes, PTHREAD_SCOPE_SYSTEM );

		while ( n-- )
		{
			if ( pthread_create( thread, &thread_attributes, consumer_worker_thread, self ) < 0 )
				if ( pthread_create( thread, NULL, consumer_worker_thread, self ) == 0 )
					mlt_deque_push_back( priv->worker_threads, thread );
			thread++;
		}
		pthread_attr_destroy( &thread_attributes );
	}

	else
	{
		while ( n-- )
		{
			if ( pthread_create( thread, NULL, consumer_worker_thread, self ) == 0 )
				mlt_deque_push_back( priv->worker_threads, thread );
			thread++;
		}
	}
	priv->started = 1;
}

/** Stop the read/render thread.
 *
 * \private \memberof mlt_consumer_s
 * \param self a consumer
 */

static void consumer_read_ahead_stop( mlt_consumer self )
{
	consumer_private *priv = self->local;

	// Make sure we're running
// TODO improve support for atomic ops in general (see libavutil/atomic.h)
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
	if ( __sync_val_compare_and_swap( &priv->started, 1, 0 ) )
	{
#else
	if ( priv->started )
	{
		priv->started = 0;
#endif
		// Inform thread to stop
		priv->ahead = 0;
		mlt_events_fire( MLT_CONSUMER_PROPERTIES(self), "consumer-stopping", NULL );

		// Broadcast to the condition in case it's waiting
		pthread_mutex_lock( &priv->queue_mutex );
		pthread_cond_broadcast( &priv->queue_cond );
		pthread_mutex_unlock( &priv->queue_mutex );

		// Broadcast to the put condition in case it's waiting
		pthread_mutex_lock( &priv->put_mutex );
		pthread_cond_broadcast( &priv->put_cond );
		pthread_mutex_unlock( &priv->put_mutex );

		// Join the thread
		mlt_thread_join( self );

		// Destroy the frame queue mutex
		pthread_mutex_destroy( &priv->queue_mutex );

		// Destroy the condition
		pthread_cond_destroy( &priv->queue_cond );
	}
}

/** Stop the worker threads.
 *
 * \private \memberof mlt_consumer_s
 * \param self a consumer
 */

static void consumer_work_stop( mlt_consumer self )
{
	consumer_private *priv = self->local;

	// Make sure we're running
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
	if ( __sync_val_compare_and_swap( &priv->started, 1, 0 ) )
	{
#else
	if ( priv->started )
	{
		priv->started = 0;
#endif
		// Inform thread to stop
		priv->ahead = 0;
		mlt_events_fire( MLT_CONSUMER_PROPERTIES(self), "consumer-stopping", NULL );

		// Broadcast to the queue condition in case it's waiting
		pthread_mutex_lock( &priv->queue_mutex );
		pthread_cond_broadcast( &priv->queue_cond );
		pthread_mutex_unlock( &priv->queue_mutex );

		// Broadcast to the put condition in case it's waiting
		pthread_mutex_lock( &priv->put_mutex );
		pthread_cond_broadcast( &priv->put_cond );
		pthread_mutex_unlock( &priv->put_mutex );

		// Broadcast to the done condition in case it's waiting
		pthread_mutex_lock( &priv->done_mutex );
		pthread_cond_broadcast( &priv->done_cond );
		pthread_mutex_unlock( &priv->done_mutex );

		// Join the threads
		pthread_t *thread;
		while ( ( thread = mlt_deque_pop_back( priv->worker_threads ) ) )
			pthread_join( *thread, NULL );

		// Deallocate the array of threads
		free( priv->threads );

		// Destroy the mutexes
		pthread_mutex_destroy( &priv->queue_mutex );
		pthread_mutex_destroy( &priv->done_mutex );

		// Destroy the conditions
		pthread_cond_destroy( &priv->queue_cond );
		pthread_cond_destroy( &priv->done_cond );

		// Wipe the queues
		while ( mlt_deque_count( priv->queue ) )
			mlt_frame_close( mlt_deque_pop_back( priv->queue ) );

		// Close the queues
		mlt_deque_close( priv->queue );
		mlt_deque_close( priv->worker_threads );

		mlt_events_fire( MLT_CONSUMER_PROPERTIES(self), "consumer-thread-stopped", NULL );
	}
}

/** Flush the read/render thread's buffer.
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 */

void mlt_consumer_purge( mlt_consumer self )
{
	if ( self )
	{
		consumer_private *priv = self->local;

		pthread_mutex_lock( &priv->put_mutex );
		if ( priv->put ) {
			mlt_frame_close( priv->put );
			priv->put = NULL;
		}
		pthread_cond_broadcast( &priv->put_cond );
		pthread_mutex_unlock( &priv->put_mutex );

		if ( self->purge )
			self->purge( self );

		if ( priv->started && priv->real_time )
			pthread_mutex_lock( &priv->queue_mutex );

		while ( priv->started && mlt_deque_count( priv->queue ) )
			mlt_frame_close( mlt_deque_pop_back( priv->queue ) );

		if ( priv->started && priv->real_time )
		{
			priv->is_purge = 1;
			pthread_cond_broadcast( &priv->queue_cond );
			pthread_mutex_unlock( &priv->queue_mutex );
			if ( abs( priv->real_time ) > 1 )
			{
				pthread_mutex_lock( &priv->done_mutex );
				pthread_cond_broadcast( &priv->done_cond );
				pthread_mutex_unlock( &priv->done_mutex );
			}
		}

		pthread_mutex_lock( &priv->put_mutex );
		if ( priv->put ) {
			mlt_frame_close( priv->put );
			priv->put = NULL;
		}
		pthread_cond_broadcast( &priv->put_cond );
		pthread_mutex_unlock( &priv->put_mutex );
	}
}

/** Use multiple worker threads and a work queue.
 */

static mlt_frame worker_get_frame( mlt_consumer self, mlt_properties properties )
{
	// Frame to return
	mlt_frame frame = NULL;
	consumer_private *priv = self->local;
	int threads = abs( priv->real_time );
	int audio_off = mlt_properties_get_int( properties, "audio_off" );
	int samples = 0;
	void *audio = NULL;
	int buffer = mlt_properties_get_int( properties, "_buffer" );
	buffer = buffer > 0 ? buffer : mlt_properties_get_int( properties, "buffer" );
	// This is a heuristic to determine a suitable minimum buffer size for the number of threads.
	int headroom = 2 + threads * threads;
	buffer = buffer < headroom ? headroom : buffer;

	// Start worker threads if not already started.
	if ( ! priv->ahead )
	{
		int prefill = mlt_properties_get_int( properties, "prefill" );
		prefill = prefill > 0 && prefill < buffer ? prefill : buffer;

		set_audio_format( self );
		set_image_format( self );
		consumer_work_start( self );

		// Fill the work queue.
		int i = buffer;
		while ( priv->ahead && i-- )
		{
			frame = mlt_consumer_get_frame( self );
			if ( frame )
			{
				// Process the audio
				if ( !audio_off )
				{
					samples = mlt_sample_calculator( priv->fps, priv->frequency, priv->aud_counter++ );
					mlt_frame_get_audio( frame, &audio, &priv->audio_format, &priv->frequency, &priv->channels, &samples );
				}
				pthread_mutex_lock( &priv->queue_mutex );
				mlt_deque_push_back( priv->queue, frame );
				pthread_cond_signal( &priv->queue_cond );
				pthread_mutex_unlock( &priv->queue_mutex );
			}
		}

		// Wait for prefill
		while ( priv->ahead && first_unprocessed_frame( self ) < prefill )
		{
			pthread_mutex_lock( &priv->done_mutex );
			pthread_cond_wait( &priv->done_cond, &priv->done_mutex );
			pthread_mutex_unlock( &priv->done_mutex );
		}
		priv->process_head = threads;
	}

//	mlt_log_verbose( MLT_CONSUMER_SERVICE(self), "size %d done count %d work count %d process_head %d\n",
//		threads, first_unprocessed_frame( self ), mlt_deque_count( priv->queue ), priv->process_head );

	// Feed the work queue
	while ( priv->ahead && mlt_deque_count( priv->queue ) < buffer )
	{
		frame = mlt_consumer_get_frame( self );
		if ( frame )
		{
			// Process the audio
			if ( !audio_off )
			{
				samples = mlt_sample_calculator( priv->fps, priv->frequency, priv->aud_counter++ );
				mlt_frame_get_audio( frame, &audio, &priv->audio_format, &priv->frequency, &priv->channels, &samples );
			}
			pthread_mutex_lock( &priv->queue_mutex );
			mlt_deque_push_back( priv->queue, frame );
			pthread_cond_signal( &priv->queue_cond );
			pthread_mutex_unlock( &priv->queue_mutex );
		}
	}

	// Wait if not realtime.
	while ( priv->ahead && priv->real_time < 0 && !priv->is_purge &&
		!( mlt_properties_get_int( MLT_FRAME_PROPERTIES( MLT_FRAME( mlt_deque_peek_front( priv->queue ) ) ), "rendered" ) ) )
	{
		pthread_mutex_lock( &priv->done_mutex );
		pthread_cond_wait( &priv->done_cond, &priv->done_mutex );
		pthread_mutex_unlock( &priv->done_mutex );
	}

	// Get the frame from the queue.
	pthread_mutex_lock( &priv->queue_mutex );
	frame = mlt_deque_pop_front( priv->queue );
	pthread_mutex_unlock( &priv->queue_mutex );
	if ( ! frame ) {
		priv->is_purge = 0;
		return frame;
	}

	// Adapt the worker process head to the runtime conditions.
	if ( priv->real_time > 0 )
	{
		if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "rendered" ) )
		{
			priv->consecutive_dropped = 0;
			if ( priv->process_head > threads && priv->consecutive_rendered >= priv->process_head )
				priv->process_head--;
			else
				priv->consecutive_rendered++;
		}
		else
		{
			priv->consecutive_rendered = 0;
			if ( priv->process_head < buffer - threads && priv->consecutive_dropped > threads )
				priv->process_head++;
			else
				priv->consecutive_dropped++;
		}
//		mlt_log_verbose( MLT_CONSUMER_SERVICE(self), "dropped %d rendered %d process_head %d\n",
//			priv->consecutive_dropped, priv->consecutive_rendered, priv->process_head );

		// Check for too many consecutively dropped frames
		if ( priv->consecutive_dropped > mlt_properties_get_int( properties, "drop_max" ) )
		{
			int orig_buffer = mlt_properties_get_int( properties, "buffer" );
			int prefill = mlt_properties_get_int( properties, "prefill" );
			mlt_log_verbose( self, "too many frames dropped - " );

			// If using a default low-latency buffer level (SDL) and below the limit
			if ( ( orig_buffer == 1 || prefill == 1 ) && buffer < (threads + 1) * 10 )
			{
				// Auto-scale the buffer to compensate
				mlt_log_verbose( self, "increasing buffer to %d\n", buffer + threads );
				mlt_properties_set_int( properties, "_buffer", buffer + threads );
				priv->consecutive_dropped = priv->fps / 2;
			}
			else
			{
				// Tell the consumer to render it
				mlt_log_verbose( self, "forcing next frame\n" );
				mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "rendered", 1 );
				priv->consecutive_dropped = 0;
			}
		}
		if ( !mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "rendered") )
		{
			int dropped = mlt_properties_get_int( properties, "drop_count" );
			mlt_properties_set_int( properties, "drop_count", ++dropped );
			mlt_log_verbose( MLT_CONSUMER_SERVICE(self), "dropped video frame %d\n", dropped );
		}
	}
	if ( priv->is_purge ) {
		priv->is_purge = 0;
		mlt_frame_close( frame );
		frame = NULL;
	}
	return frame;
}

/** Get the next frame from the producer connected to a consumer.
 *
 * Typically, one uses this instead of \p mlt_consumer_get_frame to make
 * the asynchronous/real-time behavior configurable at runtime.
 * You should close the frame returned from this when you are done with it.
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 * \return a frame
 */

mlt_frame mlt_consumer_rt_frame( mlt_consumer self )
{
	// Frame to return
	mlt_frame frame = NULL;

	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
	consumer_private *priv = self->local;

	// Check if the user has requested real time or not
	if ( priv->real_time > 1 || priv->real_time < -1 )
	{
		// see above
		return worker_get_frame( self, properties );
	}
	else if ( priv->real_time == 1 || priv->real_time == -1 )
	{
		int size = 1;

		if ( priv->preroll )
		{
			int buffer = mlt_properties_get_int( properties, "buffer" );
			int prefill = mlt_properties_get_int( properties, "prefill" );
#ifndef _WIN32
			consumer_read_ahead_start( self );
#endif
			if ( buffer > 1 )
				size = prefill > 0 && prefill < buffer ? prefill : buffer;
			priv->preroll = 0;
		}

		// Get frame from queue
		pthread_mutex_lock( &priv->queue_mutex );
		mlt_log_timings_begin();
		while( priv->ahead && mlt_deque_count( priv->queue ) < size )
			pthread_cond_wait( &priv->queue_cond, &priv->queue_mutex );
		frame = mlt_deque_pop_front( priv->queue );
		mlt_log_timings_end( NULL, "wait_for_frame_queue" );
		pthread_cond_broadcast( &priv->queue_cond );
		pthread_mutex_unlock( &priv->queue_mutex );
		if ( priv->real_time == 1 && frame &&
			 !mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "rendered" ) )
		{
			int dropped = mlt_properties_get_int( properties, "drop_count" );
			mlt_properties_set_int( properties, "drop_count", ++dropped );
			mlt_log_verbose( MLT_CONSUMER_SERVICE(self), "dropped video frame %d\n", dropped );
		}
	}
	else // real_time == 0
	{
		if ( !priv->ahead )
		{
			priv->ahead = 1;
			mlt_events_fire( properties, "consumer-thread-started", NULL );
		}
		// Get the frame in non real time
		frame = mlt_consumer_get_frame( self );

		// This isn't true, but from the consumers perspective it is
		if ( frame != NULL )
		{
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "rendered", 1 );

			// WebVfx uses this to setup a consumer-stopping event handler.
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "consumer", self, 0, NULL, NULL );
		}
	}

	return frame;
}

/** Callback for the implementation to indicate a stopped condition.
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 */

void mlt_consumer_stopped( mlt_consumer self )
{
	mlt_properties_set_int( MLT_CONSUMER_PROPERTIES( self ), "running", 0 );
	mlt_events_fire( MLT_CONSUMER_PROPERTIES( self ), "consumer-stopped", NULL );
	mlt_event_unblock( ( ( consumer_private* ) self->local )->event_listener );
}

/** Stop the consumer.
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 * \return true if there was an error
 */

int mlt_consumer_stop( mlt_consumer self )
{
	// Get the properies
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );
	consumer_private *priv = self->local;

	// Just in case...
	mlt_log( MLT_CONSUMER_SERVICE( self ), MLT_LOG_DEBUG, "stopping put waiting\n" );
	pthread_mutex_lock( &priv->put_mutex );
	priv->put_active = 0;
	pthread_cond_broadcast( &priv->put_cond );
	pthread_mutex_unlock( &priv->put_mutex );

	// Stop the consumer
	mlt_log( MLT_CONSUMER_SERVICE( self ), MLT_LOG_DEBUG, "stopping consumer\n" );
	
	// Cancel the read ahead threads
	if ( priv->started )
	{
		// Unblock the consumer calling mlt_consumer_rt_frame
		pthread_mutex_lock( &priv->queue_mutex );
		pthread_cond_broadcast( &priv->queue_cond );
		pthread_mutex_unlock( &priv->queue_mutex );
	}
	
	// Invoke the child callback
	if ( self->stop != NULL )
		self->stop( self );

	// Check if the user has requested real time or not and stop if necessary
	mlt_log( MLT_CONSUMER_SERVICE( self ), MLT_LOG_DEBUG, "stopping read_ahead\n" );
	if ( abs( priv->real_time ) == 1 )
		consumer_read_ahead_stop( self );
	else if ( abs( priv->real_time ) > 1 )
		consumer_work_stop( self );

	// Kill the test card
	mlt_properties_set_data( properties, "test_card_producer", NULL, 0, NULL, NULL );

	// Check and run a post command
	if ( mlt_properties_get( properties, "post" ) )
		if (system( mlt_properties_get( properties, "post" ) ) == -1 )
			mlt_log( MLT_CONSUMER_SERVICE( self ), MLT_LOG_ERROR, "system(%s) failed!\n", mlt_properties_get( properties, "post" ) );

	mlt_log( MLT_CONSUMER_SERVICE( self ), MLT_LOG_DEBUG, "stopped\n" );

	return 0;
}

/** Determine if the consumer is stopped.
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 * \return true if the consumer is stopped
 */

int mlt_consumer_is_stopped( mlt_consumer self )
{
	// Check if the consumer is stopped
	if ( self && self->is_stopped )
		return self->is_stopped( self );

	return 0;
}

/** Close and destroy the consumer.
 *
 * \public \memberof mlt_consumer_s
 * \param self a consumer
 */

void mlt_consumer_close( mlt_consumer self )
{
	if ( self != NULL && mlt_properties_dec_ref( MLT_CONSUMER_PROPERTIES( self ) ) <= 0 )
	{
		// Get the childs close function
		void ( *consumer_close )( ) = self->close;

		if ( consumer_close )
		{
			// Just in case...
			//mlt_consumer_stop( self );

			self->close = NULL;
			consumer_close( self );
		}
		else
		{
			consumer_private *priv = self->local;

			// Make sure it only gets called once
			self->parent.close = NULL;

			// Destroy the push mutex and condition
			pthread_mutex_destroy( &priv->put_mutex );
			pthread_cond_destroy( &priv->put_cond );

			mlt_service_close( &self->parent );
			free( priv );
		}
	}
}

/** Get the position of the last frame shown.
 *
 * \public \memberof mlt_consumer_s
 * \param consumer a consumer
 * \return the position
 */

mlt_position mlt_consumer_position( mlt_consumer consumer )
{
	return ( ( consumer_private* ) consumer->local )->position;
}

static void transmit_thread_create( mlt_listener listener, mlt_properties owner, mlt_service self, void **args )
{
	if ( listener )
		listener( owner, self,
			(void**) args[0] /* handle */, (int*) args[1] /* priority */, (thread_function_t) args[2], (void*) args[3] /* data */ );
}

static void mlt_thread_create( mlt_consumer self, thread_function_t function )
{
	consumer_private *priv = self->local;
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );

	if ( mlt_properties_get( MLT_CONSUMER_PROPERTIES( self ), "priority" ) )
	{
		struct sched_param priority;
		priority.sched_priority = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( self ), "priority" );
		if ( mlt_events_fire( properties, "consumer-thread-create",
		     &priv->ahead_thread, &priority.sched_priority, function, self, NULL ) < 1 )
		{
			pthread_attr_t thread_attributes;
			pthread_attr_init( &thread_attributes );
			pthread_attr_setschedpolicy( &thread_attributes, SCHED_OTHER );
			pthread_attr_setschedparam( &thread_attributes, &priority );
			pthread_attr_setinheritsched( &thread_attributes, PTHREAD_EXPLICIT_SCHED );
			pthread_attr_setscope( &thread_attributes, PTHREAD_SCOPE_SYSTEM );
			priv->ahead_thread = malloc( sizeof( pthread_t ) );
			pthread_t *handle = priv->ahead_thread;
			if ( pthread_create( ( pthread_t* ) &( *handle ), &thread_attributes, function, self ) < 0 )
				pthread_create( ( pthread_t* ) &( *handle ), NULL, function, self );
			pthread_attr_destroy( &thread_attributes );
		}
	}
	else
	{
		int priority = -1;
		if ( mlt_events_fire( properties, "consumer-thread-create",
		     &priv->ahead_thread, &priority, function, self, NULL ) < 1 )
		{
			priv->ahead_thread = malloc( sizeof( pthread_t ) );
			pthread_t *handle = priv->ahead_thread;
			pthread_create( ( pthread_t* ) &( *handle ), NULL, function, self );
		}
	}
}

static void transmit_thread_join( mlt_listener listener, mlt_properties owner, mlt_service self, void **args )
{
	if ( listener )
		listener( owner, self, (void*) args[0] /* handle */ );
}

static void mlt_thread_join( mlt_consumer self )
{
	consumer_private *priv = self->local;
	if ( mlt_events_fire( MLT_CONSUMER_PROPERTIES(self), "consumer-thread-join", priv->ahead_thread, NULL ) < 1 )
	{
		pthread_t *handle = priv->ahead_thread;
		pthread_join( *handle, NULL );
		free( priv->ahead_thread );
	}
	priv->ahead_thread = NULL;
}
