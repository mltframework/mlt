/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2023 Meltytech, LLC
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

#include <float.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>

#include <framework/mlt.h>

extern mlt_consumer consumer_avformat_init(mlt_profile profile, char *file);
extern mlt_filter filter_avcolour_space_init(void *arg);
extern mlt_filter filter_avdeinterlace_init(void *arg);
extern mlt_filter filter_swresample_init(mlt_profile profile, char *arg);
extern mlt_filter filter_swscale_init(mlt_profile profile, char *arg);
extern mlt_producer producer_avformat_init(mlt_profile profile, const char *service, char *file);
extern mlt_filter filter_avfilter_init(mlt_profile, mlt_service_type, const char *, char *);
extern mlt_link link_avdeinterlace_init(mlt_profile, mlt_service_type, const char *, char *);
extern mlt_link link_swresample_init(mlt_profile profile, mlt_service_type, const char *, char *);

// ffmpeg Header files
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#ifdef AVDEVICE
#include <libavdevice/avdevice.h>
#endif
#ifdef AVFILTER
#include <libavfilter/avfilter.h>
#endif
#include <libavutil/opt.h>

// A static flag used to determine if avformat has been initialised
static int avformat_initialised = 0;

static void avformat_init()
{
    // Initialise avformat if necessary
    if (avformat_initialised == 0) {
        avformat_initialised = 1;
#ifdef AVDEVICE
        avdevice_register_all();
#endif
        avformat_network_init();
        av_log_set_level(mlt_log_get_level());
        if (getenv("MLT_AVFORMAT_PRODUCER_CACHE")) {
            int n = atoi(getenv("MLT_AVFORMAT_PRODUCER_CACHE"));
            mlt_service_cache_set_size(NULL, "producer_avformat", n);
        }
    }
}

static void *create_service(mlt_profile profile, mlt_service_type type, const char *id, void *arg)
{
    avformat_init();
#ifdef CODECS
    if (!strncmp(id, "avformat", 8)) {
        if (type == mlt_service_producer_type)
            return producer_avformat_init(profile, id, arg);
        else if (type == mlt_service_consumer_type)
            return consumer_avformat_init(profile, arg);
    }
#endif
#ifdef FILTERS
    if (!strcmp(id, "avcolor_space"))
        return filter_avcolour_space_init(arg);
    if (!strcmp(id, "avcolour_space"))
        return filter_avcolour_space_init(arg);
    if (!strcmp(id, "avdeinterlace")) {
        if (type == mlt_service_filter_type)
            return filter_avdeinterlace_init(arg);
        else if (type == mlt_service_link_type)
            return link_avdeinterlace_init(profile, type, id, arg);
    }
    if (!strcmp(id, "swscale"))
        return filter_swscale_init(profile, arg);
#endif
#ifdef SWRESAMPLE
    if (!strcmp(id, "swresample")) {
        if (type == mlt_service_filter_type)
            return filter_swresample_init(profile, arg);
        else if (type == mlt_service_link_type)
            return link_swresample_init(profile, type, id, arg);
    }
#endif
    return NULL;
}

