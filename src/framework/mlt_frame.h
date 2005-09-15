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
#include "mlt_deque.h"

typedef int ( *mlt_get_image )( mlt_frame self, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable );
typedef int ( *mlt_get_audio )( mlt_frame self, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples );

struct mlt_frame_s
{
	// We're extending properties here
	struct mlt_properties_s parent;

	// Virtual methods
	uint8_t * ( *get_alpha_mask )( mlt_frame self );
	
	// Private properties
	mlt_deque stack_image;
	mlt_deque stack_audio;
	mlt_deque stack_service;
};

#define MLT_FRAME_PROPERTIES( frame )		( &( frame )->parent )
#define MLT_FRAME_SERVICE_STACK( frame )	( ( frame )->stack_service )
#define MLT_FRAME_IMAGE_STACK( frame )		( ( frame )->stack_image )
#define MLT_FRAME_AUDIO_STACK( frame )		( ( frame )->stack_audio )

extern mlt_frame mlt_frame_init( );
extern mlt_properties mlt_frame_properties( mlt_frame self );
extern int mlt_frame_is_test_card( mlt_frame self );
extern int mlt_frame_is_test_audio( mlt_frame self );
extern double mlt_frame_get_aspect_ratio( mlt_frame self );
extern int mlt_frame_set_aspect_ratio( mlt_frame self, double value );
extern mlt_position mlt_frame_get_position( mlt_frame self );
extern int mlt_frame_set_position( mlt_frame self, mlt_position value );
extern void mlt_frame_replace_image( mlt_frame self, uint8_t *image, mlt_image_format format, int width, int height );
extern int mlt_frame_get_image( mlt_frame self, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable );
extern uint8_t *mlt_frame_get_alpha_mask( mlt_frame self );
extern int mlt_frame_get_audio( mlt_frame self, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples );
extern unsigned char *mlt_frame_get_waveform( mlt_frame self, int w, int h );
extern int mlt_frame_push_get_image( mlt_frame self, mlt_get_image get_image );
extern mlt_get_image mlt_frame_pop_get_image( mlt_frame self );
extern int mlt_frame_push_frame( mlt_frame self, mlt_frame that );
extern mlt_frame mlt_frame_pop_frame( mlt_frame self );
extern int mlt_frame_push_service( mlt_frame self, void *that );
extern void *mlt_frame_pop_service( mlt_frame self );
extern int mlt_frame_push_service_int( mlt_frame self, int that );
extern int mlt_frame_pop_service_int( mlt_frame self );
extern int mlt_frame_push_audio( mlt_frame self, void *that );
extern void *mlt_frame_pop_audio( mlt_frame self );
extern mlt_deque mlt_frame_service_stack( mlt_frame self );
extern mlt_producer mlt_frame_get_original_producer( mlt_frame self );
extern void mlt_frame_close( mlt_frame self );

/* convenience functions */
extern int mlt_convert_rgb24a_to_yuv422( uint8_t *rgba, int width, int height, int stride, uint8_t *yuv, uint8_t *alpha );
extern int mlt_convert_rgb24_to_yuv422( uint8_t *rgb, int width, int height, int stride, uint8_t *yuv );
extern int mlt_convert_yuv420p_to_yuv422( uint8_t *yuv420p, int width, int height, int stride, uint8_t *yuv );
extern uint8_t *mlt_frame_resize_yuv422( mlt_frame self, int owidth, int oheight );
extern uint8_t *mlt_frame_rescale_yuv422( mlt_frame self, int owidth, int oheight );
extern void mlt_resize_yuv422( uint8_t *output, int owidth, int oheight, uint8_t *input, int iwidth, int iheight );
extern int mlt_frame_mix_audio( mlt_frame self, mlt_frame that, float weight_start, float weight_end, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples  );
extern int mlt_sample_calculator( float fps, int frequency, int64_t position );
extern int64_t mlt_sample_calculator_to_now( float fps, int frequency, int64_t position );

/* this macro scales rgb into the yuv gamut, y is scaled by 219/255 and uv by 224/255 */
#define RGB2YUV(r, g, b, y, u, v)\
  y = ((263*r + 516*g + 100*b) >> 10) + 16;\
  u = ((-152*r - 298*g + 450*b) >> 10) + 128;\
  v = ((450*r - 377*g - 73*b) >> 10) + 128;

/* this macro assumes the user has already scaled their rgb down into the broadcast limits */
#define RGB2YUV_UNSCALED(r, g, b, y, u, v)\
  y = (299*r + 587*g + 114*b) >> 10;\
  u = ((-169*r - 331*g + 500*b) >> 10) + 128;\
  v = ((500*r - 419*g - 81*b) >> 10) + 128;\
  y = y < 16 ? 16 : y;\
  u = u < 16 ? 16 : u;\
  v = v < 16 ? 16 : v;\
  y = y > 235 ? 235 : y;\
  u = u > 240 ? 240 : u;\
  v = v > 240 ? 240 : v

#define YUV2RGB( y, u, v, r, g, b ) \
  r = ((1192 * ( y - 16 ) + 1634 * ( v - 128 ) ) >> 10 ); \
  g = ((1192 * ( y - 16 ) - 832 * ( v - 128 ) - 400 * ( u - 128 ) ) >> 10 ); \
  b = ((1192 * ( y - 16 ) + 2066 * ( u - 128 ) ) >> 10 ); \
  r = r < 0 ? 0 : r > 255 ? 255 : r; \
  g = g < 0 ? 0 : g > 255 ? 255 : g; \
  b = b < 0 ? 0 : b > 255 ? 255 : b;

#endif
