/**
 * \file mlt_animation.c
 * \brief Property Animation class definition
 * \see mlt_animation_s
 *
 * Copyright (C) 2004-2023 Meltytech, LLC
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

#include "mlt_animation.h"
#include "mlt_factory.h"
#include "mlt_profile.h"
#include "mlt_properties.h"
#include "mlt_tokeniser.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** \brief animation list node pointer */
typedef struct animation_node_s *animation_node;
/** \brief private animation list node */
struct animation_node_s
{
    struct mlt_animation_item_s item;
    animation_node next, prev;
};

/** \brief Property Animation class
 *
 * This is the animation engine for a Property object. It is dependent upon
 * the mlt_property API and used by the various mlt_property_anim_* functions.
 */

struct mlt_animation_s
{
    char *data; /**< the string representing the animation */
    int length; /**< the maximum number of frames to use when interpreting negative keyframe positions */
    double fps;          /**< framerate to use when converting time clock strings to frame units */
    mlt_locale_t locale; /**< pointer to a locale to use when converting strings to numeric values */
    animation_node nodes; /**< a linked list of keyframes (and possibly non-keyframe values) */
};

/** \brief Keyframe type to string mapping
 *
 * Used for serialization and deserialization of keyframe types
 */

struct
{
    mlt_keyframe_type t;
    const char *s;
} keyframe_type_map[] = {
    // Map keyframe type to any single character except numeric values.
    {mlt_keyframe_discrete, "|"},
    {mlt_keyframe_discrete, "!"},
    {mlt_keyframe_linear, ""},
    {mlt_keyframe_smooth, "~"},
    {mlt_keyframe_smooth_loose, "~"},
    {mlt_keyframe_smooth_natural, "$"},
    {mlt_keyframe_smooth_tight, "-"},
};

static void mlt_animation_clear_string(mlt_animation self);
static int interpolate_item(mlt_animation_item item,
                            mlt_animation_item p[],
                            double fps,
                            mlt_locale_t locale);

static const char *keyframe_type_to_str(mlt_keyframe_type t)
{
    int map_count = sizeof(keyframe_type_map) / sizeof(*keyframe_type_map);
    for (int i = 0; i < map_count; i++) {
        if (keyframe_type_map[i].t == t) {
            return keyframe_type_map[i].s;
        }
    }
    return "";
}

static mlt_keyframe_type str_to_keyframe_type(const char *s)
{
    if (s && (s[0] < '0' || s[0] > '9')) {
        int map_count = sizeof(keyframe_type_map) / sizeof(*keyframe_type_map);
        for (int i = 0; i < map_count; i++) {
            if (!strncmp(s, keyframe_type_map[i].s, 1)) {
                return keyframe_type_map[i].t;
            }
        }
    }
    return mlt_keyframe_linear;
}

/** Create a new animation object.
 *
 * \public \memberof mlt_animation_s
 * \return an animation object
 */

mlt_animation mlt_animation_new()
{
    mlt_animation self = calloc(1, sizeof(*self));
    return self;
}

/** Re-interpolate non-keyframe nodes after a series of insertions or removals.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 */

void mlt_animation_interpolate(mlt_animation self)
{
    // Parse all items to ensure non-keyframes are calculated correctly.
    if (self && self->nodes) {
        animation_node current = self->nodes;
        while (current) {
            if (!current->item.is_key) {
                mlt_animation_item points[4];
                animation_node prev = current->prev;
                animation_node next = current->next;

                while (prev && !prev->item.is_key)
                    prev = prev->prev;
                while (next && !next->item.is_key)
                    next = next->next;

                if (!prev) {
                    current->item.is_key = 1;
                    prev = current;
                }
                if (!next) {
                    next = current;
                }
                points[0] = prev->prev ? &prev->prev->item : &prev->item;
                points[1] = &prev->item;
                points[2] = &next->item;
                points[3] = next->next ? &next->next->item : &next->item;
                interpolate_item(&current->item, points, self->fps, self->locale);
            }

            // Move to the next item
            current = current->next;
        }
    }
}

/** Remove a node from the linked list.
 *
 * \private \memberof mlt_animation_s
 * \param self an animation
 * \param node the node to remove
 * \return false
 */

