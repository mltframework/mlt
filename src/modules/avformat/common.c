/*
 * common.c -- the factory method interfaces
 * Copyright (C) 2003-2017 Meltytech, LLC
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

#include <string.h>
#include <pthread.h>
#include <limits.h>
#include <float.h>

#include <framework/mlt.h>
#include <framework/mlt_slices.h>

// ffmpeg Header files
#include <libavformat/avformat.h>
#ifdef AVDEVICE
#include <libavdevice/avdevice.h>
#endif
#ifdef AVFILTER
#include <libavfilter/avfilter.h>
#endif
#include <libavutil/opt.h>

#include "common.h"

#if LIBSWSCALE_VERSION_INT >= AV_VERSION_INT( 3, 1, 101 )
struct sliced_pix_fmt_conv_t
{
	int slice_w;
	AVFrame *src_frame, *dst_frame;
	const AVPixFmtDescriptor *src_desc, *dst_desc;
	int flags, src_colorspace, dst_colorspace, src_full_range, dst_full_range;
};

static int sliced_h_pix_fmt_conv_proc( int id, int idx, int jobs, void* cookie )
{
	uint8_t *out[4];
	const uint8_t *in[4];
	int in_stride[4], out_stride[4];
	int src_v_chr_pos = -513, dst_v_chr_pos = -513, ret, i, slice_x, slice_w, h, mul, field, slices, interlaced = 0;

	struct SwsContext *sws;
	struct sliced_pix_fmt_conv_t* ctx = ( struct sliced_pix_fmt_conv_t* )cookie;

	interlaced = ctx->src_frame->interlaced_frame;
	field = ( interlaced ) ? ( idx & 1 ) : 0;
	idx = ( interlaced ) ? ( idx / 2 ) : idx;
	slices = ( interlaced ) ? ( jobs / 2 ) : jobs;
	mul = ( interlaced ) ? 2 : 1;
	h = ctx->src_frame->height >> !!interlaced;
	slice_w = ctx->slice_w;
	slice_x = slice_w * idx;
	slice_w = FFMIN( slice_w, ctx->src_frame->width - slice_x );

	if ( AV_PIX_FMT_YUV420P == ctx->src_frame->format )
		src_v_chr_pos = ( !interlaced ) ? 128 : ( !field ) ? 64 : 192;

	if ( AV_PIX_FMT_YUV420P == ctx->dst_frame->format )
		dst_v_chr_pos = ( !interlaced ) ? 128 : ( !field ) ? 64 : 192;

	mlt_log_debug( NULL, "%s:%d: [id=%d, idx=%d, jobs=%d], [%s -> %s], interlaced=%d, field=%d, slices=%d, mul=%d, h=%d, slice_w=%d, slice_x=%d ctx->src_desc=[log2_chroma_h=%d, log2_chroma_w=%d], src_v_chr_pos=%d, dst_v_chr_pos=%d\n",
		__FUNCTION__, __LINE__, id, idx, jobs, av_get_pix_fmt_name( ctx->src_frame->format ), av_get_pix_fmt_name( ctx->dst_frame->format ),
		interlaced, field, slices, mul, h, slice_w, slice_x, ctx->src_desc->log2_chroma_h, ctx->src_desc->log2_chroma_w, src_v_chr_pos, dst_v_chr_pos );

	if ( slice_w <= 0 )
		return 0;

	if ( AV_PIX_FMT_RGB24 == ctx->dst_frame->format || AV_PIX_FMT_RGBA == ctx->dst_frame->format )
		ctx->flags |= SWS_FULL_CHR_H_INT;
	else if ( AV_PIX_FMT_YUYV422 == ctx->dst_frame->format || AV_PIX_FMT_YUV422P16 == ctx->dst_frame->format )
		ctx->flags |= SWS_FULL_CHR_H_INP;

	sws = sws_alloc_context();

	av_opt_set_int( sws, "srcw", slice_w, 0 );
	av_opt_set_int( sws, "srch", h, 0 );
	av_opt_set_int( sws, "src_format", ctx->src_frame->format, 0 );
	av_opt_set_int( sws, "dstw", slice_w, 0 );
	av_opt_set_int( sws, "dsth", h, 0 );
	av_opt_set_int( sws, "dst_format", ctx->dst_frame->format, 0 );
	av_opt_set_int( sws, "sws_flags", ctx->flags, 0 );

	av_opt_set_int( sws, "src_h_chr_pos", -513, 0 );
	av_opt_set_int( sws, "src_v_chr_pos", src_v_chr_pos, 0 );
	av_opt_set_int( sws, "dst_h_chr_pos", -513, 0 );
	av_opt_set_int( sws, "dst_v_chr_pos", dst_v_chr_pos, 0 );

	if ( ( ret = sws_init_context( sws, NULL, NULL ) ) < 0 )
	{
		mlt_log_error( NULL, "%s:%d: sws_init_context failed, ret=%d\n", __FUNCTION__, __LINE__, ret );
		sws_freeContext( sws );
		return 0;
	}

	avformat_set_luma_transfer( sws, ctx->src_colorspace, ctx->dst_colorspace, ctx->src_full_range, ctx->dst_full_range );

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(55, 0, 100)
#define PIX_DESC_BPP(DESC) (DESC.step_minus1 + 1)
#else
#define PIX_DESC_BPP(DESC) (DESC.step)
#endif

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 32, 100)
#define AV_PIX_FMT_FLAG_PLANAR PIX_FMT_PLANAR
#endif
	for( i = 0; i < 4; i++ )
	{
		int in_offset = (AV_PIX_FMT_FLAG_PLANAR & ctx->src_desc->flags)
			? ( ( 1 == i || 2 == i ) ? ( slice_x >> ctx->src_desc->log2_chroma_w ) : slice_x )
			: ( ( 0 == i ) ? slice_x : 0 );

		int out_offset = (AV_PIX_FMT_FLAG_PLANAR & ctx->dst_desc->flags)
			? ( ( 1 == i || 2 == i ) ? ( slice_x >> ctx->dst_desc->log2_chroma_w ) : slice_x )
			: ( ( 0 == i ) ? slice_x : 0 );

		in_offset *= PIX_DESC_BPP(ctx->src_desc->comp[i]);
		out_offset *= PIX_DESC_BPP(ctx->dst_desc->comp[i]);

		in_stride[i]  = ctx->src_frame->linesize[i] * mul;
		out_stride[i] = ctx->dst_frame->linesize[i] * mul;

		in[i] =  ctx->src_frame->data[i] + ctx->src_frame->linesize[i] * field + in_offset;
		out[i] = ctx->dst_frame->data[i] + ctx->dst_frame->linesize[i] * field + out_offset;
	}

	sws_scale( sws, in, in_stride, 0, h, out, out_stride );

	sws_freeContext( sws );

	return 0;
}

int avformat_colorspace_convert
(
	AVFrame* src, int src_colorspace, int src_full_range,
	AVFrame* dst, int dst_colorspace, int dst_full_range,
	int flags
)
{
	int c, i;

	struct sliced_pix_fmt_conv_t ctx =
	{
		.flags = flags,
		.src_frame = src,
		.dst_frame = dst,
		.src_colorspace = src_colorspace,
		.dst_colorspace = dst_colorspace,
		.src_full_range = src_full_range,
		.dst_full_range = dst_full_range,
	};

	ctx.src_desc = av_pix_fmt_desc_get( ctx.src_frame->format );
	ctx.dst_desc = av_pix_fmt_desc_get( ctx.dst_frame->format );

	if ( !getenv("MLT_AVFORMAT_SLICED_PIXFMT_DISABLE") )
		ctx.slice_w = ( src->width < 1000 )
			? ( 256 >> src->interlaced_frame )
			: ( 512 >> src->interlaced_frame );
	else
		ctx.slice_w = src->width;

	c = ( src->width + ctx.slice_w - 1 ) / ctx.slice_w;
	c *= src->interlaced_frame ? 2 : 1;

	if ( !getenv("MLT_AVFORMAT_SLICED_PIXFMT_DISABLE") )
		mlt_slices_run_normal( c, sliced_h_pix_fmt_conv_proc, &ctx );
	else
		for ( i = 0 ; i < c; i++ )
			sliced_h_pix_fmt_conv_proc( i, i, c, &ctx );

	return dst_colorspace;
}
#endif

mlt_image_format avformat_map_pixfmt_av2mlt( enum AVPixelFormat f )
{
	switch( f )
	{
		case AV_PIX_FMT_RGB24: return mlt_image_rgb24;
		case AV_PIX_FMT_RGBA: return mlt_image_rgb24a;
		case AV_PIX_FMT_YUYV422: return mlt_image_yuv422;
		case AV_PIX_FMT_YUV420P: return mlt_image_yuv420p;
		case AV_PIX_FMT_YUV422P16LE: return mlt_image_yuv422p16;

		default:
			mlt_log_error( NULL, "%s:%d Invalid av format %s\n",
				__FILE__, __LINE__,
				av_get_pix_fmt_name( f ) );
			break;

	};
	return mlt_image_invalid;
}

enum AVPixelFormat avformat_map_pixfmt_mlt2av( mlt_image_format format )
{
	enum AVPixelFormat value = AV_PIX_FMT_NONE;

	switch( format )
	{
		case mlt_image_rgb24:
			value = AV_PIX_FMT_RGB24;
			break;
		case mlt_image_rgb24a:
		case mlt_image_opengl:
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
			mlt_log_error( NULL, "%s:%d Invalid mlt format %s\n",
				__FILE__, __LINE__,
				mlt_image_format_name( format ) );
			break;
	}

	return value;
}

int avformat_set_luma_transfer( struct SwsContext *context, int src_colorspace,
	int dst_colorspace, int src_full_range, int dst_full_range )
{
	const int *src_coefficients = sws_getCoefficients( SWS_CS_DEFAULT );
	const int *dst_coefficients = sws_getCoefficients( SWS_CS_DEFAULT );
	int brightness = 0;
	int contrast = 1 << 16;
	int saturation = 1  << 16;
	int src_range = src_full_range ? 1 : 0;
	int dst_range = dst_full_range ? 1 : 0;

	switch ( src_colorspace )
	{
	case 170:
	case 470:
	case 601:
	case 624:
		src_coefficients = sws_getCoefficients( SWS_CS_ITU601 );
		break;
	case 240:
		src_coefficients = sws_getCoefficients( SWS_CS_SMPTE240M );
		break;
	case 709:
		src_coefficients = sws_getCoefficients( SWS_CS_ITU709 );
		break;
	default:
		break;
	}
	switch ( dst_colorspace )
	{
	case 170:
	case 470:
	case 601:
	case 624:
		dst_coefficients = sws_getCoefficients( SWS_CS_ITU601 );
		break;
	case 240:
		dst_coefficients = sws_getCoefficients( SWS_CS_SMPTE240M );
		break;
	case 709:
		dst_coefficients = sws_getCoefficients( SWS_CS_ITU709 );
		break;
	default:
		break;
	}
	return sws_setColorspaceDetails( context, src_coefficients, src_range, dst_coefficients, dst_range,
		brightness, contrast, saturation );
}
