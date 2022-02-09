/*
 * filter_avcolour_space.c -- Colour space filter
 * Copyright (C) 2004-2022 Meltytech, LLC
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_profile.h>
#include <framework/mlt_producer.h>

// ffmpeg Header files
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

#include <stdio.h>
#include <stdlib.h>

#define MAX_THREADS (6)
#define IMAGE_ALIGN (1)

#if 0 // This test might come in handy elsewhere someday.
static int is_big_endian( )
{
	union { int i; char c[ 4 ]; } big_endian_test;
	big_endian_test.i = 1;

	return big_endian_test.c[ 0 ] != 1;
}
#endif

static int convert_mlt_to_av_cs( mlt_image_format format )
{
	int value = 0;

	switch( format )
	{
		case mlt_image_rgb:
			value = AV_PIX_FMT_RGB24;
			break;
		case mlt_image_rgba:
			value = AV_PIX_FMT_RGBA;
			break;
		case mlt_image_yuv422:
			value = AV_PIX_FMT_YUYV422;
			break;
		case mlt_image_yuv420p:
			value = AV_PIX_FMT_YUV420P;
			break;
		case mlt_image_yuv422p16:
			value = AV_PIX_FMT_YUV422P16LE;
			break;
		default:
			mlt_log_error( NULL, "[filter avcolor_space] Invalid format %s\n",
				mlt_image_format_name( format ) );
			break;
	}

	return value;
}

// returns set_lumage_transfer result
static int av_convert_image( uint8_t *out, uint8_t *in, int out_fmt, int in_fmt,
	int out_width, int out_height, int in_width, int in_height,
	int src_colorspace, int dst_colorspace, int use_full_range )
{
	int error = -1;
	struct SwsContext *context = sws_alloc_context();

	if (context) {
		AVFrame *avinframe = av_frame_alloc();
		AVFrame *avoutframe = av_frame_alloc();
		int flags = mlt_get_sws_flags(in_width, in_height, in_fmt, out_width, out_height, out_fmt);

		av_opt_set_int(context, "srcw", in_width, 0);
		av_opt_set_int(context, "srch", in_height, 0);
		av_opt_set_int(context, "src_format", in_fmt, 0);
		av_opt_set_int(context, "dstw", out_width, 0);
		av_opt_set_int(context, "dsth", out_height, 0);
		av_opt_set_int(context, "dst_format", out_fmt, 0);
		av_opt_set_int(context, "sws_flags", flags, 0);
#if LIBSWSCALE_VERSION_MAJOR >= 6
		av_opt_set_int(context, "threads", MIN(mlt_slices_count_normal(), MAX_THREADS), 0);
#endif
		error = sws_init_context(context, NULL, NULL);
		if (error < 0) {
			mlt_log_error(NULL, "[filter avcolor_space] Initializing swscale failed with %d (%s)\n", error, av_err2str(error));
			goto exit;
		}

		// Setup the input image
		avinframe->width = in_width;
		avinframe->height = in_height;
		avinframe->format = in_fmt;
		avinframe->sample_aspect_ratio.num = avinframe->sample_aspect_ratio.den = 1;
		avinframe->interlaced_frame = 0;
		avinframe->color_range = use_full_range? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
		avinframe->colorspace = SWS_CS_ITU709;
		av_image_fill_arrays(avinframe->data, avinframe->linesize, in, in_fmt, in_width, in_height, IMAGE_ALIGN);

		// Setup the output image
		av_frame_copy_props(avoutframe, avinframe);
		avoutframe->width = out_width;
		avoutframe->height = out_height;
		avoutframe->format = out_fmt;

		error = av_frame_get_buffer(avoutframe, 0);
		if (error < 0) {
			mlt_log_error(NULL, "[filter avcolor_space] Cannot allocate output frame buffer\n");
			goto exit;
		}

		// libswscale wants the RGB colorspace to be SWS_CS_DEFAULT, which is = SWS_CS_ITU601.
		if ( out_fmt == AV_PIX_FMT_RGB24 || out_fmt == AV_PIX_FMT_RGBA )
			dst_colorspace = 601;
		error = mlt_set_luma_transfer(context, src_colorspace, dst_colorspace, use_full_range, use_full_range);

		// Do the conversion
#if LIBSWSCALE_VERSION_MAJOR >= 6
		error = sws_scale_frame(context, avoutframe, avinframe);
#else
		error = sws_scale(context, (const uint8_t* const*) avinframe->data, avinframe->linesize, 0, in_height,
			avoutframe->data, avoutframe->linesize);
#endif
		if (error < 0) {
			mlt_log_error(NULL, "[filter avcolor_space] sws_scale failed with %d (%s)\n", error, av_err2str(error));
			goto exit;
		}

		// Copy the filter output into the output buffer
		if (out_fmt == AV_PIX_FMT_YUV420P) {
			int i = 0;
			int p = 0;
			int widths[3] = { out_width, out_width / 2, out_width / 2 };
			int heights[3] = { out_height, out_height / 2, out_height / 2 };
			uint8_t* dst = out;
			for (p = 0; p < 3; p++) {
				uint8_t* src = avoutframe->data[p];
				for (i = 0; i < heights[p]; i++) {
					memcpy(dst, src, widths[p]);
					dst += widths[p];
					src += avoutframe->linesize[p];
				}
			}
		} else {
			int i;
			uint8_t* dst = out;
			uint8_t* src = avoutframe->data[0];
			int stride = out_width * (out_fmt == AV_PIX_FMT_YUYV422? 2 : out_fmt == AV_PIX_FMT_RGB24? 3 : 4);
			for (i = 0; i < out_height; i++) {
				memcpy(dst, src, stride);
				dst += stride;
				src += avoutframe->linesize[0];
			}
		}
exit:
		sws_freeContext(context);
		av_frame_free(&avinframe);
		av_frame_free(&avoutframe);
	}

	return error;
}

/** Do it :-).
*/

