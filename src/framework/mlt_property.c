/**
 * \file mlt_property.c
 * \brief Property class definition
 * \see mlt_property_s
 *
 * Copyright (C) 2003-2024 Meltytech, LLC
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

// For strtod_l
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "mlt_property.h"
#include "mlt_animation.h"
#include "mlt_properties.h"

#include <float.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Bit pattern used internally to indicated representations available.
*/

typedef enum {
    mlt_prop_none = 0,     //!< not set
    mlt_prop_int = 1,      //!< set as an integer
    mlt_prop_string = 2,   //!< set as string or already converted to string
    mlt_prop_position = 4, //!< set as a position
    mlt_prop_double = 8,   //!< set as a floating point
    mlt_prop_data = 16,    //!< set as opaque binary
    mlt_prop_int64 = 32,   //!< set as a 64-bit integer
    mlt_prop_rect = 64,    //!< set as a mlt_rect
    mlt_prop_color = 128   //!< set as a mlt_color
} mlt_property_type;

/** \brief Property class
 *
 * A property is like a variant or dynamic type. They are used for many things
 * in MLT, but in particular they are the parameter mechanism for the plugins.
 */

struct mlt_property_s
{
    /// Stores a bit pattern of types available for this property
    mlt_property_type types;

    /// Atomic type handling
    int prop_int;
    mlt_position prop_position;
    double prop_double;
    int64_t prop_int64;

    /// String handling
    char *prop_string;

    /// Generic type handling
    void *data;
    int length;
    mlt_destructor destructor;
    mlt_serialiser serialiser;

    pthread_mutex_t mutex;
    mlt_animation animation;
    mlt_properties properties;
};

/** Construct a property and initialize it
 * \public \memberof mlt_property_s
 */

mlt_property mlt_property_init()
{
    mlt_property self = calloc(1, sizeof(*self));
    if (self) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&self->mutex, &attr);
    }
    return self;
}

/** Clear (0/null) a property.
 *
 * Frees up any associated resources in the process.
 * \private \memberof mlt_property_s
 * \param self a property
 */

static void clear_property(mlt_property self)
{
    // Special case data handling
    if (self->types & mlt_prop_data && self->destructor != NULL)
        self->destructor(self->data);

    // Special case string handling
    if (self->prop_string)
        free(self->prop_string);

    mlt_animation_close(self->animation);

    mlt_properties_close(self->properties);

    // Wipe stuff
    self->types = 0;
    self->prop_int = 0;
    self->prop_position = 0;
    self->prop_double = 0;
    self->prop_int64 = 0;
    self->prop_string = NULL;
    self->data = NULL;
    self->length = 0;
    self->destructor = NULL;
    self->serialiser = NULL;
    self->animation = NULL;
    self->properties = NULL;
}

/** Clear (0/null) a property.
 *
 * Frees up any associated resources in the process.
 * \public \memberof mlt_property_s
 * \param self a property
 */

void mlt_property_clear(mlt_property self)
{
    pthread_mutex_lock(&self->mutex);
    clear_property(self);
    pthread_mutex_unlock(&self->mutex);
}

/** Check if a property is cleared.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \return true if a property is clear. false if it has been set.
 */

int mlt_property_is_clear(mlt_property self)
{
    int result = 1;
    if (self) {
        pthread_mutex_lock(&self->mutex);
        result = self->types == 0 && self->animation == NULL && self->properties == NULL;
        pthread_mutex_unlock(&self->mutex);
    }
    return result;
}

/** Set the property to an integer value.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value an integer
 * \return false
 */

int mlt_property_set_int(mlt_property self, int value)
{
    pthread_mutex_lock(&self->mutex);
    clear_property(self);
    self->types = mlt_prop_int;
    self->prop_int = value;
    pthread_mutex_unlock(&self->mutex);
    return 0;
}

/** Set the property to a floating point value.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value a double precision floating point value
 * \return false
 */

int mlt_property_set_double(mlt_property self, double value)
{
    pthread_mutex_lock(&self->mutex);
    clear_property(self);
    self->types = mlt_prop_double;
    self->prop_double = value;
    pthread_mutex_unlock(&self->mutex);
    return 0;
}

/** Set the property to a position value.
 *
 * Position is a relative time value in frame units.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value a position value
 * \return false
 */

int mlt_property_set_position(mlt_property self, mlt_position value)
{
    pthread_mutex_lock(&self->mutex);
    clear_property(self);
    self->types = mlt_prop_position;
    self->prop_position = value;
    pthread_mutex_unlock(&self->mutex);
    return 0;
}

/** Set the property to a string value.
 *
 * This makes a copy of the string you supply so you do not need to track
 * a new reference to it.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value the string to copy to the property
 * \return true if it failed
 */

int mlt_property_set_string(mlt_property self, const char *value)
{
    pthread_mutex_lock(&self->mutex);
    if (value != self->prop_string) {
        clear_property(self);
        self->types = mlt_prop_string;
        if (value != NULL)
            self->prop_string = strdup(value);
    } else {
        self->types = mlt_prop_string;
    }
    pthread_mutex_unlock(&self->mutex);
    return self->prop_string == NULL;
}

/** Set the property to a 64-bit integer value.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value a 64-bit integer
 * \return false
 */

int mlt_property_set_int64(mlt_property self, int64_t value)
{
    pthread_mutex_lock(&self->mutex);
    clear_property(self);
    self->types = mlt_prop_int64;
    self->prop_int64 = value;
    pthread_mutex_unlock(&self->mutex);
    return 0;
}

/** Set a property to an opaque binary value.
 *
 * This does not make a copy of the data. You can use a Properties object
 * with its reference tracking and the destructor function to control
 * the lifetime of the data. Otherwise, pass NULL for the destructor
 * function and control the lifetime yourself.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value an opaque pointer
 * \param length the number of bytes pointed to by value (optional)
 * \param destructor a function to use to destroy this binary data (optional, assuming you manage the resource)
 * \param serialiser a function to use to convert this binary data to a string (optional)
 * \return false
 */

int mlt_property_set_data(mlt_property self,
                          void *value,
                          int length,
                          mlt_destructor destructor,
                          mlt_serialiser serialiser)
{
    pthread_mutex_lock(&self->mutex);
    if (self->data == value)
        self->destructor = NULL;
    clear_property(self);
    self->types = mlt_prop_data;
    self->data = value;
    self->length = length;
    self->destructor = destructor;
    self->serialiser = serialiser;
    pthread_mutex_unlock(&self->mutex);
    return 0;
}

/** Parse a SMIL clock value.
 *
 * \private \memberof mlt_property_s
 * \param self a property
 * \param s the string to parse
 * \param fps frames per second
 * \param locale the locale to use for parsing a real number value
 * \return position in frames
 */

