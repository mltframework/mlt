/*
 * common.h
 * Copyright (C) 2022 Meltytech, LLC
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

#ifndef COMMON_SWR_H
#define COMMON_SWR_H

#include <framework/mlt.h>
#include <libswresample/swresample.h>

typedef struct
{
	SwrContext* ctx;
	uint8_t** in_buffers;
	uint8_t** out_buffers;
	mlt_audio_format in_format;
	mlt_audio_format out_format;
	int in_frequency;
	int out_frequency;
	int in_channels;
	int out_channels;
	mlt_channel_layout in_layout;
	mlt_channel_layout out_layout;
} mlt_swr_private_data;

int mlt_configure_swr_context( mlt_service service, mlt_swr_private_data *pdata );
int mlt_free_swr_context( mlt_swr_private_data *pdata );


#endif // COMMON_H
