/*
 * common.h
 * Copyright (C) 2018 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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
#include <libavutil/samplefmt.h>

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
			mlt_log_error( NULL, "[avformat] No matching channel layout: %s\n", mlt_channel_layout_name( layout ) );
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

mlt_channel_layout get_channel_layout_or_default( const char* name, int channels )
{
	mlt_channel_layout layout = mlt_channel_layout_id( name );
	if( layout == mlt_channel_auto ||
		( layout != mlt_channel_independent && mlt_channel_layout_channels( layout ) != channels ) )
	{
		layout = mlt_channel_layout_default( channels );
	}
	return layout;
}