static int mlt_animation_drop(mlt_animation self, animation_node node)
{
    if (node == self->nodes) {
        self->nodes = node->next;
        if (self->nodes) {
            self->nodes->prev = NULL;
            self->nodes->item.is_key = 1;
        }
    } else if (node->next && node->prev) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    } else if (node->next) {
        node->next->prev = node->prev;
    } else if (node->prev) {
        node->prev->next = node->next;
    }
    mlt_property_close(node->item.property);
    free(node);

    return 0;
}

/** Reset an animation and free all strings and properties.
 *
 * \private \memberof mlt_animation_s
 * \param self an animation
 */

static void mlt_animation_clean(mlt_animation self)
{
    if (!self)
        return;

    free(self->data);
    self->data = NULL;
    while (self->nodes)
        mlt_animation_drop(self, self->nodes);
}

/** Parse a string representing an animation.
 *
 * A semicolon is the delimiter between keyframe=value items in the string.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param data the string representing an animation
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \param fps the framerate to use when evaluating time strings
 * \param locale the locale to use when converting strings to numbers
 * \return true if there was an error
 */

int mlt_animation_parse(
    mlt_animation self, const char *data, int length, double fps, mlt_locale_t locale)
{
    if (!self)
        return 1;

    int error = 0;
    int i = 0;
    struct mlt_animation_item_s item;
    mlt_tokeniser tokens = mlt_tokeniser_init();

    // Clean the existing geometry
    mlt_animation_clean(self);

    // Update the info on the data
    if (data)
        self->data = strdup(data);
    self->length = length;
    self->fps = fps;
    self->locale = locale;
    item.property = mlt_property_init();
    item.frame = item.is_key = 0;
    item.keyframe_type = mlt_keyframe_discrete;

    // Tokenise
    if (data)
        mlt_tokeniser_parse_new(tokens, (char *) data, ";");

    // Iterate through each token
    for (i = 0; i < mlt_tokeniser_count(tokens); i++) {
        char *value = mlt_tokeniser_get_string(tokens, i);

        // If no data in keyframe, drop it (trailing semicolon)
        if (!value || !strcmp(value, ""))
            continue;

        // Reset item
        item.frame = item.is_key = 0;

        // Do not parse a string enclosed entirely within quotes as animation.
        // (mlt_tokeniser already skips splitting on delimiter inside quotes).
        if (value[0] == '\"' && value[strlen(value) - 1] == '\"') {
            // Remove the quotes.
            value[strlen(value) - 1] = '\0';
            mlt_property_set_string(item.property, &value[1]);
        } else {
            // Now parse the item
            mlt_animation_parse_item(self, &item, value);
        }

        // Now insert into place
        mlt_animation_insert(self, &item);
    }
    mlt_animation_interpolate(self);

    // Cleanup
    mlt_tokeniser_close(tokens);
    mlt_property_close(item.property);

    return error;
}

/** Conditionally refresh the animation if it is modified.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param data the string representing an animation
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \return true if there was an error
 */

int mlt_animation_refresh(mlt_animation self, const char *data, int length)
{
    if (!self)
        return 1;

    if ((length != self->length) || (data && (!self->data || strcmp(data, self->data))))
        return mlt_animation_parse(self, data, length, self->fps, self->locale);
    return 0;
}

/** Get the length of the animation.
 *
 * If the animation was initialized with a zero or negative value, then this
 * gets the maximum frame number from animation's list of nodes.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \return the number of frames
 */

int mlt_animation_get_length(mlt_animation self)
{
    int length = 0;
    if (self) {
        if (self->length > 0) {
            length = self->length;
        } else if (self->nodes) {
            animation_node node = self->nodes;
            while (node) {
                if (node->item.frame > length)
                    length = node->item.frame;
                node = node->next;
            }
        }
    }
    return length;
}

/** Set the length of the animation.
 *
 * The length is used for interpreting negative keyframe positions as relative
 * to the length. It is also used when serializing an animation as a string.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param length the length of the animation in frame units
 */

void mlt_animation_set_length(mlt_animation self, int length)
{
    if (self) {
        self->length = length;
        mlt_animation_clear_string(self);
    }
}