static void add_parameters(mlt_properties params,
                           const void *object,
                           int req_flags,
                           const char *unit,
                           const char *subclass,
                           const char *id_prefix)
{
    const AVOption *opt = NULL;

    // For each AVOption on the AVClass object
    while ((opt = av_opt_next(object, opt))) {
        // If matches flags and not a binary option (not supported by Mlt)
        if (!(opt->flags & req_flags) || (opt->type == AV_OPT_TYPE_BINARY))
            continue;

        // Ignore constants (keyword values)
        if (!unit && opt->type == AV_OPT_TYPE_CONST)
            continue;
        // When processing a groups of options (unit)...
        // ...ignore non-constants
        else if (unit && opt->type != AV_OPT_TYPE_CONST)
            continue;
        // ...ignore constants not in this group
        else if (unit && opt->type == AV_OPT_TYPE_CONST && strcmp(unit, opt->unit))
            continue;
        // ..add constants to the 'values' sequence
        else if (unit && opt->type == AV_OPT_TYPE_CONST) {
            char key[20];
            snprintf(key, 20, "%d", mlt_properties_count(params));
            mlt_properties_set(params, key, opt->name);
            continue;
        }

        // Create a map for this option.
        mlt_properties p = mlt_properties_new();
        char key[20];
        snprintf(key, 20, "%d", mlt_properties_count(params));
        // Add the map to the 'parameters' sequence.
        mlt_properties_set_data(params, key, p, 0, (mlt_destructor) mlt_properties_close, NULL);

        // Add the parameter metadata for this AVOption.
        if (id_prefix) {
            char id[200];
            snprintf(id, sizeof(id), "%s%s", id_prefix, opt->name);
            mlt_properties_set(p, "identifier", id);
        } else {
            mlt_properties_set(p, "identifier", opt->name);
        }
        if (opt->help) {
            if (subclass) {
                char *s = malloc(strlen(opt->help) + strlen(subclass) + 4);
                strcpy(s, opt->help);
                strcat(s, " (");
                strcat(s, subclass);
                strcat(s, ")");
                mlt_properties_set(p, "description", s);
                free(s);
            } else
                mlt_properties_set(p, "description", opt->help);
        }

        switch (opt->type) {
        case AV_OPT_TYPE_FLAGS:
            mlt_properties_set(p, "type", "string");
            mlt_properties_set(p, "format", "flags");
            break;
        case AV_OPT_TYPE_INT:
            if (!opt->unit) {
                mlt_properties_set(p, "type", "integer");
                if (opt->min != INT_MIN)
                    mlt_properties_set_int(p, "minimum", (int) opt->min);
                if (opt->max != INT_MAX)
                    mlt_properties_set_int(p, "maximum", (int) opt->max);
                mlt_properties_set_int(p, "default", (int) opt->default_val.i64);
            } else {
                mlt_properties_set(p, "type", "string");
                mlt_properties_set(p, "format", "integer or keyword");
            }
            break;
        case AV_OPT_TYPE_INT64:
            mlt_properties_set(p, "type", "integer");
            mlt_properties_set(p, "format", "64-bit");
            if (opt->min != INT64_MIN)
                mlt_properties_set_int64(p, "minimum", (int64_t) opt->min);
            if (opt->max != INT64_MAX)
                mlt_properties_set_int64(p, "maximum", (int64_t) opt->max);
            mlt_properties_set_int64(p, "default", (int64_t) opt->default_val.i64);
            break;
        case AV_OPT_TYPE_FLOAT:
            mlt_properties_set(p, "type", "float");
            if (opt->min != FLT_MIN && opt->min != -340282346638528859811704183484516925440.0)
                mlt_properties_set_double(p, "minimum", opt->min);
            if (opt->max != FLT_MAX)
                mlt_properties_set_double(p, "maximum", opt->max);
            mlt_properties_set_double(p, "default", opt->default_val.dbl);
            break;
        case AV_OPT_TYPE_DOUBLE:
            mlt_properties_set(p, "type", "float");
            mlt_properties_set(p, "format", "double");
            if (opt->min != DBL_MIN)
                mlt_properties_set_double(p, "minimum", opt->min);
            if (opt->max != DBL_MAX)
                mlt_properties_set_double(p, "maximum", opt->max);
            mlt_properties_set_double(p, "default", opt->default_val.dbl);
            break;
        case AV_OPT_TYPE_STRING:
            mlt_properties_set(p, "type", "string");
            if (opt->default_val.str) {
                size_t len = strlen(opt->default_val.str) + 3;
                char *quoted = malloc(len);
                snprintf(quoted, len, "'%s'", opt->default_val.str);
                mlt_properties_set(p, "default", quoted);
                free(quoted);
            }
            break;
        case AV_OPT_TYPE_RATIONAL:
            mlt_properties_set(p, "type", "string");
            mlt_properties_set(p, "format", "numerator/denominator");
            break;
        case AV_OPT_TYPE_CONST:
            mlt_properties_set(p, "type", "integer");
            mlt_properties_set(p, "format", "constant");
            break;
        case AV_OPT_TYPE_COLOR:
            mlt_properties_set(p, "type", "color");
            if (opt->default_val.str) {
                size_t len = strlen(opt->default_val.str) + 3;
                char *quoted = malloc(len);
                snprintf(quoted, len, "'%s'", opt->default_val.str);
                mlt_properties_set(p, "default", quoted);
                free(quoted);
            }
        default:
            mlt_properties_set(p, "type", "string");
            break;
        }
        // If the option belongs to a group (unit) and is not a constant (keyword value)
        if (opt->unit && opt->type != AV_OPT_TYPE_CONST) {
            // Create a 'values' sequence.
            mlt_properties values = mlt_properties_new();

            // Recurse to add constants in this group to the 'values' sequence.
            add_parameters(values, object, req_flags, opt->unit, NULL, NULL);
            if (mlt_properties_count(values))
                mlt_properties_set_data(p,
                                        "values",
                                        values,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);
            else
                mlt_properties_close(values);
        }
    }
}

