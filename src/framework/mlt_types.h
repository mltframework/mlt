/**
 * \file mlt_types.h
 * \brief Provides forward definitions of all public types
 *
 * Copyright (C) 2003-2025 Meltytech, LLC
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

#ifdef __cplusplus
extern "C" {
#endif

#include "mlt_export.h"
#include "mlt_pool.h"
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/** The set of supported image formats */

typedef enum {
    mlt_image_none = 0,       /**< image not available */
    mlt_image_rgb,            /**< 8-bit RGB */
    mlt_image_rgba,           /**< 8-bit RGB with alpha channel */
    mlt_image_yuv422,         /**< 8-bit YUV 4:2:2 packed */
    mlt_image_yuv420p,        /**< 8-bit YUV 4:2:0 planar */
    mlt_image_movit,          /**< for movit module internal use only */
    mlt_image_opengl_texture, /**< an OpenGL texture name */
    mlt_image_yuv422p16, /**< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian */
    mlt_image_yuv420p10, /**< planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian */
    mlt_image_yuv444p10, /**< planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian */
    mlt_image_rgba64, /**< 16-bit RGB with alpha channel */
    mlt_image_invalid
} mlt_image_format;

/** The set of supported audio formats */

typedef enum {
    mlt_audio_none = 0, /**< audio not available */
    mlt_audio_s16 = 1,  /**< signed 16-bit interleaved PCM */
    mlt_audio_s32,      /**< signed 32-bit non-interleaved PCM */
    mlt_audio_float,    /**< 32-bit non-interleaved floating point */
    mlt_audio_s32le,    /**< signed 32-bit interleaved PCM */
    mlt_audio_f32le,    /**< 32-bit interleaved floating point */
    mlt_audio_u8        /**< unsigned 8-bit interleaved PCM */
} mlt_audio_format;

typedef enum {
    mlt_channel_auto = 0, /**< MLT will determine the default configuration based on channel number */
    mlt_channel_independent, /**< channels are not related */
    mlt_channel_mono,
    mlt_channel_stereo,
    mlt_channel_2p1,
    mlt_channel_3p0,
    mlt_channel_3p0_back,
    mlt_channel_4p0,
    mlt_channel_quad_back,
    mlt_channel_quad_side,
    mlt_channel_3p1,
    mlt_channel_5p0_back,
    mlt_channel_5p0,
    mlt_channel_4p1,
    mlt_channel_5p1_back,
    mlt_channel_5p1,
    mlt_channel_6p0,
    mlt_channel_6p0_front,
    mlt_channel_hexagonal,
    mlt_channel_6p1,
    mlt_channel_6p1_back,
    mlt_channel_6p1_front,
    mlt_channel_7p0,
    mlt_channel_7p0_front,
    mlt_channel_7p1,
    mlt_channel_7p1_wide_side,
    mlt_channel_7p1_wide_back,
} mlt_channel_layout;

/** Colorspace definitions */

typedef enum {
    mlt_colorspace_rgb = 0, ///< order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB)
    mlt_colorspace_bt709 = 709, ///< also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
    mlt_colorspace_unspecified = 2,
    mlt_colorspace_reserved = 3,
    mlt_colorspace_fcc = 4, ///< FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
    mlt_colorspace_bt470bg
    = 470, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601
    mlt_colorspace_smpte170m = 170, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
    mlt_colorspace_smpte240m = 240,   ///< functionally identical to above
    mlt_colorspace_ycgco = 8,         ///< Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
    mlt_colorspace_bt2020_ncl = 2020, ///< ITU-R BT2020 non-constant luminance system
    mlt_colorspace_bt2020_cl = 2021,  ///< ITU-R BT2020 constant luminance system
    mlt_colorspace_smpte2085 = 11,    ///< SMPTE 2085, Y'D'zD'x
    mlt_colorspace_bt601 = 601,       ///< BT.470 (625/PAL) or SMPTE170M (525/NTSC)
    mlt_colorspace_invalid
} mlt_colorspace;

typedef enum {
    mlt_color_trc_none = 0,
    mlt_color_trc_bt709 = 1, ///< also ITU-R BT1361
    mlt_color_trc_unspecified = 2,
    mlt_color_trc_reserved = 3,
    mlt_color_trc_gamma22 = 4, ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
    mlt_color_trc_gamma28 = 5, ///< also ITU-R BT470BG
    mlt_color_trc_smpte170m
    = 6, ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
    mlt_color_trc_smpte240m = 7,
    mlt_color_trc_linear = 8,    ///< "Linear transfer characteristics"
    mlt_color_trc_log = 9,       ///< "Logarithmic transfer characteristic (100:1 range)"
    mlt_color_trc_log_sqrt = 10, ///< "Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
    mlt_color_trc_iec61966_2_4 = 11, ///< IEC 61966-2-4
    mlt_color_trc_bt1361_ecg = 12,   ///< ITU-R BT1361 Extended Colour Gamut
    mlt_color_trc_iec61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
    mlt_color_trc_bt2020_10 = 14,    ///< ITU-R BT2020 for 10-bit system
    mlt_color_trc_bt2020_12 = 15,    ///< ITU-R BT2020 for 12-bit system
    mlt_color_trc_smpte2084 = 16,    ///< SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems
    mlt_color_trc_smpte428 = 17,     ///< SMPTE ST 428-1
    mlt_color_trc_arib_std_b67 = 18, ///< ARIB STD-B67, known as "Hybrid log-gamma"
    mlt_color_trc_invalid
} mlt_color_trc;

