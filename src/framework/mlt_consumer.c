/**
 * \file mlt_consumer.c
 * \brief abstraction for all consumer services
 * \see mlt_consumer_s
 *
 * Copyright (C) 2003-2009 Ushodaya Enterprises Limited
 * \author Charles Yates <charles.yates@pandora.be>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

static void mlt_consumer_frame_render( mlt_listener listener, mlt_properties owner, mlt_service this, void **args );
static void mlt_consumer_frame_show( mlt_listener listener, mlt_properties owner, mlt_service this, void **args );
static void mlt_consumer_property_changed( mlt_properties owner, mlt_consumer this, char *name );
static void apply_profile_properties( mlt_consumer this, mlt_profile profile, mlt_properties properties );
static void on_consumer_frame_show( mlt_properties owner, mlt_consumer this, mlt_frame frame );

/** Initialize a consumer service.
 *
 * \public \memberof mlt_consumer_s
 * \param this the consumer to initialize
 * \param child a pointer to the object for the subclass
 * \param profile the \p mlt_profile_s to use (optional but recommended,
 * uses the environment variable MLT if this is NULL)
 * \return true if there was an error
 */

int mlt_consumer_init( mlt_consumer this, void *child, mlt_profile profile )
{
	int error = 0;
	memset( this, 0, sizeof( struct mlt_consumer_s ) );
	this->child = child;
	error = mlt_service_init( &this->parent, this );
	if ( error == 0 )
	{
		// Get the properties from the service
		mlt_properties properties = MLT_SERVICE_PROPERTIES( &this->parent );

		// Apply profile to properties
		if ( profile == NULL )
		{
			// Normally the application creates the profile and controls its lifetime
			// This is the fallback exception handling
			profile = mlt_profile_init( NULL );
			mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
			mlt_properties_set_data( properties, "_profile", profile, 0, (mlt_destructor)mlt_profile_close, NULL );
		}
		apply_profile_properties( this, profile, properties );

		// Default rescaler for all consumers
		mlt_properties_set( properties, "rescale", "bilinear" );

		// Default read ahead buffer size
		mlt_properties_set_int( properties, "buffer", 25 );

		// Default audio frequency and channels
		mlt_properties_set_int( properties, "frequency", 48000 );
		mlt_properties_set_int( properties, "channels", 2 );

		// Default of all consumers is real time
		mlt_properties_set_int( properties, "real_time", 1 );

		// Default to environment test card
		mlt_properties_set( properties, "test_card", mlt_environment( "MLT_TEST_CARD" ) );

		// Hmm - default all consumers to yuv422 :-/
		this->format = mlt_image_yuv422;

		mlt_events_register( properties, "consumer-frame-show", ( mlt_transmitter )mlt_consumer_frame_show );
		mlt_events_register( properties, "consumer-frame-render", ( mlt_transmitter )mlt_consumer_frame_render );
		mlt_events_register( properties, "consumer-stopped", NULL );
		mlt_events_listen( properties, this, "consumer-frame-show", ( mlt_listener )on_consumer_frame_show );

		// Register a property-changed listener to handle the profile property -
		// subsequent properties can override the profile
		this->event_listener = mlt_events_listen( properties, this, "property-changed", ( mlt_listener )mlt_consumer_property_changed );

		// Create the push mutex and condition
		pthread_mutex_init( &this->put_mutex, NULL );
		pthread_cond_init( &this->put_cond, NULL );

	}
	return error;
}

/** Convert the profile into properties on the consumer.
 *
 * \private \memberof mlt_consumer_s
 * \param this a consumer
 * \param profile a profile
 * \param properties a properties list (typically, the consumer's)
 */

static void apply_profile_properties( mlt_consumer this, mlt_profile profile, mlt_properties properties )
{
	mlt_event_block( this->event_listener );
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
	mlt_properties_set_int( properties, "display_aspect_num", profile->display_aspect_num );
	mlt_properties_set_int( properties, "colorspace", profile->colorspace );
	mlt_event_unblock( this->event_listener );
}

/** The property-changed event listener
 *
 * \private \memberof mlt_consumer_s
 * \param owner the events object
 * \param this the consumer
 * \param name the name of the property that changed
 */

