/*
 * filter_volume.c -- adjust audio volume
 * Copyright (C) 2003-2015 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#define MAX_CHANNELS 6
#define EPSILON 0.00001

/* The following normalise functions come from the normalize utility:
   Copyright (C) 1999--2002 Chris Vaill */

#define samp_width 16

#ifndef ROUND
# define ROUND(x) floor((x) + 0.5)
#endif

#define DBFSTOAMP(x) pow(10,(x)/20.0)

/** Return nonzero if the two strings are equal, ignoring case, up to
    the first n characters.
*/
int strncaseeq(const char *s1, const char *s2, size_t n)
{
	for ( ; n > 0; n--)
	{
		if (tolower(*s1++) != tolower(*s2++))
			return 0;
	}
	return 1;
}

/** Limiter function.
 
         / tanh((x + lev) / (1-lev)) * (1-lev) - lev        (for x < -lev)
         |
    x' = | x                                                (for |x| <= lev)
         |
         \ tanh((x - lev) / (1-lev)) * (1-lev) + lev        (for x > lev)
 
  With limiter level = 0, this is equivalent to a tanh() function;
  with limiter level = 1, this is equivalent to clipping.
*/
static inline double limiter( double x, double lmtr_lvl )
{
	double xp = x;

	if (x < -lmtr_lvl)
		xp = tanh((x + lmtr_lvl) / (1-lmtr_lvl)) * (1-lmtr_lvl) - lmtr_lvl;
	else if (x > lmtr_lvl)
		xp = tanh((x - lmtr_lvl) / (1-lmtr_lvl)) * (1-lmtr_lvl) + lmtr_lvl;

	return xp;
}


/** Takes a full smoothing window, and returns the value of the center
    element, smoothed.

    Currently, just does a mean filter, but we could do a median or
    gaussian filter here instead.
*/
static inline double get_smoothed_data( double *buf, int count )
{
	int i, j;
	double smoothed = 0;

	for ( i = 0, j = 0; i < count; i++ )
	{
		if ( buf[ i ] != -1.0 )
		{
			smoothed += buf[ i ];
			j++;
		}
	}
	if (j) smoothed /= j;

	return smoothed;
}

/** Get the max power level (using RMS) and peak level of the audio segment.
 */
double signal_max_power( int16_t *buffer, int channels, int samples, int16_t *peak )
{
	// Determine numeric limits
	int bytes_per_samp = (samp_width - 1) / 8 + 1;
	int16_t max = (1 << (bytes_per_samp * 8 - 1)) - 1;
	int16_t min = -max - 1;
	
	double *sums = (double *) calloc( channels, sizeof(double) );
	int c, i;
	int16_t sample;
	double pow, maxpow = 0;

	/* initialize peaks to effectively -inf and +inf */
	int16_t max_sample = min;
	int16_t min_sample = max;
  
	for ( i = 0; i < samples; i++ )
	{
		for ( c = 0; c < channels; c++ )
		{
			sample = *buffer++;
			sums[ c ] += (double) sample * (double) sample;
			
			/* track peak */
			if ( sample > max_sample )
				max_sample = sample;
			else if ( sample < min_sample )
				min_sample = sample;
		}
	}
	for ( c = 0; c < channels; c++ )
	{
		pow = sums[ c ] / (double) samples;
		if ( pow > maxpow )
			maxpow = pow;
	}
			
	free( sums );
	
	/* scale the pow value to be in the range 0.0 -- 1.0 */
	maxpow /= ( (double) min * (double) min);

	if ( -min_sample > max_sample )
		*peak = min_sample / (double) min;
	else
		*peak = max_sample / (double) max;

	return sqrt( maxpow );
}

/* ------ End normalize functions --------------------------------------- */