static int convert_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, mlt_image_format output_format )
{
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	int error = 0;
	int out_width = mlt_properties_get_int(properties, "convert_image_width");
	int out_height = mlt_properties_get_int(properties, "convert_image_height");

	mlt_properties_clear(properties, "convert_image_width");
	mlt_properties_clear(properties, "convert_image_height");

	if ( *format != output_format || out_width )
	{
		mlt_profile profile = mlt_service_profile(
			MLT_PRODUCER_SERVICE( mlt_frame_get_original_producer( frame ) ) );
		int profile_colorspace = profile ? profile->colorspace : 601;
		int colorspace = mlt_properties_get_int( properties, "colorspace" );
		int force_full_luma = 0;
		int width = mlt_properties_get_int( properties, "width" );
		int height = mlt_properties_get_int( properties, "height" );

		if (out_width <= 0)
			out_width = width;
		if (out_height <= 0)
			out_height = height;
	
		mlt_log_debug( NULL, "[filter avcolor_space] %s @ %dx%d -> %s @ %dx%d space %d->%d\n",
			mlt_image_format_name( *format ), width, height, mlt_image_format_name( output_format ),
			out_width, out_height, colorspace, profile_colorspace );

		int in_fmt = convert_mlt_to_av_cs( *format );
		int out_fmt = convert_mlt_to_av_cs( output_format );
		int size = FFMAX( av_image_get_buffer_size(out_fmt, out_width, out_height, IMAGE_ALIGN),
			mlt_image_format_size( output_format, out_width, out_height, NULL ) );
		uint8_t *output = mlt_pool_alloc( size );

		if (out_width == width && out_height == height) {
			if ( *format == mlt_image_rgba )
			{
				register int len = width * height;
				uint8_t *alpha = mlt_pool_alloc( len );
	
				if ( alpha )
				{
				// Extract the alpha mask from the RGBA image using Duff's Device
					register uint8_t *s = *image + 3; // start on the alpha component
					register uint8_t *d = alpha;
					register int n = ( len + 7 ) / 8;
	
					switch ( len % 8 )
					{
						case 0:	do { *d++ = *s; s += 4;
						case 7:		 *d++ = *s; s += 4;
						case 6:		 *d++ = *s; s += 4;
						case 5:		 *d++ = *s; s += 4;
						case 4:		 *d++ = *s; s += 4;
						case 3:		 *d++ = *s; s += 4;
						case 2:		 *d++ = *s; s += 4;
						case 1:		 *d++ = *s; s += 4;
								}
								while ( --n > 0 );
					}
					mlt_frame_set_alpha( frame, alpha, len, mlt_pool_release );
				}
			}
		} else {
			// Scaling
			mlt_properties_clear(properties, "alpha");
		}

		// Update the output
		if ( !av_convert_image( output, *image, out_fmt, in_fmt,
		                        out_width, out_height, width, height,
		                        colorspace, profile_colorspace, force_full_luma ) )
		{
			// The new colorspace is only valid if destination is YUV.
			if ( output_format == mlt_image_yuv422 ||
				output_format == mlt_image_yuv420p ||
				output_format == mlt_image_yuv422p16 )
				mlt_properties_set_int( properties, "colorspace", profile_colorspace );
		}
		*image = output;
		*format = output_format;
		mlt_frame_set_image( frame, output, size, mlt_pool_release );
		mlt_properties_set_int( properties, "format", output_format );
		mlt_properties_set_int(properties, "width", out_width);
		mlt_properties_set_int(properties, "height", out_height);

		if (out_width == width && out_height == height)
		if ( output_format == mlt_image_rgba )
		{
			register int len = width * height;
			int alpha_size = 0;
			uint8_t *alpha = mlt_frame_get_alpha( frame );
			mlt_properties_get_data( properties, "alpha", &alpha_size );

			if ( alpha && alpha_size >= len )
			{
				// Merge the alpha mask from into the RGBA image using Duff's Device
				register uint8_t *s = alpha;
				register uint8_t *d = *image + 3; // start on the alpha component
				register int n = ( len + 7 ) / 8;

				switch ( len % 8 )
				{
					case 0:	do { *d = *s++; d += 4;
					case 7:		 *d = *s++; d += 4;
					case 6:		 *d = *s++; d += 4;
					case 5:		 *d = *s++; d += 4;
					case 4:		 *d = *s++; d += 4;
					case 3:		 *d = *s++; d += 4;
					case 2:		 *d = *s++; d += 4;
					case 1:		 *d = *s++; d += 4;
							}
							while ( --n > 0 );
				}
			}
		}
	}
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	// Set a default colorspace on the frame if not yet set by the producer.
	// The producer may still change it during get_image.
	// This way we do not have to modify each producer to set a valid colorspace.
	mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
	if ( mlt_properties_get_int( properties, "colorspace" ) <= 0 )
		mlt_properties_set_int( properties, "colorspace", mlt_service_profile( MLT_FILTER_SERVICE(filter) )->colorspace );

	if ( !frame->convert_image )
		frame->convert_image = convert_image;

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_avcolour_space_init( void *arg )
{
	// Test to see if swscale accepts the arg as resolution
	if ( arg )
	{
		int *width = (int*) arg;
		if ( *width > 0 )
		{
			struct SwsContext *context = sws_getContext( *width, *width, AV_PIX_FMT_RGB32, 64, 64, AV_PIX_FMT_RGB32, SWS_BILINEAR, NULL, NULL, NULL);
			if ( context )
				sws_freeContext( context );
			else
				return NULL;
		}
	}
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
		filter->process = filter_process;
	return filter;
}