static mlt_properties avformat_metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    const char *service_type = NULL;
    mlt_properties result = NULL;

    // Convert the service type to a string.
    switch (type) {
    case mlt_service_consumer_type:
        service_type = "consumer";
        break;
    case mlt_service_filter_type:
        service_type = "filter";
        break;
    case mlt_service_producer_type:
        service_type = "producer";
        break;
    case mlt_service_transition_type:
        service_type = "transition";
        break;
    default:
        return NULL;
    }

    if (type == mlt_service_producer_type && !strcmp(id, "avformat-novalidate")) {
        id = "avformat";
    }

    // Load the yaml file
    snprintf(file, PATH_MAX, "%s/avformat/%s_%s.yml", mlt_environment("MLT_DATA"), service_type, id);
    result = mlt_properties_parse_yaml(file);
    if (result && (type == mlt_service_consumer_type || type == mlt_service_producer_type)) {
        // Annotate the yaml properties with AVOptions.
        mlt_properties params = (mlt_properties) mlt_properties_get_data(result, "parameters", NULL);
        AVFormatContext *avformat = avformat_alloc_context();
        AVCodecContext *avcodec = avcodec_alloc_context3(NULL);
        int flags = (type == mlt_service_consumer_type) ? AV_OPT_FLAG_ENCODING_PARAM
                                                        : AV_OPT_FLAG_DECODING_PARAM;

        add_parameters(params, avformat, flags, NULL, NULL, NULL);
        avformat_init();
        if (type == mlt_service_producer_type) {
            const AVInputFormat *f = NULL;
            void *iterator = NULL;
            while ((f = av_demuxer_iterate(&iterator)))
                if (f->priv_class)
                    add_parameters(params, &f->priv_class, flags, NULL, f->name, NULL);
        } else {
            const AVOutputFormat *f = NULL;
            void *iterator = NULL;
            while ((f = av_muxer_iterate(&iterator)))
                if (f->priv_class)
                    add_parameters(params, &f->priv_class, flags, NULL, f->name, NULL);
        }

        add_parameters(params, avcodec, flags, NULL, NULL, NULL);
        const AVCodec *c = NULL;
        void *iterator = NULL;
        while ((c = av_codec_iterate(&iterator)))
            if (c->priv_class)
                add_parameters(params, &c->priv_class, flags, NULL, c->name, NULL);

        av_free(avformat);
        av_free(avcodec);
    }
    return result;
}

