/**
 * \file mlt_properties.h
 * \brief Properties class declaration
 * \see mlt_properties_s
 *
 * Copyright (C) 2003-2022 Meltytech, LLC
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

#ifndef MLT_PROPERTIES_H
#define MLT_PROPERTIES_H

#include "mlt_events.h"
#include "mlt_types.h"
#include <stdio.h>
#include "mlt_api.h"

/** \brief Properties class
 *
 * Properties is a combination list/dictionary of name/::mlt_property pairs.
 * It is also a base class for many of the other MLT classes.
 *
 * \event \em property-changed a property's value changed;
 *   the event data is a string for the name of the property
 */

struct mlt_properties_s
{
    void *child; /**< \private the object of a subclass */
    void *local; /**< \private instance object */

    /** the destructor virtual function */
    mlt_destructor close;
    void *close_object; /**< the object supplied to the close virtual function */
};

MLT_API extern int mlt_properties_init(mlt_properties, void *child);
MLT_API extern mlt_properties mlt_properties_new();
MLT_API extern int mlt_properties_set_lcnumeric(mlt_properties, const char *locale);
MLT_API extern const char *mlt_properties_get_lcnumeric(mlt_properties self);
MLT_API extern mlt_properties mlt_properties_load(const char *file);
MLT_API extern int mlt_properties_preset(mlt_properties self, const char *name);
MLT_API extern int mlt_properties_inc_ref(mlt_properties self);
MLT_API extern int mlt_properties_dec_ref(mlt_properties self);
MLT_API extern int mlt_properties_ref_count(mlt_properties self);
MLT_API extern void mlt_properties_mirror(mlt_properties self, mlt_properties that);
MLT_API extern int mlt_properties_inherit(mlt_properties self, mlt_properties that);
MLT_API extern int mlt_properties_copy(mlt_properties self, mlt_properties that, const char *prefix);
MLT_API extern int mlt_properties_pass(mlt_properties self, mlt_properties that, const char *prefix);
MLT_API extern void mlt_properties_pass_property(mlt_properties self, mlt_properties that, const char *name);
MLT_API extern int mlt_properties_pass_list(mlt_properties self, mlt_properties that, const char *list);
MLT_API extern int mlt_properties_set(mlt_properties self, const char *name, const char *value);
MLT_API extern int mlt_properties_set_or_default(mlt_properties self,
                                         const char *name,
                                         const char *value,
                                         const char *def);
MLT_API extern int mlt_properties_set_string(mlt_properties self, const char *name, const char *value);
MLT_API extern int mlt_properties_parse(mlt_properties self, const char *namevalue);
MLT_API extern char *mlt_properties_get(mlt_properties self, const char *name);
MLT_API extern char *mlt_properties_get_name(mlt_properties self, int index);
MLT_API extern char *mlt_properties_get_value_tf(mlt_properties self, int index, mlt_time_format);
MLT_API extern char *mlt_properties_get_value(mlt_properties self, int index);
MLT_API extern void *mlt_properties_get_data_at(mlt_properties self, int index, int *size);
MLT_API extern int mlt_properties_get_int(mlt_properties self, const char *name);
MLT_API extern int mlt_properties_set_int(mlt_properties self, const char *name, int value);
MLT_API extern int64_t mlt_properties_get_int64(mlt_properties self, const char *name);
MLT_API extern int mlt_properties_set_int64(mlt_properties self, const char *name, int64_t value);
MLT_API extern double mlt_properties_get_double(mlt_properties self, const char *name);
MLT_API extern int mlt_properties_set_double(mlt_properties self, const char *name, double value);
MLT_API extern mlt_position mlt_properties_get_position(mlt_properties self, const char *name);
MLT_API extern int mlt_properties_set_position(mlt_properties self, const char *name, mlt_position value);
MLT_API extern int mlt_properties_set_data(
    mlt_properties self, const char *name, void *value, int length, mlt_destructor, mlt_serialiser);
