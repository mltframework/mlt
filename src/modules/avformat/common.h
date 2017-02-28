/*
 * common.h -- the factory method interfaces
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

#ifndef COMMON_H
#define COMMON_H

#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>

int avformat_set_luma_transfer( struct SwsContext *context, int src_colorspace,
	int dst_colorspace, int src_full_range, int dst_full_range );

enum AVPixelFormat avformat_map_pixfmt_mlt2av( mlt_image_format format );
mlt_image_format avformat_map_pixfmt_av2mlt( enum AVPixelFormat f );

#if LIBSWSCALE_VERSION_INT >= AV_VERSION_INT( 3, 1, 101 )
int avformat_colorspace_convert
(
	AVFrame* src, int src_colorspace, int src_full_range,
	AVFrame* dst, int dst_colorspace, int dst_full_range,
	int flags
);
#endif

#endif /* FACTORY_H */