static void mlt_consumer_property_changed( mlt_properties owner, mlt_consumer this, char *name )
{
	if ( !strcmp( name, "profile" ) )
	{
		// Get the properies
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

		// Get the current profile
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );

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
				mlt_profile_close( new_profile );
			}
			else
			{
				profile = new_profile;
			}

			// Apply to properties
			apply_profile_properties( this, profile, properties );
		}
 	}
	else if ( !strcmp( name, "frame_rate_num" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );
		if ( profile )
		{
			profile->frame_rate_num = mlt_properties_get_int( properties, "frame_rate_num" );
			mlt_properties_set_double( properties, "fps", mlt_profile_fps( profile ) );
		}
	}
	else if ( !strcmp( name, "frame_rate_den" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );
		if ( profile )
		{
			profile->frame_rate_den = mlt_properties_get_int( properties, "frame_rate_den" );
			mlt_properties_set_double( properties, "fps", mlt_profile_fps( profile ) );
		}
	}
	else if ( !strcmp( name, "width" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );
		if ( profile )
			profile->width = mlt_properties_get_int( properties, "width" );
	}
	else if ( !strcmp( name, "height" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );
		if ( profile )
			profile->height = mlt_properties_get_int( properties, "height" );
	}
	else if ( !strcmp( name, "progressive" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );
		if ( profile )
			profile->progressive = mlt_properties_get_int( properties, "progressive" );
	}
	else if ( !strcmp( name, "sample_aspect_num" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );
		profile->sample_aspect_num = mlt_properties_get_int( properties, "sample_aspect_num" );
		if ( profile )
			mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile )  );
	}
	else if ( !strcmp( name, "sample_aspect_den" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );
		profile->sample_aspect_den = mlt_properties_get_int( properties, "sample_aspect_den" );
		if ( profile )
			mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile )  );
	}
	else if ( !strcmp( name, "display_aspect_num" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );
		if ( profile )
		{
			profile->display_aspect_num = mlt_properties_get_int( properties, "display_aspect_num" );
			mlt_properties_set_double( properties, "display_ratio", mlt_profile_dar( profile )  );
		}
	}
	else if ( !strcmp( name, "display_aspect_den" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );
		if ( profile )
		{
			profile->display_aspect_den = mlt_properties_get_int( properties, "display_aspect_den" );
			mlt_properties_set_double( properties, "display_ratio", mlt_profile_dar( profile )  );
		}
	}
	else if ( !strcmp( name, "colorspace" ) )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );
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
 * \param this  a service that will be passed to \p listener
 * \param args an array of pointers - the first entry is passed as a string to \p listener
 */

static void mlt_consumer_frame_show( mlt_listener listener, mlt_properties owner, mlt_service this, void **args )
{
	if ( listener != NULL )
		listener( owner, this, ( mlt_frame )args[ 0 ] );
}

/** The transmitter for the consumer-frame-render event
 *
 * Invokes the listener.
 *
 * \private \memberof mlt_consumer_s
 * \param listener a function pointer that will be invoked
 * \param owner the events object that will be passed to \p listener
 * \param this  a service that will be passed to \p listener
 * \param args an array of pointers - the first entry is passed as a string to \p listener
 */

static void mlt_consumer_frame_render( mlt_listener listener, mlt_properties owner, mlt_service this, void **args )
{
	if ( listener != NULL )
		listener( owner, this, ( mlt_frame )args[ 0 ] );
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
		consumer->position = mlt_frame_get_position( frame );
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
	mlt_consumer this = malloc( sizeof( struct mlt_consumer_s ) );

	// Initialise it
	if ( this != NULL )
		mlt_consumer_init( this, NULL, profile );

	// Return it
	return this;
}

/** Get the parent service object.
 *
 * \public \memberof mlt_consumer_s
 * \param this a consumer
 * \return the parent service class
 * \see MLT_CONSUMER_SERVICE
 */

mlt_service mlt_consumer_service( mlt_consumer this )
{
	return this != NULL ? &this->parent : NULL;
}

/** Get the consumer properties.
 *
 * \public \memberof mlt_consumer_s
 * \param this a consumer
 * \return the consumer's properties list
 * \see MLT_CONSUMER_PROPERTIES
 */

mlt_properties mlt_consumer_properties( mlt_consumer this )
{
	return this != NULL ? MLT_SERVICE_PROPERTIES( &this->parent ) : NULL;
}

/** Connect the consumer to the producer.
 *
 * \public \memberof mlt_consumer_s
 * \param this a consumer
 * \param producer a producer
 * \return > 0 warning, == 0 success, < 0 serious error,
 *         1 = this service does not accept input,
 *         2 = the producer is invalid,
 *         3 = the producer is already registered with this consumer
 */

int mlt_consumer_connect( mlt_consumer this, mlt_service producer )
{
	return mlt_service_connect_producer( &this->parent, producer, 0 );
}

/** Start the consumer.
 *
 * \public \memberof mlt_consumer_s
 * \param this a consumer
 * \return true if there was an error
 */