/** Parse a string representing an animation keyframe=value.
 *
 * This function does not affect the animation itself! But it will use some state
 * of the animation for the parsing (e.g. fps, locale).
 * It parses into a mlt_animation_item that you provide.
 * \p item->frame should be specified if the string does not have an equal sign and time field.
 * If an exclamation point (!) or vertical bar (|) character precedes the equal sign, then
 * the keyframe interpolation is set to discrete. If a tilde (~) precedes the equal sign,
 * then the keyframe interpolation is set to smooth (spline).
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an already allocated animation item
 * \param value the string representing an animation
 * \return true if there was an error
 */

int mlt_animation_parse_item(mlt_animation self, mlt_animation_item item, const char *value)
{
    int error = 0;

    if (self && item && value && strcmp(value, "")) {
        // Determine if a position has been specified
        if (strchr(value, '=')) {
            // Parse an absolute time value.
            // Null terminate the string at the equal sign to prevent interpreting
            // a colon in the part to the right of the equal sign as indicative of a
            // a time value string.
            char *s = strdup(value);
            char *p = strchr(s, '=');
            p[0] = '\0';
            mlt_property_set_string(item->property, s);

            item->frame = mlt_property_get_int(item->property, self->fps, self->locale);
            free(s);

            // The character preceding the equal sign indicates interpolation method.
            p = strchr(value, '=') - 1;
            item->keyframe_type = str_to_keyframe_type(p);
            value = &p[2];

            // Check if the value is quoted.
            p = &p[2];
            if (p && p[0] == '\"' && p[strlen(p) - 1] == '\"') {
                // Remove the quotes.
                p[strlen(p) - 1] = '\0';
                value = &p[1];
            }
        }

        // Special case - frame < 0
        if (item->frame < 0)
            item->frame += mlt_animation_get_length(self);

        // Set remainder of string as item value.
        mlt_property_set_string(item->property, value);
        item->is_key = 1;
    } else {
        error = 1;
    }

    return error;
}

/** Load an animation item for an absolute position.
 *
 * This performs interpolation if there is no keyframe at the \p position.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an already allocated animation item that will be filled in
 * \param position the frame number for the point in time
 * \return true if there was an error
 */

int mlt_animation_get_item(mlt_animation self, mlt_animation_item item, int position)
{
    if (!self || !item)
        return 1;

    int error = 0;
    // Need to find the nearest keyframe to the position specified
    animation_node node = self->nodes;

    // Iterate through the keyframes until we reach last or have
    while (node && node->next && position >= node->next->item.frame)
        node = node->next;

    if (node) {
        item->keyframe_type = node->item.keyframe_type;

        // Position is before the first keyframe.
        if (position < node->item.frame) {
            item->is_key = 0;
            if (item->property)
                mlt_property_pass(item->property, node->item.property);
        }
        // Item exists.
        else if (position == node->item.frame) {
            item->is_key = node->item.is_key;
            if (item->property)
                mlt_property_pass(item->property, node->item.property);
        }
        // Position is after the last keyframe.
        else if (!node->next) {
            item->is_key = 0;
            if (item->property)
                mlt_property_pass(item->property, node->item.property);
        }
        // Interpolation needed.
        else {
            if (item->property) {
                mlt_animation_item points[4];
                points[0] = node->prev ? &node->prev->item : &node->item;
                points[1] = &node->item;
                points[2] = &node->next->item;
                points[3] = node->next->next ? &node->next->next->item : &node->next->item;
                item->frame = position;
                interpolate_item(item, points, self->fps, self->locale);
            }
            item->is_key = 0;
        }
    } else {
        item->frame = item->is_key = 0;
        error = 1;
    }
    item->frame = position;

    return error;
}

/** Insert an animation item.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an animation item
 * \return true if there was an error
 * \see mlt_animation_parse_item
 */

int mlt_animation_insert(mlt_animation self, mlt_animation_item item)
{
    if (!self || !item)
        return 1;

    int error = 0;
    animation_node node = calloc(1, sizeof(*node));
    node->item.frame = item->frame;
    node->item.is_key = 1;
    node->item.keyframe_type = item->keyframe_type;
    node->item.property = mlt_property_init();
    if (item->property)
        mlt_property_pass(node->item.property, item->property);

    // Determine if we need to insert or append to the list, or if it's a new list
    if (self->nodes) {
        // Get the first item
        animation_node current = self->nodes;

        // Locate an existing nearby item
        while (current->next && item->frame > current->item.frame)
            current = current->next;

        if (item->frame < current->item.frame) {
            if (current == self->nodes)
                self->nodes = node;
            if (current->prev)
                current->prev->next = node;
            node->next = current;
            node->prev = current->prev;
            current->prev = node;
        } else if (item->frame > current->item.frame) {
            if (current->next)
                current->next->prev = node;
            node->next = current->next;
            node->prev = current;
            current->next = node;
        } else {
            // Update matching node.
            current->item.frame = item->frame;
            current->item.is_key = 1;
            current->item.keyframe_type = item->keyframe_type;
            mlt_property_close(current->item.property);
            current->item.property = node->item.property;
            free(node);
        }
    } else {
        // Set the first item
        self->nodes = node;
    }
    mlt_animation_clear_string(self);

    return error;
}