static int time_clock_to_frames(mlt_property self, const char *s, double fps, mlt_locale_t locale)
{
    char *pos, *copy = strdup(s);
    int hours = 0, minutes = 0;
    double seconds;

    s = copy;
    pos = strrchr(s, ':');

#if !defined(__GLIBC__) && !defined(__APPLE__) && !defined(_WIN32) && !defined(HAVE_STRTOD_L) \
    && !defined(__OpenBSD__)
    char *orig_localename = NULL;
    if (locale) {
        // Protect damaging the global locale from a temporary locale on another thread.
        pthread_mutex_lock(&self->mutex);

        // Get the current locale
        orig_localename = strdup(setlocale(LC_NUMERIC, NULL));

        // Set the new locale
        setlocale(LC_NUMERIC, locale);
    }
#endif

    if (pos) {
#if defined(__GLIBC__) || defined(__APPLE__) || defined(HAVE_STRTOD_L) && !defined(__OpenBSD__)
        if (locale)
            seconds = strtod_l(pos + 1, NULL, locale);
        else
#endif
            seconds = strtod(pos + 1, NULL);
        *pos = 0;
        pos = strrchr(s, ':');
        if (pos) {
            minutes = atoi(pos + 1);
            *pos = 0;
            hours = atoi(s);
        } else {
            minutes = atoi(s);
        }
    } else {
#if defined(__GLIBC__) || defined(__APPLE__) || defined(HAVE_STRTOD_L) && !defined(__OpenBSD__)
        if (locale)
            seconds = strtod_l(s, NULL, locale);
        else
#endif
            seconds = strtod(s, NULL);
    }

#if !defined(__GLIBC__) && !defined(__APPLE__) && !defined(_WIN32) && !defined(HAVE_STRTOD_L) \
    && !defined(__OpenBSD__)
    if (locale) {
        // Restore the current locale
        setlocale(LC_NUMERIC, orig_localename);
        free(orig_localename);
        pthread_mutex_unlock(&self->mutex);
    }
#endif

    free(copy);

    return floor(fps * hours * 3600) + floor(fps * minutes * 60) + lrint(fps * seconds);
}

/** Parse a SMPTE timecode string.
 *
 * \private \memberof mlt_property_s
 * \param self a property
 * \param s the string to parse
 * \param fps frames per second
 * \return position in frames
 */

static int time_code_to_frames(mlt_property self, const char *s, double fps)
{
    char *pos, *copy = strdup(s);
    int hours = 0, minutes = 0, seconds = 0, frames;

    s = copy;
    pos = strrchr(s, ';');
    if (!pos)
        pos = strrchr(s, ':');
    if (pos) {
        frames = atoi(pos + 1);
        *pos = 0;
        pos = strrchr(s, ':');
        if (pos) {
            seconds = atoi(pos + 1);
            *pos = 0;
            pos = strrchr(s, ':');
            if (pos) {
                minutes = atoi(pos + 1);
                *pos = 0;
                hours = atoi(s);
            } else {
                minutes = atoi(s);
            }
        } else {
            seconds = atoi(s);
        }
    } else {
        frames = atoi(s);
    }
    free(copy);

    return floor(fps * hours * 3600) + floor(fps * minutes * 60) + ceil(fps * seconds) + frames;
}

/** Convert a string to an integer.
 *
 * The string must begin with '0x' to be interpreted as hexadecimal.
 * Otherwise, it is interpreted as base 10.
 *
 * If the string begins with '#' it is interpreted as a hexadecimal color value
 * in the form RRGGBB or AARRGGBB. Color values that begin with '0x' are
 * always in the form RRGGBBAA where the alpha components are not optional.
 * Applications and services should expect the binary color value in bytes to
 * be in the following order: RGBA. This means they will have to cast the int
 * to an unsigned int. This is especially important when they need to shift
 * right to obtain RGB without alpha in order to make it do a logical instead
 * of arithmetic shift.
 *
 * If the string contains a colon it is interpreted as a time value. If it also
 * contains a period or comma character, the string is parsed as a clock value:
 * HH:MM:SS. Otherwise, the time value is parsed as a SMPTE timecode: HH:MM:SS:FF.
 * \private \memberof mlt_property_s
 * \param self a property
 * \param fps frames per second, used when converting from time value
 * \param locale the locale to use when converting from time clock value
 * \return the resultant integer
 */
static int mlt_property_atoi(mlt_property self, double fps, mlt_locale_t locale)
{
    const char *value = self->prop_string;

    // Parse a hex color value as #RRGGBB or #AARRGGBB.
    if (value[0] == '#') {
        unsigned int rgb = strtoul(value + 1, NULL, 16);
        unsigned int alpha = (strlen(value) > 7) ? (rgb >> 24) : 0xff;
        return (rgb << 8) | alpha;
    }
    // Do hex and decimal explicitly to avoid decimal value with leading zeros
    // interpreted as octal.
    else if (value[0] == '0' && value[1] == 'x') {
        return strtoul(value + 2, NULL, 16);
    } else if (fps > 0 && strchr(value, ':')) {
        if (strchr(value, '.') || strchr(value, ','))
            return time_clock_to_frames(self, value, fps, locale);
        else
            return time_code_to_frames(self, value, fps);
    } else {
        return strtol(value, NULL, 10);
    }
}

/** Get the property as an integer.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps frames per second, used when converting from time value
 * \param locale the locale to use when converting from time clock value
 * \return an integer value
 */

int mlt_property_get_int(mlt_property self, double fps, mlt_locale_t locale)
{
    pthread_mutex_lock(&self->mutex);
    int result = 0;
    if (self->types & mlt_prop_int || self->types & mlt_prop_color)
        result = self->prop_int;
    else if (self->types & mlt_prop_double)
        result = (int) self->prop_double;
    else if (self->types & mlt_prop_position)
        result = (int) self->prop_position;
    else if (self->types & mlt_prop_int64)
        result = (int) self->prop_int64;
    else if (self->types & mlt_prop_rect && self->data)
        result = (int) ((mlt_rect *) self->data)->x;
    else {
        if (self->animation && !mlt_animation_get_string(self->animation))
            mlt_property_get_string(self);
        if ((self->types & mlt_prop_string) && self->prop_string)
            result = mlt_property_atoi(self, fps, locale);
    }
    pthread_mutex_unlock(&self->mutex);
    return result;
}

/** Convert a string to a floating point number.
 *
 * If the string contains a colon it is interpreted as a time value. If it also
 * contains a period or comma character, the string is parsed as a clock value:
 * HH:MM:SS. Otherwise, the time value is parsed as a SMPTE timecode: HH:MM:SS:FF.
 * If the numeric string ends with '%' then the value is divided by 100 to convert
 * it into a ratio.
 * \private \memberof mlt_property_s
 * \param self a property
 * \param fps frames per second, used when converting from time value
 * \param locale the locale to use when converting from time clock value
 * \return the resultant real number
 */
static double mlt_property_atof(mlt_property self, double fps, mlt_locale_t locale)
{
    const char *value = self->prop_string;

    if (fps > 0 && strchr(value, ':')) {
        if (strchr(value, '.') || strchr(value, ','))
            return time_clock_to_frames(self, value, fps, locale);
        else
            return time_code_to_frames(self, value, fps);
    } else {
        char *end = NULL;
        double result;

#if defined(__GLIBC__) || defined(__APPLE__) || defined(HAVE_STRTOD_L) && !defined(__OpenBSD__)
        if (locale)
            result = strtod_l(value, &end, locale);
        else
#elif !defined(_WIN32)
        char *orig_localename = NULL;
        if (locale) {
            // Protect damaging the global locale from a temporary locale on another thread.
            pthread_mutex_lock(&self->mutex);

            // Get the current locale
            orig_localename = strdup(setlocale(LC_NUMERIC, NULL));

            // Set the new locale
            setlocale(LC_NUMERIC, locale);
        }
#endif

            result = strtod(value, &end);
        if (end && end[0] == '%')
            result /= 100.0;

#if !defined(__GLIBC__) && !defined(__APPLE__) && !defined(_WIN32) && !defined(HAVE_STRTOD_L) \
    && !defined(__OpenBSD__)
        if (locale) {
            // Restore the current locale
            setlocale(LC_NUMERIC, orig_localename);
            free(orig_localename);
            pthread_mutex_unlock(&self->mutex);
        }
#endif

        return result;
    }
}

/** Get the property as a floating point.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps frames per second, used when converting from time value
 * \param locale the locale to use for this conversion
 * \return a floating point value
 */