int mlt_consumer_start( mlt_consumer this )
{
	// Stop listening to the property-changed event
	mlt_event_block( this->event_listener );

	// Get the properies
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Determine if there's a test card producer
	char *test_card = mlt_properties_get( properties, "test_card" );

	// Just to make sure nothing is hanging around...
	mlt_frame_close( this->put );
	this->put = NULL;
	this->put_active = 1;

	// Deal with it now.
	if ( test_card != NULL )
	{
		if ( mlt_properties_get_data( properties, "test_card_producer", NULL ) == NULL )
		{
			// Create a test card producer
			mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( this ) );
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

	// Set the frame duration in microseconds for the frame-dropping heuristic
	int frame_duration = 1000000 / mlt_properties_get_int( properties, "frame_rate_num" ) *
			mlt_properties_get_int( properties, "frame_rate_den" );
	mlt_properties_set_int( properties, "frame_duration", frame_duration );

	// Check and run an ante command
	if ( mlt_properties_get( properties, "ante" ) )
		if ( system( mlt_properties_get( properties, "ante" ) ) == -1 )
			mlt_log( MLT_CONSUMER_SERVICE( this ), MLT_LOG_ERROR, "system(%s) failed!\n", mlt_properties_get( properties, "ante" ) );

	// Set the real_time preference
	this->real_time = mlt_properties_get_int( properties, "real_time" );

	// For worker threads implementation, buffer must be at least # threads
	if ( abs( this->real_time ) > 1 && mlt_properties_get_int( properties, "buffer" ) <= abs( this->real_time ) )
		mlt_properties_set_int( properties, "buffer", abs( this->real_time ) + 1 );

	// Start the service
	if ( this->start != NULL )
		return this->start( this );

	return 0;
}

/** An alternative method to feed frames into the consumer.
 *
 * Only valid if the consumer itself is not connected.
 *
 * \public \memberof mlt_consumer_s
 * \param this a consumer
 * \param frame a frame
 * \return true (ignore this for now)
 */

int mlt_consumer_put_frame( mlt_consumer this, mlt_frame frame )
{
	int error = 1;

	// Get the service assoicated to the consumer
	mlt_service service = MLT_CONSUMER_SERVICE( this );

	if ( mlt_service_producer( service ) == NULL )
	{
		struct timeval now;
		struct timespec tm;
		pthread_mutex_lock( &this->put_mutex );
		while ( this->put_active && this->put != NULL )
		{
			gettimeofday( &now, NULL );
			tm.tv_sec = now.tv_sec + 1;
			tm.tv_nsec = now.tv_usec * 1000;
			pthread_cond_timedwait( &this->put_cond, &this->put_mutex, &tm );
		}
		if ( this->put_active && this->put == NULL )
			this->put = frame;
		else
			mlt_frame_close( frame );
		pthread_cond_broadcast( &this->put_cond );
		pthread_mutex_unlock( &this->put_mutex );
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
 * \param this a consumer
 * \return a frame
 */

mlt_frame mlt_consumer_get_frame( mlt_consumer this )
{
	// Frame to return
	mlt_frame frame = NULL;

	// Get the service assoicated to the consumer
	mlt_service service = MLT_CONSUMER_SERVICE( this );

	// Get the consumer properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Get the frame
	if ( mlt_service_producer( service ) == NULL && mlt_properties_get_int( properties, "put_mode" ) )
	{
		struct timeval now;
		struct timespec tm;
		pthread_mutex_lock( &this->put_mutex );
		while ( this->put_active && this->put == NULL )
		{
			gettimeofday( &now, NULL );
			tm.tv_sec = now.tv_sec + 1;
			tm.tv_nsec = now.tv_usec * 1000;
			pthread_cond_timedwait( &this->put_cond, &this->put_mutex, &tm );
		}
		frame = this->put;
		this->put = NULL;
		pthread_cond_broadcast( &this->put_cond );
		pthread_mutex_unlock( &this->put_mutex );
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

		// Attach the rescale property
		mlt_properties_set( frame_properties, "rescale.interp", mlt_properties_get( properties, "rescale" ) );

		// Aspect ratio and other jiggery pokery
		mlt_properties_set_double( frame_properties, "consumer_aspect_ratio", mlt_properties_get_double( properties, "aspect_ratio" ) );
		mlt_properties_set_int( frame_properties, "consumer_deinterlace", mlt_properties_get_int( properties, "progressive" ) | mlt_properties_get_int( properties, "deinterlace" ) );
		mlt_properties_set( frame_properties, "deinterlace_method", mlt_properties_get( properties, "deinterlace_method" ) );
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
	mlt_consumer this = arg;

	// Get the properties of the consumer
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Get the width and height
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );

	// See if video is turned off
	int video_off = mlt_properties_get_int( properties, "video_off" );
	int preview_off = mlt_properties_get_int( properties, "preview_off" );
	int preview_format = mlt_properties_get_int( properties, "preview_format" );

	// Get the audio settings
	mlt_audio_format afmt = mlt_audio_s16;
	int counter = 0;
	double fps = mlt_properties_get_double( properties, "fps" );
	int channels = mlt_properties_get_int( properties, "channels" );
	int frequency = mlt_properties_get_int( properties, "frequency" );
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
	int count = 1;
	int skipped = 0;
	int64_t time_wait = 0;
	int64_t time_frame = 0;
	int64_t time_process = 0;
	int skip_next = 0;

	if ( preview_off && preview_format != 0 )
		this->format = preview_format;

	// Get the first frame
	frame = mlt_consumer_get_frame( this );

	// Get the image of the first frame
	if ( !video_off )
	{
		mlt_events_fire( MLT_CONSUMER_PROPERTIES( this ), "consumer-frame-render", frame, NULL );
		mlt_frame_get_image( frame, &image, &this->format, &width, &height, 0 );
	}

	if ( !audio_off )
	{
		samples = mlt_sample_calculator( fps, frequency, counter++ );
		mlt_frame_get_audio( frame, &audio, &afmt, &frequency, &channels, &samples );
	}

	// Mark as rendered
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "rendered", 1 );

	// Get the starting time (can ignore the times above)
	gettimeofday( &ante, NULL );

	// Continue to read ahead
	while ( this->ahead )
	{
		// Fetch width/height again
		width = mlt_properties_get_int( properties, "width" );
		height = mlt_properties_get_int( properties, "height" );

		// Put the current frame into the queue
		pthread_mutex_lock( &this->frame_queue_mutex );
		while( this->ahead && mlt_deque_count( this->frame_queue ) >= buffer )
			pthread_cond_wait( &this->frame_queue_cond, &this->frame_queue_mutex );
		mlt_deque_push_back( this->frame_queue, frame );
		pthread_cond_broadcast( &this->frame_queue_cond );
		pthread_mutex_unlock( &this->frame_queue_mutex );

		time_wait += time_difference( &ante );

		// Get the next frame
		frame = mlt_consumer_get_frame( this );
		time_frame += time_difference( &ante );

		// If there's no frame, we're probably stopped...
		if ( frame == NULL )
			continue;

		// Increment the count
		count ++;

		// All non normal playback frames should be shown
		if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "_speed" ) != 1 )
		{
#ifdef DEINTERLACE_ON_NOT_NORMAL_SPEED
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "consumer_deinterlace", 1 );
#endif
			skipped = 0;
			time_frame = 0;
			time_process = 0;
			time_wait = 0;
			count = 1;
			skip_next = 0;
		}

		// Get the image
		if ( !skip_next || this->real_time == -1 )
		{
			// Get the image, mark as rendered and time it
			if ( !video_off )
			{
				mlt_events_fire( MLT_CONSUMER_PROPERTIES( this ), "consumer-frame-render", frame, NULL );
				mlt_frame_get_image( frame, &image, &this->format, &width, &height, 0 );
			}
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "rendered", 1 );
		}
		else
		{
			// Increment the number of sequentially skipped frames
			skipped ++;
			skip_next = 0;

			// If we've reached an unacceptable level, reset everything
			if ( skipped > 5 )
			{
				skipped = 0;
				time_frame = 0;
				time_process = 0;
				time_wait = 0;
				count = 1;
			}
		}

		// Always process audio
		if ( !audio_off )
		{
			samples = mlt_sample_calculator( fps, frequency, counter++ );
			mlt_frame_get_audio( frame, &audio, &afmt, &frequency, &channels, &samples );
		}

		// Increment the time take for this frame
		time_process += time_difference( &ante );

		// Determine if the next frame should be skipped
		if ( mlt_deque_count( this->frame_queue ) <= 5 )
		{
			int frame_duration = mlt_properties_get_int( properties, "frame_duration" );
			if ( ( ( time_wait + time_frame + time_process ) / count ) > frame_duration )
				skip_next = 1;
		}
	}

	// Remove the last frame
	mlt_frame_close( frame );

	return NULL;
}