/** Remove the keyframe at the specified position.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param position the frame number of the animation node to remove
 * \return true if there was an error
 */

int mlt_animation_remove(mlt_animation self, int position)
{
    if (!self)
        return 1;

    int error = 1;
    animation_node node = self->nodes;

    while (node && position != node->item.frame)
        node = node->next;

    if (node && position == node->item.frame)
        error = mlt_animation_drop(self, node);

    mlt_animation_clear_string(self);

    return error;
}

/** Get the keyfame at the position or the next following.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an already allocated animation item which will be updated
 * \param position the frame number at which to start looking for the next animation node
 * \return true if there was an error
 */

int mlt_animation_next_key(mlt_animation self, mlt_animation_item item, int position)
{
    if (!self || !item)
        return 1;

    animation_node node = self->nodes;

    while (node && position > node->item.frame)
        node = node->next;

    if (node) {
        item->frame = node->item.frame;
        item->is_key = node->item.is_key;
        item->keyframe_type = node->item.keyframe_type;
        if (item->property)
            mlt_property_pass(item->property, node->item.property);
    }

    return (node == NULL);
}

/** Get the keyfame at the position or the next preceding.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an already allocated animation item which will be updated
 * \param position the frame number at which to start looking for the previous animation node
 * \return true if there was an error
 */

int mlt_animation_prev_key(mlt_animation self, mlt_animation_item item, int position)
{
    if (!self || !item)
        return 1;

    animation_node node = self->nodes;

    while (node && node->next && position >= node->next->item.frame)
        node = node->next;

    if (position < node->item.frame)
        node = NULL;

    if (node) {
        item->frame = node->item.frame;
        item->is_key = node->item.is_key;
        item->keyframe_type = node->item.keyframe_type;
        if (item->property)
            mlt_property_pass(item->property, node->item.property);
    }

    return (node == NULL);
}

/** Serialize a cut of the animation (with time format).
 *
 * The caller is responsible for free-ing the returned string.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param in the frame at which to start serializing animation nodes
 * \param out the frame at which to stop serializing nodes
 * \param time_format the time format to use for the key frames
 * \return a string representing the animation
 */

