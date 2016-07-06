/*
 * filter_burn.c -- burning filter
 * Copyright (C) 2007 Stephane Fillod
 *
 * Filter taken from EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * BurningTV - burns incoming objects.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * Fire routine is taken from Frank Jan Sorensen's demo program.
 *
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
#include <string.h>
#include <math.h>
#include "utils.h"


#define MaxColor 120
#define Decay 15
#define MAGIC_THRESHOLD "50"

static RGB32 palette[256];

static void makePalette(void)
{
	int i, r, g, b;
	uint8_t *p = (uint8_t*) palette;

	for(i=0; i<MaxColor; i++) {
		HSItoRGB(4.6-1.5*i/MaxColor, (double)i/MaxColor, (double)i/MaxColor, &r, &g, &b);
		*p++ = r & 0xfe;
		*p++ = g & 0xfe;
		*p++ = b & 0xfe;
		p++;
	}
	for(i=MaxColor; i<256; i++) {
		if(r<255)r++;if(r<255)r++;if(r<255)r++;
		if(g<255)g++;
		if(g<255)g++;
		if(b<255)b++;
		if(b<255)b++;
		*p++ = r & 0xfe;
		*p++ = g & 0xfe;
		*p++ = b & 0xfe;
		p++;
	}
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	RGB32 *background;
	unsigned char *diff;
	unsigned char *buffer;

	// Get the filter
	mlt_filter filter = mlt_frame_pop_service( frame );

	// Get the image
	*format = mlt_image_rgb24a;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// Only process if we have no error and a valid colour space
	if ( error == 0 )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_position pos = mlt_filter_get_position( filter, frame );
		mlt_position len = mlt_filter_get_length2( filter, frame );

		// Get the "Burn the foreground" value
		int burn_foreground = mlt_properties_get_int( properties, "foreground" );
		int animated_threshold = mlt_properties_anim_get_int( properties, "threshold", pos, len );
		int y_threshold = image_set_threshold_y( animated_threshold );

		// We'll process pixel by pixel
		int x = 0;
		int y = 0;
		int i;

		int video_width = *width;
		int video_height = *height;
		int video_area = video_width * video_height;
		// We need to create a new frame as this effect modifies the input
		RGB32 *dest = (RGB32*)*image;
		RGB32 *src = (RGB32*)*image;

		unsigned char v, w;
		RGB32 a, b, c;

		mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

		diff = mlt_properties_get_data( MLT_FILTER_PROPERTIES( filter ), "_diff", NULL );
		if (diff == NULL)
		{
			// TODO: What if the image size changes?
			diff = mlt_pool_alloc(video_area*sizeof(unsigned char));
			mlt_properties_set_data( MLT_FILTER_PROPERTIES( filter ), "_diff", 
					diff, video_area*sizeof(unsigned char), mlt_pool_release, NULL );
		}

		buffer = mlt_properties_get_data( MLT_FILTER_PROPERTIES( filter ), "_buffer", NULL );
		if (buffer == NULL)
		{
			// TODO: What if the image size changes?
			buffer = mlt_pool_alloc(video_area*sizeof(unsigned char));
			memset(buffer, 0, video_area*sizeof(unsigned char));
			mlt_properties_set_data( MLT_FILTER_PROPERTIES( filter ), "_buffer", 
					buffer, video_area*sizeof(unsigned char), mlt_pool_release, NULL );
		}

		if (burn_foreground == 1) {
			/* to burn the foreground, we need a background */
			background = mlt_properties_get_data( MLT_FILTER_PROPERTIES( filter ), 
						"_background", NULL );
			if (background == NULL)
			{
				// TODO: What if the image size changes?
				background = mlt_pool_alloc(video_area*sizeof(RGB32));
				image_bgset_y(background, src, video_area, y_threshold);
				mlt_properties_set_data( MLT_FILTER_PROPERTIES( filter ), "_background", 
					background, video_area*sizeof(RGB32), mlt_pool_release, NULL );
			}
		}

		if (burn_foreground == 1) {
			image_bgsubtract_y(diff, background, src, video_area, y_threshold);
		} else {
			/* default */
			image_y_over(diff, src, video_area, y_threshold);
		}
	
		for(x=1; x<video_width-1; x++) {
			v = 0;
			for(y=0; y<video_height-1; y++) {
				w = diff[y*video_width+x];
				buffer[y*video_width+x] |= v ^ w;
				v = w;
			}
		}
		for(x=1; x<video_width-1; x++) {
			i = video_width + x;
			for(y=1; y<video_height; y++) {
				v = buffer[i];
				if(v<Decay)
					buffer[i-video_width] = 0;
				else
					buffer[i-video_width+fastrand()%3-1] = v - (fastrand()&Decay);
				i += video_width;
			}
		}
	
		i = 1;
		for(y=0; y<video_height; y++) {
			for(x=1; x<video_width-1; x++) {
				/* FIXME: endianess? */
				a = (src[i] & 0xfefeff) + palette[buffer[i]];
				b = a & 0x1010100;
				// Add alpha if necessary or use src alpha.
				c = palette[buffer[i]] ? 0xff000000 : src[i] & 0xff000000;
				dest[i] = a | (b - (b >> 8)) | c;
				i++;
			}
			i += 2;
		}

		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	// Push the frame filter
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_burn_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "foreground", "0" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "threshold", MAGIC_THRESHOLD );
	}
	if (!palette[128])
	{
		makePalette();
	}
	return filter;
}

