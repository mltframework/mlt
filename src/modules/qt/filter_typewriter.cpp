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

#include "kdenlivetitle_wrapper.h"
#include "richtextanimation.h"
#include "typewriter.h"
#include <memory>

struct FilterContainer
{
    XmlParser xp;

    // Rich text: automatic rich titles use Unicode-safe schedules.
    std::vector<std::unique_ptr<RichTextReveal::Schedule>> richSchedules;
    QStringList originalText;
    bool allowRichText{true};
    std::vector<TypeWriter> renders; // rendered data [array]
    bool init;                       // 1 if initialized

    int current_frame; // currently parsed frame

    std::string xml_data; // data field content (xml data)
    bool is_template;
    int step_length; // frame step value
    float sigma;     // sigma of fluctuations
    int seed;        // seed for random fluctuations
    int macro;       // macro type: 0 - custom, 1 - char, 2 - word, 3 - line

    int producer_type;     // 1 - kdenlivetitle
    mlt_producer producer; // hold producer pointer

    FilterContainer() { clean(); }

    void clean()
    {
        renders.clear();
        richSchedules.clear();
        originalText.clear();
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
static int get_producer_data(mlt_properties filter_p, mlt_properties frame_p, FilterContainer *cont)
{
    if (cont == nullptr)
        return 0;

    const char *d = nullptr;
    std::string sourceXml;
    bool allowRichText = true;
    int step_length = 0;
    int sigma = 0;
    int seed = 0;
    int macro = 0;

    mlt_producer producer = nullptr;
    mlt_properties producer_properties = nullptr;

    unsigned int update_mask = 0;

    /* Try with kdenlivetitle */
    producer_ktitle kt = static_cast<producer_ktitle>(
        mlt_properties_get_data(frame_p, "producer_kdenlivetitle", NULL));
    if (kt != nullptr) {
        /* Obtain properties of producer */
        producer = &kt->parent;
        producer_properties = MLT_PRODUCER_PROPERTIES(producer);

        if (producer == nullptr || producer_properties == nullptr)
            return 0;

        // Copy while holding the producer lock, then RELEASE it before
        // requesting the downstream image (which takes that lock itself).
        mlt_service_lock(MLT_PRODUCER_SERVICE(producer));
        const char *resource = mlt_properties_get(producer_properties, "resource");
        cont->is_template = resource && resource[0] != '\0';
        d = mlt_properties_get(producer_properties, cont->is_template ? "_xmldata" : "xmldata");
        if (d)
            sourceXml = d;
        const char *replacement = mlt_properties_get(producer_properties, "templatetext");
        allowRichText = !replacement || replacement[0] == '\0';
        mlt_service_unlock(MLT_PRODUCER_SERVICE(producer));
        if (sourceXml.empty())
            return 0;
        d = sourceXml.c_str();

        step_length = mlt_properties_get_int(filter_p, "step_length");
        sigma = mlt_properties_get_int(filter_p, "step_sigma");
        seed = mlt_properties_get_int(filter_p, "random_seed");
        macro = mlt_properties_get_int(filter_p, "macro_type");

        // if xml data changed, set update mask 0x1
        if (cont->xml_data != d || macro != cont->macro || allowRichText != cont->allowRichText)
            update_mask = 0x3;

        if (step_length != cont->step_length || sigma != cont->sigma || seed != cont->seed)
            update_mask |= 0x2;

        // clear and prepare for new parsing
        if (0 == update_mask)
            return 1;
    } else {
        return 0;
    }

    if (update_mask & 0x1) {
        const bool isTemplate = cont->is_template;
        cont->clean();
        cont->is_template = isTemplate;
        cont->allowRichText = allowRichText;

        // save new data field name
        cont->xml_data = d;

        // Get content data and backup in the tw container.
        cont->xp.setDocument(d);
        cont->xp.parse();
        unsigned int n = cont->xp.getContentNodesNumber();
        for (uint i = 0; i < n; ++i) {
            std::string key = cont->xp.getNodeContent(i).toStdString();
            TypeWriter data;
            cont->originalText.append(QString::fromStdString(key));
            const bool rich = allowRichText && macro >= 1 && macro <= 3 && cont->xp.hasRichText(i);
            cont->richSchedules.emplace_back(rich ? new RichTextReveal::Schedule() : nullptr);

            if (rich) {
                // No macro parsing: braces, slashes and punctuation are literal.
            } else if (macro) {
                char *buff = new char[key.length() + 5];
                char c = 0;
                switch (macro) {
                case 1:
                    c = 'c';
                    break;
                case 2:
                    c = 'w';
                    break;
                case 3:
                    c = 'l';
                    break;
                default:
                    break;
                }

                sprintf(buff, ":%c{%s}", c, key.c_str());
                data.setPattern(buff);
                delete[] buff;
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

    if (update_mask & 0x2) {
        for (size_t i = 0; i < cont->renders.size(); ++i) {
            if (cont->richSchedules[i]) {
                cont->richSchedules[i]->reset(cont->originalText.at(int(i)),
                                              std::max(1, step_length),
                                              macro,
                                              std::max(0, sigma),
                                              unsigned(seed));
                continue;
            }
            auto &render = cont->renders[i];
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

static int update_frame(mlt_frame frame, FilterContainer *cont)
{
    if (!cont->init)
        return 0;
    const mlt_position pos = mlt_frame_original_position(frame);
    const unsigned int n = cont->xp.getContentNodesNumber();
    assert(n == cont->renders.size());
    for (uint i = 0; i < n; ++i) {
        if (cont->richSchedules[i]) {
            const auto &schedule = *cont->richSchedules[i];
            cont->xp.setNodeContent(i, schedule.text().left(schedule.visible(pos)), true);
        } else {
            cont->xp.setNodeContent(i, cont->renders[i].render(pos).c_str());
        }
    }
    const QByteArray xml = cont->xp.getDocument().toUtf8();
    mlt_properties_set(MLT_FRAME_PROPERTIES(frame),
                       "_kdenlivetitle_typewriter_xml",
                       xml.constData());
    cont->current_frame = pos;
    return 1;
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int /*writable*/)
{
    int error = 0;
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);

    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

    FilterContainer *cont = (FilterContainer *) filter->child;

    mlt_service_lock(MLT_FILTER_SERVICE(filter));

    int res = get_producer_data(properties, frame_properties, cont);
    if (res != 0)
        update_frame(frame, cont);
    mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    // Rendering reads its override from this frame, never shared source XML.
    error = mlt_frame_get_image(frame, image, format, width, height, 1);

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

static void close_container(void *data)
{
    delete static_cast<FilterContainer *>(data);
}

extern "C" {
mlt_filter filter_typewriter_init(mlt_profile /*profile*/,
                                  mlt_service_type /*type*/,
                                  const char * /*id*/,
                                  char * /*arg*/)
{
    mlt_filter filter = mlt_filter_new();
    if (!filter)
        return nullptr;
    FilterContainer *cont = new FilterContainer;
    filter->process = filter_process;
    filter->child = cont;

    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_properties_set_data(properties, "_typewriter_container", cont, 0, close_container, nullptr);
    mlt_properties_set_int(properties, "step_length", 25);
    mlt_properties_set_int(properties, "step_sigma", 0);
    mlt_properties_set_int(properties, "random_seed", 0);
    mlt_properties_set_int(properties, "macro_type", 1);

    return filter;
}
}