char *mlt_animation_serialize_cut_tf(mlt_animation self,
                                     int in,
                                     int out,
                                     mlt_time_format time_format)
{
    struct mlt_animation_item_s item;
    char *ret = calloc(1, 1000);
    size_t used = 0;
    size_t size = 1000;
    mlt_property time_property = mlt_property_init();

    item.property = mlt_property_init();
    item.frame = item.is_key = 0;
    item.keyframe_type = mlt_keyframe_discrete;
    if (in == -1)
        in = 0;
    if (out == -1)
        out = mlt_animation_get_length(self);

    if (self && ret) {
        item.frame = in;

        while (1) {
            size_t item_len = 0;

            // If it's the first frame, then it's not necessarily a key
            if (item.frame == in) {
                if (mlt_animation_get_item(self, &item, item.frame))
                    break;

                // If the first keyframe is larger than the current position
                // then do nothing here
                if (self->nodes->item.frame > item.frame) {
                    item.frame++;
                    continue;
                }

                // To ensure correct seeding
                item.is_key = 1;
            }
            // Typically, we move from keyframe to keyframe
            else if (item.frame <= out) {
                if (mlt_animation_next_key(self, &item, item.frame))
                    break;

                // Special case - crop at the out point
                if (item.frame > out) {
                    mlt_animation_get_item(self, &item, out);
                    // To ensure correct seeding
                    item.is_key = 1;
                }
            }
            // We've handled the last keyframe
            else {
                break;
            }

            // Determine length of string to be appended.
            item_len += 100;
            const char *value = mlt_property_get_string_l(item.property, self->locale);
            if (item.is_key && value) {
                item_len += strlen(value);
                // Check if the value must be quoted.
                if (strchr(value, ';') || strchr(value, '='))
                    item_len += 2;
            }

            // Reallocate return string to be long enough.
            while (used + item_len + 2 > size) // +2 for ';' and NULL
            {
                size += 1000;
                ret = realloc(ret, size);
            }

            // Append item delimiter (;) if needed.
            if (ret && used > 0) {
                used++;
                strcat(ret, ";");
            }
            if (ret) {
                // Append keyframe time and keyframe/value delimiter (=).
                const char *s = keyframe_type_to_str(item.keyframe_type);
                if (time_property && self->fps > 0.0) {
                    mlt_property_set_int(time_property, item.frame - in);
                    const char *time = mlt_property_get_time(time_property,
                                                             time_format,
                                                             self->fps,
                                                             self->locale);
                    sprintf(ret + used, "%s%s=", time, s);
                } else {
                    sprintf(ret + used, "%d%s=", item.frame - in, s);
                }
                used = strlen(ret);

                // Append item value.
                if (item.is_key && value) {
                    // Check if the value must be quoted.
                    if (strchr(value, ';') || strchr(value, '='))
                        sprintf(ret + used, "\"%s\"", value);
                    else
                        strcat(ret, value);
                }
                used = strlen(ret);
            }
            item.frame++;
        }
    }
    mlt_property_close(item.property);
    mlt_property_close(time_property);

    return ret;
}

static mlt_time_format default_time_format()
{
    const char *e = getenv("MLT_ANIMATION_TIME_FORMAT");
    return e ? strtol(e, NULL, 10) : mlt_time_frames;
}

/** Serialize a cut of the animation.
 *
 * This version outputs the key frames' position as a frame number.
 * The caller is responsible for free-ing the returned string.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param in the frame at which to start serializing animation nodes
 * \param out the frame at which to stop serializing nodes
 * \return a string representing the animation
 */

char *mlt_animation_serialize_cut(mlt_animation self, int in, int out)
{
    return mlt_animation_serialize_cut_tf(self, in, out, default_time_format());
}

/** Serialize the animation (with time format).
 *
 * The caller is responsible for free-ing the returned string.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param time_format the time format to use for the key frames
 * \return a string representing the animation
 */

char *mlt_animation_serialize_tf(mlt_animation self, mlt_time_format time_format)
{
    char *ret = mlt_animation_serialize_cut_tf(self, -1, -1, time_format);
    if (self && ret) {
        free(self->data);
        self->data = ret;
        ret = strdup(ret);
    }
    return ret;
}

/** Serialize the animation.
 *
 * This version outputs the key frames' position as a frame number.
 * The caller is responsible for free-ing the returned string.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \return a string representing the animation
 */

char *mlt_animation_serialize(mlt_animation self)
{
    return mlt_animation_serialize_tf(self, default_time_format());
}

/** Get the number of keyframes.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \return the number of keyframes or -1 on error
 */

int mlt_animation_key_count(mlt_animation self)
{
    int count = -1;
    if (self) {
        animation_node node = self->nodes;
        for (count = 0; node; ++count)
            node = node->next;
    }
    return count;
}

/** Get an animation item for the N-th keyframe.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an already allocated animation item that will be filled in
 * \param index the N-th keyframe (0 based) in this animation
 * \return true if there was an error
 */

int mlt_animation_key_get(mlt_animation self, mlt_animation_item item, int index)
{
    if (!self || !item)
        return 1;

    int error = 0;
    animation_node node = self->nodes;

    // Iterate through the keyframes.
    int i = index;
    while (i-- && node)
        node = node->next;

    if (node) {
        item->is_key = node->item.is_key;
        item->frame = node->item.frame;
        item->keyframe_type = node->item.keyframe_type;
        if (item->property)
            mlt_property_pass(item->property, node->item.property);
    } else {
        item->frame = item->is_key = 0;
        error = 1;
    }

    return error;
}

/** Close the animation and deallocate all of its resources.
 *
 * \public \memberof mlt_animation_s
 * \param self the animation to destroy
 */

void mlt_animation_close(mlt_animation self)
{
    if (self) {
        mlt_animation_clean(self);
        free(self);
    }
}