double mlt_property_get_double(mlt_property self, double fps, mlt_locale_t locale)
{
    double result = 0.0;
    pthread_mutex_lock(&self->mutex);
    if (self->types & mlt_prop_double)
        result = self->prop_double;
    else if (self->types & mlt_prop_int || self->types & mlt_prop_color)
        result = (double) self->prop_int;
    else if (self->types & mlt_prop_position)
        result = (double) self->prop_position;
    else if (self->types & mlt_prop_int64)
        result = (double) self->prop_int64;
    else if (self->types & mlt_prop_rect && self->data)
        result = ((mlt_rect *) self->data)->x;
    else {
        if (self->animation && !mlt_animation_get_string(self->animation))
            mlt_property_get_string(self);
        if ((self->types & mlt_prop_string) && self->prop_string)
            result = mlt_property_atof(self, fps, locale);
    }
    pthread_mutex_unlock(&self->mutex);
    return result;
}

/** Get the property as a position.
 *
 * A position is an offset time in terms of frame units.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps frames per second, used when converting from time value
 * \param locale the locale to use when converting from time clock value
 * \return the position in frames
 */

mlt_position mlt_property_get_position(mlt_property self, double fps, mlt_locale_t locale)
{
    mlt_position result = 0;
    pthread_mutex_lock(&self->mutex);
    if (self->types & mlt_prop_position)
        result = self->prop_position;
    else if (self->types & mlt_prop_int || self->types & mlt_prop_color)
        result = (mlt_position) self->prop_int;
    else if (self->types & mlt_prop_double)
        result = (mlt_position) self->prop_double;
    else if (self->types & mlt_prop_int64)
        result = (mlt_position) self->prop_int64;
    else if (self->types & mlt_prop_rect && self->data)
        result = (mlt_position) ((mlt_rect *) self->data)->x;
    else {
        if (self->animation && !mlt_animation_get_string(self->animation))
            mlt_property_get_string(self);
        if ((self->types & mlt_prop_string) && self->prop_string)
            result = (mlt_position) mlt_property_atoi(self, fps, locale);
    }
    pthread_mutex_unlock(&self->mutex);
    return result;
}

/** Convert a string to a 64-bit integer.
 *
 * If the string begins with '0x' it is interpreted as a hexadecimal value.
 * \private \memberof mlt_property_s
 * \param value a string
 * \return a 64-bit integer
 */

static inline int64_t mlt_property_atoll(const char *value)
{
    if (value == NULL)
        return 0;
    else if (value[0] == '0' && value[1] == 'x')
        return strtoll(value + 2, NULL, 16);
    else
        return strtoll(value, NULL, 10);
}

/** Get the property as a signed integer.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \return a 64-bit integer
 */

int64_t mlt_property_get_int64(mlt_property self)
{
    int64_t result = 0;
    pthread_mutex_lock(&self->mutex);
    if (self->types & mlt_prop_int64)
        result = self->prop_int64;
    else if (self->types & mlt_prop_int || self->types & mlt_prop_color)
        result = (int64_t) self->prop_int;
    else if (self->types & mlt_prop_double)
        result = (int64_t) self->prop_double;
    else if (self->types & mlt_prop_position)
        result = (int64_t) self->prop_position;
    else if (self->types & mlt_prop_rect && self->data)
        result = (int64_t) ((mlt_rect *) self->data)->x;
    else {
        if (self->animation && !mlt_animation_get_string(self->animation))
            mlt_property_get_string(self);
        if ((self->types & mlt_prop_string) && self->prop_string)
            result = mlt_property_atoll(self->prop_string);
    }
    pthread_mutex_unlock(&self->mutex);
    return result;
}

/** Get the property as a string (with time format).
 *
 * The caller is not responsible for deallocating the returned string!
 * The string is deallocated when the Property is closed.
 * This tries its hardest to convert the property to string including using
 * a serialization function for binary data, if supplied.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param time_format the time format to use for animation
 * \return a string representation of the property or NULL if failed
 */

char *mlt_property_get_string_tf(mlt_property self, mlt_time_format time_format)
{
    // Construct a string if need be
    pthread_mutex_lock(&self->mutex);
    if (self->animation && self->serialiser) {
        free(self->prop_string);
        self->prop_string = self->serialiser(self->animation, time_format);
    } else if (!(self->types & mlt_prop_string)) {
        if (self->types & mlt_prop_int) {
            self->types |= mlt_prop_string;
            self->prop_string = malloc(32);
            sprintf(self->prop_string, "%d", self->prop_int);
        } else if (self->types & mlt_prop_color) {
            self->types |= mlt_prop_string;
            self->prop_string = malloc(10);
            uint32_t int_value = ((self->prop_int & 0xff) << 24)
                                 | ((self->prop_int >> 8) & 0xffffff);
            sprintf(self->prop_string, "#%08x", int_value);
        } else if (self->types & mlt_prop_double) {
            self->types |= mlt_prop_string;
            self->prop_string = malloc(32);
            sprintf(self->prop_string, "%g", self->prop_double);
        } else if (self->types & mlt_prop_position) {
            self->types |= mlt_prop_string;
            self->prop_string = malloc(32);
            sprintf(self->prop_string, "%d", (int) self->prop_position);
        } else if (self->types & mlt_prop_int64) {
            self->types |= mlt_prop_string;
            self->prop_string = malloc(32);
            sprintf(self->prop_string, "%" PRId64, self->prop_int64);
        } else if (self->types & mlt_prop_data && self->data && self->serialiser) {
            self->types |= mlt_prop_string;
            self->prop_string = self->serialiser(self->data, self->length);
        }
    }
    pthread_mutex_unlock(&self->mutex);

    // Return the string (may be NULL)
    return self->prop_string;
}

static mlt_time_format default_time_format()
{
    const char *e = getenv("MLT_ANIMATION_TIME_FORMAT");
    return e ? strtol(e, NULL, 10) : mlt_time_frames;
}

/** Get the property as a string.
 *
 * The caller is not responsible for deallocating the returned string!
 * The string is deallocated when the Property is closed.
 * This tries its hardest to convert the property to string including using
 * a serialization function for binary data, if supplied.
 * \public \memberof mlt_property_s
 * \param self a property
 * \return a string representation of the property or NULL if failed
 */

char *mlt_property_get_string(mlt_property self)
{
    return mlt_property_get_string_tf(self, default_time_format());
}

/** Get the property as a string (with locale and time format).
 *
 * The caller is not responsible for deallocating the returned string!
 * The string is deallocated when the Property is closed.
 * This tries its hardest to convert the property to string including using
 * a serialization function for binary data, if supplied.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param locale the locale to use for this conversion
 * \param time_format the time format to use for animation
 * \return a string representation of the property or NULL if failed
 */

