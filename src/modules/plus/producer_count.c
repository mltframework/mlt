/*
 * producer_count.c -- counting producer
 * Copyright (C) 2013 Brian Matherly
 * Author: Brian Matherly <pez4brian@yahoo.com>
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

/* Private Constants */
#define MAX_TEXT_LEN        512
#define LINE_PIXEL_VALUE    0x00
#define RING_PIXEL_VALUE    0xff
#define CLOCK_PIXEL_VALUE   0x50
#define FRAME_BACKGROUND_COLOR   "0xd0d0d0ff"
#define TEXT_BACKGROUND_COLOR    "0x00000000"
#define TEXT_FOREGROUND_COLOR    "0x000000ff"
// Ratio of graphic elements relative to image size
#define LINE_WIDTH_RATIO     1
#define OUTER_RING_RATIO    90
#define INNER_RING_RATIO    80
#define TEXT_SIZE_RATIO     70

typedef struct
{
	mlt_position position;
	int fps;
	int hours;
	int minutes;
	int seconds;
	int frames;
	char sep; // Either : or ; (for ndf)
} time_info;

static void get_time_info( mlt_producer producer, mlt_frame frame, time_info* info )
{
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	mlt_position position = mlt_frame_original_position( frame );

	info->fps = ceil( mlt_producer_get_fps( producer ) );

	char* direction = mlt_properties_get( producer_properties, "direction" );
	if( !strcmp( direction, "down" ) )
	{
		mlt_position length = mlt_properties_get_int( producer_properties, "length" );
		info->position = length - 1 - position;
	}
	else
	{
		info->position = position;
	}

	char* tc_str = NULL;
	if( mlt_properties_get_int( producer_properties, "drop" ) )
	{
		tc_str = mlt_properties_frames_to_time( producer_properties, info->position, mlt_time_smpte_df );
	}
	else
	{
		tc_str = mlt_properties_frames_to_time( producer_properties, info->position, mlt_time_smpte_ndf );
	}
	sscanf( tc_str, "%02d:%02d:%02d%c%d", &info->hours, &info->minutes, &info->seconds, &info->sep, &info->frames );
}

static inline void mix_pixel( uint8_t* image, int width, int x, int y, int value, float mix )
{
	uint8_t* p = image + (( y * width ) + x ) * 4;

	if( mix != 1.0 )
	{
		value = ((float)value * mix) + ((float)*p * (1.0 - mix));
	}

	*p = value;
	p++;
	*p = value;
	p++;
	*p = value;
}

/** Fill an audio buffer with 1kHz samples.
*/

static void fill_beep( mlt_properties producer_properties, float* buffer, int frequency, int channels, int samples )
{
	int s = 0;
	int c = 0;

	for( s = 0; s < samples; s++ )
	{
		float f = 1000.0;
		float t = (float)s/(float)frequency;
		float value = sin( 2*M_PI*f*t );

		for( c = 0; c < channels; c++ )
		{
			float* sample_ptr = buffer + (c * samples) + s;
			*sample_ptr = value;
		}
	}
}