/** Change the interpolation for the N-th keyframe.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param index the N-th keyframe (0 based) in this animation
 * \param type the method of interpolation for this key frame
 * \return true if there was an error
 */

int mlt_animation_key_set_type(mlt_animation self, int index, mlt_keyframe_type type)
{
    if (!self)
        return 1;

    int error = 0;
    animation_node node = self->nodes;

    // Iterate through the keyframes.
    int i = index;
    while (i-- && node)
        node = node->next;

    if (node) {
        node->item.keyframe_type = type;
        mlt_animation_interpolate(self);
        mlt_animation_clear_string(self);
    } else {
        error = 1;
    }

    return error;
}

/** Change the frame number for the N-th keyframe.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param index the N-th keyframe (0 based) in this animation
 * \param frame the position of this keyframe in frame units
 * \return true if there was an error
 */

int mlt_animation_key_set_frame(mlt_animation self, int index, int frame)
{
    if (!self)
        return 1;

    int error = 0;
    animation_node node = self->nodes;

    // Iterate through the keyframes.
    int i = index;
    while (i-- && node)
        node = node->next;

    if (node) {
        node->item.frame = frame;
        mlt_animation_interpolate(self);
        mlt_animation_clear_string(self);
    } else {
        error = 1;
    }

    return error;
}

/** Shift the frame value for all nodes.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param shift the value to add to all frame values
 */

void mlt_animation_shift_frames(mlt_animation self, int shift)
{
    animation_node node = self->nodes;
    while (node) {
        node->item.frame += shift;
        node = node->next;
    }
    mlt_animation_clear_string(self);
    mlt_animation_interpolate(self);
}

/** Get the cached serialization string.
 *
 * This can be used to determine if the animation has been modified because the
 * string is cleared whenever the animation is changed.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \return the cached serialization string
 */

const char *mlt_animation_get_string(mlt_animation self)
{
    if (!self)
        return NULL;
    return self->data;
}

/** Clear the cached serialization string.
 *
 * \private \memberof mlt_animation_s
 * \param self an animation
 */

void mlt_animation_clear_string(mlt_animation self)
{
    if (!self)
        return;
    free(self->data);
    self->data = NULL;
}

/** A linear interpolation function.
 *
 * \private \memberof mlt_animation_s
 */

static inline double linear_interpolate(double y1, double y2, double t)
{
    return y1 + (y2 - y1) * t;
}

/** Calculate the distance between two points.
 *
 * \private \memberof mlt_animation_s
 */

static inline double distance(double x0, double y0, double x1, double y1)
{
    return sqrt(pow(x1 - x0, 2) + pow(y1 - y0, 2));
}

/** A Catmull–Rom interpolation function.
 *
 * As described here:
 *   https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline
 * And further reduced here with tension added:
 *   https://qroph.github.io/2018/07/30/smooth-paths-using-catmull-rom-splines.html
 *
 * This imlementation supports the alpha value which can be set to 0.5 to result in
 * centripetal Catmull–Rom splines. Centripital Catmull–Rom splines are guaranteed
 * to not have any cusps or loops. These are not desirable because they result in the
 * value reversing direction when interpolation from one point to the next.
 *
 * To use this function for animation item interpolation, provide 4 points: two points preceeding t
 * and two points following t. Use the item frame number as the x and the item value as y for each
 * point. t should represent the fractional progress between point 1 and point 2.
 *
 * If fewer than 2 points are available, then duplicate the first and/or last points as necessary to
 * meet the requirement for 4 points.
 
 * \private \memberof mlt_animation_s
 * \param alpha
 *      0.0 for the uniform spline
 *      0.5 for the centripetal spline (no cusps)
 *      1.0 for the chordal spline.
 * \param tension
 *      1.0 results in the most natural slope at x1,y1 and x2,y2
 *      0.0 results in a horizontal tangent at x1,y1 and x2,y2 (slope of 0).
 *      -1.0 results in the most natural slope at x1,y1 and x2,y2 unless x1 or x2 represents a peak.
 *      In the case of peaks, a horizontal tangent will be used to avoid overshoot.

 */