char *mlt_property_get_string_l_tf(mlt_property self,
                                   mlt_locale_t locale,
                                   mlt_time_format time_format)
{
    // Optimization for no locale
    if (!locale)
        return mlt_property_get_string_tf(self, time_format);

    // Construct a string if need be
    pthread_mutex_lock(&self->mutex);
    if (self->animation && self->serialiser) {
        free(self->prop_string);
        self->prop_string = self->serialiser(self->animation, time_format);
    } else if (!(self->types & mlt_prop_string)) {
#if !defined(_WIN32)
        // TODO: when glibc gets sprintf_l, start using it! For now, hack on setlocale.
        // Save the current locale
#if defined(__APPLE__)
        const char *localename = querylocale(LC_NUMERIC_MASK, locale);
#elif defined(__GLIBC__)
        const char *localename = locale->__names[LC_NUMERIC];
#else
        const char *localename = locale;
#endif
        // Get the current locale
        char *orig_localename = strdup(setlocale(LC_NUMERIC, NULL));

        // Set the new locale
        setlocale(LC_NUMERIC, localename);
#endif // _WIN32

        if (self->types & mlt_prop_int) {
            self->types |= mlt_prop_string;
            self->prop_string = malloc(32);
            sprintf(self->prop_string, "%d", self->prop_int);
        } else if (self->types & mlt_prop_color) {
            self->types |= mlt_prop_string;
            self->prop_string = malloc(10);
            uint32_t int_value = ((self->prop_int & 0xff) << 24)
                                 | ((self->prop_int >> 8) & 0xffffff);
            sprintf(self->prop_string, "#%08x", int_value);
        } else if (self->types & mlt_prop_double) {
            self->types |= mlt_prop_string;
            self->prop_string = malloc(32);
            sprintf(self->prop_string, "%g", self->prop_double);
        } else if (self->types & mlt_prop_position) {
            self->types |= mlt_prop_string;
            self->prop_string = malloc(32);
            sprintf(self->prop_string, "%d", (int) self->prop_position);
        } else if (self->types & mlt_prop_int64) {
            self->types |= mlt_prop_string;
            self->prop_string = malloc(32);
            sprintf(self->prop_string, "%" PRId64, self->prop_int64);
        } else if (self->types & mlt_prop_data && self->data && self->serialiser) {
            self->types |= mlt_prop_string;
            self->prop_string = self->serialiser(self->data, self->length);
        }
#if !defined(_WIN32)
        // Restore the current locale
        setlocale(LC_NUMERIC, orig_localename);
        free(orig_localename);
#endif
    }
    pthread_mutex_unlock(&self->mutex);

    // Return the string (may be NULL)
    return self->prop_string;
}

/** Get the property as a string (with locale).
 *
 * The caller is not responsible for deallocating the returned string!
 * The string is deallocated when the Property is closed.
 * This tries its hardest to convert the property to string including using
 * a serialization function for binary data, if supplied.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param locale the locale to use for this conversion
 * \return a string representation of the property or NULL if failed
 */

char *mlt_property_get_string_l(mlt_property self, mlt_locale_t locale)
{
    return mlt_property_get_string_l_tf(self, locale, default_time_format());
}

/** Get the binary data from a property.
 *
 * This only works if you previously put binary data into the property.
 * This does not return a copy of the data; it returns a pointer to it.
 * If you supplied a destructor function when setting the binary data,
 * the destructor is used when the Property is closed to free the memory.
 * Therefore, only free the returned pointer if you did not supply a
 * destructor function.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param[out] length the size of the binary object in bytes (optional)
 * \return an opaque data pointer or NULL if not available
 */

void *mlt_property_get_data(mlt_property self, int *length)
{
    // Assign length if not NULL
    if (length != NULL)
        *length = self->length;

    // Return the data (note: there is no conversion here)
    pthread_mutex_lock(&self->mutex);
    void *result = self->data;
    pthread_mutex_unlock(&self->mutex);
    return result;
}

/** Destroy a property and free all related resources.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 */

void mlt_property_close(mlt_property self)
{
    clear_property(self);
    pthread_mutex_destroy(&self->mutex);
    free(self);
}

/** Copy a property.
 *
 * A Property holding binary data only copies the data if a serialiser
 * function was supplied when you set the Property.
 * \public \memberof mlt_property_s
 * \author Zach <zachary.drew@gmail.com>
 * \param self a property
 * \param that another property
 */
void mlt_property_pass(mlt_property self, mlt_property that)
{
    pthread_mutex_lock(&self->mutex);
    clear_property(self);

    self->types = that->types;

    if (self->types & mlt_prop_int64)
        self->prop_int64 = that->prop_int64;
    else if (self->types & mlt_prop_int || self->types & mlt_prop_color)
        self->prop_int = that->prop_int;
    else if (self->types & mlt_prop_double)
        self->prop_double = that->prop_double;
    else if (self->types & mlt_prop_position)
        self->prop_position = that->prop_position;
    if (self->types & mlt_prop_string) {
        if (that->prop_string != NULL)
            self->prop_string = strdup(that->prop_string);
    } else if (that->types & mlt_prop_rect) {
        clear_property(self);
        self->types = mlt_prop_rect | mlt_prop_data;
        self->length = that->length;
        self->data = calloc(1, self->length);
        memcpy(self->data, that->data, self->length);
        self->destructor = free;
        self->serialiser = that->serialiser;
    } else if (that->animation && that->serialiser) {
        self->types = mlt_prop_string;
        self->prop_string = that->serialiser(that->animation, default_time_format());
    } else if (that->types & mlt_prop_data && that->serialiser) {
        self->types = mlt_prop_string;
        self->prop_string = that->serialiser(that->data, that->length);
    }
    pthread_mutex_unlock(&self->mutex);
}

/** Convert frame count to a SMPTE timecode string.
 *
 * \private \memberof mlt_property_s
 * \param frames a frame count
 * \param fps frames per second
 * \param[out] s the string to write into - must have enough space to hold largest time string
 */

static void time_smpte_from_frames(int frames, double fps, char *s, int drop)
{
    int hours, mins, secs;
    char frame_sep = ':';
    int save_frames = frames;

    if (fps == 30000.0 / 1001.0) {
        fps = 30.0;
        if (drop) {
            int i;
            for (i = 1800; i <= frames; i += 1800) {
                if (i % 18000)
                    frames += 2;
            }
            frame_sep = ';';
        }
    } else if (fps == 60000.0 / 1001.0) {
        fps = 60.0;
        if (drop) {
            int i;
            for (i = 3600; i <= frames; i += 3600) {
                if (i % 36000)
                    frames += 4;
            }
            frame_sep = ';';
        }
    } else if (!drop) {
        fps = lrint(fps);
    } else if (fps != lrint(fps)) {
        frame_sep = ';';
    }
    hours = frames / (fps * 3600);
    frames -= floor(hours * 3600 * fps);

    mins = frames / (fps * 60);
    if (mins == 60) { // floating point error
        ++hours;
        frames = save_frames - floor(hours * 3600 * fps);
        mins = 0;
    }
    save_frames = frames;
    frames -= floor(mins * 60 * fps);

    secs = frames / fps;
    if (secs == 60) { // floating point error
        ++mins;
        frames = save_frames - floor(mins * 60 * fps);
        secs = 0;
    }
    frames -= ceil(secs * fps);

    sprintf(s,
            "%02d:%02d:%02d%c%0*d",
            hours,
            mins,
            secs,
            frame_sep,
            (fps > 999  ? 4
             : fps > 99 ? 3
                        : 2),
            frames);
}

/** Convert frame count to a SMIL clock value string.
 *
 * \private \memberof mlt_property_s
 * \param frames a frame count
 * \param fps frames per second
 * \param[out] s the string to write into - must have enough space to hold largest time string
 */

static void time_clock_from_frames(int frames, double fps, char *s)
{
    int hours, mins;
    double secs;
    int save_frames = frames;

    hours = frames / (fps * 3600);
    frames -= floor(hours * 3600 * fps);

    mins = frames / (fps * 60);
    if (mins == 60) { // floating point error
        ++hours;
        frames = save_frames - floor(hours * 3600 * fps);
        mins = 0;
    }
    save_frames = frames;
    frames -= floor(mins * 60 * fps);

    secs = frames / fps;
    if (secs >= 60.0) { // floating point error
        ++mins;
        frames = save_frames - floor(mins * 60 * fps);
        secs = frames / fps;
    }

    sprintf(s, "%02d:%02d:%06.3f", hours, mins, secs);
}

/** Get the property as a time string.
 *
 * The time value can be either a SMPTE timecode or SMIL clock value.
 * The caller is not responsible for deallocating the returned string!
 * The string is deallocated when the property is closed.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param format the time format that you want
 * \param fps frames per second
 * \param locale the locale to use for this conversion
 * \return a string representation of the property or NULL if failed
 */

