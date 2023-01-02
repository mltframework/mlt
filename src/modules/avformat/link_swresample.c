/*
 * link_swresample.c
 * Copyright (C) 2022 Meltytech, LLC
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

#include "common.h"
#include "common_swr.h"

#include <framework/mlt_link.h>
#include <framework/mlt_log.h>
#include <framework/mlt_frame.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Private Types
#define FRAME_CACHE_SIZE 3

typedef struct
{
	// Used by get_frame
	mlt_frame frame_cache[FRAME_CACHE_SIZE];
	// Used by get_audio
	mlt_position expected_frame;
	mlt_position continuity_frame;
	mlt_swr_private_data swr;
} private_data;

static void link_configure( mlt_link self, mlt_profile chain_profile )
{
	mlt_service_set_profile( MLT_LINK_SERVICE( self ), chain_profile );
}

static int link_get_audio( mlt_frame frame, void** buffer, mlt_audio_format* format, int* frequency, int* channels, int* samples )
{
	int requested_frequency = *frequency <= 0 ? 48000 : *frequency;
	int requested_samples = *samples;
	mlt_link self = (mlt_link)mlt_frame_pop_audio( frame );
	private_data* pdata = (private_data*)self->child;

	// Validate the request
	*channels = *channels <= 0 ? 2 : *channels;

	int src_frequency = mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "audio_frequency" );
	src_frequency = src_frequency <= 0 ? *frequency : src_frequency;
	int src_samples = mlt_audio_calculate_frame_samples( mlt_producer_get_fps( MLT_LINK_PRODUCER( self ) ), src_frequency, mlt_frame_get_position( frame ) );
	int src_channels = mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "audio_channels" );
	src_channels = src_channels <= 0 ? *channels : src_channels;

	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	struct mlt_audio_s in;
	struct mlt_audio_s out;

	mlt_audio_set_values( &in, *buffer, src_frequency, *format, src_samples, src_channels );
	mlt_audio_set_values( &out, NULL, requested_frequency, *format, requested_samples, *channels );

	// Get the producer's audio
	int error = mlt_frame_get_audio( frame, &in.data, &in.format, &in.frequency, &in.channels, &in.samples );
	if ( error ||
			in.format == mlt_audio_none || out.format == mlt_audio_none ||
			in.frequency <= 0 || out.frequency <= 0 ||
			in.channels <= 0 || out.channels <= 0 )
	{
		// Error situation. Do not attempt to convert.
		mlt_audio_get_values( &in, buffer, frequency, format, samples, channels );
		mlt_log_error( MLT_LINK_SERVICE(self), "Invalid Parameters: %dS - %dHz %dC %s -> %dHz %dC %s\n", in.samples, in.frequency, in.channels, mlt_audio_format_name( in.format ), out.frequency, out.channels, mlt_audio_format_name( out.format ) );
		return error;
	}

	if (in.samples == 0)
	{
		// Noting to convert.
		return error;
	}

	// Determine the input/output channel layout.
	in.layout = mlt_get_channel_layout_or_default( mlt_properties_get( frame_properties, "channel_layout" ), in.channels );
	out.layout = mlt_get_channel_layout_or_default( mlt_properties_get( frame_properties, "consumer.channel_layout" ), out.channels );

	if( in.format == out.format &&
		in.frequency == out.frequency &&
		in.channels == out.channels &&
		in.layout == out.layout )
	{
		// No change necessary
		mlt_audio_get_values( &in, buffer, frequency, format, samples, channels );
		return error;
	}

	mlt_service_lock( MLT_LINK_SERVICE(self) );

	// Detect configuration change
	if( !pdata->swr.ctx ||
		pdata->swr.in_format != in.format ||
		pdata->swr.out_format != out.format ||
		pdata->swr.in_frequency != in.frequency ||
		pdata->swr.out_frequency != out.frequency ||
		pdata->swr.in_channels != in.channels ||
		pdata->swr.out_channels != out.channels ||
		pdata->swr.in_layout != in.layout ||
		pdata->swr.out_layout != out.layout ||
		pdata->expected_frame != mlt_frame_get_position( frame ) )
	{
		// Save the configuration
		pdata->swr.in_format = in.format;
		pdata->swr.out_format = out.format;
		pdata->swr.in_frequency = in.frequency;
		pdata->swr.out_frequency = out.frequency;
		pdata->swr.in_channels = in.channels;
		pdata->swr.out_channels = out.channels;
		pdata->swr.in_layout = in.layout;
		pdata->swr.out_layout = out.layout;
		// Reconfigure the context
		error = mlt_configure_swr_context( MLT_LINK_SERVICE(self), &pdata->swr );
		pdata->continuity_frame = mlt_frame_get_position( frame );
		pdata->expected_frame = mlt_frame_get_position( frame );
	}

	if( !error )
	{
		int total_received_samples = 0;
		out.samples = requested_samples;
		mlt_audio_alloc_data( &out );

		if ( pdata->continuity_frame == mlt_frame_get_position( frame ) )
		{
			// This is the nominal case when sample rate is not changing
			mlt_audio_get_planes( &in, pdata->swr.in_buffers );
			mlt_audio_get_planes( &out, pdata->swr.out_buffers );
			total_received_samples = swr_convert( pdata->swr.ctx, pdata->swr.out_buffers, out.samples, (const uint8_t**)pdata->swr.in_buffers, in.samples );
			if( total_received_samples < 0 )
			{
				mlt_log_error( MLT_LINK_SERVICE(self), "swr_convert() failed. Needed: %d\tIn: %d\tOut: %d\n", out.samples, in.samples, total_received_samples );
				error = 1;
			}
			pdata->continuity_frame++;
		}

		while ( total_received_samples < requested_samples && !error )
		{
			// The input frame is insufficient to fill the output frame.
			// This happens when sample rate conversion is occurring.
			// Request data from future frames.
			mlt_properties unique_properties = mlt_frame_get_unique_properties( frame, MLT_LINK_SERVICE(self) );
			if ( !unique_properties )
			{
				error = 1;
				break;
			}
			char key[19];
			sprintf( key, "%d", pdata->continuity_frame );
			mlt_frame src_frame = (mlt_frame)mlt_properties_get_data( unique_properties, key, NULL );
			if ( !src_frame )
			{
				mlt_log_error( MLT_LINK_SERVICE(self), "Frame not found: %s\n", key );
				break;
			}

			//  Get the audio from the in frame
			in.samples = mlt_audio_calculate_frame_samples( mlt_producer_get_fps( MLT_LINK_PRODUCER( self ) ), in.frequency, pdata->continuity_frame );
			error = mlt_frame_get_audio( src_frame, &in.data, &in.format, &in.frequency, &in.channels, &in.samples );
			if (error)
			{
				break;
			}

			// Set up the SWR buffer for the audio from the in frame
			mlt_audio_get_planes( &in, pdata->swr.in_buffers );

			// Set up the SWR buffer for the audio from the out frame,
			// shifting according to what has already been received.
			int plane_count = mlt_audio_plane_count( &out );
			int plane_size = mlt_audio_plane_size( &out );
			int out_step_size = plane_size / out.samples;
			int p = 0;
			for( p = 0; p < plane_count; p++ )
			{
				uint8_t* pAudio = (uint8_t*)out.data + (out_step_size * total_received_samples);
				pdata->swr.out_buffers[p] = pAudio + ( p * plane_size );
			}

			int samples_needed = requested_samples - total_received_samples;
			int received_samples = swr_convert( pdata->swr.ctx, pdata->swr.out_buffers, samples_needed, (const uint8_t**)pdata->swr.in_buffers, in.samples );
			if( received_samples < 0 )
			{
				mlt_log_error( MLT_LINK_SERVICE(self), "swr_convert() failed. Needed: %d\tIn: %d\tOut: %d\n", samples_needed, in.samples, received_samples );
				error = 1;
			} else {
				total_received_samples += received_samples;
			}
			pdata->continuity_frame++;
		}

		if ( total_received_samples == 0 )
		{
			mlt_log_info( MLT_LINK_SERVICE(self), "Failed to get any samples - return silence\n" );
			mlt_audio_silence( &out, out.samples, 0 );
		}
		else if( total_received_samples < out.samples )
		{
			// Duplicate samples to return the exact number requested.
			mlt_audio_copy( &out, &out, total_received_samples, 0, out.samples - total_received_samples );
		}
		mlt_frame_set_audio( frame, out.data, out.format, 0, out.release_data );
		mlt_audio_get_values( &out, buffer, frequency, format, samples, channels );
		mlt_properties_set( frame_properties, "channel_layout", mlt_audio_channel_layout_name( out.layout ) );
		pdata->expected_frame = mlt_frame_get_position( frame ) + 1;
	}

	mlt_service_unlock( MLT_LINK_SERVICE(self) );
	return error;
}

static int link_get_frame( mlt_link self, mlt_frame_ptr frame, int index )
{
	int result = 0;
	mlt_properties properties = MLT_LINK_PROPERTIES( self );
	private_data* pdata = (private_data*)self->child;
	mlt_position frame_pos = mlt_producer_position( MLT_LINK_PRODUCER(self) );

	// Cycle out the first frame in the cache
	mlt_frame_close( pdata->frame_cache[0] );
	// Shift all the frames or get new
	int i = 0;
	for ( i = 0; i < FRAME_CACHE_SIZE - 1; i++ )
	{
		mlt_position pos = frame_pos + i;
		mlt_frame next_frame = pdata->frame_cache[ i + 1 ];
		if ( next_frame && mlt_frame_get_position( next_frame ) == pos )
		{
			// Shift the frame if it is correct
			pdata->frame_cache[ i ] = next_frame;
		}
		else
		{
			// Get a new frame if the next cache frame is not the needed frame
			mlt_frame_close( next_frame );
			mlt_producer_seek( self->next, pos );
			result = mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), &pdata->frame_cache[ i ], index );
			if ( result )
			{
				mlt_log_error( MLT_LINK_SERVICE(self), "Error getting frame: %d\n", (int)pos );
			}
		}
	}
	// Get the new last frame in the cache
	mlt_producer_seek( self->next, frame_pos + i );
	result = mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), &pdata->frame_cache[ i ], index );
	if ( result )
	{
		mlt_log_error( MLT_LINK_SERVICE(self), "Error getting frame: %d\n", (int)frame_pos + i );
	}

	*frame = pdata->frame_cache[0];
	mlt_properties_inc_ref( MLT_FRAME_PROPERTIES(*frame) );

	// Attach the next frames to the current frame in case they are needed for sample rate conversion
	mlt_properties unique_properties = mlt_frame_unique_properties( *frame, MLT_LINK_SERVICE(self) );
	for ( int i = 1; i < FRAME_CACHE_SIZE; i++ )
	{
		char key[19];
		sprintf( key, "%d", (int)mlt_frame_get_position( pdata->frame_cache[i] ) );
		mlt_properties_inc_ref( MLT_FRAME_PROPERTIES(pdata->frame_cache[i]) );
		mlt_properties_set_data( unique_properties, key, pdata->frame_cache[i], 0, (mlt_destructor)mlt_frame_close, NULL );
	}
	mlt_frame_push_audio( *frame, (void*)self );
	mlt_frame_push_audio( *frame, link_get_audio );

	mlt_producer_prepare_next( MLT_LINK_PRODUCER( self ) );

	return result;
}

static void link_close( mlt_link self )
{
	if ( self )
	{
		private_data* pdata = (private_data*)self->child;
		if ( pdata )
		{
			for ( int i = 0; i < FRAME_CACHE_SIZE; i++ )
			{
				mlt_frame_close( pdata->frame_cache[ i ] );
			}
			if( pdata->swr.ctx )
			{
				mlt_free_swr_context( &pdata->swr );
			}
			free( pdata );
		}
		self->close = NULL;
		self->child = NULL;
		mlt_link_close( self );
		free( self );
	}
}

mlt_link link_swresample_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_link self = mlt_link_init();
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if ( self && pdata )
	{
		pdata->continuity_frame = -1;
		pdata->expected_frame = -1;
		self->child = pdata;

		// Callback registration
		self->configure = link_configure;
		self->get_frame = link_get_frame;
		self->close = link_close;
	}
	else
	{
		if ( pdata )
		{
			free( pdata );
		}

		if ( self )
		{
			mlt_link_close( self );
			self = NULL;
		}
	}
	return self;
}
