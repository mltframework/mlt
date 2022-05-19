/*
 * filter_freeze.c -- simple frame freezing filter
 * Copyright (C) 2022 Eoin O'Neill <eoinoneill1991@gmail.com>
 * Copyright (C) 2022 Emmet O'Neill <emmetoneill.pdx@gmail.com>
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_service.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_property.h>

#include <stdio.h>
#include <string.h>

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter self = mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( self );

	int frame_start = mlt_properties_get_int( properties, "frame_start" );
	int frame_end = mlt_properties_get_int( properties, "frame_end" );
	mlt_position pos = mlt_producer_get_in( mlt_frame_get_original_producer( frame ) );
	mlt_position currentpos = mlt_filter_get_position( self, frame );
	int real_frame_index = (currentpos % (frame_end - frame_start) + frame_start);

	mlt_service_lock( MLT_FILTER_SERVICE( self ) );

	//Get producer
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	mlt_producer_seek( producer, pos );

	//Get and set frame data
	mlt_frame real_frame = NULL;;
	mlt_service_get_frame( mlt_producer_service(producer), &real_frame, 0 );
	mlt_properties_set_position( properties, "_frame", real_frame_index );

	// Get real image from producer
	uint8_t *buffer = NULL;
	int error = mlt_frame_get_image( real_frame, &buffer, format, width, height, 1 );
	mlt_service_unlock( MLT_FILTER_SERVICE( self ) );

	// Copy its data to current frame
	int size = mlt_image_format_size( *format, *width, *height, NULL );
	uint8_t *image_copy = mlt_pool_alloc( size );
	memcpy( image_copy, buffer, size );
	*image = image_copy;
	mlt_frame_set_image( frame, *image, size, mlt_pool_release );

	uint8_t *alpha_buffer = mlt_frame_get_alpha( real_frame );
	if ( alpha_buffer )
	{
		int alphasize = *width * *height;
		uint8_t *alpha_copy = mlt_pool_alloc( alphasize );
		memcpy( alpha_copy, alpha_buffer, alphasize );
		mlt_frame_set_alpha( frame, alpha_copy, alphasize, mlt_pool_release );
	}

	return error;
}

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_filter self = mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( self );

	int frame_start = mlt_properties_get_int( properties, "frame_start" );
	int frame_end = mlt_properties_get_int( properties, "frame_end" );
	mlt_position currentpos = mlt_filter_get_position( self, frame );
	int real_frame_index = (currentpos % (frame_end - frame_start) + frame_start);


	mlt_service_lock( MLT_FILTER_SERVICE( self ) );

	//Get producer
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	mlt_frame_set_position( frame, real_frame_index );
	mlt_frame real_frame = NULL;
	mlt_service_get_frame( MLT_SERVICE(producer), &real_frame, real_frame_index);
	mlt_frame_get_audio(real_frame, buffer, format, frequency, channels, samples);
	mlt_service_unlock( MLT_FILTER_SERVICE(self) );

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter self, mlt_frame frame )
{

	// Push the filter on to the stack
	//mlt_frame_push_service( frame, self);

	// Push the frame filter
	//mlt_frame_push_get_image( frame, filter_get_image );

	mlt_frame_push_service( frame, self );
	mlt_frame_push_audio( frame, filter_get_audio );

// 	mlt_properties properties = MLT_FILTER_PROPERTIES( self );
//
// 	int frame_start = mlt_properties_get_int( properties, "frame_start" );
// 	int frame_end = mlt_properties_get_int( properties, "frame_end" );
//
// 	mlt_position position = mlt_frame_get_position( frame );
// 	mlt_position real_frame_index = (position % (frame_end - frame_start) + frame_start);
// 	mlt_frame_set_position( frame, real_frame_index );


	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_framerange_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		// Set the frame which will be chosen for freeze
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "frame_start", "0" );

		// If freeze_after = 1, only frames after the "frame" value will be frozen
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "frame_end", "100" );

	}
	return filter;
}



mlt_producer producer_timewarp_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create a new producer object
	mlt_producer producer = mlt_producer_new( profile );
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if ( arg && producer && pdata )
	{
		double frame_rate_num_scaled;
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
		pdata->pitch_filter = NULL;

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
		frame_rate_num_scaled = (double)pdata->clip_profile->frame_rate_num / fabs(pdata->speed);
		if (frame_rate_num_scaled > INT_MAX) // Check for overflow in case speed < 1.0
		{
			//scale by denominator to avoid overflow.
			pdata->clip_profile->frame_rate_den = (double)pdata->clip_profile->frame_rate_den * fabs(pdata->speed);
		}
		else
		{
			//scale by numerator
			pdata->clip_profile->frame_rate_num = frame_rate_num_scaled;
		}

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
			mlt_repository repository = mlt_factory_repository();
			mlt_properties clip_metadata = mlt_repository_metadata( repository, mlt_service_producer_type, mlt_properties_get( clip_properties, "mlt_service" ) );
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

