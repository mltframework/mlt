/*
 * filter_volume.c -- adjust audio volume
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "filter_volume.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#define MAX_CHANNELS 6
#define SMOOTH_BUFFER_SIZE 75  /* smooth over 3 seconds on PAL */
#define EPSILON 0.00001

/* The normalise functions come from the normalize utility:
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

//	if ( x != xp )
//		fprintf( stderr, "filter_volume: sample %f limited %f\n", x, xp );

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
	smoothed /= j;
//	fprintf( stderr, "smoothed over %d values, result %f\n", j, smoothed );

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

static int filter_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the properties of the a frame
	mlt_properties properties = mlt_frame_properties( frame );
	double gain = mlt_properties_get_double( properties, "gain" );
	double max_gain = mlt_properties_get_double( properties, "volume.max_gain" );
	double limiter_level = 0.5; /* -6 dBFS */
	int normalise =  mlt_properties_get_int( properties, "volume.normalise" );
	double amplitude =  mlt_properties_get_double( properties, "volume.amplitude" );
	int i;
	double sample;
	int16_t peak;

	if ( mlt_properties_get( properties, "volume.limiter" ) != NULL )
		limiter_level = mlt_properties_get_double( properties, "volume.limiter" );
	
	// Restore the original get_audio
	frame->get_audio = mlt_properties_get_data( properties, "volume.get_audio", NULL );

	// Get the producer's audio
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	// Determine numeric limits
	int bytes_per_samp = (samp_width - 1) / 8 + 1;
	int samplemax = (1 << (bytes_per_samp * 8 - 1)) - 1;
	int samplemin = -samplemax - 1;

	if ( normalise )
	{
		double *smooth_buffer = mlt_properties_get_data( properties, "volume.smooth_buffer", NULL );
		int *smooth_index = mlt_properties_get_data( properties, "volume.smooth_index", NULL );

		// Compute the signal power and put into smoothing buffer
		smooth_buffer[ *smooth_index ] = signal_max_power( *buffer, *channels, *samples, &peak );
//		fprintf( stderr, "filter_volume: raw power %f ", smooth_buffer[ *smooth_index ] );
		if ( smooth_buffer[ *smooth_index ] > EPSILON )
		{
			*smooth_index = ( *smooth_index + 1 ) % SMOOTH_BUFFER_SIZE;

			// Smooth the data and compute the gain
//			fprintf( stderr, "smoothed %f\n", get_smoothed_data( smooth_buffer, SMOOTH_BUFFER_SIZE ) );
			gain *= amplitude / get_smoothed_data( smooth_buffer, SMOOTH_BUFFER_SIZE );
		}
	}
	
	if ( gain > 1.0 && normalise )
		fprintf(stderr, "filter_volume: limiter level %f gain %f\n", limiter_level, gain );

	if ( max_gain > 0 && gain > max_gain )
		gain = max_gain;

	// Apply the gain
	for ( i = 0; i < ( *channels * *samples ); i++ )
	{
		sample = (*buffer)[i] * gain;
		(*buffer)[i] = ROUND( sample );
		
		if ( gain > 1.0 )
		{
			/* use limiter function instead of clipping */
			if ( normalise )
				(*buffer)[i] = ROUND( samplemax * limiter( sample / (double) samplemax, limiter_level ) );
				
			/* perform clipping */
			else if ( sample > samplemax )
				(*buffer)[i] = samplemax;
			else if ( sample < samplemin )
				(*buffer)[i] = samplemin;
		}
	}
	
	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_properties properties = mlt_frame_properties( frame );
	mlt_properties filter_props = mlt_filter_properties( this );

	// Propogate the gain property
	if ( mlt_properties_get( properties, "gain" ) == NULL )
	{
		double gain = 1.0; // no adjustment
		
		if ( mlt_properties_get( filter_props, "gain" ) != NULL )
		{
			char *p = mlt_properties_get( filter_props, "gain" );
			
			if ( strncaseeq( p, "normalise", 9 ) )
				mlt_properties_set( filter_props, "normalise", "" );
			else
			{
				if ( strcmp( p, "" ) != 0 )
					gain = fabs( strtod( p, &p) );

				while ( isspace( *p ) )
					p++;

				/* check if "dB" is given after number */
				if ( strncaseeq( p, "db", 2 ) )
					gain = DBFSTOAMP( gain );
			}
		}
		mlt_properties_set_double( properties, "gain", gain );
	}
	
	// Propogate the maximum gain property
	if ( mlt_properties_get( filter_props, "max_gain" ) != NULL )
	{
		char *p = mlt_properties_get( filter_props, "max_gain" );
		double gain = fabs( strtod( p, &p) ); // 0 = no max
			
		while ( isspace( *p ) )
			p++;

		/* check if "dB" is given after number */
		if ( strncaseeq( p, "db", 2 ) )
			gain = DBFSTOAMP( gain );
			
		mlt_properties_set_double( properties, "volume.max_gain", gain );
	}

	// Parse and propogate the limiter property
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
		mlt_properties_set_double( properties, "volume.limiter", level );
	}

	// Parse and propogate the normalise property
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
		mlt_properties_set_int( properties, "volume.normalise", 1 );
		mlt_properties_set_double( properties, "volume.amplitude", amplitude );
	}

	// Propogate the smoothing buffer properties
	mlt_properties_set_data( properties, "volume.smooth_buffer",
		mlt_properties_get_data( filter_props, "smooth_buffer", NULL ), 0, NULL, NULL );
	mlt_properties_set_data( properties, "volume.smooth_index",
		mlt_properties_get_data( filter_props, "smooth_index", NULL ), 0, NULL, NULL );

	// Backup the original get_audio (it's still needed)
	mlt_properties_set_data( properties, "volume.get_audio", frame->get_audio, 0, NULL, NULL );

	// Override the get_audio method
	frame->get_audio = filter_get_audio;

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_volume_init( char *arg )
{
	mlt_filter this = calloc( sizeof( struct mlt_filter_s ), 1 );
	if ( this != NULL && mlt_filter_init( this, NULL ) == 0 )
	{
		mlt_properties properties = mlt_filter_properties( this );
		this->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set( properties, "gain", arg );

		// Create a smoothing buffer for the calculated "max power" of frame of audio used in normalisation
		double *smooth_buffer = (double*) calloc( SMOOTH_BUFFER_SIZE, sizeof( double ) );
		int i;
		for ( i = 0; i < SMOOTH_BUFFER_SIZE; i++ )
			smooth_buffer[ i ] = -1.0;
		mlt_properties_set_data( mlt_filter_properties( this ), "smooth_buffer", smooth_buffer, 0, free, NULL );
		int *smooth_index = calloc( 1, sizeof( int ) );
		mlt_properties_set_data( mlt_filter_properties( this ), "smooth_index", smooth_index, 0, free, NULL );
	}
	return this;
}