char *mlt_property_get_time(mlt_property self,
                            mlt_time_format format,
                            double fps,
                            mlt_locale_t locale)
{
#if !defined(_WIN32)
    char *orig_localename = NULL;
#endif
    int frames = 0;

    // Remove existing string
    if (self->prop_string)
        mlt_property_set_int(self, mlt_property_get_int(self, fps, locale));

    // Optimization for mlt_time_frames
    if (format == mlt_time_frames)
        return mlt_property_get_string_l(self, locale);

#if !defined(_WIN32)
    // Use the specified locale
    if (locale) {
        // TODO: when glibc gets sprintf_l, start using it! For now, hack on setlocale.
        // Save the current locale
#if defined(__APPLE__)
        const char *localename = querylocale(LC_NUMERIC_MASK, locale);
#elif defined(__GLIBC__)
        const char *localename = locale->__names[LC_NUMERIC];
#else
        // TODO: not yet sure what to do on other platforms
        const char *localename = locale;
#endif // _WIN32

        // Protect damaging the global locale from a temporary locale on another thread.
        pthread_mutex_lock(&self->mutex);

        // Get the current locale
        orig_localename = strdup(setlocale(LC_NUMERIC, NULL));

        // Set the new locale
        setlocale(LC_NUMERIC, localename);
    } else
#endif // _WIN32
    {
        // Make sure we have a lock before accessing self->types
        pthread_mutex_lock(&self->mutex);
    }

    // Convert number to string
    if (self->types & mlt_prop_int) {
        frames = self->prop_int;
    } else if (self->types & mlt_prop_position) {
        frames = (int) self->prop_position;
    } else if (self->types & mlt_prop_double) {
        frames = self->prop_double;
    } else if (self->types & mlt_prop_int64) {
        frames = (int) self->prop_int64;
    }

    self->types |= mlt_prop_string;
    self->prop_string = malloc(32);

    if (format == mlt_time_clock)
        time_clock_from_frames(frames, fps, self->prop_string);
    else if (format == mlt_time_smpte_ndf)
        time_smpte_from_frames(frames, fps, self->prop_string, 0);
    else // Use smpte drop frame by default
        time_smpte_from_frames(frames, fps, self->prop_string, 1);

#if !defined(_WIN32)
    // Restore the current locale
    if (locale) {
        setlocale(LC_NUMERIC, orig_localename);
        free(orig_localename);
        pthread_mutex_unlock(&self->mutex);
    } else
#endif // _WIN32
    {
        // Make sure we have a lock before accessing self->types
        pthread_mutex_unlock(&self->mutex);
    }

    // Return the string (may be NULL)
    return self->prop_string;
}

/** Determine if the property holds a numeric or numeric string value.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param locale the locale to use for string evaluation
 * \return true if it is numeric
 */

int mlt_property_is_numeric(mlt_property self, mlt_locale_t locale)
{
    int result = (self->types & mlt_prop_int) || (self->types & mlt_prop_color)
                 || (self->types & mlt_prop_int64) || (self->types & mlt_prop_double)
                 || (self->types & mlt_prop_position) || (self->types & mlt_prop_rect);

    // If not already numeric but string is numeric.
    if ((!result && self->types & mlt_prop_string) && self->prop_string) {
        char *p = NULL;

#if defined(__GLIBC__) || defined(__APPLE__) || defined(HAVE_STRTOD_L) && !defined(__OpenBSD__)
        if (locale)
            strtod_l(self->prop_string, &p, locale);
        else
#elif !defined(_WIN32)
        char *orig_localename = NULL;
        if (locale) {
            // Protect damaging the global locale from a temporary locale on another thread.
            pthread_mutex_lock(&self->mutex);

            // Get the current locale
            orig_localename = strdup(setlocale(LC_NUMERIC, NULL));

            // Set the new locale
            setlocale(LC_NUMERIC, locale);
        }
#endif

            strtod(self->prop_string, &p);

#if !defined(__GLIBC__) && !defined(__APPLE__) && !defined(_WIN32) && !defined(HAVE_STRTOD_L) \
    && !defined(__OpenBSD__)
        if (locale) {
            // Restore the current locale
            setlocale(LC_NUMERIC, orig_localename);
            free(orig_localename);
            pthread_mutex_unlock(&self->mutex);
        }
#endif

        result = (p != self->prop_string);
    }
    return result;
}

/** A linear interpolation function for animation.
 *
 * \deprecated
 * \private \memberof mlt_property_s
 */

static inline double linear_interpolate(double y1, double y2, double t)
{
    return y1 + (y2 - y1) * t;
}

/** A smooth spline interpolation for animation.
 *
 * \deprecated
 * For non-closed curves, you need to also supply the tangent vector at the first and last control point.
 * This is commonly done: T(P[0]) = P[1] - P[0] and T(P[n]) = P[n] - P[n-1].
 * \private \memberof mlt_property_s
 */

static inline double catmull_rom_interpolate(double y0, double y1, double y2, double y3, double t)
{
    double t2 = t * t;
    double a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
    double a1 = y0 - 2.5 * y1 + 2 * y2 - 0.5 * y3;
    double a2 = -0.5 * y0 + 0.5 * y2;
    double a3 = y1;
    return a0 * t * t2 + a1 * t2 + a2 * t + a3;
}

/** Interpolate a new property value given a set of other properties.
 *
 * \deprecated
 * \public \memberof mlt_property_s
 * \param self the property onto which to set the computed value
 * \param p an array of at least 1 value in p[1] if \p interp is discrete,
 *  2 values in p[1] and p[2] if \p interp is linear, or
 *  4 values in p[0] - p[3] if \p interp is smooth
 * \param progress a ratio in the range [0, 1] to indicate how far between p[1] and p[2]
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param interp the interpolation method to use
 * \return true if there was an error
 */

