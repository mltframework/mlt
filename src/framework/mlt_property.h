/**
 * \file mlt_property.h
 * \brief Property class declaration
 * \see mlt_property_s
 *
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#if defined(__FreeBSD__)
/* This header has existed since 1994 and defines __FreeBSD_version below. */
#include <sys/param.h>
#endif

#if defined(__GLIBC__) || defined(__APPLE__) || (__FreeBSD_version >= 900506)
#include <xlocale.h>
#else
typedef char* locale_t;
#endif

extern mlt_property mlt_property_init( );
extern int mlt_property_set_int( mlt_property self, int value );
extern int mlt_property_set_double( mlt_property self, double value );
extern int mlt_property_set_position( mlt_property self, mlt_position value );
extern int mlt_property_set_int64( mlt_property self, int64_t value );
extern int mlt_property_set_string( mlt_property self, const char *value );
extern int mlt_property_set_data( mlt_property self, void *value, int length, mlt_destructor destructor, mlt_serialiser serialiser );
extern int mlt_property_get_int( mlt_property self, double fps, locale_t );
extern double mlt_property_get_double( mlt_property self, double fps, locale_t );
extern mlt_position mlt_property_get_position( mlt_property self, double fps, locale_t );
extern int64_t mlt_property_get_int64( mlt_property self );
extern char *mlt_property_get_string( mlt_property self );
extern char *mlt_property_get_string_l( mlt_property self, locale_t );
extern void *mlt_property_get_data( mlt_property self, int *length );
extern void mlt_property_close( mlt_property self );
extern void mlt_property_pass( mlt_property self, mlt_property that );
extern char *mlt_property_get_time( mlt_property self, mlt_time_format, double fps, locale_t );

extern int mlt_property_interpolate( mlt_property self, mlt_property points[], double progress, double fps, locale_t locale, mlt_keyframe_type interp );
extern double mlt_property_anim_get_double( mlt_property self, double fps, locale_t locale, int position, int length );
extern int mlt_property_anim_get_int( mlt_property self, double fps, locale_t locale, int position, int length );
extern char* mlt_property_anim_get_string( mlt_property self, double fps, locale_t locale, int position, int length );
extern int mlt_property_anim_set_double( mlt_property self, double value, double fps, locale_t locale, int position, int length, mlt_keyframe_type keyframe_type );
extern int mlt_property_anim_set_int( mlt_property self, int value, double fps, locale_t locale, int position, int length, mlt_keyframe_type keyframe_type );
extern int mlt_property_anim_set_string( mlt_property self, const char *value, double fps, locale_t locale, int position, int length );
extern mlt_animation mlt_property_get_animation( mlt_property self );

extern int mlt_property_set_rect( mlt_property self, mlt_rect value );
extern mlt_rect mlt_property_get_rect( mlt_property self, locale_t locale );
extern int mlt_property_anim_set_rect( mlt_property self, mlt_rect value, double fps, locale_t locale, int position, int length, mlt_keyframe_type keyframe_type );
extern mlt_rect mlt_property_anim_get_rect( mlt_property self, double fps, locale_t locale, int position, int length );

#endif