static int compare_frame_position( mlt_frame a, mlt_frame b )
{
	return mlt_frame_get_position( a ) - mlt_frame_get_position( b );
}

/** The worker thread procedure for parallel processing frames.
 *
 * \private \memberof mlt_consumer_s
 * \param arg a consumer
 */

static void *consumer_worker_thread( void *arg )
{
	// The argument is the consumer
	mlt_consumer this = arg;

	// Get the properties of the consumer
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Get the width and height
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );
	mlt_image_format format = this->format;

	// See if video is turned off
	int video_off = mlt_properties_get_int( properties, "video_off" );
	int preview_off = mlt_properties_get_int( properties, "preview_off" );
	int preview_format = mlt_properties_get_int( properties, "preview_format" );

	// Get the audio settings
	mlt_audio_format afmt = mlt_audio_s16;
	int counter = 0;
	double fps = mlt_properties_get_double( properties, "fps" );
	int channels = mlt_properties_get_int( properties, "channels" );
	int frequency = mlt_properties_get_int( properties, "frequency" );
	int samples = 0;
	void *audio = NULL;

	// See if audio is turned off
	int audio_off = mlt_properties_get_int( properties, "audio_off" );

	// General frame variable
	mlt_frame frame = NULL;
	uint8_t *image = NULL;

	if ( preview_off && preview_format != 0 )
		format = preview_format;

	// Get the first frame from the work queue
	pthread_mutex_lock( &this->frame_queue_mutex );
	while( this->ahead && mlt_deque_count( this->frame_queue ) <= 0 )
		pthread_cond_wait( &this->frame_queue_cond, &this->frame_queue_mutex );
	frame = mlt_deque_pop_front( this->frame_queue );
	pthread_cond_signal( &this->frame_queue_cond );
	pthread_mutex_unlock( &this->frame_queue_mutex );

	// Get the image of the first frame
	if ( !video_off )
	{
		mlt_events_fire( MLT_CONSUMER_PROPERTIES( this ), "consumer-frame-render", frame, NULL );
		mlt_frame_get_image( frame, &image, &format, &width, &height, 0 );
	}

	if ( !audio_off )
	{
		samples = mlt_sample_calculator( fps, frequency, counter++ );
		mlt_frame_get_audio( frame, &audio, &afmt, &frequency, &channels, &samples );
	}

	// Mark as rendered
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "rendered", 1 );

	// Continue to read ahead
	while ( this->ahead )
	{
		// Fetch width/height again
		width = mlt_properties_get_int( properties, "width" );
		height = mlt_properties_get_int( properties, "height" );

		// Put the processed frame into the done queue
		if ( frame )
		{
			pthread_mutex_lock( &this->done_queue_mutex );
			mlt_deque_insert( this->done_queue, frame, ( mlt_deque_compare ) compare_frame_position );
			pthread_cond_signal( &this->done_queue_cond );
			pthread_mutex_unlock( &this->done_queue_mutex );
		}

		// Get the next frame from the work queue
		pthread_mutex_lock( &this->frame_queue_mutex );
		while( this->ahead && mlt_deque_count( this->frame_queue ) <= 0 )
			pthread_cond_wait( &this->frame_queue_cond, &this->frame_queue_mutex );
		frame = mlt_deque_pop_front( this->frame_queue );
		pthread_cond_signal( &this->frame_queue_cond );
		pthread_mutex_unlock( &this->frame_queue_mutex );

		// If there's no frame, we're probably stopped...
		if ( frame == NULL )
			continue;

		// All non normal playback frames should be shown
		if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "_speed" ) != 1 )
		{
#ifdef DEINTERLACE_ON_NOT_NORMAL_SPEED
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "consumer_deinterlace", 1 );
#endif
		}

		// Get the image
		if ( !video_off )
		{
			mlt_events_fire( MLT_CONSUMER_PROPERTIES( this ), "consumer-frame-render", frame, NULL );
			mlt_frame_get_image( frame, &image, &format, &width, &height, 0 );
		}
		mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "rendered", 1 );

		// Always process audio
		if ( !audio_off )
		{
			samples = mlt_sample_calculator( fps, frequency, counter++ );
			mlt_frame_get_audio( frame, &audio, &afmt, &frequency, &channels, &samples );
		}
	}

	// Remove the last frame
	mlt_frame_close( frame );

	return NULL;
}