static int producer_get_audio( mlt_frame frame, int16_t** buffer, mlt_audio_format* format, int* frequency, int* channels, int* samples )
{
	mlt_producer producer = (mlt_producer)mlt_frame_pop_audio( frame );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	char* sound = mlt_properties_get( producer_properties, "sound" );
	double fps = mlt_producer_get_fps( producer );
	mlt_position position = mlt_frame_original_position( frame );
	int size = 0;
	int do_beep = 0;
	time_info info;

	if( fps == 0 ) fps = 25;

	// Correct the returns if necessary
	*format = mlt_audio_float;
	*frequency = *frequency <= 0 ? 48000 : *frequency;
	*channels = *channels <= 0 ? 2 : *channels;
	*samples = *samples <= 0 ? mlt_sample_calculator( fps, *frequency, position ) : *samples;

	// Allocate the buffer
	size = *samples * *channels * sizeof( float );
	*buffer = mlt_pool_alloc( size );

	mlt_service_lock( MLT_PRODUCER_SERVICE( producer ) );

	get_time_info( producer, frame, &info );

	// Determine if this should be a tone or silence.
	if( strcmp( sound, "none") )
	{
		if( !strcmp( sound, "2pop" ) )
		{
			mlt_position out = mlt_properties_get_int( producer_properties, "out" );
			mlt_position frames = out - position;

			if( frames == ( info.fps * 2 ) )
			{
				do_beep = 1;
			}
		}
		else if( !strcmp( sound, "frame0" ) )
		{
			if( info.frames == 0 )
			{
				do_beep = 1;
			}
		}
	}

	if( do_beep )
	{
		fill_beep( producer_properties, (float*)*buffer, *frequency, *channels, *samples );
	}
	else
	{
		// Fill silence.
		memset( *buffer, 0, size );
	}

	mlt_service_unlock( MLT_PRODUCER_SERVICE( producer ) );

	// Set the buffer for destruction
	mlt_frame_set_audio( frame, *buffer, *format, size, mlt_pool_release );
	return 0;
}

static mlt_frame get_background_frame( mlt_producer producer )
{
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	mlt_frame bg_frame = NULL;
	mlt_producer color_producer = mlt_properties_get_data( producer_properties, "_color_producer", NULL );

	if( !color_producer )
	{
		mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );
		color_producer = mlt_factory_producer( profile, mlt_environment( "MLT_PRODUCER" ), "colour:" );
		mlt_properties_set_data( producer_properties, "_color_producer", color_producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

		mlt_properties color_properties = MLT_PRODUCER_PROPERTIES( color_producer );
		mlt_properties_set( color_properties, "colour", FRAME_BACKGROUND_COLOR );
	}

	if( color_producer )
	{
		mlt_producer_seek( color_producer, 0 );
		mlt_service_get_frame( MLT_PRODUCER_SERVICE( color_producer ), &bg_frame, 0 );
	}

	return bg_frame;
}

static mlt_frame get_text_frame( mlt_producer producer, time_info* info  )
{
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	mlt_producer text_producer = mlt_properties_get_data( producer_properties, "_text_producer", NULL );
	mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );
	mlt_frame text_frame = NULL;

	if( !text_producer )
	{
		text_producer = mlt_factory_producer( profile, mlt_environment( "MLT_PRODUCER" ), "qtext:" );

		// Save the producer for future use.
		mlt_properties_set_data( producer_properties, "_text_producer", text_producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

		// Calculate the font size.
		char font_size[MAX_TEXT_LEN];
		snprintf( font_size, MAX_TEXT_LEN - 1, "%dpx", profile->height * TEXT_SIZE_RATIO / 100 );

		// Configure the producer.
		mlt_properties text_properties = MLT_PRODUCER_PROPERTIES( text_producer );
		mlt_properties_set( text_properties, "size", font_size );
		mlt_properties_set( text_properties, "weight", "400" );
		mlt_properties_set( text_properties, "fgcolour", TEXT_FOREGROUND_COLOR );
		mlt_properties_set( text_properties, "bgcolour", TEXT_BACKGROUND_COLOR );
		mlt_properties_set( text_properties, "pad", "0" );
		mlt_properties_set( text_properties, "outline", "0" );
		mlt_properties_set( text_properties, "align", "center" );
	}

	if( text_producer )
	{
		mlt_properties text_properties = MLT_PRODUCER_PROPERTIES( text_producer );
		char* style = mlt_properties_get( producer_properties, "style" );
		char text[MAX_TEXT_LEN] = "";

		// Apply the time style
		if( !strcmp( style, "frames" ) )
		{
			snprintf( text, MAX_TEXT_LEN - 1, MLT_POSITION_FMT, info->position );
		}
		else if( !strcmp( style, "timecode" ) )
		{
			snprintf( text, MAX_TEXT_LEN - 1, "%02d:%02d:%02d%c%0*d",
					info->hours, info->minutes, info->seconds, info->sep,
					( info->fps > 999? 4 : info->fps > 99? 3 : 2 ), info->frames );
		}
		else if( !strcmp( style, "clock" ) )
		{
			snprintf( text, MAX_TEXT_LEN - 1, "%.2d:%.2d:%.2d", info->hours, info->minutes, info->seconds );
		}
		else if( !strcmp( style, "seconds+1" ) )
		{
			snprintf( text, MAX_TEXT_LEN - 1, "%d", info->seconds + 1 );
		}
		else // seconds
		{
			snprintf( text, MAX_TEXT_LEN - 1, "%d", info->seconds );
		}

		mlt_properties_set( text_properties, "text", text );

		// Get the frame.
		mlt_service_get_frame( MLT_PRODUCER_SERVICE( text_producer ), &text_frame, 0 );
	}

	return text_frame;
}