typedef enum {
    mlt_color_pri_none = 0,
    mlt_color_pri_bt709 = 1,  ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
    mlt_color_pri_bt470m = 4, ///< also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
    mlt_color_pri_bt470bg
    = 5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
    mlt_color_pri_smpte170m = 6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
    mlt_color_pri_film = 8,      ///< colour filters using Illuminant C
    mlt_color_pri_bt2020 = 9,    ///< ITU-R BT2020
    mlt_color_pri_smpte428 = 10, ///< SMPTE ST 428-1 (CIE 1931 XYZ)
    mlt_color_pri_smpte431 = 11, ///< SMPTE ST 431-2 (2011) / DCI P3
    mlt_color_pri_smpte432 = 12, ///< SMPTE ST 432-1 (2010) / P3 D65 / Display P3
    mlt_color_pri_invalid
} mlt_color_primaries;

typedef enum {
    mlt_deinterlacer_none,
    mlt_deinterlacer_onefield,
    mlt_deinterlacer_linearblend,
    mlt_deinterlacer_weave,
    mlt_deinterlacer_bob,
    mlt_deinterlacer_greedy,
    mlt_deinterlacer_yadif_nospatial,
    mlt_deinterlacer_yadif,
    mlt_deinterlacer_bwdif,
    mlt_deinterlacer_estdif,
    mlt_deinterlacer_invalid,
} mlt_deinterlacer;

/** The time string formats */

typedef enum {
    mlt_time_frames = 0, /**< frame count */
    mlt_time_clock,      /**< SMIL clock-value as [[hh:]mm:]ss[.fraction] */
    mlt_time_smpte_df,   /**< SMPTE timecode as [[[hh:]mm:]ss{:|;}]frames */
    mlt_time_smpte_ndf   /**< SMPTE NDF timecode as [[[hh:]mm:]ss:]frames */
} mlt_time_format;

/** Interpolation methods for animation keyframes */

typedef enum {
    mlt_keyframe_discrete
    = 0,                 /**< non-interpolated; value changes instantaneously at the key frame */
    mlt_keyframe_linear, /**< simple, constant pace from this key frame to the next */
    mlt_keyframe_smooth, /**< deprecated use mlt_keyframe_smooth_loose */
    mlt_keyframe_smooth_loose
    = mlt_keyframe_smooth, /**< Unity Catmull-Rom spline interpolation. May have cusps or overshoots*/
    mlt_keyframe_smooth_natural, /**< Centripetal Catmull-Rom spline interpolation with natural slope at each keyframe. Will not have cusps or overshoots*/
    mlt_keyframe_smooth_tight, /**< Centripetal Catmull-Rom spline interpolation with 0 slope at each keyframe. Will not have cusps or overshoots*/
    mlt_keyframe_sinusoidal_in,
    mlt_keyframe_sinusoidal_out,
    mlt_keyframe_sinusoidal_in_out,
    mlt_keyframe_quadratic_in,
    mlt_keyframe_quadratic_out,
    mlt_keyframe_quadratic_in_out,
    mlt_keyframe_cubic_in,
    mlt_keyframe_cubic_out,
    mlt_keyframe_cubic_in_out,
    mlt_keyframe_quartic_in,
    mlt_keyframe_quartic_out,
    mlt_keyframe_quartic_in_out,
    mlt_keyframe_quintic_in,
    mlt_keyframe_quintic_out,
    mlt_keyframe_quintic_in_out,
    mlt_keyframe_exponential_in,
    mlt_keyframe_exponential_out,
    mlt_keyframe_exponential_in_out,
    mlt_keyframe_circular_in,
    mlt_keyframe_circular_out,
    mlt_keyframe_circular_in_out,
    mlt_keyframe_back_in,
    mlt_keyframe_back_out,
    mlt_keyframe_back_in_out,
    mlt_keyframe_elastic_in,
    mlt_keyframe_elastic_out,
    mlt_keyframe_elastic_in_out,
    mlt_keyframe_bounce_in,
    mlt_keyframe_bounce_out,
    mlt_keyframe_bounce_in_out,
} mlt_keyframe_type;

/** The relative time qualifiers */

typedef enum {
    mlt_whence_relative_start = 0, /**< relative to the beginning */
    mlt_whence_relative_current,   /**< relative to the current position */
    mlt_whence_relative_end        /**< relative to the end */
} mlt_whence;

