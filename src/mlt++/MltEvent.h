/**
 * MltEvent.h - MLT Wrapper
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

#ifndef MLTPP_EVENT_H
#define MLTPP_EVENT_H

#include "MltConfig.h"

#include <framework/mlt.h>

namespace Mlt {
class Frame;

class MLTPP_DECLSPEC Event
{
private:
    mlt_event instance;

public:
    Event(mlt_event);
    Event(Event &);
    ~Event();
    mlt_event get_event();
    bool is_valid();
    void block();
    void unblock();
};

class MLTPP_DECLSPEC EventData
{
private:
    mlt_event_data instance;

public:
    EventData(mlt_event_data);
    EventData(EventData &);
    EventData(const EventData &);
    EventData &operator=(const EventData &);
    ~EventData(){};
    mlt_event_data get_event_data() const;
    int to_int() const;
    const char *to_string() const;
    Frame to_frame() const;
    void *to_object() const;
};
} // namespace Mlt

#endif