int mlt_property_interpolate(mlt_property self,
                             mlt_property p[],
                             double progress,
                             double fps,
                             mlt_locale_t locale,
                             mlt_keyframe_type interp)
{
    int error = 0;
    int colorstring = 0;
    const char *value = self->prop_string;
    if (value
        && ((strlen(value) > 6 && value[0] == '#')
            || (strlen(value) > 7 && value[0] == '0' && value[1] == 'x'))) {
        colorstring = 1;
    }

    if (interp != mlt_keyframe_discrete && (self->types & mlt_prop_color || colorstring)) {
        mlt_color value = {0xff, 0xff, 0xff, 0xff};
        if (interp == mlt_keyframe_linear) {
            mlt_color colors[2];
            mlt_color zero = {0xff, 0xff, 0xff, 0xff};
            colors[0] = p[1] ? mlt_property_get_color(p[1], fps, locale) : zero;
            if (p[2]) {
                colors[1] = mlt_property_get_color(p[2], fps, locale);
                value.r = linear_interpolate(colors[0].r, colors[1].r, progress);
                value.g = linear_interpolate(colors[0].g, colors[1].g, progress);
                value.b = linear_interpolate(colors[0].b, colors[1].b, progress);
                value.a = linear_interpolate(colors[0].a, colors[1].a, progress);
            } else {
                value = colors[0];
            }
        } else if (interp == mlt_keyframe_smooth) {
            mlt_color colors[4];
            mlt_color zero = {0xff, 0xff, 0xff, 0xff};
            colors[1] = p[1] ? mlt_property_get_color(p[1], fps, locale) : zero;
            if (p[2]) {
                colors[0] = p[0] ? mlt_property_get_color(p[0], fps, locale) : zero;
                colors[2] = p[2] ? mlt_property_get_color(p[2], fps, locale) : zero;
                colors[3] = p[3] ? mlt_property_get_color(p[3], fps, locale) : zero;
                value.r = CLAMP(catmull_rom_interpolate(colors[0].r,
                                                        colors[1].r,
                                                        colors[2].r,
                                                        colors[3].r,
                                                        progress),
                                0,
                                255);
                value.g = CLAMP(catmull_rom_interpolate(colors[0].g,
                                                        colors[1].g,
                                                        colors[2].g,
                                                        colors[3].g,
                                                        progress),
                                0,
                                255);
                value.b = CLAMP(catmull_rom_interpolate(colors[0].b,
                                                        colors[1].b,
                                                        colors[2].b,
                                                        colors[3].b,
                                                        progress),
                                0,
                                255);
                value.a = CLAMP(catmull_rom_interpolate(colors[0].a,
                                                        colors[1].a,
                                                        colors[2].a,
                                                        colors[3].a,
                                                        progress),
                                0,
                                255);
            } else {
                value = colors[1];
            }
        }
        error = mlt_property_set_color(self, value);
    } else if (interp != mlt_keyframe_discrete && mlt_property_is_numeric(p[1], locale)
               && mlt_property_is_numeric(p[2], locale)) {
        if (self->types & mlt_prop_rect) {
            mlt_rect value = {DBL_MIN, DBL_MIN, DBL_MIN, DBL_MIN, DBL_MIN};
            if (interp == mlt_keyframe_linear) {
                mlt_rect points[2];
                mlt_rect zero = {0, 0, 0, 0, 0};
                points[0] = p[1] ? mlt_property_get_rect(p[1], locale) : zero;
                if (p[2]) {
                    points[1] = mlt_property_get_rect(p[2], locale);
                    value.x = linear_interpolate(points[0].x, points[1].x, progress);
                    value.y = linear_interpolate(points[0].y, points[1].y, progress);
                    value.w = linear_interpolate(points[0].w, points[1].w, progress);
                    value.h = linear_interpolate(points[0].h, points[1].h, progress);
                    value.o = linear_interpolate(points[0].o, points[1].o, progress);
                } else {
                    value = points[0];
                }
            } else if (interp == mlt_keyframe_smooth) {
                mlt_rect points[4];
                mlt_rect zero = {0, 0, 0, 0, 0};
                points[1] = p[1] ? mlt_property_get_rect(p[1], locale) : zero;
                if (p[2]) {
                    points[0] = p[0] ? mlt_property_get_rect(p[0], locale) : zero;
                    points[2] = p[2] ? mlt_property_get_rect(p[2], locale) : zero;
                    points[3] = p[3] ? mlt_property_get_rect(p[3], locale) : zero;
                    value.x = catmull_rom_interpolate(points[0].x,
                                                      points[1].x,
                                                      points[2].x,
                                                      points[3].x,
                                                      progress);
                    value.y = catmull_rom_interpolate(points[0].y,
                                                      points[1].y,
                                                      points[2].y,
                                                      points[3].y,
                                                      progress);
                    value.w = catmull_rom_interpolate(points[0].w,
                                                      points[1].w,
                                                      points[2].w,
                                                      points[3].w,
                                                      progress);
                    value.h = catmull_rom_interpolate(points[0].h,
                                                      points[1].h,
                                                      points[2].h,
                                                      points[3].h,
                                                      progress);
                    value.o = catmull_rom_interpolate(points[0].o,
                                                      points[1].o,
                                                      points[2].o,
                                                      points[3].o,
                                                      progress);
                } else {
                    value = points[1];
                }
            }
            error = mlt_property_set_rect(self, value);
        } else {
            double value = 0.0;
            if (interp == mlt_keyframe_linear) {
                double points[2];
                points[0] = p[1] ? mlt_property_get_double(p[1], fps, locale) : 0;
                points[1] = p[2] ? mlt_property_get_double(p[2], fps, locale) : 0;
                value = p[2] ? linear_interpolate(points[0], points[1], progress) : points[0];
            } else if (interp == mlt_keyframe_smooth) {
                double points[4];
                points[0] = p[0] ? mlt_property_get_double(p[0], fps, locale) : 0;
                points[1] = p[1] ? mlt_property_get_double(p[1], fps, locale) : 0;
                points[2] = p[2] ? mlt_property_get_double(p[2], fps, locale) : 0;
                points[3] = p[3] ? mlt_property_get_double(p[3], fps, locale) : 0;
                value = p[2] ? catmull_rom_interpolate(points[0],
                                                       points[1],
                                                       points[2],
                                                       points[3],
                                                       progress)
                             : points[1];
            }
            error = mlt_property_set_double(self, value);
        }
    } else {
        mlt_property_pass(self, p[1]);
    }
    return error;
}

/** Create a new animation or refresh an existing one.
 *
 * \private \memberof mlt_property_s
 * \param self a property
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 */

static void refresh_animation(mlt_property self, double fps, mlt_locale_t locale, int length)
{
    if (!self->animation) {
        self->animation = mlt_animation_new();
        self->serialiser = (mlt_serialiser) mlt_animation_serialize_tf;
        mlt_animation_parse(self->animation, self->prop_string, length, fps, locale);
    } else if (!mlt_animation_get_string(self->animation)) {
        // The animation clears its string if it is modified.
        // Do not use a property string that is out of sync.
        self->types &= ~mlt_prop_string;
        free(self->prop_string);
        self->prop_string = NULL;
    } else if ((self->types & mlt_prop_string) && self->prop_string) {
        mlt_animation_refresh(self->animation, self->prop_string, length);
    } else if (length >= 0) {
        mlt_animation_set_length(self->animation, length);
    }
}

/** Get the real number at a frame position.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param position the frame number
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \return the real number
 */

double mlt_property_anim_get_double(
    mlt_property self, double fps, mlt_locale_t locale, int position, int length)
{
    double result;
    pthread_mutex_lock(&self->mutex);
    if (mlt_property_is_anim(self)) {
        struct mlt_animation_item_s item;
        item.property = mlt_property_init();

        refresh_animation(self, fps, locale, length);
        mlt_animation_get_item(self->animation, &item, position);
        pthread_mutex_unlock(&self->mutex);
        result = mlt_property_get_double(item.property, fps, locale);

        mlt_property_close(item.property);
    } else {
        pthread_mutex_unlock(&self->mutex);
        result = mlt_property_get_double(self, fps, locale);
    }
    return result;
}

/** Get the property as an integer number at a frame position.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param position the frame number
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \return an integer value
 */

int mlt_property_anim_get_int(
    mlt_property self, double fps, mlt_locale_t locale, int position, int length)
{
    int result;
    pthread_mutex_lock(&self->mutex);
    if (mlt_property_is_anim(self)) {
        struct mlt_animation_item_s item;
        item.property = mlt_property_init();

        refresh_animation(self, fps, locale, length);
        mlt_animation_get_item(self->animation, &item, position);
        pthread_mutex_unlock(&self->mutex);
        result = mlt_property_get_int(item.property, fps, locale);

        mlt_property_close(item.property);
    } else {
        pthread_mutex_unlock(&self->mutex);
        result = mlt_property_get_int(self, fps, locale);
    }
    return result;
}

/** Get the string at certain a frame position.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param position the frame number
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \return the string representation of the property or NULL if failed
 */

char *mlt_property_anim_get_string(
    mlt_property self, double fps, mlt_locale_t locale, int position, int length)
{
    char *result;
    pthread_mutex_lock(&self->mutex);
    if (mlt_property_is_anim(self)) {
        struct mlt_animation_item_s item;
        item.property = mlt_property_init();

        if (!self->animation)
            refresh_animation(self, fps, locale, length);
        mlt_animation_get_item(self->animation, &item, position);

        free(self->prop_string);

        pthread_mutex_unlock(&self->mutex);
        self->prop_string = mlt_property_get_string_l(item.property, locale);
        pthread_mutex_lock(&self->mutex);

        if (self->prop_string)
            self->prop_string = strdup(self->prop_string);
        self->types |= mlt_prop_string;

        result = self->prop_string;
        mlt_property_close(item.property);
        pthread_mutex_unlock(&self->mutex);
    } else {
        pthread_mutex_unlock(&self->mutex);
        result = mlt_property_get_string_l(self, locale);
    }
    return result;
}

