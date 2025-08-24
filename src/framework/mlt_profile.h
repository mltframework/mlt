/**
 * \file mlt_profile.h
 * \brief video output definition
 * \see mlt_profile_s
 *
 * Copyright (C) 2007-2025 Meltytech, LLC
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

#ifndef MLT_PROFILE_H
#define MLT_PROFILE_H

#include "mlt_types.h"
#include "mlt_export.h"

/** \brief Profile class
 *
 * \envvar \em MLT_PROFILES_PATH overrides the default full path to the profile preset files, defaults to \p MLT_DATA/profiles
 * \envvar \em MLT_PROFILE the profile preset to use, defaults to "dv_pal"
 */

struct mlt_profile_s
{
    char *description;  /**< a brief description suitable as a label in UI menu */
    int frame_rate_num; /**< the numerator of the video frame rate */
    int frame_rate_den; /**< the denominator of the video frame rate */
    int width;          /**< the horizontal resolution of the video */
    int height;         /**< the vertical resolution of the video */
    int progressive; /**< a flag to indicate if the video is progressive scan, interlace if not set */
    int sample_aspect_num;  /**< the numerator of the pixel aspect ratio */
    int sample_aspect_den;  /**< the denominator of the pixel aspect ratio */
    int display_aspect_num; /**< the numerator of the image aspect ratio in case it can not be simply derived (e.g. ITU-R 601) */
    int display_aspect_den; /**< the denominator of the image aspect ratio in case it can not be simply derived (e.g. ITU-R 601) */
    int colorspace; /**< the Y'CbCr colorspace standard: `601` for ITU-R 601, `709` for ITU-R 709, `240` for SMPTE240M, `2020` for ITU-R BT.2020 non-constant luminance, `2021` for ITU-R BT.2020 constant luminance */
    int is_explicit; /**< used internally to indicate if the profile was requested explicitly or computed or defaulted */
};

MLT_API mlt_profile mlt_profile_init(const char *name);
MLT_API mlt_profile mlt_profile_load_file(const char *file);
MLT_API mlt_profile mlt_profile_load_properties(mlt_properties properties);
MLT_API mlt_profile mlt_profile_load_string(const char *string);
MLT_API double mlt_profile_fps(mlt_profile profile);
MLT_API double mlt_profile_sar(mlt_profile profile);
MLT_API double mlt_profile_dar(mlt_profile profile);
MLT_API void mlt_profile_close(mlt_profile profile);
MLT_API mlt_profile mlt_profile_clone(mlt_profile profile);
MLT_API mlt_properties mlt_profile_list();
MLT_API void mlt_profile_from_producer(mlt_profile profile, mlt_producer producer);
MLT_API char *mlt_profile_lumas_dir(mlt_profile profile);
MLT_API double mlt_profile_scale_width(mlt_profile profile, int width);
MLT_API double mlt_profile_scale_height(mlt_profile profile, int height);
#endif
