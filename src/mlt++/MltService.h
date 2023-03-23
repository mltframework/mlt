/**
 * MltService.h - MLT Wrapper
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

#ifndef MLTPP_SERVICE_H
#define MLTPP_SERVICE_H

#include "MltConfig.h"

#include <framework/mlt.h>

#include "MltFrame.h"
#include "MltProperties.h"

namespace Mlt {
class Properties;
class Filter;
class Frame;
class Profile;

class MLTPP_DECLSPEC Service : public Properties
{
private:
    mlt_service instance;

public:
    Service();
    Service(Service &service);
    Service(const Service &service);
    Service(mlt_service service);
    Service(Service *service);
    virtual ~Service();
    Service &operator=(const Service &service);
    virtual mlt_service get_service();
    void lock();
    void unlock();
    virtual mlt_properties get_properties() override;
    int connect_producer(Service &producer, int index = 0);
    int insert_producer(Service &producer, int index = 0);
    int disconnect_producer(int index = 0);
    int disconnect_all_producers();
    Service *consumer();
    Service *producer();
    Profile *profile();
    mlt_profile get_profile();
    Frame *get_frame(int index = 0);
    mlt_service_type type();
    int attach(Filter &filter);
    int detach(Filter &filter);
    int filter_count();
    int move_filter(int from, int to);
    Filter *filter(int index);
    void set_profile(mlt_profile profile);
    void set_profile(Profile &profile);
};
} // namespace Mlt

#endif