#ifdef AVFILTER
static mlt_properties avfilter_metadata(mlt_service_type type, const char *id, void *name)
{
    AVFilter *f = (AVFilter *) avfilter_get_by_name(name);
    if (!f)
        return NULL;

    mlt_properties metadata = mlt_properties_new();
    mlt_properties_set_double(metadata, "schema_version", 0.3);
    mlt_properties_set(metadata, "title", f->name);
    mlt_properties_set(metadata, "version", LIBAVFILTER_IDENT);
    mlt_properties_set(metadata, "identifier", id);
    mlt_properties_set(metadata, "description", f->description);
    mlt_properties_set(
        metadata,
        "notes",
        "Many parameters support animated values (keyframes) but only the numeric ones. Many "
        "numeric properties have type string because they accept an expression (see FFmpeg "
        "documentation) even though they evaluate to a numeric value.");
    mlt_properties_set(metadata, "creator", "libavfilter maintainers");
    mlt_properties_set(metadata, "type", "filter");

    mlt_properties tags = mlt_properties_new();
    mlt_properties_set_data(metadata, "tags", tags, 0, (mlt_destructor) mlt_properties_close, NULL);
    if (avfilter_pad_get_type(f->inputs, 0) == AVMEDIA_TYPE_VIDEO) {
        mlt_properties_set(tags, "0", "Video");
    }
    if (avfilter_pad_get_type(f->inputs, 0) == AVMEDIA_TYPE_AUDIO) {
        mlt_properties_set(tags, "0", "Audio");
    }

    if (f->priv_class) {
        mlt_properties params = mlt_properties_new();
        mlt_properties_set_data(metadata,
                                "parameters",
                                params,
                                0,
                                (mlt_destructor) mlt_properties_close,
                                NULL);
        add_parameters(params,
                       &f->priv_class,
                       AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_AUDIO_PARAM
                           | AV_OPT_FLAG_FILTERING_PARAM,
                       NULL,
                       NULL,
                       "av.");

        // Add the parameters common to all avfilters.
        if (f->flags & AVFILTER_FLAG_SLICE_THREADS) {
            mlt_properties p = mlt_properties_new();
            char key[20];
            snprintf(key, 20, "%d", mlt_properties_count(params));
            mlt_properties_set_data(params, key, p, 0, (mlt_destructor) mlt_properties_close, NULL);
            mlt_properties_set(p, "identifier", "av.threads");
            mlt_properties_set(p, "description", "Maximum number of threads");
            mlt_properties_set(p, "type", "integer");
            mlt_properties_set_int(p, "minimum", 0);
            mlt_properties_set_int(p, "default", 0);
        }
        {
            mlt_properties p = mlt_properties_new();
            char key[20];
            int i = 0;
            snprintf(key, 20, "%d", mlt_properties_count(params));
            mlt_properties_set_data(params, key, p, 0, (mlt_destructor) mlt_properties_close, NULL);
            mlt_properties_set(p, "identifier", "position");
            mlt_properties_set(p, "description", "The MLT position value to set on avfilter frames");
            mlt_properties_set(p, "type", "string");
            mlt_properties_set(p, "default", "frame");
            mlt_properties values = mlt_properties_new();
            mlt_properties_set_data(p,
                                    "values",
                                    values,
                                    0,
                                    (mlt_destructor) mlt_properties_close,
                                    NULL);
            snprintf(key, 20, "%d", i++);
            mlt_properties_set(values, key, "frame");
            snprintf(key, 20, "%d", i++);
            mlt_properties_set(values, key, "filter");
            snprintf(key, 20, "%d", i++);
            mlt_properties_set(values, key, "source");
            snprintf(key, 20, "%d", i++);
            mlt_properties_set(values, key, "producer");
        }
    }

    return metadata;
}
#endif

static mlt_properties metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    snprintf(file, PATH_MAX, "%s/avformat/%s", mlt_environment("MLT_DATA"), (char *) data);
    return mlt_properties_parse_yaml(file);
}

