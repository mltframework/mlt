/*
 * filter_swresample.c -- convert from one format/ configuration to another
 * Copyright (C) 2018 Meltytech, LLC
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

#include "common.h"

#include <framework/mlt.h>

#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>

typedef struct
{
	SwrContext* ctx;
	uint8_t** in_buffers;
	uint8_t** out_buffers;
	mlt_audio_format in_format;
	mlt_audio_format out_format;
	int in_frequency;
	int out_frequency;
	int in_channels;
	int out_channels;
	mlt_channel_layout in_layout;
	mlt_channel_layout out_layout;
} private_data;

static int audio_plane_count( mlt_audio_format format, int channels )
{
	switch ( format )
	{
		case mlt_audio_none:  return 0;
		case mlt_audio_s16:   return 1;
		case mlt_audio_s32le: return 1;
		case mlt_audio_s32:   return channels;
		case mlt_audio_f32le: return 1;
		case mlt_audio_float: return channels;
		case mlt_audio_u8:    return 1;
	}
	return 0;
}

static int audio_plane_size( mlt_audio_format format, int samples, int channels )
{
	switch ( format )
	{
		case mlt_audio_none:  return 0;
		case mlt_audio_s16:   return samples * channels * sizeof( int16_t );
		case mlt_audio_s32le: return samples * channels * sizeof( int32_t );
		case mlt_audio_s32:   return samples * sizeof( int32_t );
		case mlt_audio_f32le: return samples * channels * sizeof( float );
		case mlt_audio_float: return samples * sizeof( float );
		case mlt_audio_u8:    return samples * channels;
	}
	return 0;
}

static void audio_format_planes( mlt_audio_format format, int samples, int channels, uint8_t* buffer, uint8_t** planes )
{
	int plane_count = audio_plane_count( format, channels );
	size_t plane_size = audio_plane_size( format, samples, channels );
	int p = 0;
	for( p = 0; p < plane_count; p++ )
	{
		planes[p] = buffer + ( p * plane_size );
	}
}

static void collapse_channels( mlt_audio_format format, int channels, int allocated_samples, int used_samples, uint8_t* buffer )
{
	int plane_count = audio_plane_count( format, channels );
	if( plane_count > 1 && allocated_samples != used_samples )
	{
		size_t src_plane_size = audio_plane_size( format, allocated_samples, channels );
		size_t dst_plane_size = audio_plane_size( format, used_samples, channels );
		int p = 0;
		for( p = 0; p < plane_count; p++ )
		{
			uint8_t* src = buffer + ( p * src_plane_size );
			uint8_t* dst = buffer + ( p * dst_plane_size );
			memmove( dst, src, dst_plane_size );
		}
	}
}

static int configure_swr_context( mlt_filter filter )
{
	private_data* pdata = (private_data*)filter->child;
	int error = 0;

	mlt_log_info( MLT_FILTER_SERVICE(filter), "%d(%s) %s %dHz -> %d(%s) %s %dHz\n",
				   pdata->in_channels, mlt_channel_layout_name( pdata->in_layout ), mlt_audio_format_name( pdata->in_format ), pdata->in_frequency, pdata->out_channels, mlt_channel_layout_name( pdata->out_layout ), mlt_audio_format_name( pdata->out_format ), pdata->out_frequency );

	swr_free( &pdata->ctx );
	pdata->ctx = swr_alloc();
	if( !pdata->ctx )
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Cannot allocate context\n" );
		return 1;
	}

	// Configure format, frequency and channels.
	av_opt_set_int( pdata->ctx, "osf", mlt_to_av_sample_format( pdata->out_format ), 0 );
	av_opt_set_int( pdata->ctx, "osr", pdata->out_frequency, 0 );
	av_opt_set_int( pdata->ctx, "och", pdata->out_channels, 0);
	av_opt_set_int( pdata->ctx, "isf", mlt_to_av_sample_format( pdata->in_format ), 0 );
	av_opt_set_int( pdata->ctx, "isr", pdata->in_frequency,  0 );
	av_opt_set_int( pdata->ctx, "ich", pdata->in_channels, 0 );

	if( pdata->in_layout != mlt_channel_independent && pdata->out_layout != mlt_channel_independent )
	{
		// Use standard channel layout and matrix for known channel configurations.
		av_opt_set_int( pdata->ctx, "ocl", mlt_to_av_channel_layout( pdata->out_layout ), 0 );
		av_opt_set_int( pdata->ctx, "icl", mlt_to_av_channel_layout( pdata->in_layout ), 0 );
	}
	else
	{
		// Use a custom channel layout and matrix for independent channels.
		// This matrix will simply map input channels to output channels in order.
		// If input channels > output channels, channels will be dropped.
		// If input channels < output channels, silent channels will be added.
		int64_t custom_in_layout = 0;
		int64_t custom_out_layout = 0;
		double* matrix = av_mallocz_array( pdata->in_channels * pdata->out_channels, sizeof(double) );
		int stride = pdata->in_channels;
		int i = 0;

		for( i = 0; i < pdata->in_channels; i++ )
		{
			custom_in_layout = (custom_in_layout << 1) | 0x01;
		}
		for( i = 0; i < pdata->out_channels; i++ )
		{
			custom_out_layout = (custom_out_layout << 1) | 0x01;
			if( i <= pdata->in_channels )
			{
				double* matrix_row = matrix + (stride * i);
				matrix_row[i] = 1.0;
			}
		}
		av_opt_set_int( pdata->ctx, "ocl", custom_out_layout, 0 );
		av_opt_set_int( pdata->ctx, "icl", custom_in_layout, 0 );
		error = swr_set_matrix( pdata->ctx, matrix, stride );
		av_free( matrix );
		if( error != 0 )
		{
			swr_free( &pdata->ctx );
			mlt_log_error( MLT_FILTER_SERVICE(filter), "Unable to create custom matrix\n" );
			return error;
		}
	}

	error = swr_init( pdata->ctx );
	if( error != 0 )
	{
		swr_free( &pdata->ctx );
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Cannot initialize context\n" );
		return error;
	}

	// Allocate the channel buffer pointers
	av_freep( &pdata->in_buffers );
	pdata->in_buffers = av_mallocz_array( pdata->in_channels, sizeof(uint8_t*) );
	av_freep( &pdata->out_buffers );
	pdata->out_buffers = av_mallocz_array( pdata->out_channels, sizeof(uint8_t*) );

	return error;
}

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_filter filter = mlt_frame_pop_audio( frame );
	private_data* pdata = (private_data*)filter->child;
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	mlt_audio_format in_format = *format;
	mlt_audio_format out_format = *format;
	int in_frequency = *frequency;
	int out_frequency = *frequency;
	int in_channels = *channels;
	int out_channels = *channels;
	mlt_channel_layout in_layout;
	mlt_channel_layout out_layout;

	// Get the producer's audio
	int error = mlt_frame_get_audio( frame, buffer, &in_format, &in_frequency, &in_channels, samples );
	if ( error ||
			in_format == mlt_audio_none || out_format == mlt_audio_none ||
			in_frequency == 0 || out_frequency == 0 ||
			in_channels == 0 || out_channels == 0 ||
			*samples == 0 )
	{
		// Error situation. Do not attempt to convert.
		*format = in_format;
		*frequency = in_frequency;
		*channels = in_channels;
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Invalid Parameters: %dS - %dHz %dC %s -> %dHz %dC %s\n", *samples, in_frequency, in_channels, mlt_audio_format_name( in_format ), out_frequency, out_channels, mlt_audio_format_name( out_format ) );
		return error;
	}

	// Determine the input/output channel layout.
	in_layout = mlt_get_channel_layout_or_default( mlt_properties_get( frame_properties, "channel_layout" ), in_channels );
	out_layout = mlt_get_channel_layout_or_default( mlt_properties_get( frame_properties, "consumer_channel_layout" ), out_channels );

	if( in_format == out_format &&
		in_frequency == out_frequency &&
		in_channels == out_channels &&
		in_layout == out_layout )
	{
		// No change necessary
		return error;
	}

	mlt_service_lock( MLT_FILTER_SERVICE(filter) );

	// Detect configuration change
	if( !pdata->ctx ||
		pdata->in_format != in_format ||
		pdata->out_format != out_format ||
		pdata->in_frequency != in_frequency ||
		pdata->out_frequency != out_frequency ||
		pdata->in_channels != in_channels ||
		pdata->out_channels != out_channels ||
		pdata->in_layout != in_layout ||
		pdata->out_layout != out_layout )
	{
		// Save the configuration
		pdata->in_format = in_format;
		pdata->out_format = out_format;
		pdata->in_frequency = in_frequency;
		pdata->out_frequency = out_frequency;
		pdata->in_channels = in_channels;
		pdata->out_channels = out_channels;
		pdata->in_layout = in_layout;
		pdata->out_layout = out_layout;
		// Reconfigure the context
		error = configure_swr_context( filter );
	}

	if( !error )
	{
		int in_samples = *samples;
		int out_samples = 0;
		int alloc_samples = in_samples;
		if( in_frequency != out_frequency )
		{
			// Number of output samples will change if sampling frequency changes.
			alloc_samples = in_samples * out_frequency / in_frequency;
			// Round up to make sure all available samples are received from swresample.
			alloc_samples += 1;
		}
		int size = mlt_audio_format_size( out_format, alloc_samples, out_channels );
		uint8_t* out_buffer = mlt_pool_alloc( size );

		audio_format_planes( in_format, in_samples, in_channels, *buffer, pdata->in_buffers );
		audio_format_planes( out_format, alloc_samples, out_channels, out_buffer, pdata->out_buffers );

		out_samples = swr_convert( pdata->ctx, pdata->out_buffers, alloc_samples, (const uint8_t**)pdata->in_buffers, in_samples );
		if( out_samples > 0 )
		{
			collapse_channels( out_format, out_channels, alloc_samples, out_samples, out_buffer );
			mlt_frame_set_audio( frame, out_buffer, out_format, size, mlt_pool_release );
			*buffer = out_buffer;
			*samples = out_samples;
			*format = out_format;
			*channels = out_channels;
			mlt_properties_set( frame_properties, "channel_layout", mlt_channel_layout_name( pdata->out_layout ) );
		}
		else
		{
			mlt_log_error( MLT_FILTER_SERVICE(filter), "swr_convert() failed. Alloc: %d\tIn: %d\tOut: %d\n", alloc_samples, in_samples, out_samples );
			mlt_pool_release( out_buffer );
			error = 1;
		}
	}

	mlt_service_unlock( MLT_FILTER_SERVICE(filter) );
	return error;
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_audio( frame, filter );
	mlt_frame_push_audio( frame, filter_get_audio );
	return frame;
}

static void filter_close( mlt_filter filter )
{
	private_data* pdata = (private_data*)filter->child;

	if( pdata )
	{
		swr_free( &pdata->ctx );
		av_freep( &pdata->in_buffers );
		av_freep( &pdata->out_buffers );
		free( pdata );
	}
	filter->child = NULL;
	filter->close = NULL;
	filter->parent.close = NULL;
	mlt_service_close( &filter->parent );
}

mlt_filter filter_swresample_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if( filter && pdata )
	{
		memset( pdata, 0, sizeof( *pdata ) );
		filter->close = filter_close;
		filter->process = filter_process;
		filter->child = pdata;
	}
	else
	{
		mlt_filter_close( filter );
		free( pdata );
	}

	return filter;
}
