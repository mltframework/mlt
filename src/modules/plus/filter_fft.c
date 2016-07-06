/*
 * filter_fft.c -- perform fft on audio
 * Copyright (C) 2015 Meltytech, LLC
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
#include <stdlib.h> // calloc(), free()
#include <string.h> // memset(), memmove()
#include <math.h>   // sqrt()
#include <fftw3.h>

// Private Constants
static const float MAX_S16_AMPLITUDE = 32768.0;
static const int MIN_WINDOW_SIZE = 500;
static const double PI = 3.14159265358979323846;

// Private Types
typedef struct
{
	int initialized;
	unsigned int window_size;
	double* fft_in;
	fftw_complex* fft_out;
	fftw_plan fft_plan;
	int bin_count;
	int sample_buff_count;
	float* sample_buff;
	float* hann;
	float* out_bins;
	mlt_position expected_pos;
} private_data;

static int initFft( mlt_filter filter )
{
	int error = 0;
	private_data* private = (private_data*)filter->child;
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	if( private->window_size < MIN_WINDOW_SIZE )
	{
		private->window_size = mlt_properties_get_int( filter_properties, "window_size" );
		if( private->window_size >= MIN_WINDOW_SIZE )
		{
			private->initialized = 1;
			private->bin_count = private->window_size / 2 + 1;
			private->sample_buff_count = 0;
			private->out_bins = mlt_pool_alloc( private->bin_count * sizeof(*private->out_bins));

			// Initialize sample buffer
			private->sample_buff = mlt_pool_alloc( private->window_size * sizeof(*private->sample_buff));
			memset( private->sample_buff, 0, sizeof(*private->sample_buff) * private->window_size );

			// Initialize fftw variables
			private->fft_in = fftw_alloc_real( private->window_size );
			private->fft_out = fftw_alloc_complex( private->bin_count );
			private->fft_plan = fftw_plan_dft_r2c_1d( private->window_size, private->fft_in, private->fft_out, FFTW_ESTIMATE );

			// Initialize the hanning window function
			private->hann = mlt_pool_alloc( private->window_size * sizeof(*private->hann));
			int i = 0;
			for ( i = 0; i < private->window_size; i++ )
			{
				private->hann[i] = 0.5 * (1 - cos( 2 * PI * i / private->window_size ) );
			}

			mlt_properties_set_int( filter_properties, "bin_count", private->bin_count );
			mlt_properties_set_data( filter_properties, "bins", private->out_bins, 0, 0, 0 );
		}

		if( private->window_size < MIN_WINDOW_SIZE || !private->fft_in || !private->fft_out || !private->fft_plan )
		{
			mlt_log_error( MLT_FILTER_SERVICE( filter ), "Unable to initialize FFT\n" );
			error = 1;
			private->window_size = 0;
		}
	}
	return error;
}

static int filter_get_audio( mlt_frame frame, void** buffer, mlt_audio_format* format, int* frequency, int* channels, int* samples )
{
	mlt_filter filter = (mlt_filter)mlt_frame_pop_audio( frame );
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	private_data* private = (private_data*)filter->child;
	int c = 0;
	int s = 0;

	// Sanity
	if ( *format != mlt_audio_s16 && *format != mlt_audio_float )
	{
		*format = mlt_audio_float;
	}
	// Get the audio
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	// The service must stay locked while using the private FFT data
	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	if( !private->initialized )
	{
		private->expected_pos = mlt_frame_get_position( frame );
	}

	if( !initFft( filter ) )
	{
		if( private->expected_pos != mlt_frame_get_position( frame ) )
		{
			// Reset the sample buffer when seeking occurs.
			memset( private->sample_buff, 0, sizeof(*private->sample_buff) * private->window_size );
			private->sample_buff_count = 0;
			mlt_log_info( MLT_FILTER_SERVICE(filter), "Buffer Reset %d:%d\n",
							private->expected_pos,
							mlt_frame_get_position( frame ) );
			private->expected_pos = mlt_frame_get_position( frame );
		}

		int new_samples = 0;
		int old_samples = 0;
		if( *samples >= private->window_size )
		{
			// Ignore samples that don't fit in the window
			new_samples = private->window_size;
			old_samples = 0;
		}
		else
		{
			new_samples = *samples;
			// Shift the previous samples (discarding oldest samples)
			old_samples = private->window_size - new_samples;
			memmove( private->sample_buff, private->sample_buff + new_samples, sizeof(*private->sample_buff) * old_samples);
		}

		// Zero out the space for the new samples
		memset( private->sample_buff + old_samples, 0, sizeof(*private->sample_buff) * new_samples );

		// Copy the new samples into the sample buffer
		if( *format == mlt_audio_s16 )
		{
			int16_t* aud = (int16_t*)*buffer;
			// For each sample, add all channels
			for( c = 0; c < *channels; c++ )
			{
				for( s = 0; s < new_samples; s++ )
				{
					double sample = aud[s * *channels + c];
					// Scale to +/-1
					sample /= MAX_S16_AMPLITUDE;
					sample /= (double)*channels;
					private->sample_buff[old_samples + s] += sample;
				}
			}
		}
		else if( *format == mlt_audio_float )
		{
			float* aud = (float*)*buffer;
			// For each sample, add all channels
			for( c = 0; c < *channels; c++ )
			{
				for( s = 0; s < new_samples; s++ )
				{
					double sample = aud[c * *samples + s];
					sample /= (double)*channels;
					private->sample_buff[old_samples + s] += sample;
				}
			}
		}
		else
		{
			mlt_log_error( MLT_FILTER_SERVICE(filter), "Unsupported format %d\n", *format );
		}
		private->sample_buff_count += *samples;
		if( private->sample_buff_count > private->window_size )
		{
			private->sample_buff_count = private->window_size;
		}

		// Copy samples to fft input while applying window function
		for (s = 0; s < private->window_size; s++)
		{
			private->fft_in[s] = private->sample_buff[s] * private->hann[s];
		}

		// Perform the FFT
		fftw_execute( private->fft_plan );

		// Convert to magnitudes
		int bin = 0;
		for( bin = 0; bin < private->bin_count; bin++ )
		{
			// Convert FFT output to magnitudes
			private->out_bins[bin] = sqrt( private->fft_out[bin][0] * private->fft_out[bin][0]
												+ private->fft_out[bin][1] * private->fft_out[bin][1] );
			// Scale to 0.0 - 1.0
			private->out_bins[bin] = (4.0 * private->out_bins[bin]) / (float)private->window_size;
		}

		private->expected_pos++;
	}

	mlt_properties_set_double( filter_properties, "bin_width", (double)*frequency / (double)private->window_size );
	mlt_properties_set_double( filter_properties, "window_level", (double)private->sample_buff_count / (double)private->window_size );

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_audio( frame, filter );
	mlt_frame_push_audio( frame, filter_get_audio );
	return frame;
}

static void filter_close( mlt_filter filter )
{
	private_data* private = (private_data*)filter->child;

	if ( private )
	{
		fftw_free( private->fft_in );
		fftw_free( private->fft_out );
		fftw_destroy_plan( private->fft_plan );
		mlt_pool_release( private->sample_buff );
		mlt_pool_release( private->hann );
		mlt_pool_release( private->out_bins );
		free( private );
	}
	filter->child = NULL;
	filter->close = NULL;
	filter->parent.close = NULL;
	mlt_service_close( &filter->parent );
}

/** Constructor for the filter.
*/
mlt_filter filter_fft_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	private_data* private = (private_data*)calloc( 1, sizeof(private_data) );

	if( filter && private )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set_int( properties, "_filter_private", 1 );
		mlt_properties_set_int( properties, "window_size", 2048 );
		mlt_properties_set_double( properties, "window_level", 0.0 );
		mlt_properties_set_double( properties, "bin_width", 0.0 );
		mlt_properties_set_int( properties, "bin_count", 0 );
		mlt_properties_set_data( properties, "bins", 0, 0, 0, 0 );

		memset( private, 0, sizeof(private_data) );

		filter->close = filter_close;
		filter->process = filter_process;
		filter->child = private;
	}
	else
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Filter FFT failed\n" );

		if( filter )
		{
			mlt_filter_close( filter );
		}

		if( private )
		{
			free( private );
		}

		filter = NULL;
	}
	return filter;
}
