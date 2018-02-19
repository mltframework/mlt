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

#ifndef COMMON_H
#define COMMON_H

#include <framework/mlt.h>

int mlt_to_av_sample_format( mlt_audio_format format );
int64_t mlt_to_av_chan_layout( mlt_chan_cfg cfg );
mlt_chan_cfg av_chan_layout_to_mlt( int64_t layout );
mlt_chan_cfg get_chan_cfg_or_default( const char* chan_cfg_name, int channels );

#endif // COMMON_H
