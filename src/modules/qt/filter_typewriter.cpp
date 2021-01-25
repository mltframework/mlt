/*
 * filter_typewriter.cpp -- typewriter filter
 * Copyright (c) 2021 <rafallalik@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied wrenderedanty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt.h>
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "typewriter.h"

struct producer_ktitle_s
{
    struct mlt_producer_s parent;
    uint8_t *rgba_image;
    uint8_t *current_image;
    uint8_t *current_alpha;
    mlt_image_format format;
    int current_width;
    int current_height;
    int has_alpha;
    pthread_mutex_t mutex;
};

typedef struct producer_ktitle_s *producer_ktitle;

struct FilterContainer{
    XmlParser xp;

    std::vector<TypeWriter> renders;      // rendered data [array]
    bool init;               // 1 if initialized

    int current_frame;      // currently parsed frame

    std::string xml_data;   // data field content (xml data)
    bool is_template;
    int step_length;        // frame step value
    float sigma;            // sigma of fluctuations
    int seed;               // seed for random fluctuations
    int macro;              // macro type: 0 - custom, 1 - char, 2 - word, 3 - line

    int producer_type;      // 1 - kdenlivetitle
    mlt_producer producer;  // hold producer pointer

    FilterContainer() {
        clean();
    }

    void clean()
    {
        renders.clear();
        init = false;
        current_frame = -1;
        xml_data.clear();
        is_template = false;
        step_length = 0;
        sigma = 0;
        seed = 0;
        macro = 0;
        producer_type = 0;
        producer = nullptr;
    }
};

/*
 * Get data for display.
 */
static int get_producer_data(mlt_properties filter_p, mlt_properties frame_p, FilterContainer * cont)
{
    if (cont == nullptr)
        return 0;

    char * d = nullptr;
    int step_length = 0;
    int sigma = 0;
    int seed = 0;
    int macro = 0;

    mlt_producer producer = nullptr;
    mlt_properties producer_properties = nullptr;

    unsigned int update_mask = 0;

    /* Try with kdenlivetitle */
    producer_ktitle kt = static_cast<producer_ktitle>(mlt_properties_get_data( frame_p, "producer_kdenlivetitle", NULL ) );
    if (kt != nullptr)
    {
        /* Obtain properties of producer */
        producer = &kt->parent;
        producer_properties = MLT_PRODUCER_PROPERTIES( producer );

        if (producer == nullptr || producer_properties == nullptr)
            return 0;

        d = mlt_properties_get( producer_properties, "resource" );
        cont->is_template = (d && d[0] != '\0');

        if (cont->is_template)
            d = mlt_properties_get( producer_properties, "_xmldata");
        else
            d = mlt_properties_get( producer_properties, "xmldata" );

        if (d == nullptr)
            return 0;

        step_length = mlt_properties_get_int(filter_p, "step_length");
        sigma = mlt_properties_get_int(filter_p, "step_sigma");
        seed = mlt_properties_get_int(filter_p, "random_seed");
        macro = mlt_properties_get_int(filter_p, "macro_type");

        // if xml data changed, set update mask 0x1
        if (cont->xml_data != d || macro != cont->macro)
            update_mask = 0x3;

        if (step_length != cont->step_length || sigma != cont->sigma || seed != cont->seed)
            update_mask |= 0x2;

        // clear and prepare for new parsing
        if (0 == update_mask)
            return 1;
    }
    else
    {
        return 0;
    }

    if (update_mask & 0x1)
    {
        cont->clean();

        // save new data field name
        cont->xml_data = d;

        // Get content data and backup in the tw container.
        cont->xp.setDocument(d);
        cont->xp.parse();
        unsigned int n = cont->xp.getContentNodesNumber();
        for (uint i = 0; i < n; ++i)
        {
            std::string key = cont->xp.getNodeContent(i).toStdString();
            TypeWriter data;

            if (macro) {
                char * buff = new char[key.length()+5];
                char c = 0;
                switch (macro) {
                    case 1: c = 'c'; break;
                    case 2: c = 'w'; break;
                    case 3: c = 'l'; break;
                    default: break;
                }

                sprintf(buff, ":%c{%s}", c, key.c_str());
                data.setPattern(buff);
                delete [] buff;
            } else {
                data.setPattern(key);
            }
            cont->renders.push_back(data);
        }

        cont->macro = macro;
        cont->producer_type = 1;
        cont->producer = producer;

        // mark as inited
        cont->init = true;
    }

    if (update_mask & 0x2)
    {
        for (auto & render : cont->renders)
        {
            render.setFrameStep(step_length);
            render.setStepSigma(sigma);
            render.setStepSeed(seed);
            render.parse();
        }
        cont->step_length = step_length;
        cont->sigma = sigma;
        cont->seed = seed;
    }

    return 1;
}

