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

#ifndef COMMON_H
#define COMMON_H

#include <framework/mlt.h>
#include <libswscale/swscale.h>

int mlt_to_av_sample_format( mlt_audio_format format );
int64_t mlt_to_av_channel_layout( mlt_channel_layout layout );
mlt_channel_layout av_channel_layout_to_mlt( int64_t layout );
mlt_channel_layout get_channel_layout_or_default( const char* name, int channels );
int set_luma_transfer( struct SwsContext *context, int src_colorspace,
	int dst_colorspace, int src_full_range, int dst_full_range );

#endif // COMMON_H
