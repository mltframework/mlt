/**
 * MltLink.cpp - MLT Wrapper
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

#include "MltLink.h"
#include "MltProfile.h"

#include <stdlib.h>
#include <string.h>

using namespace Mlt;

Link::Link()
    : instance(nullptr)
{}

Link::Link(mlt_link link)
    : instance(link)
{
    inc_ref();
}

Link::Link(const char *id, const char *arg)
    : instance(NULL)
{
    if (arg != NULL) {
        instance = mlt_factory_link(id, arg);
    } else {
        if (strchr(id, ':')) {
            char *temp = strdup(id);
            char *arg = strchr(temp, ':') + 1;
            *(arg - 1) = '\0';
            instance = mlt_factory_link(temp, arg);
            free(temp);
        } else {
            instance = mlt_factory_link(id, NULL);
        }
    }
}

Link::Link(Link *link)
    : Link(link ? link->get_link() : nullptr)
{}

Link::Link(Service &link)
    : instance(nullptr)
{
    if (link.type() == mlt_service_link_type) {
        instance = (mlt_link) link.get_service();
        inc_ref();
    }
}

Link::Link(Link &link)
{
    if (link.type() == mlt_service_link_type) {
        instance = (mlt_link) link.get_service();
        inc_ref();
    }
}

Link::Link(const Link &link)
    : Link(const_cast<Link &>(link))
{}

Link &Link::operator=(const Link &link)
{
    if (this != &link) {
        mlt_link_close(instance);
        instance = link.instance;
        inc_ref();
    }
    return *this;
}

Link::~Link()
{
    mlt_link_close(instance);
}

mlt_link Link::get_link()
{
    return instance;
}

mlt_producer Link::get_producer()
{
    return MLT_LINK_PRODUCER(instance);
}

int Link::connect_next(Mlt::Producer &next, Mlt::Profile &default_profile)
{
    return mlt_link_connect_next(instance, next.get_producer(), default_profile.get_profile());
}