/** Set a property animation keyframe to a real number.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value a double precision floating point value
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param position the frame number
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \param keyframe_type the interpolation method for this keyframe
 * \return false if successful, true to indicate error
 */

int mlt_property_anim_set_double(mlt_property self,
                                 double value,
                                 double fps,
                                 mlt_locale_t locale,
                                 int position,
                                 int length,
                                 mlt_keyframe_type keyframe_type)
{
    int result;
    struct mlt_animation_item_s item;

    item.property = mlt_property_init();
    item.frame = position;
    item.keyframe_type = keyframe_type;
    mlt_property_set_double(item.property, value);

    pthread_mutex_lock(&self->mutex);
    refresh_animation(self, fps, locale, length);
    result = mlt_animation_insert(self->animation, &item);
    mlt_animation_interpolate(self->animation);
    pthread_mutex_unlock(&self->mutex);
    mlt_property_close(item.property);

    return result;
}

/** Set a property animation keyframe to an integer value.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value an integer
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param position the frame number
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \param keyframe_type the interpolation method for this keyframe
 * \return false if successful, true to indicate error
 */

int mlt_property_anim_set_int(mlt_property self,
                              int value,
                              double fps,
                              mlt_locale_t locale,
                              int position,
                              int length,
                              mlt_keyframe_type keyframe_type)
{
    int result;
    struct mlt_animation_item_s item;

    item.property = mlt_property_init();
    item.frame = position;
    item.keyframe_type = keyframe_type;
    mlt_property_set_int(item.property, value);

    pthread_mutex_lock(&self->mutex);
    refresh_animation(self, fps, locale, length);
    result = mlt_animation_insert(self->animation, &item);
    mlt_animation_interpolate(self->animation);
    pthread_mutex_unlock(&self->mutex);
    mlt_property_close(item.property);

    return result;
}

/** Set a property animation keyframe to a string.
 *
 * Strings only support discrete animation. Do not use this to set a property's
 * animation string that contains a semicolon-delimited set of values; use
 * mlt_property_set() for that.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value a string
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param position the frame number
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \return false if successful, true to indicate error
 */

int mlt_property_anim_set_string(
    mlt_property self, const char *value, double fps, mlt_locale_t locale, int position, int length)
{
    int result;
    struct mlt_animation_item_s item;

    item.property = mlt_property_init();
    item.frame = position;
    item.keyframe_type = mlt_keyframe_discrete;
    mlt_property_set_string(item.property, value);

    pthread_mutex_lock(&self->mutex);
    refresh_animation(self, fps, locale, length);
    result = mlt_animation_insert(self->animation, &item);
    mlt_animation_interpolate(self->animation);
    pthread_mutex_unlock(&self->mutex);
    mlt_property_close(item.property);

    return result;
}

/** Get an object's animation object.
 *
 * You might need to call another mlt_property_anim_ function to actually construct
 * the animation, as this is a simple accessor function.
 * \public \memberof mlt_property_s
 * \param self a property
 * \return the animation object or NULL if there is no animation
 */

mlt_animation mlt_property_get_animation(mlt_property self)
{
    pthread_mutex_lock(&self->mutex);
    mlt_animation result = self->animation;
    pthread_mutex_unlock(&self->mutex);
    return result;
}

/** Set the property to a color value.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value an integer
 * \return false
 */

int mlt_property_set_color(mlt_property self, mlt_color value)
{
    pthread_mutex_lock(&self->mutex);
    clear_property(self);
    self->types = mlt_prop_color;
    uint32_t int_value = (value.r << 24) | (value.g << 16) | (value.b << 8) | value.a;
    self->prop_int = int_value;
    pthread_mutex_unlock(&self->mutex);
    return 0;
}

/** Get the property as a color.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps frames per second, used when converting from time value
 * \param locale the locale to use for when converting from a string
 * \return a color value
 */

mlt_color mlt_property_get_color(mlt_property self, double fps, mlt_locale_t locale)
{
    mlt_color result = {0xff, 0xff, 0xff, 0xff};
    int color_int = mlt_property_get_int(self, fps, locale);

    if ((self->types & mlt_prop_string) && self->prop_string) {
        const char *color = mlt_property_get_string_l(self, locale);

        if (!strcmp(color, "red")) {
            result.r = 0xff;
            result.g = 0x00;
            result.b = 0x00;
            return result;
        }
        if (!strcmp(color, "green")) {
            result.r = 0x00;
            result.g = 0xff;
            result.b = 0x00;
            return result;
        }
        if (!strcmp(color, "blue")) {
            result.r = 0x00;
            result.g = 0x00;
            result.b = 0xff;
            return result;
        }
        if (!strcmp(color, "black")) {
            result.r = 0x00;
            result.g = 0x00;
            result.b = 0x00;
            return result;
        }
        if (!strcmp(color, "white")) {
            return result;
        }
    }

    result.r = (color_int >> 24) & 0xff;
    result.g = (color_int >> 16) & 0xff;
    result.b = (color_int >> 8) & 0xff;
    result.a = (color_int) &0xff;
    return result;
}

/** Set a property animation keyframe to a color.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value a color
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param position the frame number
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \param keyframe_type the interpolation method for this keyframe
 * \return false if successful, true to indicate error
 */

int mlt_property_anim_set_color(mlt_property self,
                                mlt_color value,
                                double fps,
                                mlt_locale_t locale,
                                int position,
                                int length,
                                mlt_keyframe_type keyframe_type)
{
    int result;
    struct mlt_animation_item_s item;

    item.property = mlt_property_init();
    item.frame = position;
    item.keyframe_type = keyframe_type;
    mlt_property_set_color(item.property, value);

    pthread_mutex_lock(&self->mutex);
    refresh_animation(self, fps, locale, length);
    result = mlt_animation_insert(self->animation, &item);
    mlt_animation_interpolate(self->animation);
    pthread_mutex_unlock(&self->mutex);
    mlt_property_close(item.property);

    return result;
}

/** Get a color at a frame position.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param position the frame number
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \return the color
 */

mlt_color mlt_property_anim_get_color(
    mlt_property self, double fps, mlt_locale_t locale, int position, int length)
{
    mlt_color result;
    pthread_mutex_lock(&self->mutex);
    if (mlt_property_is_anim(self)) {
        struct mlt_animation_item_s item;
        item.property = mlt_property_init();
        item.property->types = mlt_prop_color;

        refresh_animation(self, fps, locale, length);
        mlt_animation_get_item(self->animation, &item, position);
        pthread_mutex_unlock(&self->mutex);
        result = mlt_property_get_color(item.property, fps, locale);

        mlt_property_close(item.property);
    } else {
        pthread_mutex_unlock(&self->mutex);
        result = mlt_property_get_color(self, fps, locale);
    }
    return result;
}

/** Convert a rectangle value into a string.
 *
 * The canonical form of a mlt_rect
 * is a space delimited "x y w h o" even though many kinds of field delimiters
 * may be used to convert a string to a rectangle.
 * \private \memberof mlt_property_s
 * \param rect the rectangle to convert
 * \param length not used
 * \return the string representation of a rectangle
 */

static char *serialise_mlt_rect(mlt_rect *rect, int length)
{
    char *result = calloc(1, 100);
    if (rect->x != DBL_MIN)
        sprintf(result + strlen(result), "%g", rect->x);
    if (rect->y != DBL_MIN)
        sprintf(result + strlen(result), " %g", rect->y);
    if (rect->w != DBL_MIN)
        sprintf(result + strlen(result), " %g", rect->w);
    if (rect->h != DBL_MIN)
        sprintf(result + strlen(result), " %g", rect->h);
    if (rect->o != DBL_MIN)
        sprintf(result + strlen(result), " %g", rect->o);
    return result;
}