/** Start the read/render thread.
 *
 * \private \memberof mlt_consumer_s
 * \param this a consumer
 */

static void consumer_read_ahead_start( mlt_consumer this )
{
	// We're running now
	this->ahead = 1;

	// Create the frame queue
	this->frame_queue = mlt_deque_init( );

	// Create the frame_queue mutex
	pthread_mutex_init( &this->frame_queue_mutex, NULL );

	// Create the condition
	pthread_cond_init( &this->frame_queue_cond, NULL );

	// Create the read ahead
	if ( mlt_properties_get( MLT_CONSUMER_PROPERTIES( this ), "priority" ) )
	{
		struct sched_param priority;
		priority.sched_priority = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( this ), "priority" );
		pthread_attr_t thread_attributes;
		pthread_attr_init( &thread_attributes );
		pthread_attr_setschedpolicy( &thread_attributes, SCHED_OTHER );
		pthread_attr_setschedparam( &thread_attributes, &priority );
		pthread_attr_setinheritsched( &thread_attributes, PTHREAD_EXPLICIT_SCHED );
		pthread_attr_setscope( &thread_attributes, PTHREAD_SCOPE_SYSTEM );
		if ( pthread_create( &this->ahead_thread, &thread_attributes, consumer_read_ahead_thread, this ) < 0 )
			pthread_create( &this->ahead_thread, NULL, consumer_read_ahead_thread, this );
		pthread_attr_destroy( &thread_attributes );
	}
	else
	{
		pthread_create( &this->ahead_thread, NULL, consumer_read_ahead_thread, this );
	}
}