static void draw_ring( uint8_t* image, mlt_profile profile, int radius, int line_width )
{
	float sar = mlt_profile_sar( profile );
	int x_center = profile->width / 2;
	int y_center = profile->height / 2;
	int max_radius = radius + line_width;
	int a = max_radius + 1;
	int b = 0;

	line_width += 1; // Compensate for aliasing.

	// Scan through each pixel in one quadrant of the circle.
	while( a-- )
	{
		b = ( max_radius / sar ) + 1.0;
		while( b-- )
		{
			// Use Pythagorean theorem to determine the distance from this pixel to the center.
			float a2 = a*a;
			float b2 = b*sar*b*sar;
			float c = sqrtf( a2 + b2 );
			float distance = c - radius;

			if( distance > 0 && distance < line_width )
			{
				// This pixel is within the ring.
				float mix = 1.0;

				if( distance < 1.0 )
				{
					// Antialias the outside of the ring
					mix = distance;
				}
				else if( (float)line_width - distance < 1.0 )
				{
					// Antialias the inside of the ring
					mix = (float)line_width - distance;
				}

				// Apply this value to all 4 quadrants of the circle.
				mix_pixel( image, profile->width, x_center + b, y_center - a, RING_PIXEL_VALUE, mix );
				mix_pixel( image, profile->width, x_center - b, y_center - a, RING_PIXEL_VALUE, mix );
				mix_pixel( image, profile->width, x_center + b, y_center + a, RING_PIXEL_VALUE, mix );
				mix_pixel( image, profile->width, x_center - b, y_center + a, RING_PIXEL_VALUE, mix );
			}
		}
	}
}

static void draw_cross( uint8_t* image, mlt_profile profile, int line_width )
{
	int x = 0;
	int y = 0;
	int i = 0;

	// Draw a horizontal line
	i = line_width;
	while( i-- )
	{
		y = ( profile->height - line_width ) / 2 + i;
		x = profile->width - 1;
		while( x-- )
		{
			mix_pixel( image, profile->width, x, y, LINE_PIXEL_VALUE, 1.0 );
		}
	}

	// Draw a vertical line
	line_width = lrint((float)line_width * mlt_profile_sar( profile ));
	i = line_width;
	while( i-- )
	{
		x = ( profile->width - line_width ) / 2 + i;
		y = profile->height - 1;
		while( y-- )
		{
			mix_pixel( image, profile->width, x, y, LINE_PIXEL_VALUE, 1.0 );
		}
	}
}