/** Get the audio.
*/

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the filter from the frame
	mlt_filter filter = mlt_frame_pop_audio( frame );

	// Get the properties from the filter
	mlt_properties filter_props = MLT_FILTER_PROPERTIES( filter );

	// Get the frame's filter instance properties
	mlt_properties instance_props = mlt_frame_unique_properties( frame, MLT_FILTER_SERVICE( filter ) );

	// Get the parameters
	double gain = mlt_properties_get_double( instance_props, "gain" );
	double max_gain = mlt_properties_get_double( instance_props, "max_gain" );
	double limiter_level = 0.5; /* -6 dBFS */
	int normalise =  mlt_properties_get_int( instance_props, "normalise" );
	double amplitude =  mlt_properties_get_double( instance_props, "amplitude" );
	int i, j;
	double sample;
	int16_t peak;
	
	// Use animated value for gain if "level" property is set 
	char* level_property = mlt_properties_get( filter_props, "level" );
	if ( level_property != NULL )
	{
		mlt_position position = mlt_filter_get_position( filter, frame );
		mlt_position length = mlt_filter_get_length2( filter, frame );
		gain = mlt_properties_anim_get_double( filter_props, "level", position, length );
		gain = DBFSTOAMP( gain );
	}

	if ( mlt_properties_get( instance_props, "limiter" ) != NULL )
		limiter_level = mlt_properties_get_double( instance_props, "limiter" );
	
	// Get the producer's audio
	*format = mlt_audio_s16;
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	// Determine numeric limits
	int bytes_per_samp = (samp_width - 1) / 8 + 1;
	int samplemax = (1 << (bytes_per_samp * 8 - 1)) - 1;
	int samplemin = -samplemax - 1;

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	if ( normalise )
	{
		int window = mlt_properties_get_int( filter_props, "window" );
		double *smooth_buffer = mlt_properties_get_data( filter_props, "smooth_buffer", NULL );

		if ( window > 0 && smooth_buffer != NULL )
		{
			int smooth_index = mlt_properties_get_int( filter_props, "_smooth_index" );
			
			// Compute the signal power and put into smoothing buffer
			smooth_buffer[ smooth_index ] = signal_max_power( *buffer, *channels, *samples, &peak );

			if ( smooth_buffer[ smooth_index ] > EPSILON )
			{
				mlt_properties_set_int( filter_props, "_smooth_index", ( smooth_index + 1 ) % window );

				// Smooth the data and compute the gain
				gain *= amplitude / get_smoothed_data( smooth_buffer, window );
			}
		}
		else
		{
			gain *= amplitude / signal_max_power( (int16_t*) *buffer, *channels, *samples, &peak );
		}
	}

	if ( max_gain > 0 && gain > max_gain )
		gain = max_gain;

	// Initialise filter's previous gain value to prevent an inadvertant jump from 0
	mlt_position last_position = mlt_properties_get_position( filter_props, "_last_position" );
	mlt_position current_position = mlt_frame_get_position( frame );
	if ( mlt_properties_get( filter_props, "_previous_gain" ) == NULL
	     || current_position != last_position + 1 )
		mlt_properties_set_double( filter_props, "_previous_gain", gain );

	// Start the gain out at the previous
	double previous_gain = mlt_properties_get_double( filter_props, "_previous_gain" );

	// Determine ramp increment
	double gain_step = ( gain - previous_gain ) / *samples;

	// Save the current gain for the next iteration
	mlt_properties_set_double( filter_props, "_previous_gain", gain );
	mlt_properties_set_position( filter_props, "_last_position", current_position );

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

	// Ramp from the previous gain to the current
	gain = previous_gain;

	int16_t *p = (int16_t*) *buffer;

	// Apply the gain
	for ( i = 0; i < *samples; i++ )
	{
		for ( j = 0; j < *channels; j++ )
		{
			sample = *p * gain;
			*p = ROUND( sample );
		
			if ( gain > 1.0 )
			{
				/* use limiter function instead of clipping */
				if ( normalise )
					*p = ROUND( samplemax * limiter( sample / (double) samplemax, limiter_level ) );
				
				/* perform clipping */
				else if ( sample > samplemax )
					*p = samplemax;
				else if ( sample < samplemin )
					*p = samplemin;
			}
			p++;
		}
		gain += gain_step;
	}
	
	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_properties filter_props = MLT_FILTER_PROPERTIES( filter );
	mlt_properties instance_props = mlt_frame_unique_properties( frame, MLT_FILTER_SERVICE( filter ) );

	double gain = 1.0; // no adjustment
	char *gain_str = mlt_properties_get( filter_props, "gain" );

	// Parse the gain property
	if ( gain_str )
	{
		char *p_orig = strdup( gain_str );
		char *p =  p_orig;

		if ( strncaseeq( p, "normalise", 9 ) )
			mlt_properties_set( filter_props, "normalise", "" );
		else
		{
			if ( strcmp( p, "" ) != 0 )
				gain = strtod( p, &p );

			while ( isspace( *p ) )
				p++;

			/* check if "dB" is given after number */
			if ( strncaseeq( p, "db", 2 ) )
				gain = DBFSTOAMP( gain );
			else
				gain = fabs( gain );

			// If there is an end adjust gain to the range
			if ( mlt_properties_get( filter_props, "end" ) != NULL )
			{
				double end = -1;
				char *p = mlt_properties_get( filter_props, "end" );
				if ( strcmp( p, "" ) != 0 )
					end = strtod( p, &p );

				while ( isspace( *p ) )
					p++;

				/* check if "dB" is given after number */
				if ( strncaseeq( p, "db", 2 ) )
					end = DBFSTOAMP( end );
				else
					end = fabs( end );

				if ( end != -1 )
					gain += ( end - gain ) * mlt_filter_get_progress( filter, frame );
			}
		}
		free( p_orig );
	}
	mlt_properties_set_double( instance_props, "gain", gain );
	
	// Parse the maximum gain property
	if ( mlt_properties_get( filter_props, "max_gain" ) != NULL )
	{
		char *p = mlt_properties_get( filter_props, "max_gain" );
		double gain = strtod( p, &p ); // 0 = no max
			
		while ( isspace( *p ) )
			p++;

		/* check if "dB" is given after number */
		if ( strncaseeq( p, "db", 2 ) )
			gain = DBFSTOAMP( gain );
		else
			gain = fabs( gain );
			
		mlt_properties_set_double( instance_props, "max_gain", gain );
	}

	// Parse the limiter property
	if ( mlt_properties_get( filter_props, "limiter" ) != NULL )
	{
		char *p = mlt_properties_get( filter_props, "limiter" );
		double level = 0.5; /* -6dBFS */ 
		if ( strcmp( p, "" ) != 0 )
			level = strtod( p, &p);
		
		while ( isspace( *p ) )
			p++;
		
		/* check if "dB" is given after number */
		if ( strncaseeq( p, "db", 2 ) )
		{
			if ( level > 0 )
				level = -level;
			level = DBFSTOAMP( level );
		}
		else
		{
			if ( level < 0 )
				level = -level;
		}
		mlt_properties_set_double( instance_props, "limiter", level );
	}

	// Parse the normalise property
	if ( mlt_properties_get( filter_props, "normalise" ) != NULL )
	{
		char *p = mlt_properties_get( filter_props, "normalise" );
		double amplitude = 0.2511886431509580; /* -12dBFS */
		if ( strcmp( p, "" ) != 0 )
			amplitude = strtod( p, &p);

		while ( isspace( *p ) )
			p++;

		/* check if "dB" is given after number */
		if ( strncaseeq( p, "db", 2 ) )
		{
			if ( amplitude > 0 )
				amplitude = -amplitude;
			amplitude = DBFSTOAMP( amplitude );
		}
		else
		{
			if ( amplitude < 0 )
				amplitude = -amplitude;
			if ( amplitude > 1.0 )
				amplitude = 1.0;
		}
		
		// If there is an end adjust gain to the range
		if ( mlt_properties_get( filter_props, "end" ) != NULL )
		{
			amplitude *= mlt_filter_get_progress( filter, frame );
		}
		mlt_properties_set_int( instance_props, "normalise", 1 );
		mlt_properties_set_double( instance_props, "amplitude", amplitude );
	}

	// Parse the window property and allocate smoothing buffer if needed
	int window = mlt_properties_get_int( filter_props, "window" );
	if ( mlt_properties_get( filter_props, "smooth_buffer" ) == NULL && window > 1 )
	{
		// Create a smoothing buffer for the calculated "max power" of frame of audio used in normalisation
		double *smooth_buffer = (double*) calloc( window, sizeof( double ) );
		int i;
		for ( i = 0; i < window; i++ )
			smooth_buffer[ i ] = -1.0;
		mlt_properties_set_data( filter_props, "smooth_buffer", smooth_buffer, 0, free, NULL );
	}
	
	// Push the filter onto the stack
	mlt_frame_push_audio( frame, filter );

	// Override the get_audio method
	mlt_frame_push_audio( frame, filter_get_audio );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_volume_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = calloc( 1, sizeof( struct mlt_filter_s ) );
	if ( filter != NULL && mlt_filter_init( filter, NULL ) == 0 )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		filter->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set( properties, "gain", arg );

		mlt_properties_set_int( properties, "window", 75 );
		mlt_properties_set( properties, "max_gain", "20dB" );

		mlt_properties_set( properties, "level", NULL );
	}
	return filter;
}
