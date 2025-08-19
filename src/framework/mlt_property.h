/**
 * \file mlt_property.h
 * \brief Property class declaration
 * \see mlt_property_s
 *
 * Copyright (C) 2003-2023 Meltytech, LLC
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

#ifndef MLT_PROPERTY_H
#define MLT_PROPERTY_H

#include "mlt_types.h"
#include "mlt_api.h"

#if defined(__FreeBSD__)
/* This header has existed since 1994 and defines __FreeBSD_version below. */
#include <sys/param.h>
#endif

#if defined(__GLIBC__)
#include <locale.h>
typedef locale_t mlt_locale_t;
#elif defined(__APPLE__) || (defined(__FreeBSD_version) && __FreeBSD_version >= 900506)
#include <xlocale.h>
typedef locale_t mlt_locale_t;
#elif defined(__OpenBSD__)
/* XXX matches __nop_locale glue in libc++ */
typedef void *mlt_locale_t;
#elif (defined _WIN32 && defined _LIBCPP_VERSION)
struct mlt_locale_t;
#else
typedef char *mlt_locale_t;
#endif

MLT_API mlt_property mlt_property_init();
MLT_API void mlt_property_clear(mlt_property self);
MLT_API int mlt_property_is_clear(mlt_property self);
MLT_API int mlt_property_set_int(mlt_property self, int value);
MLT_API int mlt_property_set_double(mlt_property self, double value);
MLT_API int mlt_property_set_position(mlt_property self, mlt_position value);
MLT_API int mlt_property_set_int64(mlt_property self, int64_t value);
MLT_API int mlt_property_set_string(mlt_property self, const char *value);
MLT_API int mlt_property_set_data(mlt_property self,
                                 void *value,
                                 int length,
                                 mlt_destructor destructor,
                                 mlt_serialiser serialiser);
MLT_API int mlt_property_get_int(mlt_property self, double fps, mlt_locale_t);
MLT_API double mlt_property_get_double(mlt_property self, double fps, mlt_locale_t);
MLT_API mlt_position mlt_property_get_position(mlt_property self, double fps, mlt_locale_t);
MLT_API int64_t mlt_property_get_int64(mlt_property self);
MLT_API char *mlt_property_get_string_tf(mlt_property self, mlt_time_format);
MLT_API char *mlt_property_get_string(mlt_property self);
MLT_API char *mlt_property_get_string_l_tf(mlt_property self, mlt_locale_t, mlt_time_format);
MLT_API char *mlt_property_get_string_l(mlt_property self, mlt_locale_t);
MLT_API void *mlt_property_get_data(mlt_property self, int *length);
MLT_API void mlt_property_close(mlt_property self);
MLT_API void mlt_property_pass(mlt_property self, mlt_property that);
MLT_API char *mlt_property_get_time(mlt_property self, mlt_time_format, double fps, mlt_locale_t);

MLT_API int mlt_property_interpolate(mlt_property self,
                                    mlt_property points[],
                                    double progress,
                                    double fps,
                                    mlt_locale_t locale,
                                    mlt_keyframe_type interp);
MLT_API double mlt_property_anim_get_double(
    mlt_property self, double fps, mlt_locale_t locale, int position, int length);
MLT_API int mlt_property_anim_get_int(
    mlt_property self, double fps, mlt_locale_t locale, int position, int length);
MLT_API char *mlt_property_anim_get_string(
    mlt_property self, double fps, mlt_locale_t locale, int position, int length);
MLT_API int mlt_property_anim_set_double(mlt_property self,
                                        double value,
                                        double fps,
                                        mlt_locale_t locale,
                                        int position,
                                        int length,
                                        mlt_keyframe_type keyframe_type);
MLT_API int mlt_property_anim_set_int(mlt_property self,
                                     int value,
                                     double fps,
                                     mlt_locale_t locale,
                                     int position,
                                     int length,
                                     mlt_keyframe_type keyframe_type);
MLT_API int mlt_property_anim_set_string(
    mlt_property self, const char *value, double fps, mlt_locale_t locale, int position, int length);
MLT_API mlt_animation mlt_property_get_animation(mlt_property self);
MLT_API int mlt_property_is_anim(mlt_property self);

MLT_API int mlt_property_set_color(mlt_property self, mlt_color value);
MLT_API mlt_color mlt_property_get_color(mlt_property self, double fps, mlt_locale_t locale);
MLT_API int mlt_property_anim_set_color(mlt_property self,
                                       mlt_color value,
                                       double fps,
                                       mlt_locale_t locale,
                                       int position,
                                       int length,
                                       mlt_keyframe_type keyframe_type);
MLT_API mlt_color mlt_property_anim_get_color(
    mlt_property self, double fps, mlt_locale_t locale, int position, int length);

MLT_API int mlt_property_set_rect(mlt_property self, mlt_rect value);
MLT_API mlt_rect mlt_property_get_rect(mlt_property self, mlt_locale_t locale);
MLT_API int mlt_property_anim_set_rect(mlt_property self,
                                      mlt_rect value,
                                      double fps,
                                      mlt_locale_t locale,
                                      int position,
                                      int length,
                                      mlt_keyframe_type keyframe_type);
MLT_API mlt_rect mlt_property_anim_get_rect(
    mlt_property self, double fps, mlt_locale_t locale, int position, int length);

MLT_API int mlt_property_set_properties(mlt_property self, mlt_properties properties);
MLT_API mlt_properties mlt_property_get_properties(mlt_property self);
MLT_API int mlt_property_is_color(mlt_property self);
MLT_API int mlt_property_is_numeric(mlt_property self, mlt_locale_t locale);
MLT_API int mlt_property_is_rect(mlt_property self);

#endif