/** The recognized subclasses of mlt_service */

typedef enum {
    mlt_service_invalid_type = 0, /**< invalid service */
    mlt_service_unknown_type,     /**< unknown class */
    mlt_service_producer_type,    /**< Producer class */
    mlt_service_tractor_type,     /**< Tractor class */
    mlt_service_playlist_type,    /**< Playlist class */
    mlt_service_multitrack_type,  /**< Multitrack class */
    mlt_service_filter_type,      /**< Filter class */
    mlt_service_transition_type,  /**< Transition class */
    mlt_service_consumer_type,    /**< Consumer class */
    mlt_service_field_type,       /**< Field class */
    mlt_service_link_type,        /**< Link class */
    mlt_service_chain_type        /**< Chain class */
} mlt_service_type;

/* I don't want to break anyone's applications without warning. -Zach */
#ifdef DOUBLE_MLT_POSITION
#define MLT_POSITION_FMT "%f"
#define MLT_POSITION_MOD(A, B) ((A) - (B) * ((int) ((A) / (B))))
typedef double mlt_position;
#else
#define MLT_POSITION_MOD(A, B) ((A) % (B))
#define MLT_POSITION_FMT "%d"
typedef int32_t mlt_position;
#endif

/** A rectangle type with coordinates, size, and opacity */

typedef struct
{
    double x; /**< X coordinate */
    double y; /**< Y coordinate */
    double w; /**< width */
    double h; /**< height */
    double o; /**< opacity / mix-level */
} mlt_rect;

/** A tuple of color components */

typedef struct
{
    uint8_t r; /**< red */
    uint8_t g; /**< green */
    uint8_t b; /**< blue */
    uint8_t a; /**< alpha */
} mlt_color;

typedef struct mlt_audio_s *mlt_audio;                  /**< pointer to Audio object */
typedef struct mlt_image_s *mlt_image;                  /**< pointer to Image object */
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
typedef struct mlt_slices_s *mlt_slices; /**< pointer to Sliced processing context object */
typedef struct mlt_link_s *mlt_link;     /**< pointer to Link object */
typedef struct mlt_chain_s *mlt_chain;   /**< pointer to Chain object */

typedef void (*mlt_destructor)(void *);              /**< pointer to destructor function */
typedef char *(*mlt_serialiser)(void *, int length); /**< pointer to serialization function */
typedef void *(*mlt_thread_function_t)(void *);      /**< generic thread function pointer */

#define MLT_SERVICE(x) ((mlt_service) (x))       /**< Cast to a Service pointer */
#define MLT_PRODUCER(x) ((mlt_producer) (x))     /**< Cast to a Producer pointer */
#define MLT_MULTITRACK(x) ((mlt_multitrack) (x)) /**< Cast to a Multitrack pointer */
#define MLT_PLAYLIST(x) ((mlt_playlist) (x))     /**< Cast to a Playlist pointer */
#define MLT_TRACTOR(x) ((mlt_tractor) (x))       /**< Cast to a Tractor pointer */
#define MLT_FILTER(x) ((mlt_filter) (x))         /**< Cast to a Filter pointer */
#define MLT_TRANSITION(x) ((mlt_transition) (x)) /**< Cast to a Transition pointer */
#define MLT_CONSUMER(x) ((mlt_consumer) (x))     /**< Cast to a Consumer pointer */
#define MLT_FRAME(x) ((mlt_frame) (x))           /**< Cast to a Frame pointer */
#define MLT_LINK(x) ((mlt_link) (x))             /**< Cast to a Link pointer */
#define MLT_CHAIN(x) ((mlt_chain) (x))           /**< Cast to a Chain pointer */

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
#if !defined(__MINGW32__)
MLT_EXPORT int usleep(unsigned int useconds);
#endif
#ifndef WIN_PTHREADS_TIME_H
MLT_EXPORT int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
#endif
MLT_EXPORT int setenv(const char *name, const char *value, int overwrite);
MLT_EXPORT char *getlocale();
MLT_EXPORT FILE *win32_fopen(const char *filename_utf8, const char *mode_utf8);
#include <sys/stat.h>
#include <sys/types.h>
MLT_EXPORT int win32_stat(const char *filename_utf8, struct stat *buffer);
#include <time.h>
MLT_EXPORT char *strptime(const char *buf, const char *fmt, struct tm *tm);
#define mlt_fopen win32_fopen
#define mlt_stat win32_stat
#define MLT_DIRLIST_DELIMITER ";"
#else
#define mlt_fopen fopen
#define mlt_stat stat
#define MLT_DIRLIST_DELIMITER ":"
#endif /* ifdef _WIN32 */

MLT_EXPORT const char *mlt_deinterlacer_name(mlt_deinterlacer method);
MLT_EXPORT mlt_deinterlacer mlt_deinterlacer_id(const char *name);

#ifdef __cplusplus
}
#endif

#endif
