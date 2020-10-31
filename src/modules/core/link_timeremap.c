/*
 * link_timeremap.c
 * Copyright (C) 2020 Meltytech, LLC
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

#include <framework/mlt_link.h>
#include <framework/mlt_log.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void link_configure( mlt_link self, mlt_profile default_profile )
{
	mlt_service_set_profile( MLT_LINK_SERVICE( self ), default_profile );
}

static int link_get_audio( mlt_frame frame, void** audio, mlt_audio_format* format, int* frequency, int* channels, int* samples )
{
	mlt_link self = (mlt_link)mlt_frame_pop_audio( frame );
	mlt_properties unique_properties = mlt_frame_get_unique_properties( frame, MLT_LINK_SERVICE(self) );
	if ( !unique_properties )
	{
		return 1;
	}
	double source_time = mlt_properties_get_double( unique_properties, "source_time" );
	double source_duration = mlt_properties_get_double( unique_properties, "source_duration" );
	double source_fps = mlt_properties_get_double( unique_properties, "source_fps" );
	double link_fps = mlt_producer_get_fps( MLT_LINK_PRODUCER( self ) );
	double frame_duration = 1.0 / link_fps;
	double speed = 0.0;
	if ( source_duration != 0.0 )
	{
		speed = fabs(source_duration) / frame_duration;
	}

	// Validate the request
	*channels = *channels <= 0 ? 2 : *channels;
	int src_frequency = mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "audio_frequency" );
	if ( src_frequency > 0 )
	{
		*frequency = src_frequency;
	}
	else if ( *frequency <= 0 )
	{
		*frequency = 48000;
	}

	if ( speed < 0.1 || speed > 10 )
	{
		// Return silent samples for speeds less than 0.1 or > 10
		mlt_position position = mlt_frame_original_position( frame );
		*samples = mlt_sample_calculator( link_fps, *frequency, position );
		int size = mlt_audio_format_size( *format, *samples, *channels );
		*audio = mlt_pool_alloc( size );
		memset( *audio, 0, size );
		mlt_frame_set_audio( frame, *audio, *format, size, mlt_pool_release );
		return 0;
	}
	else
	{
		int sample_count = mlt_sample_calculator( link_fps, *frequency, mlt_frame_get_position( frame ) );
		sample_count = lrint( (double)sample_count * speed );
		mlt_position in_frame_pos = floor( source_time * source_fps );

		// Calculate the samples to get from the input frames
		int64_t first_out_sample = source_time * (double)*frequency;
		int64_t first_in_sample = mlt_sample_calculator_to_now( source_fps, *frequency, in_frame_pos );
		int samples_to_skip = first_out_sample - first_in_sample;
		if ( samples_to_skip < 0 )
		{
			mlt_log_error( MLT_LINK_SERVICE(self), "Audio too late: %d\t%d\n", (int)first_out_sample, (int)first_in_sample );
			samples_to_skip = 0;
		}

		// Allocate the out buffer
		struct mlt_audio_s out;
		mlt_audio_set_values( &out, NULL, *frequency, *format, sample_count, *channels );
		mlt_audio_alloc_data( &out );

		// Copy audio from the input frames to the output buffer
		int samples_copied = 0;
		int samples_needed = sample_count;

		while ( samples_needed > 0 )
		{
			char key[19];
			sprintf( key, "%d", in_frame_pos );
			mlt_frame src_frame = (mlt_frame)mlt_properties_get_data( unique_properties, key, NULL );
			if ( !src_frame )
			{
				mlt_log_error( MLT_LINK_SERVICE(self), "Frame not found: %d\n", in_frame_pos );
				break;
			}

			int in_samples = mlt_sample_calculator( source_fps, *frequency, in_frame_pos );
			struct mlt_audio_s in;
			mlt_audio_set_values( &in, NULL, *frequency, *format, in_samples, *channels );

			int error = mlt_frame_get_audio( src_frame, &in.data, &in.format, &in.frequency, &in.channels, &in.samples );
			if ( error )
			{
				mlt_log_error( MLT_LINK_SERVICE(self), "No audio: %d\n", in_frame_pos );
				break;
			}

			int samples_to_copy = in.samples - samples_to_skip;
			if ( samples_to_copy > samples_needed )
			{
				samples_to_copy = samples_needed;
			}
			mlt_log_debug( MLT_LINK_SERVICE(self), "Copy: %d\t%d\t%d\t%d\n", samples_to_skip, samples_to_skip + samples_to_copy -1, samples_to_copy, in.samples );

			if ( samples_to_copy > 0 )
			{
				mlt_audio_copy( &out, &in, samples_to_copy, samples_to_skip, samples_copied );
				samples_copied += samples_to_copy;
				samples_needed -= samples_to_copy;
			}

			samples_to_skip = 0;
			in_frame_pos++;
		}

		if ( samples_copied != sample_count )
		{
			mlt_log_error( MLT_LINK_SERVICE(self), "Sample under run: %d\t%d\n", samples_copied, sample_count );
			mlt_audio_shrink( &out , samples_copied );
		}

		if ( source_duration < 0.0 )
		{
			// Going backwards
			mlt_audio_reverse( &out );
		}

		out.frequency = lrint( (double)out.frequency * speed );
		mlt_frame_set_audio( frame, out.data, out.format, 0, out.release_data );
		mlt_audio_get_values( &out, audio, frequency, format, samples, channels );
		return 0;
	}

	return 1;
}

static int link_get_image_blend( mlt_frame frame, uint8_t** image, mlt_image_format* format, int* width, int* height, int writable )
{
	static const int MAX_BLEND_IMAGES = 10;
	mlt_link self = (mlt_link)mlt_frame_pop_get_image( frame );
	mlt_properties unique_properties = mlt_frame_get_unique_properties( frame, MLT_LINK_SERVICE(self) );
	if ( !unique_properties )
	{
		return 1;
	}
	double source_time = mlt_properties_get_double( unique_properties, "source_time");
	double source_fps = mlt_properties_get_double( unique_properties, "source_fps");

	// Get pointers to all the images for this frame
	uint8_t* images[MAX_BLEND_IMAGES];
	int image_count = 0;
	mlt_position in_frame_pos = floor( source_time * source_fps );
	while ( image_count < MAX_BLEND_IMAGES )
	{
		char key[19];
		sprintf( key, "%d", in_frame_pos );
		mlt_frame src_frame = (mlt_frame)mlt_properties_get_data( unique_properties, key, NULL );
		if ( src_frame && !mlt_frame_get_image( src_frame, &images[image_count], format, width, height, 0 ) )
		{
			in_frame_pos++;
			image_count++;
		}
		else
		{
			break;
		}
	}

	if ( image_count <= 0 )
	{
		return 1;
	}

	// Sum all the images into one image with 16 bit components
	int size = mlt_image_format_size( *format, *width, *height, NULL );
	*image = mlt_pool_alloc( size );
	int s = 0;
	uint8_t* p = *image;
	for( s = 0; s < size; s++ )
	{
		int16_t sum = 0;
		int i = 0;
		for( i = 0; i < image_count; i++ )
		{
			sum += *(images[i]);
			images[i]++;
		}
		*p = sum / image_count;
		p++;
	}
	mlt_frame_set_image( frame, *image, size, mlt_pool_release );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES(frame), "format", *format );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES(frame), "width", *width );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES(frame), "height", *height );

	return 0;
}

static int link_get_image_nearest( mlt_frame frame, uint8_t** image, mlt_image_format* format, int* width, int* height, int writable )
{
	mlt_link self = (mlt_link)mlt_frame_pop_get_image( frame );
	mlt_properties unique_properties = mlt_frame_get_unique_properties( frame, MLT_LINK_SERVICE(self) );
	if ( !unique_properties )
	{
		return 1;
	}
	double source_time = mlt_properties_get_double( unique_properties, "source_time");
	double source_fps = mlt_properties_get_double( unique_properties, "source_fps");
	mlt_position in_frame_pos = floor( source_time * source_fps );
	char key[19];
	sprintf( key, "%d", in_frame_pos );

	mlt_frame src_frame = (mlt_frame)mlt_properties_get_data( unique_properties, key, NULL );
	if ( src_frame )
	{
		uint8_t* in_image;
		if ( !mlt_frame_get_image( src_frame, &in_image, format, width, height, 0 ) )
		{
			int size = mlt_image_format_size( *format, *width, *height, NULL );
			*image = mlt_pool_alloc( size );
			memcpy( *image, in_image, size );
			mlt_frame_set_image( frame, *image, size, mlt_pool_release );
			mlt_properties_set_int( MLT_FRAME_PROPERTIES(frame), "format", *format );
			mlt_properties_set_int( MLT_FRAME_PROPERTIES(frame), "width", *width );
			mlt_properties_set_int( MLT_FRAME_PROPERTIES(frame), "height", *height );

			uint8_t* in_alpha = mlt_frame_get_alpha( src_frame );
			if ( in_alpha )
			{
				size = *width * *height;
				uint8_t* out_alpha = mlt_pool_alloc( size );
				memcpy( out_alpha, in_alpha, size );
				mlt_frame_set_alpha( frame, out_alpha, size, mlt_pool_release );
			};
			return 0;
		}
	}

	return 1;
}

static int link_get_frame( mlt_link self, mlt_frame_ptr frame, int index )
{
	mlt_properties properties = MLT_LINK_PROPERTIES( self );
	mlt_position position = mlt_producer_position( MLT_LINK_PRODUCER( self ) );
	mlt_position length = mlt_producer_get_length( MLT_LINK_PRODUCER( self ) );
	double source_time = 0.0;
	double source_duration = 0.0;
	double source_fps = mlt_producer_get_fps( self->next );

	int result = 0;

	// Create a  frame
	*frame = mlt_frame_init( MLT_LINK_SERVICE(self) );
	mlt_frame_set_position( *frame, mlt_producer_position( MLT_LINK_PRODUCER(self) ) );
	mlt_properties unique_properties = mlt_frame_unique_properties( *frame, MLT_LINK_SERVICE(self) );

	// Calculate the frames from the next link to be used
	if ( !mlt_properties_exists( properties, "map" ) )
	{
		double link_fps = mlt_producer_get_fps( MLT_LINK_PRODUCER( self ) );
		source_time = (double)position / link_fps;
		source_duration = 1.0 / link_fps;
	}
	else
	{
		source_time = mlt_properties_anim_get_double( properties, "map", position, length );
		double next_source_time = mlt_properties_anim_get_double( properties, "map", position + 1, length );
		source_duration = next_source_time - source_time;
	}

	mlt_properties_set_double( unique_properties, "source_fps", source_fps );
	mlt_properties_set_double( unique_properties, "source_time", source_time );
	mlt_properties_set_double( unique_properties, "source_duration", source_duration );

	mlt_log_debug( MLT_LINK_SERVICE(self), "Get Frame: %f -> %f\t%d\n", source_fps, mlt_producer_get_fps( MLT_LINK_PRODUCER(self) ), position );

	// Get frames from the next link and pass them along with the new frame
	int in_frame_count = 0;
	mlt_frame src_frame = NULL;
	mlt_frame prev_frame = mlt_properties_get_data( properties, "_prev_frame", NULL );
	mlt_position prev_frame_position = prev_frame ? mlt_frame_get_position( prev_frame ) : -1;
	mlt_position in_frame_pos = floor( source_time * source_fps );
	double frame_time = (double)in_frame_pos / source_fps;
	double source_end_time = source_time + fabs(source_duration);
	if ( frame_time == source_end_time )
	{
		// Force one frame to be sent.
		source_end_time += 0.0000000001;
	}
	while ( frame_time < source_end_time )
	{
		if( in_frame_pos == prev_frame_position )
		{
			// Reuse the previous frame to avoid seeking.
			src_frame = prev_frame;
			mlt_properties_inc_ref( MLT_FRAME_PROPERTIES(src_frame) );
		}
		else
		{
			mlt_producer_seek( self->next, in_frame_pos );
			result = mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), &src_frame, index );
			if ( result )
			{
				break;
			}
		}
		// Save the source frame on the output frame
		char key[19];
		sprintf( key, "%d", in_frame_pos );
		mlt_properties_set_data( unique_properties, key, src_frame, 0, (mlt_destructor)mlt_frame_close, NULL );

		// Calculate the next frame
		in_frame_pos++;
		frame_time = (double)in_frame_pos / source_fps;
		in_frame_count++;
	}

	if ( !src_frame )
	{
		mlt_frame_close( *frame );
		*frame = NULL;
		return 1;
	}

	// Copy some useful properties from one of the source frames.
	(*frame)->convert_image = src_frame->convert_image;
	(*frame)->convert_audio = src_frame->convert_audio;
	mlt_properties_pass_list( MLT_FRAME_PROPERTIES(*frame), MLT_FRAME_PROPERTIES(src_frame), "audio_frequency" );

	if ( src_frame != prev_frame )
	{
		// Save the last source frame because it might be requested for the next frame.
		mlt_properties_inc_ref( MLT_FRAME_PROPERTIES(src_frame) );
		mlt_properties_set_data( properties, "_prev_frame", src_frame, 0, (mlt_destructor)mlt_frame_close, NULL );
	}

	// Setup callbacks
	char* mode = mlt_properties_get( properties, "image_mode" );
	mlt_frame_push_get_image( *frame, (void*)self );
	if ( in_frame_count == 1 || !mode || !strcmp( mode, "nearest" ) )
	{
		mlt_frame_push_get_image( *frame, link_get_image_nearest );
	}
	else
	{
		mlt_frame_push_get_image( *frame, link_get_image_blend );
	}

	mlt_frame_push_audio( *frame, (void*)self );
	mlt_frame_push_audio( *frame, link_get_audio );

	// Apply a resampler
	mlt_filter resampler = (mlt_filter)mlt_properties_get_data( properties, "_resampler", NULL );
	if ( !resampler )
	{
		resampler = mlt_factory_filter( mlt_service_profile( MLT_LINK_SERVICE(self) ), "resample", NULL );
		if ( !resampler )
		{
			resampler = mlt_factory_filter( mlt_service_profile( MLT_LINK_SERVICE(self) ), "swresample", NULL );
		}
		if( resampler )
		{
			mlt_properties_set_data( properties, "_resampler", resampler, 0, (mlt_destructor)mlt_filter_close, NULL );
		}
	}
	if ( resampler )
	{
		mlt_filter_process( resampler, *frame );
	}

	mlt_producer_prepare_next( MLT_LINK_PRODUCER( self ) );

	return result;
}

static void link_close( mlt_link self )
{
	self->close = NULL;
	mlt_link_close( self );
	free( self );
}

mlt_link link_timeremap_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_link self = mlt_link_init();
	if ( self != NULL )
	{
		// Callback registration
		self->configure = link_configure;
		self->get_frame = link_get_frame;
		self->close = link_close;
	}
	return self;
}
