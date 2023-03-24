/**
 * MltFilter.h - MLT Wrapper
 * Copyright (C) 2004-2021 Meltytech, LLC
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

#ifndef MLTPP_FILTER_H
#define MLTPP_FILTER_H

#include "MltConfig.h"

#include <framework/mlt.h>

#include "MltService.h"

namespace Mlt {
class Service;
class Profile;
class Frame;

class MLTPP_DECLSPEC Filter : public Service
{
private:
    mlt_filter instance;

public:
    Filter();
    Filter(Profile &profile, const char *id, const char *service = NULL);
    Filter(mlt_profile profile, const char *id, const char *service = NULL);
    Filter(Service &filter);
    Filter(Filter &filter);
    Filter(const Filter &filter);
    Filter(mlt_filter filter);
    Filter(Filter *filter);
    virtual ~Filter();
    Filter &operator=(const Filter &filter);
    virtual mlt_filter get_filter();
    mlt_service get_service() override;
    int connect(Service &service, int index = 0);
    void set_in_and_out(int in, int out);
    int get_in();
    int get_out();
    int get_length();
    int get_length2(Frame &frame);
    int get_track();
    int get_position(Frame &frame);
    double get_progress(Frame &frame);
    void process(Frame &frame);
};
} // namespace Mlt

#endif
