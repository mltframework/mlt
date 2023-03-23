/**
 * MltLink.h - MLT Wrapper
 * Copyright (C) 2020-2021 Meltytech, LLC
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

#ifndef MLTPP_LINK_H
#define MLTPP_LINK_H

#include "MltConfig.h"
#include "MltProducer.h"

#include <framework/mlt.h>

namespace Mlt {
class Producer;
class Profile;

class MLTPP_DECLSPEC Link : public Producer
{
private:
    mlt_link instance;

public:
    Link();
    Link(mlt_link link);
    Link(const char *id, const char *service = NULL);
    Link(Link *link);
    Link(Service &link);
    Link(Link &link);
    Link(const Link &link);
    Link &operator=(const Link &link);
    virtual ~Link();
    virtual mlt_link get_link();
    mlt_producer get_producer() override;
    int connect_next(Mlt::Producer &next, Mlt::Profile &default_profile);
};
} // namespace Mlt

#endif