static void draw_clock( uint8_t* image, mlt_profile profile, int angle, int line_width )
{
	float sar = mlt_profile_sar( profile );
	int q = 0;
	int x_center = profile->width / 2;
	int y_center = profile->height / 2;

	line_width += 1; // Compensate for aliasing.

	// Look at each quadrant of the frame to see what should be done.
	for( q = 1; q <= 4; q++ )
	{
		int max_angle = q * 90;
		int x_sign = ( q == 1 || q == 2 ) ? 1 : -1;
		int y_sign = ( q == 1 || q == 4 ) ? 1 : -1;
		int x_start = x_center * x_sign;
		int y_start = y_center * y_sign;

		// Compensate for rounding error of even lengths
		// (there is no "middle" pixel so everything is offset).
		if( x_sign == 1 && profile->width % 2 == 0 ) x_start--;
		if( y_sign == -1 && profile->height % 2 == 0 ) y_start++;

		if( angle >= max_angle )
		{
			// This quadrant is completely behind the clock hand. Fill it in.
			int dx = x_start + x_sign;
			while( dx )
			{
				dx -= x_sign;
				int dy = y_start + y_sign;
				while( dy )
				{
					dy -= y_sign;
					mix_pixel( image, profile->width, x_center + dx, y_center - dy, CLOCK_PIXEL_VALUE, 1.0 );
				}
			}
		}
		else if ( max_angle - angle < 90 )
		{
			// This quadrant is partially filled
			// Calculate a point (vx,vy) that lies on the line created by the angle from 0,0.
			int vx = 0;
			int vy = y_start;
			float lv = 0;

			// Assume maximum y and calculate the corresponding x value
			// for a point at the other end of this line.
			if( x_sign * y_sign == 1 )
			{
				vx = x_sign * sar * y_center / tan( ( max_angle - angle ) * M_PI / 180.0 );
			}
			else
			{
				vx = x_sign * sar * y_center * tan( ( max_angle - angle ) * M_PI / 180.0 );
			}

			// Calculate the length of the line defined by vx,vy
			lv = sqrtf((float)(vx*vx)*sar*sar + (float)vy*vy);

			// Scan through each pixel in the quadrant counting up/down to 0,0.
			int dx = x_start + x_sign;
			while( dx )
			{
				dx -= x_sign;
				int dy = y_start + y_sign;
				while( dy )
				{
					dy -= y_sign;
					// Calculate the cross product to determine which side of
					// the line this pixel lies on.
					int xp = vx * (vy - dy) - vy * (vx - dx);
					xp = xp * -1; // Easier to work with positive. Positive number means "behind" the line.
					if( xp > 0 )
					{
						// This pixel is behind the clock hand and should be filled in.
						// Calculate the distance from the pixel to the line to determine
						// if it is part of the clock hand.
						float distance = (float)xp / lv;
						int val = CLOCK_PIXEL_VALUE;
						float mix = 1.0;

						if( distance < line_width )
						{
							// This pixel makes up the clock hand.
							val = LINE_PIXEL_VALUE;

							if( distance < 1.0 )
							{
								// Antialias the outside of the clock hand
								mix = distance;
							}
							else if( (float)line_width - distance < 1.0 )
							{
								// Antialias the inside of the clock hand
								mix_pixel( image, profile->width, x_center + dx, y_center - dy, CLOCK_PIXEL_VALUE, 1.0 );
								mix = (float)line_width - distance;
							}
						}

						mix_pixel( image, profile->width, x_center + dx, y_center - dy, val, mix );
					}
				}
			}
		}
	}
}

static void add_clock_to_frame( mlt_producer producer, mlt_frame frame, time_info* info )
{
	mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	uint8_t* image = NULL;
	mlt_image_format format = mlt_image_rgb24a;
	int size = 0;
	int width = profile->width;
	int height = profile->height;
	int line_width = LINE_WIDTH_RATIO * (width > height ? height : width) / 100;
	int radius = (width > height ? height : width) / 2;
	char* direction = mlt_properties_get( producer_properties, "direction" );
	int clock_angle = 0;

	mlt_frame_get_image( frame, &image, &format, &width, &height, 1 );

	// Calculate the angle for the clock.
	int frames = info->frames;
	if( !strcmp( direction, "down" ) )
	{
		frames = info->fps - info->frames - 1;
	}
	clock_angle = (frames + 1) * 360 / info->fps;

	draw_clock( image, profile, clock_angle, line_width );
	draw_cross( image, profile, line_width );
	draw_ring( image, profile, ( radius * OUTER_RING_RATIO ) / 100, line_width );
	draw_ring( image, profile, ( radius * INNER_RING_RATIO ) / 100, line_width );

	size = mlt_image_format_size( format, width, height, NULL );
	mlt_frame_set_image( frame, image, size, mlt_pool_release );
}