/** Set a property to a mlt_rect rectangle.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value a rectangle
 * \return false
 */

int mlt_property_set_rect(mlt_property self, mlt_rect value)
{
    pthread_mutex_lock(&self->mutex);
    clear_property(self);
    self->types = mlt_prop_rect | mlt_prop_data;
    self->length = sizeof(value);
    self->data = calloc(1, self->length);
    memcpy(self->data, &value, self->length);
    self->destructor = free;
    self->serialiser = (mlt_serialiser) serialise_mlt_rect;
    pthread_mutex_unlock(&self->mutex);
    return 0;
}

/** Get the property as a rectangle.
 *
 * You can use any non-numeric character(s) as a field delimiter.
 * If the number has a '%' immediately following it, the number is divided by
 * 100 to convert it into a real number.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param locale the locale to use for when converting from a string
 * \return a rectangle value
 */

mlt_rect mlt_property_get_rect(mlt_property self, mlt_locale_t locale)
{
    mlt_rect rect = {DBL_MIN, DBL_MIN, DBL_MIN, DBL_MIN, DBL_MIN};
    if ((self->types & mlt_prop_rect) && self->data)
        rect = *((mlt_rect *) self->data);
    else if (self->types & mlt_prop_double)
        rect.x = self->prop_double;
    else if (self->types & mlt_prop_int || self->types & mlt_prop_color)
        rect.x = (double) self->prop_int;
    else if (self->types & mlt_prop_position)
        rect.x = (double) self->prop_position;
    else if (self->types & mlt_prop_int64)
        rect.x = (double) self->prop_int64;
    else if ((self->types & mlt_prop_string) && self->prop_string) {
        char *value = self->prop_string;
        char *p = NULL;
        int count = 0;

#if !defined(__GLIBC__) && !defined(__APPLE__) && !defined(_WIN32) && !defined(HAVE_STRTOD_L) \
    && !defined(__OpenBSD__)
        char *orig_localename = NULL;
        if (locale) {
            // Protect damaging the global locale from a temporary locale on another thread.
            pthread_mutex_lock(&self->mutex);

            // Get the current locale
            orig_localename = strdup(setlocale(LC_NUMERIC, NULL));

            // Set the new locale
            setlocale(LC_NUMERIC, locale);
        }
#endif

        while (*value) {
            double temp;
#if defined(__GLIBC__) || defined(__APPLE__) || defined(HAVE_STRTOD_L) && !defined(__OpenBSD__)
            if (locale)
                temp = strtod_l(value, &p, locale);
            else
#endif
                temp = strtod(value, &p);
            if (p != value) {
                if (p[0] == '%') {
                    temp /= 100.0;
                    p++;
                }

                // Chomp the delimiter.
                if (*p)
                    p++;

                // Assign the value to appropriate field.
                switch (count) {
                case 0:
                    rect.x = temp;
                    break;
                case 1:
                    rect.y = temp;
                    break;
                case 2:
                    rect.w = temp;
                    break;
                case 3:
                    rect.h = temp;
                    break;
                case 4:
                    rect.o = temp;
                    break;
                }
            } else {
                p++;
            }
            value = p;
            count++;
        }

#if !defined(__GLIBC__) && !defined(__APPLE__) && !defined(_WIN32) && !defined(HAVE_STRTOD_L) \
    && !defined(__OpenBSD__)
        if (locale) {
            // Restore the current locale
            setlocale(LC_NUMERIC, orig_localename);
            free(orig_localename);
            pthread_mutex_unlock(&self->mutex);
        }
#endif
    }
    return rect;
}

/** Set a property animation keyframe to a rectangle.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value a rectangle
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param position the frame number
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \param keyframe_type the interpolation method for this keyframe
 * \return false if successful, true to indicate error
 */

int mlt_property_anim_set_rect(mlt_property self,
                               mlt_rect value,
                               double fps,
                               mlt_locale_t locale,
                               int position,
                               int length,
                               mlt_keyframe_type keyframe_type)
{
    int result;
    struct mlt_animation_item_s item;

    item.property = mlt_property_init();
    item.frame = position;
    item.keyframe_type = keyframe_type;
    mlt_property_set_rect(item.property, value);

    pthread_mutex_lock(&self->mutex);
    refresh_animation(self, fps, locale, length);
    result = mlt_animation_insert(self->animation, &item);
    mlt_animation_interpolate(self->animation);
    pthread_mutex_unlock(&self->mutex);
    mlt_property_close(item.property);

    return result;
}

/** Get a rectangle at a frame position.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps the frame rate, which may be needed for converting a time string to frame units
 * \param locale the locale, which may be needed for converting a string to a real number
 * \param position the frame number
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \return the rectangle
 */

mlt_rect mlt_property_anim_get_rect(
    mlt_property self, double fps, mlt_locale_t locale, int position, int length)
{
    mlt_rect result;
    pthread_mutex_lock(&self->mutex);
    if (mlt_property_is_anim(self)) {
        struct mlt_animation_item_s item;
        item.property = mlt_property_init();
        item.property->types = mlt_prop_rect;

        refresh_animation(self, fps, locale, length);
        mlt_animation_get_item(self->animation, &item, position);
        pthread_mutex_unlock(&self->mutex);
        result = mlt_property_get_rect(item.property, locale);

        mlt_property_close(item.property);
    } else {
        pthread_mutex_unlock(&self->mutex);
        result = mlt_property_get_rect(self, locale);
    }
    return result;
}

/** Set a nested properties object.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param properties the properties list to nest into \p self with \p name
 * \return true if error
 */

int mlt_property_set_properties(mlt_property self, mlt_properties properties)
{
    pthread_mutex_lock(&self->mutex);
    clear_property(self);
    self->properties = properties;
    mlt_properties_inc_ref(properties);
    pthread_mutex_unlock(&self->mutex);
    return 0;
}

/** Get a nested properties object.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \return the nested properties list
 */

mlt_properties mlt_property_get_properties(mlt_property self)
{
    mlt_properties properties = NULL;
    pthread_mutex_lock(&self->mutex);
    properties = self->properties;
    pthread_mutex_unlock(&self->mutex);
    return properties;
}

/** Check if a property is animated.
 *
 * This is not a thread-safe function because it is used internally by
 * mlt_property_s under a lock. However, external callers should protect it.
 * \public \memberof mlt_property_s
 * \param self a property
 * \return true if the property is animated
 */

int mlt_property_is_anim(mlt_property self)
{
    return self->animation || (self->prop_string && strchr(self->prop_string, '='));
}

/** Check if a property is a color.

 * \public \memberof mlt_property_s
 * \param self a property
 * \return true if the property is color
 */

extern int mlt_property_is_color(mlt_property self)
{
    int result = 0;
    if (self) {
        pthread_mutex_lock(&self->mutex);
        if (self->types & mlt_prop_color) {
            result = 1;
        } else {
            const char *value = self->prop_string;
            if (value
                && ((strlen(value) > 6 && value[0] == '#')
                    || (strlen(value) > 7 && value[0] == '0' && value[1] == 'x'))) {
                result = 1;
            }
        }
        pthread_mutex_unlock(&self->mutex);
    }
    return result;
}

/** Check if a property is a rect.

 * \public \memberof mlt_property_s
 * \param self a property
 * \return true if the property is a rect
 */

extern int mlt_property_is_rect(mlt_property self)
{
    int result = 0;
    if (self) {
        result = self->types & mlt_prop_rect;
    }
    return result;
}
