/*
 * mlt_types.h -- provides forward definitions of all public types
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

#ifndef _MLT_TYPES_H_
#define _MLT_TYPES_H_

#include <stdint.h>

#include "mlt_pool.h"

typedef enum
{
	mlt_image_none = 0,
	mlt_image_rgb24,
	mlt_image_rgb24a,
	mlt_image_yuv422,
	mlt_image_yuv420p
}
mlt_image_format;

typedef enum
{
	mlt_audio_none = 0,
	mlt_audio_pcm
}
mlt_audio_format;

typedef enum
{
	mlt_whence_relative_start,
	mlt_whence_relative_current,
	mlt_whence_relative_end
}
mlt_whence;

typedef enum 
{
	invalid_type,
	unknown_type,
	producer_type,
	playlist_type,
	tractor_type,
	multitrack_type,
	filter_type,
	transition_type,
	consumer_type,
	field_type
}
mlt_service_type;

typedef int32_t mlt_position;
typedef struct mlt_frame_s *mlt_frame, **mlt_frame_ptr;
typedef struct mlt_properties_s *mlt_properties;
typedef struct mlt_event_struct *mlt_event;
typedef struct mlt_service_s *mlt_service;
typedef struct mlt_producer_s *mlt_producer;
typedef struct mlt_manager_s *mlt_manager;
typedef struct mlt_playlist_s *mlt_playlist;
typedef struct mlt_multitrack_s *mlt_multitrack;
typedef struct mlt_filter_s *mlt_filter;
typedef struct mlt_transition_s *mlt_transition;
typedef struct mlt_tractor_s *mlt_tractor;
typedef struct mlt_field_s *mlt_field;
typedef struct mlt_consumer_s *mlt_consumer;
typedef struct mlt_parser_s *mlt_parser;
typedef struct mlt_deque_s *mlt_deque;

typedef void ( *mlt_destructor )( void * );
typedef char *( *mlt_serialiser )( void *, int length );

#define MLT_SERVICE(x) ( ( mlt_service )( x ) )
#define MLT_PRODUCER(x) ( ( mlt_producer )( x ) )
#define MLT_MULTITRACK(x) ( ( mlt_multitrack )( x ) )
#define MLT_PLAYLIST(x) ( ( mlt_playlist )( x ) )
#define MLT_TRACTOR(x) ( ( mlt_tractor )( x ) )
#define MLT_FILTER(x) ( ( mlt_filter )( x ) )
#define MLT_TRANSITION(x) ( ( mlt_transition )( x ) )

#endif