/** Start the worker threads.
 *
 * \private \memberof mlt_consumer_s
 * \param this a consumer
 */

static void consumer_work_start( mlt_consumer this )
{
	int n = abs( this->real_time );
	pthread_t thread;

	// We're running now
	this->ahead = 1;

	// Create the queues
	this->frame_queue = mlt_deque_init();
	this->done_queue = mlt_deque_init();
	this->worker_threads = mlt_deque_init();

	// Create the mutexes
	pthread_mutex_init( &this->frame_queue_mutex, NULL );
	pthread_mutex_init( &this->done_queue_mutex, NULL );

	// Create the conditions
	pthread_cond_init( &this->frame_queue_cond, NULL );
	pthread_cond_init( &this->done_queue_cond, NULL );

	// Create the read ahead
	if ( mlt_properties_get( MLT_CONSUMER_PROPERTIES( this ), "priority" ) )
	{

		struct sched_param priority;
		pthread_attr_t thread_attributes;

		priority.sched_priority = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( this ), "priority" );
		pthread_attr_init( &thread_attributes );
		pthread_attr_setschedpolicy( &thread_attributes, SCHED_OTHER );
		pthread_attr_setschedparam( &thread_attributes, &priority );
		pthread_attr_setinheritsched( &thread_attributes, PTHREAD_EXPLICIT_SCHED );
		pthread_attr_setscope( &thread_attributes, PTHREAD_SCOPE_SYSTEM );

		while ( n-- )
		{
			if ( pthread_create( &thread, &thread_attributes, consumer_read_ahead_thread, this ) < 0 )
				if ( pthread_create( &thread, NULL, consumer_read_ahead_thread, this ) == 0 )
					mlt_deque_push_back( this->worker_threads, (void*) thread );
		}
		pthread_attr_destroy( &thread_attributes );
	}

	else
	{
		while ( n-- )
		{
			if ( pthread_create( &thread, NULL, consumer_worker_thread, this ) == 0 )
				mlt_deque_push_back( this->worker_threads, (void*) thread );
		}
	}
}

/** Stop the read/render thread.
 *
 * \private \memberof mlt_consumer_s
 * \param this a consumer
 */

static void consumer_read_ahead_stop( mlt_consumer this )
{
	// Make sure we're running
	if ( this->ahead )
	{
		// Inform thread to stop
		this->ahead = 0;

		// Broadcast to the condition in case it's waiting
		pthread_mutex_lock( &this->frame_queue_mutex );
		pthread_cond_broadcast( &this->frame_queue_cond );
		pthread_mutex_unlock( &this->frame_queue_mutex );

		// Broadcast to the put condition in case it's waiting
		pthread_mutex_lock( &this->put_mutex );
		pthread_cond_broadcast( &this->put_cond );
		pthread_mutex_unlock( &this->put_mutex );

		// Join the thread
		pthread_join( this->ahead_thread, NULL );

		// Destroy the frame queue mutex
		pthread_mutex_destroy( &this->frame_queue_mutex );

		// Destroy the condition
		pthread_cond_destroy( &this->frame_queue_cond );

		// Wipe the queue
		while ( mlt_deque_count( this->frame_queue ) )
			mlt_frame_close( mlt_deque_pop_back( this->frame_queue ) );

		// Close the queue
		mlt_deque_close( this->frame_queue );
	}
}

/** Stop the worker threads.
 *
 * \private \memberof mlt_consumer_s
 * \param this a consumer
 */

static void consumer_work_stop( mlt_consumer this )
{
	// Make sure we're running
	if ( this->ahead )
	{
		// Inform thread to stop
		this->ahead = 0;

		// Broadcast to the frame_queue condition in case it's waiting
		pthread_mutex_lock( &this->frame_queue_mutex );
		pthread_cond_broadcast( &this->frame_queue_cond );
		pthread_mutex_unlock( &this->frame_queue_mutex );

		// Broadcast to the done_queue condition in case it's waiting
		pthread_mutex_lock( &this->done_queue_mutex );
		pthread_cond_broadcast( &this->done_queue_cond );
		pthread_mutex_unlock( &this->done_queue_mutex );

		// Broadcast to the put condition in case it's waiting
		pthread_mutex_lock( &this->put_mutex );
		pthread_cond_broadcast( &this->put_cond );
		pthread_mutex_unlock( &this->put_mutex );

		// Join the threads
		pthread_t thread;
		while ( ( thread = (pthread_t) mlt_deque_pop_front( this->worker_threads ) ) )
			pthread_join( thread, NULL );

		// Destroy the mutexes
		pthread_mutex_destroy( &this->frame_queue_mutex );
		pthread_mutex_destroy( &this->done_queue_mutex );

		// Destroy the conditions
		pthread_cond_destroy( &this->frame_queue_cond );
		pthread_cond_destroy( &this->done_queue_cond );

		// Wipe the queues
		while ( mlt_deque_count( this->frame_queue ) )
			mlt_frame_close( mlt_deque_pop_back( this->frame_queue ) );
		while ( mlt_deque_count( this->done_queue ) )
			mlt_frame_close( mlt_deque_pop_back( this->done_queue ) );

		// Close the queues
		mlt_deque_close( this->frame_queue );
		mlt_deque_close( this->done_queue );
		mlt_deque_close( this->worker_threads );
	}
}

