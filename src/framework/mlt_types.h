/**
 * \file mlt_types.h
 * \brief Provides forward definitions of all public types
 *
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

#ifndef MLT_TYPES_H
#define MLT_TYPES_H

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#include <inttypes.h>
#include <limits.h>
#include "mlt_pool.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/** The set of supported image formats */

typedef enum
{
	mlt_image_none = 0,/**< image not available */
	mlt_image_rgb24,   /**< 8-bit RGB */
	mlt_image_rgb24a,  /**< 8-bit RGB with alpha channel */
	mlt_image_yuv422,  /**< 8-bit YUV 4:2:2 packed */
	mlt_image_yuv420p, /**< 8-bit YUV 4:2:0 planar */
	mlt_image_opengl,  /**< (deprecated) suitable for OpenGL texture */
	mlt_image_glsl,    /**< for opengl module internal use only */
	mlt_image_glsl_texture, /**< an OpenGL texture name */
	mlt_image_yuv422p16, /**< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian */
	mlt_image_invalid
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
	mlt_audio_s32le,   /**< signed 32-bit interleaved PCM */
	mlt_audio_f32le,   /**< 32-bit interleaved floating point */
	mlt_audio_u8       /**< unsigned 8-bit interleaved PCM */
}
mlt_audio_format;

/** The time string formats */

typedef enum
{
	mlt_time_frames = 0, /**< frame count */
	mlt_time_clock,      /**< SMIL clock-value as [[hh:]mm:]ss[.fraction] */
	mlt_time_smpte_df,   /**< SMPTE timecode as [[[hh:]mm:]ss{:|;}]frames */
	mlt_time_smpte = mlt_time_smpte_df,   /**< Deprecated */
	mlt_time_smpte_ndf   /**< SMPTE NDF timecode as [[[hh:]mm:]ss:]frames */
}
mlt_time_format;

/** Interpolation methods for animation keyframes */

typedef enum {
	mlt_keyframe_discrete = 0, /**< non-interpolated; value changes instantaneously at the key frame */
	mlt_keyframe_linear,       /**< simple, constant pace from this key frame to the next */
	mlt_keyframe_smooth        /**< eased pacing from this keyframe to the next using a Catmull-Rom spline */
}
mlt_keyframe_type;

/** The relative time qualifiers */

typedef enum
{
	mlt_whence_relative_start = 0, /**< relative to the beginning */
	mlt_whence_relative_current,   /**< relative to the current position */
	mlt_whence_relative_end        /**< relative to the end */
}
mlt_whence;

/** The recognized subclasses of mlt_service */

typedef enum
{
	invalid_type = 0,           /**< invalid service */
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
#ifdef DOUBLE_MLT_POSITION
#define MLT_POSITION_FMT "%f"
#define MLT_POSITION_MOD(A, B) (A - B * ((int)(A / B)))
typedef double mlt_position;
#else
#define MLT_POSITION_MOD(A, B) A % B
#define MLT_POSITION_FMT "%d"
typedef int32_t mlt_position;
#endif

/** A rectangle type with coordinates, size, and opacity */

typedef struct {
	double x; /**< X coordinate */
	double y; /**< Y coordinate */
	double w; /**< width */
	double h; /**< height */
	double o; /**< opacity / mix-level */
}
mlt_rect;

/** A tuple of color components */

typedef struct {
	uint8_t r; /**< red */
	uint8_t g; /**< green */
	uint8_t b; /**< blue */
	uint8_t a; /**< alpha */
}
mlt_color;

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
typedef struct mlt_animation_s *mlt_animation;          /**< pointer to Property Animation object */
typedef struct mlt_slices_s *mlt_slices;                /**< pointer to Sliced processing context object */

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

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef CLAMP
#define CLAMP(x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))
#endif

#ifdef _WIN32
#include <pthread.h>
/* Win32 compatibility function declarations */
extern int usleep(unsigned int useconds);
#ifndef WIN_PTHREADS_TIME_H
extern int nanosleep( const struct timespec * rqtp, struct timespec * rmtp );
#endif
extern int setenv(const char *name, const char *value, int overwrite);
extern char* getlocale();

#define MLT_DIRLIST_DELIMITER ";"
#else
#define MLT_DIRLIST_DELIMITER ":"
#endif /* ifdef _WIN32 */

#endif
