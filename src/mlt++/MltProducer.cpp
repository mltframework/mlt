/**
 * MltProducer.cpp - MLT Wrapper
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

#include "MltProducer.h"
#include "MltConsumer.h"
#include "MltEvent.h"
#include "MltFilter.h"
#include "MltProfile.h"
using namespace Mlt;

Producer::Producer()
    : instance(NULL)
    , parent_(NULL)
{}

Producer::Producer(Profile &profile, const char *id, const char *service)
    : Producer(profile.get_profile(), id, service)
{}

Producer::Producer(mlt_profile profile, const char *id, const char *service)
    : instance(NULL)
    , parent_(NULL)
{
    if (id != NULL && service != NULL)
        instance = mlt_factory_producer(profile, id, service);
    else
        instance = mlt_factory_producer(profile, NULL, id != NULL ? id : service);
}

Producer::Producer(Service &producer)
    : instance(NULL)
    , parent_(NULL)
{
    mlt_service_type type = producer.type();
    if (type == mlt_service_producer_type || type == mlt_service_playlist_type
        || type == mlt_service_tractor_type || type == mlt_service_multitrack_type
        || type == mlt_service_transition_type || type == mlt_service_chain_type
        || type == mlt_service_link_type) {
        instance = (mlt_producer) producer.get_service();
        inc_ref();
    }
}

Producer::Producer(mlt_producer producer)
    : instance(producer)
    , parent_(NULL)
{
    inc_ref();
}

Producer::Producer(Producer &producer)
    : Mlt::Service(producer)
    , instance(producer.get_producer())
    , parent_(NULL)
{
    inc_ref();
}

Producer::Producer(const Producer &producer)
    : Producer(const_cast<Producer &>(producer))
{}

Producer::Producer(Producer *producer)
    : instance(producer != NULL ? producer->get_producer() : NULL)
    , parent_(NULL)
{
    if (is_valid())
        inc_ref();
}

Producer::~Producer()
{
    delete parent_;
    mlt_producer_close(instance);
    instance = NULL;
}

Producer &Producer::operator=(const Producer &producer)
{
    if (this != &producer) {
        delete parent_;
        parent_ = nullptr;
        mlt_producer_close(instance);
        instance = producer.instance;
        inc_ref();
    }
    return *this;
}

mlt_producer Producer::get_producer()
{
    return instance;
}

mlt_producer Producer::get_parent()
{
    return get_producer() != NULL && mlt_producer_cut_parent(get_producer()) != NULL
               ? mlt_producer_cut_parent(get_producer())
               : get_producer();
}

Producer &Producer::parent()
{
    if (is_cut() && parent_ == NULL)
        parent_ = new Producer(get_parent());
    return parent_ == NULL ? *this : *parent_;
}

mlt_service Producer::get_service()
{
    return mlt_producer_service(get_producer());
}

int Producer::seek(int position)
{
    return mlt_producer_seek(get_producer(), position);
}

int Producer::seek(const char *time)
{
    return mlt_producer_seek_time(get_producer(), time);
}

int Producer::position()
{
    return mlt_producer_position(get_producer());
}

int Producer::frame()
{
    return mlt_producer_frame(get_producer());
}

char *Producer::frame_time(mlt_time_format format)
{
    return mlt_producer_frame_time(get_producer(), format);
}

int Producer::set_speed(double speed)
{
    return mlt_producer_set_speed(get_producer(), speed);
}

int Producer::pause()
{
    int result = 0;

    if (get_speed() != 0) {
        Consumer consumer((mlt_consumer) mlt_service_consumer(get_service()));
        Event *event = consumer.setup_wait_for("consumer-sdl-paused");

        result = mlt_producer_set_speed(get_producer(), 0);
        if (result == 0 && consumer.is_valid() && !consumer.is_stopped())
            consumer.wait_for(event);
        delete event;
    }

    return result;
}

double Producer::get_speed()
{
    return mlt_producer_get_speed(get_producer());
}

double Producer::get_fps()
{
    return mlt_producer_get_fps(get_producer());
}

int Producer::set_in_and_out(int in, int out)
{
    return mlt_producer_set_in_and_out(get_producer(), in, out);
}

int Producer::get_in()
{
    return mlt_producer_get_in(get_producer());
}

int Producer::get_out()
{
    return mlt_producer_get_out(get_producer());
}

int Producer::get_length()
{
    return mlt_producer_get_length(get_producer());
}

char *Producer::get_length_time(mlt_time_format format)
{
    return mlt_producer_get_length_time(get_producer(), format);
}

int Producer::get_playtime()
{
    return mlt_producer_get_playtime(get_producer());
}

Producer *Producer::cut(int in, int out)
{
    mlt_producer producer = mlt_producer_cut(get_producer(), in, out);
    Producer *result = new Producer(producer);
    mlt_producer_close(producer);
    return result;
}

bool Producer::is_cut()
{
    return mlt_producer_is_cut(get_producer()) != 0;
}

bool Producer::is_blank()
{
    return mlt_producer_is_blank(get_producer()) != 0;
}

bool Producer::same_clip(Producer &that)
{
    return mlt_producer_cut_parent(get_producer()) == mlt_producer_cut_parent(that.get_producer());
}

bool Producer::runs_into(Producer &that)
{
    return same_clip(that) && get_out() == (that.get_in() - 1);
}

void Producer::optimise()
{
    mlt_producer_optimise(get_producer());
}

int Producer::clear()
{
    return mlt_producer_clear(get_producer());
}

int64_t Producer::get_creation_time()
{
    return mlt_producer_get_creation_time(get_producer());
}

void Producer::set_creation_time(int64_t creation_time)
{
    mlt_producer_set_creation_time(get_producer(), creation_time);
}

int64_t Producer::get_timecode()
{
    int64_t tc = mlt_producer_get_timecode(get_producer());
    return tc;
}

void Producer::set_timecode(int64_t timecode)
{
    mlt_producer_set_timecode(get_producer(), timecode);
}

bool Producer::probe()
{
    return mlt_producer_probe(get_producer());
}
