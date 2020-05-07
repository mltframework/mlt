/*
 * common.h
 * Copyright (C) 2018 Meltytech, LLC
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

#include <libavutil/channel_layout.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>

int mlt_get_sws_flags(int srcwidth, int srcheight, int srcformat, int dstwidth, int dstheight, int dstformat)
{
	// Use default flags unless there is a reason to use something different.
	int flags = SWS_BICUBIC | SWS_FULL_CHR_H_INP | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND;

	if( srcwidth != dstwidth || srcheight != dstheight )
	{
		// Any resolution change should use default flags
		return flags;
	}

	const AVPixFmtDescriptor* srcDesc = av_pix_fmt_desc_get(srcformat);
	const AVPixFmtDescriptor* dstDesc = av_pix_fmt_desc_get(dstformat);

	if( !srcDesc || !dstDesc )
	{
		return flags;
	}

	if( (srcDesc->flags & AV_PIX_FMT_FLAG_RGB) != 0 &&
		(dstDesc->flags & AV_PIX_FMT_FLAG_RGB) == 0)
	{
		// RGB -> YUV
		flags = SWS_BICUBIC;
		flags |=  SWS_ACCURATE_RND;   // Can improve precision by one bit
		flags |=  SWS_FULL_CHR_H_INT; // Avoids luma reduction. Causes chroma bleeding when used with SWS_BICUBIC
	}
	else if( (srcDesc->flags & AV_PIX_FMT_FLAG_RGB) == 0 &&
			(dstDesc->flags & AV_PIX_FMT_FLAG_RGB) != 0)
	{
		// YUV -> RGB
		// Going from lower sampling to full sampling - so pick the closest sample without interpolation
		flags = SWS_POINT;
		flags |=  SWS_ACCURATE_RND;   // Can improve precision by one bit
		flags |=  SWS_FULL_CHR_H_INT; // Avoids luma reduction. Does not cause chroma bleeding when used with SWS_POINT
	}
	else if( (srcDesc->flags & AV_PIX_FMT_FLAG_RGB) == 0 &&
			(dstDesc->flags & AV_PIX_FMT_FLAG_RGB) == 0)
	{
		// YUV -> YUV
		if( srcDesc->log2_chroma_w == dstDesc->log2_chroma_w &&
			srcDesc->log2_chroma_h == dstDesc->log2_chroma_h )
		{
			// No chroma subsampling conversion. No interpolation required
			flags = SWS_POINT;
			flags |=  SWS_ACCURATE_RND; // Can improve precision by one bit
		}
		else
		{
			// Chroma will be interpolated. Bilinear is suitable.
			flags = SWS_BILINEAR | SWS_ACCURATE_RND;
		}
	}

	return flags;
}

int mlt_to_av_sample_format( mlt_audio_format format )
{
	switch( format )
	{
	case mlt_audio_none:
		return AV_SAMPLE_FMT_NONE;
	case mlt_audio_s16:
		return AV_SAMPLE_FMT_S16;
	case mlt_audio_s32:
		return AV_SAMPLE_FMT_S32P;
	case mlt_audio_float:
		return AV_SAMPLE_FMT_FLTP;
	case mlt_audio_s32le:
		return AV_SAMPLE_FMT_S32;
	case mlt_audio_f32le:
		return AV_SAMPLE_FMT_FLT;
	case mlt_audio_u8:
		return AV_SAMPLE_FMT_U8;
	}
	mlt_log_error( NULL, "[avformat] Unknown audio format: %d\n", format );
	return AV_SAMPLE_FMT_NONE;
}

int64_t mlt_to_av_channel_layout( mlt_channel_layout layout )
{
	switch( layout )
	{
		case mlt_channel_auto:
		case mlt_channel_independent:
			mlt_log_error( NULL, "[avformat] No matching channel layout: %s\n", mlt_audio_channel_layout_name( layout ) );
			return 0;
		case mlt_channel_mono:           return AV_CH_LAYOUT_MONO;
		case mlt_channel_stereo:         return AV_CH_LAYOUT_STEREO;
		case mlt_channel_2p1:            return AV_CH_LAYOUT_2POINT1;
		case mlt_channel_3p0:            return AV_CH_LAYOUT_SURROUND;
		case mlt_channel_3p0_back:       return AV_CH_LAYOUT_2_1;
		case mlt_channel_3p1:            return AV_CH_LAYOUT_3POINT1;
		case mlt_channel_4p0:            return AV_CH_LAYOUT_4POINT0;
		case mlt_channel_quad_back:      return AV_CH_LAYOUT_QUAD;
		case mlt_channel_quad_side:      return AV_CH_LAYOUT_2_2;
		case mlt_channel_5p0:            return AV_CH_LAYOUT_5POINT0;
		case mlt_channel_5p0_back:       return AV_CH_LAYOUT_5POINT0_BACK;
		case mlt_channel_4p1:            return AV_CH_LAYOUT_4POINT1;
		case mlt_channel_5p1:            return AV_CH_LAYOUT_5POINT1;
		case mlt_channel_5p1_back:       return AV_CH_LAYOUT_5POINT1_BACK;
		case mlt_channel_6p0:            return AV_CH_LAYOUT_6POINT0;
		case mlt_channel_6p0_front:      return AV_CH_LAYOUT_6POINT0_FRONT;
		case mlt_channel_hexagonal:      return AV_CH_LAYOUT_HEXAGONAL;
		case mlt_channel_6p1:            return AV_CH_LAYOUT_6POINT1;
		case mlt_channel_6p1_back:       return AV_CH_LAYOUT_6POINT1_BACK;
		case mlt_channel_6p1_front:      return AV_CH_LAYOUT_6POINT1_FRONT;
		case mlt_channel_7p0:            return AV_CH_LAYOUT_7POINT0;
		case mlt_channel_7p0_front:      return AV_CH_LAYOUT_7POINT0_FRONT;
		case mlt_channel_7p1:            return AV_CH_LAYOUT_7POINT1;
		case mlt_channel_7p1_wide_side:  return AV_CH_LAYOUT_7POINT1_WIDE;
		case mlt_channel_7p1_wide_back:  return AV_CH_LAYOUT_7POINT1_WIDE_BACK;
	}
	mlt_log_error( NULL, "[avformat] Unknown channel configuration: %d\n", layout );
	return 0;
}

mlt_channel_layout av_channel_layout_to_mlt( int64_t layout )
{
	switch( layout )
	{
		case 0:                              return mlt_channel_independent;
		case AV_CH_LAYOUT_MONO:              return mlt_channel_mono;
		case AV_CH_LAYOUT_STEREO:            return mlt_channel_stereo;
		case AV_CH_LAYOUT_STEREO_DOWNMIX:    return mlt_channel_stereo;
		case AV_CH_LAYOUT_2POINT1:           return mlt_channel_2p1;
		case AV_CH_LAYOUT_SURROUND:          return mlt_channel_3p0;
		case AV_CH_LAYOUT_2_1:               return mlt_channel_3p0_back;
		case AV_CH_LAYOUT_3POINT1:           return mlt_channel_3p1;
		case AV_CH_LAYOUT_4POINT0:           return mlt_channel_4p0;
		case AV_CH_LAYOUT_QUAD:              return mlt_channel_quad_back;
		case AV_CH_LAYOUT_2_2:               return mlt_channel_quad_side;
		case AV_CH_LAYOUT_5POINT0:           return mlt_channel_5p0;
		case AV_CH_LAYOUT_5POINT0_BACK:      return mlt_channel_5p0_back;
		case AV_CH_LAYOUT_4POINT1:           return mlt_channel_4p1;
		case AV_CH_LAYOUT_5POINT1:           return mlt_channel_5p1;
		case AV_CH_LAYOUT_5POINT1_BACK:      return mlt_channel_5p1_back;
		case AV_CH_LAYOUT_6POINT0:           return mlt_channel_6p0;
		case AV_CH_LAYOUT_6POINT0_FRONT:     return mlt_channel_6p0_front;
		case AV_CH_LAYOUT_HEXAGONAL:         return mlt_channel_hexagonal;
		case AV_CH_LAYOUT_6POINT1:           return mlt_channel_6p1;
		case AV_CH_LAYOUT_6POINT1_BACK:      return mlt_channel_6p1_back;
		case AV_CH_LAYOUT_6POINT1_FRONT:     return mlt_channel_6p1_front;
		case AV_CH_LAYOUT_7POINT0:           return mlt_channel_7p0;
		case AV_CH_LAYOUT_7POINT0_FRONT:     return mlt_channel_7p0_front;
		case AV_CH_LAYOUT_7POINT1:           return mlt_channel_7p1;
		case AV_CH_LAYOUT_7POINT1_WIDE:      return mlt_channel_7p1_wide_side;
		case AV_CH_LAYOUT_7POINT1_WIDE_BACK: return mlt_channel_7p1_wide_back;
	}
	mlt_log_error( NULL, "[avformat] Unknown channel layout: %lu\n", (unsigned long)layout );
	return mlt_channel_independent;
}

mlt_channel_layout mlt_get_channel_layout_or_default( const char* name, int channels )
{
	mlt_channel_layout layout = mlt_audio_channel_layout_id( name );
	if( layout == mlt_channel_auto ||
		( layout != mlt_channel_independent && mlt_audio_channel_layout_channels( layout ) != channels ) )
	{
		layout = mlt_audio_channel_layout_default( channels );
	}
	return layout;
}

int mlt_set_luma_transfer( struct SwsContext *context, int src_colorspace,
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
