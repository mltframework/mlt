/**
 * \file mlt_types.h
 * \brief Provides forward definitions of all public types
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

#ifndef _MLT_TYPES_H_
#define _MLT_TYPES_H_

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#include <inttypes.h>

#include "mlt_pool.h"

/** The set of supported image formats */

typedef enum
{
	mlt_image_none = 0,/**< image not available */
	mlt_image_rgb24,   /**< 8-bit RGB */
	mlt_image_rgb24a,  /**< 8-bit RGB with alpha channel */
	mlt_image_yuv422,  /**< 8-bit YUV 4:2:2 packed */
	mlt_image_yuv420p, /**< 8-bit YUV 4:2:0 planar */
	mlt_image_opengl   /**< suitable for OpenGL texture */
}
mlt_image_format;

/** The set of supported audio formats */

typedef enum
{
	mlt_audio_none = 0,/**< audio not available */
	mlt_audio_pcm = 1, /**< \deprecated signed 16-bit interleaved PCM */
	mlt_audio_s16 = 1, /**< signed 16-bit interleaved PCM */
	mlt_audio_s32,     /**< signed 32-bit non-interleaved PCM */
	mlt_audio_float,   /**< 32-bit non-interleaved floating point */
	mlt_audio_s32le,   /**< signed 32-bit interleaved PCM, may only used by producers */
	mlt_audio_f32le    /**< 32-bit interleaved floating point, may only be used by producers */
}
mlt_audio_format;

/** The relative time qualifiers */

typedef enum
{
	mlt_whence_relative_start,  /**< relative to the beginning */
	mlt_whence_relative_current,/**< relative to the current position */
	mlt_whence_relative_end     /**< relative to the end */
}
mlt_whence;

/** The recognized subclasses of mlt_service */

typedef enum
{
	invalid_type,               /**< invalid service */
	unknown_type,               /**< unknown class */
	producer_type,              /**< Producer class */
	tractor_type,               /**< Tractor class */
	playlist_type,              /**< Playlist class */
	multitrack_type,            /**< Multitrack class */
	filter_type,                /**< Filter class */
	transition_type,            /**< Transition class */
	consumer_type,              /**< Consumer class */
	field_type                  /**< Field class */
}
mlt_service_type;

/* I don't want to break anyone's applications without warning. -Zach */
#undef DOUBLE_MLT_POSITION
#ifdef DOUBLE_MLT_POSITION
typedef double mlt_position;
#else
typedef int32_t mlt_position;
#endif

typedef struct mlt_frame_s *mlt_frame, **mlt_frame_ptr; /**< pointer to Frame object */
typedef struct mlt_property_s *mlt_property;            /**< pointer to Property object */
typedef struct mlt_properties_s *mlt_properties;        /**< pointer to Properties object */
typedef struct mlt_event_struct *mlt_event;             /**< pointer to Event object */
typedef struct mlt_service_s *mlt_service;              /**< pointer to Service object */
typedef struct mlt_producer_s *mlt_producer;            /**< pointer to Producer object */
typedef struct mlt_playlist_s *mlt_playlist;            /**< pointer to Playlist object */
typedef struct mlt_multitrack_s *mlt_multitrack;        /**< pointer to Multitrack object */
typedef struct mlt_filter_s *mlt_filter;                /**< pointer to Filter object */
typedef struct mlt_transition_s *mlt_transition;        /**< pointer to Transition object */
typedef struct mlt_tractor_s *mlt_tractor;              /**< pointer to Tractor object */
typedef struct mlt_field_s *mlt_field;                  /**< pointer to Field object */
typedef struct mlt_consumer_s *mlt_consumer;            /**< pointer to Consumer object */
typedef struct mlt_parser_s *mlt_parser;                /**< pointer to Properties object */
typedef struct mlt_deque_s *mlt_deque;                  /**< pointer to Deque object */
typedef struct mlt_geometry_s *mlt_geometry;            /**< pointer to Geometry object */
typedef struct mlt_geometry_item_s *mlt_geometry_item;  /**< pointer to Geometry Item object */
typedef struct mlt_profile_s *mlt_profile;              /**< pointer to Profile object */
typedef struct mlt_repository_s *mlt_repository;        /**< pointer to Repository object */
typedef struct mlt_cache_s *mlt_cache;                  /**< pointer to Cache object */
typedef struct mlt_cache_item_s *mlt_cache_item;        /**< pointer to CacheItem object */

typedef void ( *mlt_destructor )( void * );             /**< pointer to destructor function */
typedef char *( *mlt_serialiser )( void *, int length );/**< pointer to serialization function */

#define MLT_SERVICE(x)    ( ( mlt_service )( x ) )      /**< Cast to a Service pointer */
#define MLT_PRODUCER(x)   ( ( mlt_producer )( x ) )     /**< Cast to a Producer pointer */
#define MLT_MULTITRACK(x) ( ( mlt_multitrack )( x ) )   /**< Cast to a Multitrack pointer */
#define MLT_PLAYLIST(x)   ( ( mlt_playlist )( x ) )     /**< Cast to a Playlist pointer */
#define MLT_TRACTOR(x)    ( ( mlt_tractor )( x ) )      /**< Cast to a Tractor pointer */
#define MLT_FILTER(x)     ( ( mlt_filter )( x ) )       /**< Cast to a Filter pointer */
#define MLT_TRANSITION(x) ( ( mlt_transition )( x ) )   /**< Cast to a Transition pointer */
#define MLT_CONSUMER(x) ( ( mlt_consumer )( x ) )       /**< Cast to a Consumer pointer */
#define MLT_FRAME(x)      ( ( mlt_frame )( x ) )        /**< Cast to a Frame pointer */

#endif