static int update_producer(mlt_frame frame, mlt_properties /*frame_p*/, FilterContainer * cont, bool restore)
{
    if (cont->init == false)
        return 0;

    mlt_position pos = mlt_frame_original_position( frame );

    mlt_properties producer_properties = nullptr;
    if (cont->producer_type == 1)
    {
        producer_properties = MLT_PRODUCER_PROPERTIES( cont->producer );
        if (restore)
            mlt_properties_set_int( producer_properties, "force_reload", 0 );
        else
            mlt_properties_set_int( producer_properties, "force_reload", 1 );
    }

    if (producer_properties == nullptr)
        return 0;

    if (restore == true)
    {
        if (cont->is_template)
            mlt_properties_set( producer_properties, "_xmldata", cont->xml_data.c_str() );
        else
            mlt_properties_set( producer_properties, "xmldata", cont->xml_data.c_str() );
        return 1;
    }

    assert((cont->xp.getContentNodesNumber() == cont->renders.size()));
    // render the string and set as a content value
    unsigned int n = cont->xp.getContentNodesNumber();
    for (uint i = 0; i < n; ++i) {
        cont->xp.setNodeContent(i, cont->renders[i].render(pos).c_str());
    }

    // update producer for rest of the frame
    QString dom = cont->xp.getDocument();

    if (cont->is_template)
        mlt_properties_set( producer_properties, "_xmldata", dom.toStdString().c_str() );
    else
        mlt_properties_set( producer_properties, "xmldata", dom.toStdString().c_str() );

    cont->current_frame = pos;

    return 1;
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int /*writable*/ )
{
    int error = 0;
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

    mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

    FilterContainer * cont = (FilterContainer*) filter->child;

    mlt_service_lock(MLT_FILTER_SERVICE(filter));

    int res = get_producer_data(properties, frame_properties, cont);
    if (res == 0)
        return mlt_frame_get_image( frame, image, format, width, height, 1 );

    update_producer(frame, frame_properties, cont, false);

    error = mlt_frame_get_image( frame, image, format, width, height, 1 );

    update_producer(frame, frame_properties, cont, true);

    mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    return error;
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
    mlt_frame_push_service( frame, filter );
    mlt_frame_push_get_image( frame, filter_get_image );
    return frame;
}

static void filter_close( mlt_filter filter)
{
    FilterContainer * cont = (FilterContainer *) filter->child;

    cont->clean();
}

extern "C" {
mlt_filter filter_typewriter_init( mlt_profile /*profile*/, mlt_service_type /*type*/, const char */*id*/, char */*arg*/ )
{
    mlt_filter filter = mlt_filter_new( );
    FilterContainer* cont = new FilterContainer;

    if ( filter != nullptr && cont != nullptr)
    {
        filter->process = filter_process;
        filter->child = cont;
        filter->close = filter_close;
    }

    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_properties_set_int(properties, "step_length", 25 );
    mlt_properties_set_int(properties, "step_sigma", 0);
    mlt_properties_set_int(properties, "random_seed", 0);
    mlt_properties_set_int(properties, "macro_type", 1);

    return filter;
}
}