static inline double catmull_rom_interpolate(double x0,
                                             double y0,
                                             double x1,
                                             double y1,
                                             double x2,
                                             double y2,
                                             double x3,
                                             double y3,
                                             double t,
                                             double alpha,
                                             double tension)
{
    double m1 = 0;
    double m2 = 0;
    double t12 = pow(distance(x1, y1, x2, y2), alpha);
    if (tension > 0.0 || (y1 < y0 && y1 > y2) || (y1 > y0 && y1 < y2)) {
        double t01 = pow(distance(x0, y0, x1, y1), alpha);
        m1 = abs(tension) * (y2 - y1 + t12 * ((y1 - y0) / t01 - (y2 - y0) / (t01 + t12)));
    }
    if (tension > 0.0 || (y2 < y1 && y2 > y3) || (y2 > y1 && y2 < y3)) {
        double t23 = pow(distance(x2, y2, x3, y3), alpha);
        m2 = abs(tension) * (y2 - y1 + t12 * ((y3 - y2) / t23 - (y3 - y1) / (t12 + t23)));
    }
    double a = 2.0 * (y1 - y2) + m1 + m2;
    double b = -3.0 * (y1 - y2) - m1 - m1 - m2;
    double c = m1;
    double d = y1;
    return a * t * t * t + b * t * t + c * t + d;
}

static inline double interpolate_value(double x0,
                                       double y0,
                                       double x1,
                                       double y1,
                                       double x2,
                                       double y2,
                                       double x3,
                                       double y3,
                                       double t,
                                       mlt_keyframe_type type)
{
    // Correct first and last values.
    // If points are duplicated (e.g. for the first and last segments) assume the duplicated point
    // is far away to create a horizontal segment.
    if (x0 == x1) {
        x0 -= 10000;
    }
    if (x3 == x2) {
        x3 += 10000;
    }
    switch (type) {
    case mlt_keyframe_discrete:
        return y1;
    case mlt_keyframe_linear:
        return linear_interpolate(y1, y2, t);
    case mlt_keyframe_smooth_loose:
        return catmull_rom_interpolate(x0, y0, x1, y1, x2, y2, x3, y3, t, 0.0, 1.0);
    case mlt_keyframe_smooth_natural:
        return catmull_rom_interpolate(x0, y0, x1, y1, x2, y2, x3, y3, t, 0.5, -1.0);
    case mlt_keyframe_smooth_tight:
        return catmull_rom_interpolate(x0, y0, x1, y1, x2, y2, x3, y3, t, 0.5, 0.0);
    }
    return y1;
}

/** Interpolate a new animation item given a set of other items.
 *
 * \private \memberof mlt_animation_s
 *
 * \param item an unpopulated animation item to be interpolated.
 *  The frame and keyframe_type fields must already be set. The value for "frame" is the postion
 *  at which the value will be interpolated. The value for "keyframe_type" determines which
 *  interpolation will be used.
 * \param p a sequential array of 4 animation items. The frame value for item must lie between the
    frame values for p[1] and p[2].
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
  * \return true if there was an error
 */

