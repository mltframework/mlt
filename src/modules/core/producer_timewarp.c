/*
 * producer_timewarp.c -- modify speed and direction of a clip
 * Copyright (C) 2015-2016 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#include <framework/mlt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Private Types
typedef struct
{
	int first_frame;
	double speed;
	int reverse;
	mlt_producer clip_producer;
	mlt_profile clip_profile;
	mlt_properties clip_parameters;
} private_data;

// Private Functions

static void timewarp_property_changed( mlt_service owner, mlt_producer producer, char *name )
{
	private_data* pdata = (private_data*)producer->child;
	if ( mlt_properties_get_int( pdata->clip_parameters, name ) ||
		 !strcmp( name, "length" ) ||
		 !strcmp( name, "in" ) ||
		 !strcmp( name, "out" ) ||
		 !strcmp( name, "ignore_points" ) ||
		 !strcmp( name, "eof" ) ||
		 !strncmp( name, "meta.", 5 ) )
	{
		// Pass parameter changes from this producer to the encapsulated clip
		// producer.
		mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
		mlt_properties clip_properties = MLT_PRODUCER_PROPERTIES( pdata->clip_producer );
		mlt_events_block( clip_properties, producer );
		mlt_properties_pass_property( clip_properties, producer_properties, name );
		mlt_events_unblock( clip_properties, producer );
	}
}

static void clip_property_changed( mlt_service owner, mlt_producer producer, char *name )
{
	private_data* pdata = (private_data*)producer->child;
	if ( mlt_properties_get_int( pdata->clip_parameters, name ) ||
		 !strcmp( name, "length" ) ||
		 !strcmp( name, "in" ) ||
		 !strcmp( name, "out" ) ||
		 !strcmp( name, "ignore_points" ) ||
		 !strcmp( name, "eof" ) ||
		 !strncmp( name, "meta.", 5 ) )
	{
		// The encapsulated clip producer might change its own parameters.
		// Pass those changes to this producer.
		mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
		mlt_properties clip_properties = MLT_PRODUCER_PROPERTIES( pdata->clip_producer );
		mlt_events_block( producer_properties, producer );
		mlt_properties_pass_property( producer_properties, clip_properties, name );
		mlt_events_unblock( producer_properties, producer );
	}
}

static int producer_get_audio( mlt_frame frame, void** buffer, mlt_audio_format* format, int* frequency, int* channels, int* samples )
{
	mlt_producer producer = mlt_frame_pop_audio( frame );
	private_data* pdata = (private_data*)producer->child;

	int error = mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	// Scale the frequency to account for the speed change.
	// The resample normalizer will convert it to the requested frequency
	*frequency = (double)*frequency * fabs(pdata->speed);

	if( pdata->speed < 0.0 )
	{
		// Reverse the audio in this frame
		int c = 0;
		switch ( *format )
		{
			// Interleaved 8bit formats
			case mlt_audio_u8:
			{
				int8_t tmp;
				for ( c = 0; c < *channels; c++ )
				{
					// Pointer to first sample
					int8_t* a = (int8_t*)*buffer + c;
					// Pointer to last sample
					int8_t* b = (int8_t*)*buffer + ((*samples - 1) * *channels) + c;
					while( a < b )
					{
						tmp = *a;
						*a = *b;
						*b = tmp;
						a += *channels;
						b -= *channels;
					}
				}
				break;
			}
			// Interleaved 16bit formats
			case mlt_audio_s16:
			{
				int16_t tmp;
				for ( c = 0; c < *channels; c++ )
				{
					// Pointer to first sample
					int16_t *a = (int16_t*)*buffer + c;
					// Pointer to last sample
					int16_t *b = (int16_t*)*buffer + ((*samples - 1) * *channels) + c;
					while( a < b )
					{
						tmp = *a;
						*a = *b;
						*b = tmp;
						a += *channels;
						b -= *channels;
					}
				}
				break;
			}
			// Interleaved 32bit formats
			case mlt_audio_s32le:
			case mlt_audio_f32le:
			{
				int32_t tmp;
				for ( c = 0; c < *channels; c++ )
				{
					// Pointer to first sample
					int32_t *a = (int32_t*)*buffer + c;
					// Pointer to last sample
					int32_t *b = (int32_t*)*buffer + ((*samples - 1)* *channels) + c;
					while( a < b )
					{
						tmp = *a;
						*a = *b;
						*b = tmp;
						a += *channels;
						b -= *channels;
					}
				}
				break;
			}
			// Non-Interleaved 32bit formats
			case mlt_audio_s32:
			case mlt_audio_float:
			{
				int32_t tmp;
				for ( c = 0; c < *channels; c++ )
				{
					// Pointer to first sample
					int32_t *a = (int32_t*)*buffer + (c * *samples);
					// Pointer to last sample
					int32_t *b = (int32_t*)*buffer + ((c + 1) * *samples) - 1;
					while( a < b )
					{
						tmp = *a;
						*a = *b;
						*b = tmp;
						a++;
						b--;
					}
				}
				break;
			}
			case mlt_audio_none:
				break;
			default:
				mlt_log_error( MLT_PRODUCER_SERVICE(producer),
						"Unknown Audio Format %s\n",
						mlt_audio_format_name( *format ) );
				break;
		}
	}

	return error;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	private_data* pdata = (private_data*)producer->child;
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	if( pdata->first_frame && pdata->clip_producer )
	{
		// Pass parameters from this producer to the clip producer
		// Properties that are set after initialization are not always caught by
		// the property_changed event - so do this once after init.
		int n = mlt_properties_count( pdata->clip_parameters );
		int i = 0;
		mlt_properties clip_properties = MLT_PRODUCER_PROPERTIES( pdata->clip_producer );

		mlt_events_block( clip_properties, producer );
		for ( i = 0; i < n; i++ )
		{
			char* name = mlt_properties_get_name( pdata->clip_parameters, i );
			if( mlt_properties_get_int( clip_properties, name ) &&
				mlt_properties_get( producer_properties, name ) )
			{
				mlt_properties_pass_property( clip_properties, producer_properties, name );
			}
		}
		mlt_events_unblock( clip_properties, producer );

		pdata->first_frame = 0;
	}

	if( pdata->clip_producer )
	{
		// Seek the clip producer to the appropriate position
		mlt_position clip_position = mlt_producer_position( producer );
		if( pdata->speed < 0.0 )
		{
			clip_position = mlt_properties_get_int( producer_properties, "out" ) - clip_position;
		}
		mlt_producer_seek( pdata->clip_producer, clip_position );

		// Get the frame from the clip producer
		mlt_service_get_frame( MLT_PRODUCER_SERVICE( pdata->clip_producer), frame, index );

		// Configure callbacks
		if ( !mlt_frame_is_test_audio( *frame ) )
		{
			mlt_frame_push_audio( *frame, producer );
			mlt_frame_push_audio( *frame, producer_get_audio );
		}
	}
	else
	{
		*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );
	}

	// Set the correct position on the frame
	mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

	// Calculate the next time code
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer producer )
{
	private_data* pdata = (private_data*)producer->child;

	if ( pdata )
	{
		mlt_producer_close( pdata->clip_producer );
		mlt_profile_close( pdata->clip_profile );
		mlt_properties_close( pdata->clip_parameters );
		free( pdata );
	}

	producer->child = NULL;
	producer->close = NULL;
	mlt_producer_close( producer );
	free( producer );
}

mlt_producer producer_timewarp_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create a new producer object
	mlt_producer producer = mlt_producer_new( profile );
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if ( arg && producer && pdata )
	{
		mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

		// Initialize the producer
		mlt_properties_set( producer_properties, "resource", arg );
		producer->child = pdata;
		producer->get_frame = producer_get_frame;
		producer->close = (mlt_destructor)producer_close;

		// Get the resource to be passed to the clip producer
		char* resource = strchr( arg, ':' );
		if ( resource == NULL )
			resource = arg; // Apparently speed was not specified.
		else
			resource++; // move past the delimiter.

		// Initialize private data
		pdata->first_frame = 1;
		pdata->speed = atof( arg );
		if( pdata->speed == 0.0 )
		{
			pdata->speed = 1.0;
		}
		pdata->clip_profile = NULL;
		pdata->clip_parameters = NULL;
		pdata->clip_producer = NULL;

		// Create a false profile to be used by the clip producer.
		pdata->clip_profile = mlt_profile_clone( mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) ) );
		// Frame rate must be recalculated for the clip profile to change the time base.
		if( pdata->clip_profile->frame_rate_num < 1000 )
		{
			// Scale the frame rate fraction so we keep more accuracy when
			// the speed is factored in.
			pdata->clip_profile->frame_rate_num *= 1000;
			pdata->clip_profile->frame_rate_den *= 1000;
		}
		pdata->clip_profile->frame_rate_num = (double)pdata->clip_profile->frame_rate_num / fabs(pdata->speed);

		// Create a producer for the clip using the false profile.
		pdata->clip_producer = mlt_factory_producer( pdata->clip_profile, "abnormal", resource );

		if( pdata->clip_producer )
		{
			mlt_properties clip_properties = MLT_PRODUCER_PROPERTIES( pdata->clip_producer );
			int n = 0;
			int i = 0;

			// Set the speed to 0 since we will control the seeking
			mlt_producer_set_speed( pdata->clip_producer, 0 );

			// Create a list of all parameters used by the clip producer so that
			// they can be passed between the clip producer and this producer.
			pdata->clip_parameters = mlt_properties_new();
			mlt_properties_init( pdata->clip_parameters, NULL );
			mlt_repository repository = mlt_factory_repository();
			mlt_properties clip_metadata = mlt_repository_metadata( repository, producer_type, mlt_properties_get( clip_properties, "mlt_service" ) );
			if ( clip_metadata )
			{
				mlt_properties params = (mlt_properties) mlt_properties_get_data( clip_metadata, "parameters", NULL );
				if ( params )
				{
					n = mlt_properties_count( params );
					for ( i = 0; i < n; i++ )
					{
						mlt_properties param = (mlt_properties) mlt_properties_get_data( params, mlt_properties_get_name( params, i ), NULL );
						char* identifier = mlt_properties_get( param, "identifier" );
						if ( identifier )
						{
							// Set the value to 1 to indicate the parameter exists.
							mlt_properties_set_int( pdata->clip_parameters, identifier, 1 );
						}
					}
					// Explicitly exclude the "resource" parameter since it needs to  be different.
					mlt_properties_set_int( pdata->clip_parameters, "resource", 0 );
				}
			}

			// Pass parameters and properties from the clip producer to this producer.
			// Some properties may have been set during initialization.
			n = mlt_properties_count( clip_properties );
			for ( i = 0; i < n; i++ )
			{
				char* name = mlt_properties_get_name( clip_properties, i );
				if ( mlt_properties_get_int( pdata->clip_parameters, name ) ||
					 !strcmp( name, "length" ) ||
					 !strcmp( name, "in" ) ||
					 !strcmp( name, "out" ) ||
					 !strncmp( name, "meta.", 5 ) )
				{
					mlt_properties_pass_property( producer_properties, clip_properties, name );
				}
			}

			// Initialize warp producer properties
			mlt_properties_set_double( producer_properties, "warp_speed", pdata->speed );
			mlt_properties_set( producer_properties, "warp_resource", mlt_properties_get( clip_properties, "resource" ) );

			// Monitor property changes from both producers so that the clip
			// parameters can be passed back and forth.
			mlt_events_listen( clip_properties, producer, "property-changed", ( mlt_listener )clip_property_changed );
			mlt_events_listen( producer_properties, producer, "property-changed", ( mlt_listener )timewarp_property_changed );
		}
	}

	if ( !producer || !pdata || !pdata->clip_producer )
	{
		if ( pdata )
		{
			mlt_producer_close( pdata->clip_producer );
			mlt_profile_close( pdata->clip_profile );
			mlt_properties_close( pdata->clip_parameters );
			free( pdata );
		}

		if ( producer )
		{
			producer->child = NULL;
			producer->close = NULL;
			mlt_producer_close( producer );
			free( producer );
			producer = NULL;
		}
	}

	return producer;
}