/** Flush the read/render thread's buffer.
 *
 * \public \memberof mlt_consumer_s
 * \param this a consumer
 */

void mlt_consumer_purge( mlt_consumer this )
{
	if ( this->ahead )
	{
		pthread_mutex_lock( &this->frame_queue_mutex );
		while ( mlt_deque_count( this->frame_queue ) )
			mlt_frame_close( mlt_deque_pop_back( this->frame_queue ) );
		pthread_cond_broadcast( &this->frame_queue_cond );
		pthread_mutex_unlock( &this->frame_queue_mutex );

		if ( this->done_queue )
		{
			pthread_mutex_lock( &this->done_queue_mutex );
			while ( mlt_deque_count( this->done_queue ) )
				mlt_frame_close( mlt_deque_pop_back( this->done_queue ) );
			pthread_cond_broadcast( &this->done_queue_cond );
			pthread_mutex_unlock( &this->done_queue_mutex );
		}
	}
}

/** Get the next frame from the producer connected to a consumer.
 *
 * Typically, one uses this instead of \p mlt_consumer_get_frame to make
 * the asynchronous/real-time behavior configurable at runtime.
 * You should close the frame returned from this when you are done with it.
 *
 * \public \memberof mlt_consumer_s
 * \param this a consumer
 * \return a frame
 */

mlt_frame mlt_consumer_rt_frame( mlt_consumer this )
{
	// Frame to return
	mlt_frame frame = NULL;

	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Check if the user has requested real time or not
	if ( this->real_time == 1 || this->real_time == -1 )
	{
		int size = 1;

		// Is the read ahead running?
		if ( this->ahead == 0 )
		{
			int buffer = mlt_properties_get_int( properties, "buffer" );
			int prefill = mlt_properties_get_int( properties, "prefill" );
			consumer_read_ahead_start( this );
			if ( buffer > 1 )
				size = prefill > 0 && prefill < buffer ? prefill : buffer;
		}

		// Get frame from queue
		pthread_mutex_lock( &this->frame_queue_mutex );
		while( this->ahead && mlt_deque_count( this->frame_queue ) < size )
			pthread_cond_wait( &this->frame_queue_cond, &this->frame_queue_mutex );
		frame = mlt_deque_pop_front( this->frame_queue );
		pthread_cond_broadcast( &this->frame_queue_cond );
		pthread_mutex_unlock( &this->frame_queue_mutex );
	}
	else if ( this->real_time > 1 || this->real_time < -1 )
	{
		// Use multiple worker threads and a work queue
		int size = abs( this->real_time );
		int buffer = mlt_properties_get_int( properties, "buffer" );
		buffer = buffer < size ? size : buffer;

		// Start worker threads if not already
		if ( ! this->ahead )
		{
			int prefill = mlt_properties_get_int( properties, "prefill" );
			prefill = prefill > 0 && prefill < buffer ? prefill : buffer;

			consumer_work_start( this );

			// Fill the work queue
			int i = buffer;
			while ( i-- )
			{
				frame = mlt_consumer_get_frame( this );
				if ( frame )
				{
					pthread_mutex_lock( &this->frame_queue_mutex );
					mlt_deque_push_back( this->frame_queue, frame );
					pthread_cond_signal( &this->frame_queue_cond );
					pthread_mutex_unlock( &this->frame_queue_mutex );
				}
			}

			// Wait for prefill
			pthread_mutex_lock( &this->done_queue_mutex );
			while( this->ahead && mlt_deque_count( this->done_queue ) < prefill )
				pthread_cond_wait( &this->done_queue_cond, &this->done_queue_mutex );
			pthread_mutex_unlock( &this->done_queue_mutex );
		}

		// Try to get frame from the done queue
		mlt_log_debug( MLT_CONSUMER_SERVICE(this), "size %d done count %d work count %d\n",
			size, mlt_deque_count( this->done_queue ), mlt_deque_count( this->frame_queue ) );
		if ( this->real_time > 0 && size == this->real_time && mlt_deque_count( this->done_queue ) <= size )
		{
			// Non-realtime and no ready frames
			frame = mlt_consumer_get_frame( this );
		}
		else
		{
			// Wait for work queue space
			pthread_mutex_lock( &this->frame_queue_mutex );
			while( this->ahead && mlt_deque_count( this->frame_queue ) >= buffer )
				pthread_cond_wait( &this->frame_queue_cond, &this->frame_queue_mutex );
			pthread_mutex_unlock( &this->frame_queue_mutex );

			// Feed the work queue
			frame = mlt_consumer_get_frame( this );
			if ( frame )
			{
				pthread_mutex_lock( &this->frame_queue_mutex );
				mlt_deque_push_back( this->frame_queue, frame );
				pthread_cond_signal( &this->frame_queue_cond );
				pthread_mutex_unlock( &this->frame_queue_mutex );
			}

			// Get the frame from the done queue
			pthread_mutex_lock( &this->done_queue_mutex );
			while( this->ahead && mlt_deque_count( this->done_queue ) <= size )
				pthread_cond_wait( &this->done_queue_cond, &this->done_queue_mutex );
			frame = mlt_deque_pop_front( this->done_queue );
			pthread_mutex_unlock( &this->done_queue_mutex );
		}
	}
	else
	{
		// Get the frame in non real time
		frame = mlt_consumer_get_frame( this );

		// This isn't true, but from the consumers perspective it is
		if ( frame != NULL )
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "rendered", 1 );
	}

	return frame;
}

