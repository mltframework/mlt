/*
 * producer_glaxnimate.cpp -- a Glaxnimate/Qt based producer for MLT
 * Copyright (C) 2022 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
 */

#include <framework/mlt.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "io/io_registry.hpp"
using namespace glaxnimate;


class Glaxnimate
{
private:
    mlt_producer m_producer = nullptr;
    std::unique_ptr<model::Document> m_document;

public:
    mlt_profile m_profile = nullptr;

    void setProducer( mlt_producer producer )
        { m_producer = producer; }

    mlt_producer producer() const
        { return m_producer; }

    mlt_service service() const
        { return MLT_PRODUCER_SERVICE(m_producer); }

    mlt_properties properties() const
        { return MLT_PRODUCER_PROPERTIES(m_producer); }

    QSize size() const { return m_document->size(); }
    int duration() const { return qRound(m_document->main()->animation->last_frame.get() - m_document->main()->animation->first_frame.get() + 1.f); }

    Glaxnimate()
    {
    }

    ~Glaxnimate()
    {
    }

    int getImage(mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable)
    {
        int error = 0;
        auto pos = mlt_frame_original_position(frame);
        if (mlt_properties_get(properties(), "eof") && !::strcmp("loop", mlt_properties_get(properties(), "eof"))) {
            pos %= duration();
        }
        auto bg = mlt_properties_get_color(properties(), "background");
        auto background = QColor(bg.r, bg.g, bg.b, bg.a);
        auto image = m_document->render_image(m_document->main()->animation->first_frame.get() + pos, {*width, *height}, background);

        *format = mlt_image_rgba;
        int size = mlt_image_format_size(*format, *width, *height, NULL);
        *buffer = static_cast<uint8_t*>(mlt_pool_alloc(size));
        memcpy(*buffer, image.constBits(), size);
        error = mlt_frame_set_image(frame, *buffer, size, mlt_pool_release);

        return error;
    }

    bool open(const char* fileName)
    {
        auto filename = QString::fromUtf8(fileName);
        auto importer = io::IoRegistry::instance().from_filename(filename, io::ImportExport::Import);
        if (!importer || !importer->can_open()) {
            mlt_log_error(service(), "Unknown importer\n");
            return false;
        }

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            mlt_log_error(service(), "Could not open input file for reading\n");
            return false;
        }

        m_document.reset(new model::Document(filename));
        QVariantMap settings;
        if ( !importer->open(file, filename, m_document.get(), settings) ) {
            mlt_log_error(service(), "Error loading input file\n");
            return false;
        }

        return true;
    }
};

extern "C" {

static int get_image(mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable)
{
    auto producer = static_cast<mlt_producer>(mlt_frame_pop_service(frame));
    auto glax = static_cast<Glaxnimate*>(producer->child);

    if (mlt_properties_get_int(glax->properties(), "refresh")) {
        mlt_properties_clear(glax->properties(), "refresh");
        glax->open(mlt_properties_get(glax->properties(), "resource"));
        if (glax->duration() > mlt_properties_get_int(glax->properties(), "length")) {
            mlt_properties_set_int(glax->properties(), "length", glax->duration());
        }
    }

    return glax->getImage(frame, buffer, format, width, height, writable);
}

static int get_frame(mlt_producer producer, mlt_frame_ptr frame, int index)
{
    *frame = mlt_frame_init(MLT_PRODUCER_SERVICE(producer));
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES( *frame );

    // Set frame properties
    mlt_properties_set_int(frame_properties, "progressive", 1);
    double force_ratio = mlt_properties_get_double(MLT_PRODUCER_PROPERTIES(producer), "force_aspect_ratio");
    if (force_ratio > 0.0)
        mlt_properties_set_double(frame_properties, "aspect_ratio", force_ratio);
    else
        mlt_properties_set_double(frame_properties, "aspect_ratio", 1.0);

    mlt_frame_set_position(*frame, mlt_producer_position(producer));
    mlt_frame_push_service(*frame, producer);
    mlt_frame_push_get_image(*frame, get_image);
    mlt_producer_prepare_next(producer);
    return 0;
}

static void producer_close(mlt_producer producer)
{
    delete static_cast<Glaxnimate*>(producer->child);
    producer->close = nullptr;
    mlt_producer_close(producer);
}

mlt_producer producer_glaxnimate_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    // Allocate the producer
    Glaxnimate* glax = new Glaxnimate();
    mlt_producer producer = (mlt_producer) calloc(1, sizeof( *producer ));

    // If allocated and initializes
    if (glax && !mlt_producer_init(producer, glax) && glax->open(arg)) {
        glax->setProducer(producer);
        producer->close = (mlt_destructor) producer_close;
        producer->get_frame = get_frame;

        auto properties = glax->properties();
        mlt_properties_set(properties, "resource", arg);
        mlt_properties_set(properties, "background", "#00000000");
        mlt_properties_set_int(properties, "aspect_ratio", 1);
        mlt_properties_set_int(properties, "progressive", 1);
        mlt_properties_set_int(properties, "seekable", 1);
        mlt_properties_set_int(properties, "meta.media.width", glax->size().width());
        mlt_properties_set_int(properties, "meta.media.height", glax->size().height());
        mlt_properties_set_int(properties, "meta.media.sample_aspect_num", 1);
        mlt_properties_set_int(properties, "meta.media.sample_aspect_den", 1);
        mlt_properties_set_int(properties, "out", glax->duration() - 1);
        mlt_properties_set_int(properties, "length", glax->duration());
        mlt_properties_set(properties, "eof", "loop");
    }
    return producer;
}

static mlt_properties metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    const char *service_type = NULL;
    switch (type) {
        case mlt_service_producer_type:
            service_type = "producer";
            break;
        default:
            return NULL;
    }
    snprintf(file, PATH_MAX, "%s/glaxnimate/%s_%s.yml", mlt_environment("MLT_DATA"), service_type, id);
    return mlt_properties_parse_yaml(file);
}

MLT_REPOSITORY
{
    MLT_REGISTER(mlt_service_producer_type, "glaxnimate", producer_glaxnimate_init);
    MLT_REGISTER_METADATA(mlt_service_producer_type, "glaxnimate", metadata, NULL);
}

} // extern C
