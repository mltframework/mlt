/**
 * MltProfile.h - MLT Wrapper
 * Copyright (C) 2008-2020 Meltytech, LLC
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

#ifndef MLTPP_PROFILE_H
#define MLTPP_PROFILE_H

#include "MltConfig.h"

#ifdef SWIG
#define MLTPP_DECLSPEC
#endif

#include <framework/mlt.h>

namespace Mlt {
class Properties;
class Producer;

class MLTPP_DECLSPEC Profile
{
private:
    mlt_profile instance;

public:
    Profile();
    Profile(const char *name);
    Profile(Properties &properties);
    Profile(mlt_profile profile);
    ~Profile();

    bool is_valid() const;
    mlt_profile get_profile() const;
    char *description() const;
    int frame_rate_num() const;
    int frame_rate_den() const;
    double fps() const;
    int width() const;
    int height() const;
    bool progressive() const;
    int sample_aspect_num() const;
    int sample_aspect_den() const;
    double sar() const;
    int display_aspect_num() const;
    int display_aspect_den() const;
    double dar() const;
    int is_explicit() const;
    int colorspace() const;
    static Properties *list();
    void from_producer(Producer &producer);
    void set_width(int width);
    void set_height(int height);
    void set_sample_aspect(int numerator, int denominator);
    void set_display_aspect(int numerator, int denominator);
    void set_progressive(int progressive);
    void set_colorspace(int colorspace);
    void set_frame_rate(int numerator, int denominator);
    void set_explicit(int boolean);
    double scale_width(int width);
    double scale_height(int height);
};
} // namespace Mlt

#endif