static void add_text_to_bg( mlt_producer producer, mlt_frame bg_frame, mlt_frame text_frame )
{
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	mlt_transition transition = mlt_properties_get_data( producer_properties, "_transition", NULL );

	if( !transition )
	{
		mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );
		transition = mlt_factory_transition( profile, "composite", NULL );

		// Save the transition for future use.
		mlt_properties_set_data( producer_properties, "_transition", transition, 0, ( mlt_destructor )mlt_transition_close, NULL );

		// Configure the transition.
		mlt_properties transition_properties = MLT_TRANSITION_PROPERTIES( transition );
		mlt_properties_set( transition_properties, "geometry", "0%/0%:100%x100%:100" );
		mlt_properties_set( transition_properties, "halign", "center" );
		mlt_properties_set( transition_properties, "valign", "middle" );
	}

	if( transition && bg_frame && text_frame )
	{
		// Apply the transition.
		mlt_transition_process( transition, bg_frame, text_frame );
	}
}

static int producer_get_image( mlt_frame frame, uint8_t** image, mlt_image_format* format, int* width, int* height, int writable )
{
	mlt_producer producer = mlt_frame_pop_service( frame );
	mlt_frame bg_frame = NULL;
	mlt_frame text_frame = NULL;
	int error = 1;
	int size = 0;
	char* background = mlt_properties_get( MLT_PRODUCER_PROPERTIES( producer ), "background" );
	time_info info;

	mlt_service_lock( MLT_PRODUCER_SERVICE( producer ) );

	get_time_info( producer, frame, &info );

	bg_frame = get_background_frame( producer );
	if( !strcmp( background, "clock" ) )
	{
		add_clock_to_frame( producer, bg_frame, &info );
	}
	text_frame = get_text_frame( producer, &info );
	add_text_to_bg( producer, bg_frame, text_frame );

	if( bg_frame )
	{
		// Get the image from the background frame.
		error = mlt_frame_get_image( bg_frame, image, format, width, height, writable );
		size = mlt_image_format_size( *format, *width, *height, NULL );
		// Detach the image from the bg_frame so it is not released.
		mlt_frame_set_image( bg_frame, *image, size, NULL );
		// Attach the image to the input frame.
		mlt_frame_set_image( frame, *image, size, mlt_pool_release );
		mlt_frame_close( bg_frame );
	}

	if( text_frame )
	{
		mlt_frame_close( text_frame );
	}

	mlt_service_unlock( MLT_PRODUCER_SERVICE( producer ) );

	return error;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );
	mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );

	if ( *frame != NULL )
	{
		// Obtain properties of frame
		mlt_properties frame_properties = MLT_FRAME_PROPERTIES( *frame );

		// Update time code on the frame
		mlt_frame_set_position( *frame, mlt_producer_frame( producer ) );

		mlt_properties_set_int( frame_properties, "progressive", 1 );
		mlt_properties_set_double( frame_properties, "aspect_ratio", mlt_profile_sar( profile ) );
		mlt_properties_set_int( frame_properties, "meta.media.width", profile->width );
		mlt_properties_set_int( frame_properties, "meta.media.height", profile->height );

		// Configure callbacks
		mlt_frame_push_service( *frame, producer );
		mlt_frame_push_get_image( *frame, producer_get_image );
		mlt_frame_push_audio( *frame, producer );
		mlt_frame_push_audio( *frame, producer_get_audio );
	}

	// Calculate the next time code
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer this )
{
	this->close = NULL;
	mlt_producer_close( this );
	free( this );
}

/** Initialize.
*/

mlt_producer producer_count_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create a new producer object
	mlt_producer producer = mlt_producer_new( profile );

	// Initialize the producer
	if ( producer )
	{
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
		mlt_properties_set( properties, "direction", "down" );
		mlt_properties_set( properties, "style", "seconds+1" );
		mlt_properties_set( properties, "sound", "none" );
		mlt_properties_set( properties, "background", "clock" );
		mlt_properties_set( properties, "drop", "0" );

		// Callback registration
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;
	}

	return producer;
}