MLT_API extern void *mlt_properties_get_data(mlt_properties self, const char *name, int *length);
MLT_API extern int mlt_properties_rename(mlt_properties self, const char *source, const char *dest);
MLT_API extern int mlt_properties_count(mlt_properties self);
MLT_API extern void mlt_properties_dump(mlt_properties self, FILE *output);
MLT_API extern void mlt_properties_debug(mlt_properties self, const char *title, FILE *output);
MLT_API extern int mlt_properties_save(mlt_properties, const char *);
MLT_API extern int mlt_properties_dir_list(mlt_properties, const char *, const char *, int);
MLT_API extern void mlt_properties_close(mlt_properties self);
MLT_API extern int mlt_properties_is_sequence(mlt_properties self);
MLT_API extern mlt_properties mlt_properties_parse_yaml(const char *file);
MLT_API extern char *mlt_properties_serialise_yaml(mlt_properties self);
MLT_API extern void mlt_properties_lock(mlt_properties self);
MLT_API extern void mlt_properties_unlock(mlt_properties self);
MLT_API extern void mlt_properties_clear(mlt_properties self, const char *name);
MLT_API extern int mlt_properties_exists(mlt_properties self, const char *name);

MLT_API extern char *mlt_properties_get_time(mlt_properties, const char *name, mlt_time_format);
MLT_API extern char *mlt_properties_frames_to_time(mlt_properties, mlt_position, mlt_time_format);
MLT_API extern mlt_position mlt_properties_time_to_frames(mlt_properties, const char *time);

MLT_API extern int mlt_properties_set_color(mlt_properties, const char *name, mlt_color value);
MLT_API extern mlt_color mlt_properties_get_color(mlt_properties, const char *name);
MLT_API extern int mlt_properties_anim_set_color(mlt_properties self,
                                         const char *name,
                                         mlt_color value,
                                         int position,
                                         int length,
                                         mlt_keyframe_type keyframe_type);
MLT_API extern mlt_color mlt_properties_anim_get_color(mlt_properties self,
                                               const char *name,
                                               int position,
                                               int length);

MLT_API extern char *mlt_properties_anim_get(mlt_properties self,
                                     const char *name,
                                     int position,
                                     int length);
MLT_API extern int mlt_properties_anim_set(
    mlt_properties self, const char *name, const char *value, int position, int length);
MLT_API extern int mlt_properties_anim_get_int(mlt_properties self,
                                       const char *name,
                                       int position,
                                       int length);
MLT_API extern int mlt_properties_anim_set_int(mlt_properties self,
                                       const char *name,
                                       int value,
                                       int position,
                                       int length,
                                       mlt_keyframe_type keyframe_type);
MLT_API extern double mlt_properties_anim_get_double(mlt_properties self,
                                             const char *name,
                                             int position,
                                             int length);
MLT_API extern int mlt_properties_anim_set_double(mlt_properties self,
                                          const char *name,
                                          double value,
                                          int position,
                                          int length,
                                          mlt_keyframe_type keyframe_type);
MLT_API extern mlt_animation mlt_properties_get_animation(mlt_properties self, const char *name);
MLT_API extern int mlt_properties_is_anim(mlt_properties self, const char *name);

MLT_API extern int mlt_properties_set_rect(mlt_properties self, const char *name, mlt_rect value);
MLT_API extern mlt_rect mlt_properties_get_rect(mlt_properties self, const char *name);
MLT_API extern int mlt_properties_anim_set_rect(mlt_properties self,
                                        const char *name,
                                        mlt_rect value,
                                        int position,
                                        int length,
                                        mlt_keyframe_type keyframe_type);
MLT_API extern mlt_rect mlt_properties_anim_get_rect(mlt_properties self,
                                             const char *name,
                                             int position,
                                             int length);

MLT_API extern int mlt_properties_from_utf8(mlt_properties properties,
                                    const char *name_from,
                                    const char *name_to);
MLT_API extern int mlt_properties_to_utf8(mlt_properties properties,
                                  const char *name_from,
                                  const char *name_to);

MLT_API extern int mlt_properties_set_properties(mlt_properties self,
                                         const char *name,
                                         mlt_properties properties);
MLT_API extern mlt_properties mlt_properties_get_properties(mlt_properties self, const char *name);
MLT_API extern mlt_properties mlt_properties_get_properties_at(mlt_properties self, int index);

#endif
