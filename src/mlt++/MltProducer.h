/**
 * MltProducer.h - MLT Wrapper
 * Copyright (C) 2004-2019 Meltytech, LLC
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

#ifndef MLTPP_PRODUCER_H
#define MLTPP_PRODUCER_H

#include "MltConfig.h"

#include <framework/mlt.h>

#include "MltService.h"

namespace Mlt {
class Service;
class Filter;
class Profile;
class Frame;

class MLTPP_DECLSPEC Producer : public Service
{
private:
    mlt_producer instance;
    Producer *parent_;

public:
    Producer();
    Producer(Profile &profile, const char *id, const char *service = NULL);
    Producer(mlt_profile profile, const char *id, const char *service = NULL);
    Producer(Service &producer);
    Producer(mlt_producer producer);
    Producer(Producer &producer);
    Producer(const Producer &producer);
    Producer(Producer *producer);
    virtual ~Producer();
    Producer &operator=(const Producer &producer);
    virtual mlt_producer get_producer();
    Producer &parent();
    mlt_producer get_parent();
    mlt_service get_service() override;
    int seek(int position);
    int seek(const char *time);
    int position();
    int frame();
    char *frame_time(mlt_time_format = mlt_time_smpte_df);
    int set_speed(double speed);
    int pause();
    double get_speed();
    double get_fps();
    int set_in_and_out(int in, int out);
    int get_in();
    int get_out();
    int get_length();
    char *get_length_time(mlt_time_format = mlt_time_smpte_df);
    int get_playtime();
    Producer *cut(int in = 0, int out = -1);
    bool is_cut();
    bool is_blank();
    bool same_clip(Producer &that);
    bool runs_into(Producer &that);
    void optimise();
    int clear();
    int64_t get_creation_time();
    void set_creation_time(int64_t creation_time);
    bool probe();
};
} // namespace Mlt

#endif