MLT_REPOSITORY
{
#ifdef CODECS
    MLT_REGISTER(mlt_service_consumer_type, "avformat", create_service);
    MLT_REGISTER(mlt_service_producer_type, "avformat", create_service);
    MLT_REGISTER(mlt_service_producer_type, "avformat-novalidate", create_service);
    MLT_REGISTER_METADATA(mlt_service_consumer_type, "avformat", avformat_metadata, NULL);
    MLT_REGISTER_METADATA(mlt_service_producer_type, "avformat", avformat_metadata, NULL);
    MLT_REGISTER_METADATA(mlt_service_producer_type,
                          "avformat-novalidate",
                          metadata,
                          "producer_avformat-novalidate.yml");
#endif
#ifdef FILTERS
    MLT_REGISTER(mlt_service_filter_type, "avcolour_space", create_service);
    MLT_REGISTER(mlt_service_filter_type, "avcolor_space", create_service);
    MLT_REGISTER(mlt_service_filter_type, "avdeinterlace", create_service);
    MLT_REGISTER(mlt_service_filter_type, "swscale", create_service);
    MLT_REGISTER(mlt_service_link_type, "avcolour_space", mlt_link_filter_init);
    MLT_REGISTER(mlt_service_link_type, "avcolor_space", mlt_link_filter_init);
    MLT_REGISTER(mlt_service_link_type, "avdeinterlace", create_service);
    MLT_REGISTER(mlt_service_link_type, "swscale", mlt_link_filter_init);
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "avcolour_space",
                          metadata,
                          "filter_avcolour_space.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "avcolor_space",
                          metadata,
                          "filter_avcolour_space.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "avdeinterlace",
                          metadata,
                          "filter_avdeinterlace.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "swscale", metadata, "filter_swscale.yml");
    MLT_REGISTER_METADATA(mlt_service_link_type, "avcolour_space", mlt_link_filter_metadata, NULL);
    MLT_REGISTER_METADATA(mlt_service_link_type, "avcolor_space", mlt_link_filter_metadata, NULL);
    MLT_REGISTER_METADATA(mlt_service_link_type,
                          "avdeinterlace",
                          metadata,
                          "link_avdeinterlace.yml");
    MLT_REGISTER_METADATA(mlt_service_link_type, "swscale", mlt_link_filter_metadata, NULL);

#ifdef AVFILTER
    char dirname[PATH_MAX];
    snprintf(dirname, PATH_MAX, "%s/avformat/blacklist.txt", mlt_environment("MLT_DATA"));
    mlt_properties blacklist = mlt_properties_load(dirname);

    snprintf(dirname, PATH_MAX, "%s/avformat/yuv_only.txt", mlt_environment("MLT_DATA"));
    mlt_properties_set_data(mlt_global_properties(),
                            "avfilter.yuv_only",
                            mlt_properties_load(dirname),
                            0,
                            (mlt_destructor) mlt_properties_close,
                            NULL);

    // Load a list of parameters impacted by consumer scale into global properties.
    snprintf(dirname, PATH_MAX, "%s/avformat/resolution_scale.yml", mlt_environment("MLT_DATA"));
    mlt_properties_set_data(mlt_global_properties(),
                            "avfilter.resolution_scale",
                            mlt_properties_parse_yaml(dirname),
                            0,
                            (mlt_destructor) mlt_properties_close,
                            NULL);

    const AVFilter *f = NULL;
    void *iterator = NULL;
    while ((f = (AVFilter *) av_filter_iterate(&iterator))) {
        // Support filters that have one input and one output of the same type.
#if LIBAVFILTER_VERSION_INT < ((8 << 16) + (3 << 8) + 101)
        if (avfilter_pad_count(f->inputs) == 1 && avfilter_pad_count(f->outputs) == 1 &&
#else
        if (avfilter_filter_pad_count(f, 0) == 1 && avfilter_filter_pad_count(f, 1) == 1 &&
#endif
            avfilter_pad_get_type(f->inputs, 0) == avfilter_pad_get_type(f->outputs, 0)
            && !mlt_properties_get(blacklist, f->name)) {
            char service_name[1024] = "avfilter.";
            strncat(service_name, f->name, sizeof(service_name) - strlen(service_name) - 1);
            MLT_REGISTER(mlt_service_filter_type, service_name, filter_avfilter_init);
            MLT_REGISTER_METADATA(mlt_service_filter_type,
                                  service_name,
                                  avfilter_metadata,
                                  (void *) f->name);
        }
    }
    mlt_properties_close(blacklist);
#endif // AVFILTER
#endif
#ifdef SWRESAMPLE
    MLT_REGISTER(mlt_service_filter_type, "swresample", create_service);
    MLT_REGISTER_METADATA(mlt_service_filter_type, "swresample", metadata, "filter_swresample.yml");
    MLT_REGISTER(mlt_service_link_type, "swresample", create_service);
    MLT_REGISTER_METADATA(mlt_service_link_type, "swresample", metadata, "link_swresample.yml");
#endif
}
