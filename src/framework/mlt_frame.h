/**
 * \file mlt_frame.h
 * \brief interface for all frame classes
 * \see mlt_frame_s
 *
 * Copyright (C) 2003-2009 Ushodaya Enterprises Limited
 * \author Charles Yates <charles.yates@pandora.be>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MLT_FRAME_H_
#define _MLT_FRAME_H_

#include "mlt_properties.h"
#include "mlt_deque.h"
#include "mlt_service.h"

/** Callback function to get video data.
 *
 */

typedef int ( *mlt_get_image )( mlt_frame self, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable );

/** Callback function to get audio data.
 *
 */

typedef int ( *mlt_get_audio )( mlt_frame self, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples );

/** \brief Frame class
 *
 * The frame is the primary data object that gets passed around to and through services.
 *
 * \extends mlt_properties
 * \properties \em test_image set if the frame holds a "test card" image
 * \properties \em test_audio set if the frame holds "test card" audio
 * \properties \em _producer holds a reference to the frame's end producer
 * \properties \em _speed the current speed of the producer that generated the frame
 * \properties \em _position the position of the frame
 * \properties \em meta.* holds metadata
 * \properties \em hide set to 1 to hide the video, 2 to mute the audio
 * \properties \em last_track a flag to indicate an end-of-tracks frame
 * \properties \em previous \em frame a reference to the unfiltered preceding frame
 * (no speed factor applied, only available when \em _need_previous_next is set on the producer)
 * \properties \em next \em frame a reference to the unfiltered following frame
 * (no speed factor applied, only available when \em _need_previous_next is set on the producer)
 * \properties \em colorspace the standard for luma coefficients
 * \properties \em force_full_luma luma range handling, set to -1 for pass-through, 1 for full range, 0 for scaling
 * \properties \em audio_frequency the sample rate of the audio
 * \properties \em audio_channels the number of audio channels
 * \properties \em audio_samples the number of audio samples
 * \properties \em audio_format the mlt_audio_format for the audio on this frame
 * \properties \em format the mlt_image_format of the image on this frame
 * \properties \em width the horizontal resolution of the image
 * \properties \em height the vertical resolution of the image
 * \properties \em aspect_ratio the sample aspect ratio of the image
 */

struct mlt_frame_s
{
	struct mlt_properties_s parent; /**< \private A frame extends properties. */

	/** Get the alpha channel (callback function).
	 * \param self a frame
	 * \return the 8-bit alpha channel
	 */
	uint8_t * ( *get_alpha_mask )( mlt_frame self );

	/** Convert the image format (callback function).
	 * \param self a frame
	 * \param[in,out] image a buffer of image data
	 * \param[in,out] input the image format of supplied image data
	 * \param output the image format to which to convert
	 * \return true if error
	 */
	int ( *convert_image )( mlt_frame self, uint8_t **image, mlt_image_format *input, mlt_image_format output );

	/** Convert the audio format (callback function).
	 * \param self a frame
	 * \param[in,out] audio a buffer of audio data
	 * \param[in,out] input the audio format of supplied data
	 * \param output the audio format to which to convert
	 * \return true if error
	 */
	int ( *convert_audio )( mlt_frame self, void **audio, mlt_audio_format *input, mlt_audio_format output );

	mlt_deque stack_image;   /**< \private the image processing stack of operations and data */
	mlt_deque stack_audio;   /**< \private the audio processing stack of operations and data */
	mlt_deque stack_service; /**< \private a general purpose data stack */
	int is_processing;       /**< \private indicates if a frame is or was processed by the parallel consumer */
};

#define MLT_FRAME_PROPERTIES( frame )		( &( frame )->parent )
#define MLT_FRAME_SERVICE_STACK( frame )	( ( frame )->stack_service )
#define MLT_FRAME_IMAGE_STACK( frame )		( ( frame )->stack_image )
#define MLT_FRAME_AUDIO_STACK( frame )		( ( frame )->stack_audio )

extern mlt_frame mlt_frame_init( mlt_service service );
extern mlt_properties mlt_frame_properties( mlt_frame self );
extern int mlt_frame_is_test_card( mlt_frame self );
extern int mlt_frame_is_test_audio( mlt_frame self );
extern double mlt_frame_get_aspect_ratio( mlt_frame self );
extern int mlt_frame_set_aspect_ratio( mlt_frame self, double value );
extern mlt_position mlt_frame_get_position( mlt_frame self );
extern int mlt_frame_set_position( mlt_frame self, mlt_position value );
extern int mlt_frame_set_image( mlt_frame self, uint8_t *image, int size, mlt_destructor destroy );
extern int mlt_frame_set_alpha( mlt_frame self, uint8_t *alpha, int size, mlt_destructor destroy );
extern void mlt_frame_replace_image( mlt_frame self, uint8_t *image, mlt_image_format format, int width, int height );
extern int mlt_frame_get_image( mlt_frame self, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable );
extern uint8_t *mlt_frame_get_alpha_mask( mlt_frame self );
extern int mlt_frame_get_audio( mlt_frame self, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples );
extern int mlt_frame_set_audio( mlt_frame self, void *buffer, mlt_audio_format, int size, mlt_destructor );
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
extern mlt_properties mlt_frame_unique_properties( mlt_frame self, mlt_service service );

/* convenience functions */
extern int mlt_sample_calculator( float fps, int frequency, int64_t position );
extern int64_t mlt_sample_calculator_to_now( float fps, int frequency, int64_t position );
extern const char * mlt_image_format_name( mlt_image_format format );
extern int mlt_image_format_size( mlt_image_format format, int width, int height, int *bpp );
extern const char * mlt_audio_format_name( mlt_audio_format format );
extern int mlt_audio_format_size( mlt_audio_format format, int samples, int channels );
extern void mlt_frame_write_ppm( mlt_frame frame );

/** This macro scales RGB into the YUV gamut - y is scaled by 219/255 and uv by 224/255. */
#define RGB2YUV_601_SCALED(r, g, b, y, u, v)\
  y = ((263*r + 516*g + 100*b) >> 10) + 16;\
  u = ((-152*r - 300*g + 450*b) >> 10) + 128;\
  v = ((450*r - 377*g - 73*b) >> 10) + 128;

#endif