static int interpolate_item(mlt_animation_item item,
                            mlt_animation_item p[],
                            double fps,
                            mlt_locale_t locale)
{
    int error = 0;
    double progress = (double) (item->frame - p[1]->frame) / (double) (p[2]->frame - p[1]->frame);
    if (item->keyframe_type == mlt_keyframe_discrete) {
        mlt_property_pass(item->property, p[1]->property);
    } else if (mlt_property_is_color(p[1]->property)) {
        mlt_color value = {0xff, 0xff, 0xff, 0xff};
        mlt_color colors[4];
        mlt_color zero = {0xff, 0xff, 0xff, 0xff};
        colors[1] = p[1] ? mlt_property_get_color(p[1]->property, fps, locale) : zero;
        if (p[2]) {
            colors[0] = p[0] ? mlt_property_get_color(p[0]->property, fps, locale) : zero;
            colors[2] = p[2] ? mlt_property_get_color(p[2]->property, fps, locale) : zero;
            colors[3] = p[3] ? mlt_property_get_color(p[3]->property, fps, locale) : zero;
            value.r = CLAMP(interpolate_value(p[0]->frame,
                                              colors[0].r,
                                              p[1]->frame,
                                              colors[1].r,
                                              p[2]->frame,
                                              colors[2].r,
                                              p[3]->frame,
                                              colors[3].r,
                                              progress,
                                              item->keyframe_type),
                            0,
                            255);
            value.g = CLAMP(interpolate_value(p[0]->frame,
                                              colors[0].g,
                                              p[1]->frame,
                                              colors[1].g,
                                              p[2]->frame,
                                              colors[2].g,
                                              p[3]->frame,
                                              colors[3].g,
                                              progress,
                                              item->keyframe_type),
                            0,
                            255);
            value.b = CLAMP(interpolate_value(p[0]->frame,
                                              colors[0].b,
                                              p[1]->frame,
                                              colors[1].b,
                                              p[2]->frame,
                                              colors[2].b,
                                              p[3]->frame,
                                              colors[3].b,
                                              progress,
                                              item->keyframe_type),
                            0,
                            255);
            value.a = CLAMP(interpolate_value(p[0]->frame,
                                              colors[0].a,
                                              p[1]->frame,
                                              colors[1].a,
                                              p[2]->frame,
                                              colors[2].a,
                                              p[3]->frame,
                                              colors[3].a,
                                              progress,
                                              item->keyframe_type),
                            0,
                            255);
        } else {
            value = colors[1];
        }
        error = mlt_property_set_color(item->property, value);
    } else if (mlt_property_is_rect(item->property)) {
        mlt_rect value = {DBL_MIN, DBL_MIN, DBL_MIN, DBL_MIN, DBL_MIN};
        mlt_rect points[4];
        mlt_rect zero = {0, 0, 0, 0, 0};
        points[1] = p[1] ? mlt_property_get_rect(p[1]->property, locale) : zero;
        if (p[2]) {
            points[0] = p[0] ? mlt_property_get_rect(p[0]->property, locale) : zero;
            points[2] = p[2] ? mlt_property_get_rect(p[2]->property, locale) : zero;
            points[3] = p[3] ? mlt_property_get_rect(p[3]->property, locale) : zero;
            value.x = interpolate_value(p[0]->frame,
                                        points[0].x,
                                        p[1]->frame,
                                        points[1].x,
                                        p[2]->frame,
                                        points[2].x,
                                        p[3]->frame,
                                        points[3].x,
                                        progress,
                                        item->keyframe_type);
            value.y = interpolate_value(p[0]->frame,
                                        points[0].y,
                                        p[1]->frame,
                                        points[1].y,
                                        p[2]->frame,
                                        points[2].y,
                                        p[3]->frame,
                                        points[3].y,
                                        progress,
                                        item->keyframe_type);
            value.w = interpolate_value(p[0]->frame,
                                        points[0].w,
                                        p[1]->frame,
                                        points[1].w,
                                        p[2]->frame,
                                        points[2].w,
                                        p[3]->frame,
                                        points[3].w,
                                        progress,
                                        item->keyframe_type);
            value.h = interpolate_value(p[0]->frame,
                                        points[0].h,
                                        p[1]->frame,
                                        points[1].h,
                                        p[2]->frame,
                                        points[2].h,
                                        p[3]->frame,
                                        points[3].h,
                                        progress,
                                        item->keyframe_type);
            value.o = interpolate_value(p[0]->frame,
                                        points[0].o,
                                        p[1]->frame,
                                        points[1].o,
                                        p[2]->frame,
                                        points[2].o,
                                        p[3]->frame,
                                        points[3].o,
                                        progress,
                                        item->keyframe_type);
        } else {
            value = points[1];
        }
        error = mlt_property_set_rect(item->property, value);
    } else if (mlt_property_is_numeric(p[1]->property, locale)) {
        double value = 0.0;
        double points[4];
        points[0] = p[0] ? mlt_property_get_double(p[0]->property, fps, locale) : 0;
        points[1] = p[1] ? mlt_property_get_double(p[1]->property, fps, locale) : 0;
        points[2] = p[2] ? mlt_property_get_double(p[2]->property, fps, locale) : 0;
        points[3] = p[3] ? mlt_property_get_double(p[3]->property, fps, locale) : 0;
        value = p[2] ? interpolate_value(p[0]->frame,
                                         points[0],
                                         p[1]->frame,
                                         points[1],
                                         p[2]->frame,
                                         points[2],
                                         p[3]->frame,
                                         points[3],
                                         progress,
                                         item->keyframe_type)
                     : points[1];
        error = mlt_property_set_double(item->property, value);
    } else {
        mlt_property_pass(item->property, p[1]->property);
    }
    return error;
}