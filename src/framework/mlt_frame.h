/*
 * mlt_frame.h -- interface for all frame classes
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#ifndef _MLT_FRAME_H_
#define _MLT_FRAME_H_

#include "mlt_properties.h"

typedef enum
{
	mlt_image_none = 0,
	mlt_image_rgb24,
	mlt_image_rgb24a,
	mlt_image_yuv422,
	mlt_image_yuv420p
}
mlt_image_format;

typedef enum
{
	mlt_video_standard_pal = 0,
	mlt_video_standard_ntsc
}
mlt_video_standard;

typedef enum
{
	mlt_audio_none = 0,
	mlt_audio_pcm
}
mlt_audio_format;

typedef int ( *mlt_get_image )( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable );

struct mlt_frame_s
{
	// We're extending properties here
	struct mlt_properties_s parent;

	// Virtual methods
	int ( *get_audio )( mlt_frame this, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples );
	uint8_t * ( *get_alpha_mask )( mlt_frame this );
	
	// Private properties
	mlt_get_image stack_get_image[ 10 ];
	int stack_get_image_size;
	mlt_frame stack_frame[ 10 ];
	int stack_frame_size;
};

extern mlt_frame mlt_frame_init( );
extern mlt_properties mlt_frame_properties( mlt_frame this );
extern int mlt_frame_is_test_card( mlt_frame this );
extern double mlt_frame_get_aspect_ratio( mlt_frame this );
extern int mlt_frame_set_aspect_ratio( mlt_frame this, double value );
extern mlt_timecode mlt_frame_get_timecode( mlt_frame this );
extern int mlt_frame_set_timecode( mlt_frame this, mlt_timecode value );

extern int mlt_frame_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable );
extern uint8_t *mlt_frame_get_alpha_mask( mlt_frame this );
extern int mlt_frame_get_audio( mlt_frame this, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples );

extern int mlt_frame_push_get_image( mlt_frame this, mlt_get_image get_image );
extern mlt_get_image mlt_frame_pop_get_image( mlt_frame this );
extern int mlt_frame_push_frame( mlt_frame this, mlt_frame that );
extern mlt_frame mlt_frame_pop_frame( mlt_frame this );
extern void mlt_frame_close( mlt_frame this );

/* convenience functions */
extern int mlt_convert_rgb24a_to_yuv422( uint8_t *rgba, int width, int height, int stride, uint8_t *yuv, uint8_t *alpha );
extern int mlt_convert_rgb24_to_yuv422( uint8_t *rgb, int width, int height, int stride, uint8_t *yuv );
extern int mlt_frame_composite_yuv( mlt_frame this, mlt_frame that, int x, int y, float weight );
extern uint8_t *mlt_frame_resize_yuv422( mlt_frame this, int owidth, int oheight );
extern uint8_t *mlt_frame_rescale_yuv422( mlt_frame this, int owidth, int oheight );
extern void mlt_resize_yuv422( uint8_t *output, int owidth, int oheight, uint8_t *input, int iwidth, int iheight );

#endif