/** Callback for the implementation to indicate a stopped condition.
 *
 * \public \memberof mlt_consumer_s
 * \param this a consumer
 */

void mlt_consumer_stopped( mlt_consumer this )
{
	mlt_properties_set_int( MLT_CONSUMER_PROPERTIES( this ), "running", 0 );
	mlt_events_fire( MLT_CONSUMER_PROPERTIES( this ), "consumer-stopped", NULL );
	mlt_event_unblock( this->event_listener );
}

/** Stop the consumer.
 *
 * \public \memberof mlt_consumer_s
 * \param this a consumer
 * \return true if there was an error
 */

int mlt_consumer_stop( mlt_consumer this )
{
	// Get the properies
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Just in case...
	mlt_log( MLT_CONSUMER_SERVICE( this ), MLT_LOG_DEBUG, "stopping put waiting\n" );
	pthread_mutex_lock( &this->put_mutex );
	this->put_active = 0;
	pthread_cond_broadcast( &this->put_cond );
	pthread_mutex_unlock( &this->put_mutex );

	// Stop the consumer
	mlt_log( MLT_CONSUMER_SERVICE( this ), MLT_LOG_DEBUG, "stopping consumer\n" );
	if ( this->stop != NULL )
		this->stop( this );

	// Check if the user has requested real time or not and stop if necessary
	mlt_log( MLT_CONSUMER_SERVICE( this ), MLT_LOG_DEBUG, "stopping read_ahead\n" );
	if ( abs( this->real_time ) == 1 )
		consumer_read_ahead_stop( this );
	else if ( abs( this->real_time ) > 1 )
		consumer_work_stop( this );

	// Kill the test card
	mlt_properties_set_data( properties, "test_card_producer", NULL, 0, NULL, NULL );

	// Check and run a post command
	if ( mlt_properties_get( properties, "post" ) )
		if (system( mlt_properties_get( properties, "post" ) ) == -1 )
			mlt_log( MLT_CONSUMER_SERVICE( this ), MLT_LOG_ERROR, "system(%s) failed!\n", mlt_properties_get( properties, "post" ) );

	mlt_log( MLT_CONSUMER_SERVICE( this ), MLT_LOG_DEBUG, "stopped\n" );

	return 0;
}

/** Determine if the consumer is stopped.
 *
 * \public \memberof mlt_consumer_s
 * \param this a consumer
 * \return true if the consumer is stopped
 */

int mlt_consumer_is_stopped( mlt_consumer this )
{
	// Check if the consumer is stopped
	if ( this->is_stopped != NULL )
		return this->is_stopped( this );

	return 0;
}

/** Close and destroy the consumer.
 *
 * \public \memberof mlt_consumer_s
 * \param this a consumer
 */

void mlt_consumer_close( mlt_consumer this )
{
	if ( this != NULL && mlt_properties_dec_ref( MLT_CONSUMER_PROPERTIES( this ) ) <= 0 )
	{
		// Get the childs close function
		void ( *consumer_close )( ) = this->close;

		if ( consumer_close )
		{
			// Just in case...
			//mlt_consumer_stop( this );

			this->close = NULL;
			consumer_close( this );
		}
		else
		{
			// Make sure it only gets called once
			this->parent.close = NULL;

			// Destroy the push mutex and condition
			pthread_mutex_destroy( &this->put_mutex );
			pthread_cond_destroy( &this->put_cond );

			mlt_service_close( &this->parent );
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
	return consumer->position;
}
		
