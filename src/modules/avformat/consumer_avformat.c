/*
 * consumer_avformat.c -- an encoder based on avformat
 * Copyright (C) 2003-2025 Meltytech, LLC
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

#include "common.h"

// mlt Header files
#include <framework/mlt_consumer.h>
#include <framework/mlt_events.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_profile.h>

// System header files
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// avformat header files
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
#include <libavutil/version.h>
#include <libswscale/swscale.h>

#define MAX_AUDIO_STREAMS (8)
#define MAX_SUBTITLE_STREAMS (8)
#define AUDIO_ENCODE_BUFFER_SIZE (48000 * 2 * MAX_AUDIO_STREAMS)
#define AUDIO_BUFFER_SIZE (1024 * 42)
#define VIDEO_BUFFER_SIZE (8192 * 8192)
#define IMAGE_ALIGN (4)

//
// This structure should be extended and made globally available in mlt
//

typedef struct
{
    uint8_t *buffer;
    int size;
    int used;
    double time;
    int frequency;
    int channels;
} * sample_fifo, sample_fifo_s;

sample_fifo sample_fifo_init(int frequency, int channels)
{
    sample_fifo fifo = calloc(1, sizeof(sample_fifo_s));
    fifo->frequency = frequency;
    fifo->channels = channels;
    return fifo;
}

// count is the number of samples multiplied by the number of bytes per sample
void sample_fifo_append(sample_fifo fifo, uint8_t *samples, int count)
{
    if ((fifo->size - fifo->used) < count) {
        fifo->size += count * 5;
        fifo->buffer = realloc(fifo->buffer, fifo->size);
    }

    memcpy(&fifo->buffer[fifo->used], samples, count);
    fifo->used += count;
}

int sample_fifo_used(sample_fifo fifo)
{
    return fifo->used;
}

int sample_fifo_fetch(sample_fifo fifo, uint8_t *samples, int count)
{
    if (count > fifo->used)
        count = fifo->used;

    memcpy(samples, fifo->buffer, count);
    fifo->used -= count;
    memmove(fifo->buffer, &fifo->buffer[count], fifo->used);

    fifo->time += (double) count / fifo->channels / fifo->frequency;

    return count;
}

void sample_fifo_close(sample_fifo fifo)
{
    free(fifo->buffer);
    free(fifo);
}

static AVFilterGraph *vfilter_graph;

static int setup_hwupload_filter(mlt_properties properties,
                                 AVStream *stream,
                                 AVCodecContext *codec_context)
{
    AVFilterContext *vfilter_in;
    AVFilterContext *vfilter_out;
    AVFilterContext *vfilter_hwupload;

    vfilter_graph = avfilter_graph_alloc();
    mlt_properties_set_data(properties,
                            "vfilter_graph",
                            &vfilter_graph,
                            0,
                            (mlt_destructor) avfilter_graph_free,
                            NULL);

    // From ffplay.c:configure_video_filters().
    char buffersrc_args[256];
    snprintf(buffersrc_args,
             sizeof(buffersrc_args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d:frame_rate=%d/%d",
             codec_context->width,
             codec_context->height,
             AV_PIX_FMT_NV12,
             stream->time_base.num,
             stream->time_base.den,
             codec_context->sample_aspect_ratio.num,
             codec_context->sample_aspect_ratio.den,
             codec_context->time_base.den,
             codec_context->time_base.num);

    int result = avfilter_graph_create_filter(&vfilter_in,
                                              avfilter_get_by_name("buffer"),
                                              "mlt_buffer",
                                              buffersrc_args,
                                              NULL,
                                              vfilter_graph);
    if (result < 0) {
        mlt_log_error(MLT_CONSUMER(properties),
                      "avfilter_graph_create_filter(buffer) failed with %d\n",
                      result);
        return result;
    }

    vfilter_out = avfilter_graph_alloc_filter(vfilter_graph,
                                              avfilter_get_by_name("buffersink"),
                                              "mlt_buffersink");
    if (!vfilter_out) {
        mlt_log_error(MLT_CONSUMER(properties), "avfilter_graph_alloc_filter(buffersink) failed\n");
        return result;
    }

    enum AVPixelFormat pix_fmts[] = {codec_context->pix_fmt, AV_PIX_FMT_NONE};
#if LIBAVFILTER_VERSION_INT >= ((10 << 16) + (6 << 8) + 100)
    result = av_opt_set_array(vfilter_out,
                              "pixel_formats",
                              AV_OPT_SEARCH_CHILDREN,
                              0,
                              1,
                              AV_OPT_TYPE_PIXEL_FMT,
                              pix_fmts);
#else
    result = av_opt_set_int_list(vfilter_out,
                                 "pix_fmts",
                                 pix_fmts,
                                 AV_PIX_FMT_NONE,
                                 AV_OPT_SEARCH_CHILDREN);
#endif
    if (result < 0) {
        mlt_log_error(MLT_CONSUMER(properties), "av_opt_set_array() failed with %d\n", result);
        return result;
    }

    result = avfilter_init_dict(vfilter_out, NULL);
    if (result < 0) {
        mlt_log_error(MLT_CONSUMER(properties), "avfilter_init_dict() failed with %d\n", result);
        return result;
    }

    vfilter_hwupload = avfilter_graph_alloc_filter(vfilter_graph,
                                                   avfilter_get_by_name("hwupload"),
                                                   "mlt_hwupload");
    if (!vfilter_hwupload) {
        mlt_log_error(MLT_CONSUMER(properties), "avfilter_graph_alloc_filter(hwupload) failed\n");
        return result;
    }

    vfilter_hwupload->hw_device_ctx = av_buffer_ref(codec_context->hw_device_ctx);
    result = avfilter_init_dict(vfilter_hwupload, NULL);
    if (result < 0) {
        mlt_log_error(MLT_CONSUMER(properties),
                      "avfilter_graph_create_filter(hwupload) failed with %d\n",
                      result);
        return result;
    }

    result = avfilter_link(vfilter_in, 0, vfilter_hwupload, 0);
    if (result < 0) {
        mlt_log_error(MLT_CONSUMER(properties),
                      "avfilter_link(vfilter_in, vfilter_hwupload) failed with %d\n",
                      result);
        return result;
    }

    result = avfilter_link(vfilter_hwupload, 0, vfilter_out, 0);
    if (result < 0) {
        mlt_log_error(MLT_CONSUMER(properties),
                      "avfilter_link(vfilter_hwupload, vfilter_out) failed with %d\n",
                      result);
        return result;
    }

    result = avfilter_graph_config(vfilter_graph, NULL);
    if (result < 0) {
        mlt_log_error(MLT_CONSUMER(properties), "avfilter_graph_config() failed with %d\n", result);
        return result;
    }

    codec_context->hw_frames_ctx = av_buffer_ref(av_buffersink_get_hw_frames_ctx(vfilter_out));
    mlt_properties_set_data(properties, "vfilter_in", vfilter_in, 0, NULL, NULL);
    mlt_properties_set_data(properties, "vfilter_out", vfilter_out, 0, NULL, NULL);

    return result;
}

static AVBufferRef *hw_device_ctx;

static int init_vaapi(mlt_properties properties, AVCodecContext *codec_context)
{
    int err = 0;
    const char *vaapi_device = mlt_properties_get(properties, "vaapi_device");
    AVDictionary *opts = NULL;

    av_dict_set(&opts, "connection_type", mlt_properties_get(properties, "connection_type"), 0);
    av_dict_set(&opts, "driver", mlt_properties_get(properties, "driver"), 0);
    av_dict_set(&opts, "kernel_driver", mlt_properties_get(properties, "kernel_driver"), 0);

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_VAAPI, vaapi_device, opts, 0))
        < 0) {
        mlt_log_warning(NULL, "Failed to create VAAPI device.\n");
    } else {
        codec_context->hw_device_ctx = av_buffer_ref(hw_device_ctx);
        mlt_properties_set_data(properties,
                                "hw_device_ctx",
                                &hw_device_ctx,
                                0,
                                (mlt_destructor) av_buffer_unref,
                                NULL);
    }
    av_dict_free(&opts);
    return err;
}

// Forward references.
static void property_changed(mlt_properties owner, mlt_consumer self, mlt_event_data);
static int consumer_start(mlt_consumer consumer);
static int consumer_stop(mlt_consumer consumer);
static int consumer_is_stopped(mlt_consumer consumer);
static void *consumer_thread(void *arg);
static void consumer_close(mlt_consumer consumer);

/** Initialise the consumer.
*/

mlt_consumer consumer_avformat_init(mlt_profile profile, char *arg)
{
    // Allocate the consumer
    mlt_consumer consumer = mlt_consumer_new(profile);

    // If memory allocated and initialises without error
    if (consumer != NULL) {
        // Get properties from the consumer
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);

        // Assign close callback
        consumer->close = consumer_close;

        // Interpret the argument
        if (arg != NULL)
            mlt_properties_set(properties, "target", arg);

        // sample and frame queue
        mlt_properties_set_data(properties,
                                "frame_queue",
                                mlt_deque_init(),
                                0,
                                (mlt_destructor) mlt_deque_close,
                                NULL);

        // Audio options not fully handled by AVOptions
#define QSCALE_NONE (-99999)
        mlt_properties_set_int(properties, "aq", QSCALE_NONE);

        // Video options not fully handled by AVOptions
        mlt_properties_set_int(properties, "dc", 8);

        // Muxer options not fully handled by AVOptions
        mlt_properties_set_double(properties, "muxdelay", 0.7);
        mlt_properties_set_double(properties, "muxpreload", 0.5);

        // Ensure termination at end of the stream
        mlt_properties_set_int(properties, "terminate_on_pause", 1);

        // Default to separate processing threads for producer and consumer with no frame dropping!
        mlt_properties_set_int(properties, "real_time", -1);
        mlt_properties_set_int(properties, "prefill", 1);

        // Set up start/stop/terminated callbacks
        consumer->start = consumer_start;
        consumer->stop = consumer_stop;
        consumer->is_stopped = consumer_is_stopped;

        mlt_events_register(properties, "consumer-fatal-error");
        mlt_event event = mlt_events_listen(properties,
                                            consumer,
                                            "property-changed",
                                            (mlt_listener) property_changed);
        mlt_properties_set_data(properties, "property-changed event", event, 0, NULL, NULL);
    }

    // Return consumer
    return consumer;
}

static void recompute_aspect_ratio(mlt_properties properties)
{
    double ar = mlt_properties_get_double(properties, "aspect");
    if (ar > 0.0) {
        AVRational rational = av_d2q(ar, 255);
        int width = mlt_properties_get_int(properties, "width");
        int height = mlt_properties_get_int(properties, "height");

        // Update the profile and properties as well since this is an alias
        // for mlt properties that correspond to profile settings
        mlt_properties_set_int(properties, "display_aspect_num", rational.num);
        mlt_properties_set_int(properties, "display_aspect_den", rational.den);

        // Now compute the sample aspect ratio
        rational = av_d2q(ar * height / FFMAX(width, 1), 255);

        // Update the profile and properties as well since this is an alias
        // for mlt properties that correspond to profile settings
        mlt_properties_set_int(properties, "sample_aspect_num", rational.num);
        mlt_properties_set_int(properties, "sample_aspect_den", rational.den);
    }
}

static void color_trc_from_colorspace(mlt_properties properties)
{
    // Default color transfer characteristic from MLT colorspace.
    const char *colorspace_str = mlt_properties_get(properties, "colorspace");
    mlt_colorspace colorspace = mlt_image_colorspace_id(colorspace_str);
    switch (colorspace) {
    case mlt_colorspace_bt709:
        mlt_properties_set_int(properties, "color_trc", mlt_color_trc_bt709);
        break;
    case mlt_colorspace_bt470bg:
        mlt_properties_set_int(properties, "color_trc", mlt_color_trc_gamma28);
        break;
    case mlt_colorspace_smpte240m:
        mlt_properties_set_int(properties, "color_trc", mlt_color_trc_smpte240m);
        break;
    case mlt_colorspace_rgb: // sRGB
        mlt_properties_set_int(properties, "color_trc", mlt_color_trc_iec61966_2_1);
        break;
    case mlt_colorspace_bt601:
    case mlt_colorspace_smpte170m:
        mlt_properties_set_int(properties, "color_trc", mlt_color_trc_smpte170m);
        break;
    case mlt_colorspace_bt2020_ncl:
        mlt_properties_set_int(properties, "color_trc", mlt_color_trc_bt2020_10);
        break;
    default:
        break;
    }
}

static void color_primaries_from_colorspace(mlt_properties properties)
{
    // Default color_primaries from MLT colorspace.
    const char *colorspace_str = mlt_properties_get(properties, "colorspace");
    mlt_colorspace colorspace = mlt_image_colorspace_id(colorspace_str);
    int height = mlt_properties_get_int(properties, "height");
    mlt_color_primaries primaries = mlt_color_primaries_from_colorspace(colorspace, height);
    if (primaries != mlt_color_pri_none)
        mlt_properties_set_int(properties, "color_primaries", primaries);
}

static void property_changed(mlt_properties owner, mlt_consumer self, mlt_event_data event_data)
{
    mlt_properties properties = MLT_CONSUMER_PROPERTIES(self);
    const char *name = mlt_event_data_to_string(event_data);

    if (name && !strcmp(name, "s")) {
        // Obtain the size property
        char *size = mlt_properties_get(properties, "s");
        int width = mlt_properties_get_int(properties, "width");
        int height = mlt_properties_get_int(properties, "height");
        int tw, th;

        if (sscanf(size, "%dx%d", &tw, &th) == 2 && tw > 0 && th > 0) {
            width = tw;
            height = th;
        } else {
            mlt_log_warning(MLT_CONSUMER_SERVICE(self),
                            "Invalid size property %s - ignoring.\n",
                            size);
        }

        // Now ensure we honour the multiple of two requested by libavformat
        width = (width / 2) * 2;
        height = (height / 2) * 2;
        mlt_properties_set_int(properties, "width", width);
        mlt_properties_set_int(properties, "height", height);
        recompute_aspect_ratio(properties);
    }
    // "-aspect" on ffmpeg command line is display aspect ratio
    else if (!strcmp(name, "aspect") || !strcmp(name, "width") || !strcmp(name, "height")) {
        recompute_aspect_ratio(properties);
    }
    // Handle the ffmpeg command line "-r" property for frame rate
    else if (!strcmp(name, "r")) {
        double frame_rate = mlt_properties_get_double(properties, "r");
        AVRational rational = av_d2q(frame_rate, 255);
        mlt_properties_set_int(properties, "frame_rate_num", rational.num);
        mlt_properties_set_int(properties, "frame_rate_den", rational.den);
    }
    // Apply AVOptions that are synonyms for standard mlt_consumer options
    else if (!strcmp(name, "ac")) {
        mlt_properties_set_int(properties, "channels", mlt_properties_get_int(properties, "ac"));
    } else if (!strcmp(name, "ar")) {
        mlt_properties_set_int(properties, "frequency", mlt_properties_get_int(properties, "ar"));
    }
}

/** Start the consumer.
*/

static int consumer_start(mlt_consumer consumer)
{
    // Get the properties
    mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
    int error = 0;

    // Report information about available muxers and codecs as YAML Tiny
    char *s = mlt_properties_get(properties, "f");
    if (s && strcmp(s, "list") == 0) {
        mlt_properties doc = mlt_properties_new();
        mlt_properties formats = mlt_properties_new();
        char key[20];
        const AVOutputFormat *format = NULL;
        void *iterator = NULL;

        mlt_properties_set_data(properties,
                                "f",
                                formats,
                                0,
                                (mlt_destructor) mlt_properties_close,
                                NULL);
        mlt_properties_set_data(doc, "formats", formats, 0, NULL, NULL);
        while ((format = av_muxer_iterate(&iterator))) {
            snprintf(key, sizeof(key), "%d", mlt_properties_count(formats));
            mlt_properties_set(formats, key, format->name);
        }
        s = mlt_properties_serialise_yaml(doc);
        fprintf(stdout, "%s", s);
        free(s);
        mlt_properties_close(doc);
        error = 1;
    }
    s = mlt_properties_get(properties, "acodec");
    if (s && strcmp(s, "list") == 0) {
        mlt_properties doc = mlt_properties_new();
        mlt_properties codecs = mlt_properties_new();
        char key[20];
        const AVCodec *codec = NULL;

        mlt_properties_set_data(properties,
                                "acodec",
                                codecs,
                                0,
                                (mlt_destructor) mlt_properties_close,
                                NULL);
        mlt_properties_set_data(doc, "audio_codecs", codecs, 0, NULL, NULL);
        void *iterator = NULL;
        while ((codec = av_codec_iterate(&iterator)))
            if (av_codec_is_encoder(codec) && codec->type == AVMEDIA_TYPE_AUDIO) {
                snprintf(key, sizeof(key), "%d", mlt_properties_count(codecs));
                mlt_properties_set(codecs, key, codec->name);
            }
        s = mlt_properties_serialise_yaml(doc);
        fprintf(stdout, "%s", s);
        free(s);
        mlt_properties_close(doc);
        error = 1;
    }
    s = mlt_properties_get(properties, "vcodec");
    if (s && strcmp(s, "list") == 0) {
        mlt_properties doc = mlt_properties_new();
        mlt_properties codecs = mlt_properties_new();
        char key[20];
        const AVCodec *codec = NULL;
        void *iterator = NULL;

        mlt_properties_set_data(properties,
                                "vcodec",
                                codecs,
                                0,
                                (mlt_destructor) mlt_properties_close,
                                NULL);
        mlt_properties_set_data(doc, "video_codecs", codecs, 0, NULL, NULL);
        while ((codec = av_codec_iterate(&iterator)))
            if (av_codec_is_encoder(codec) && codec->type == AVMEDIA_TYPE_VIDEO) {
                snprintf(key, sizeof(key), "%d", mlt_properties_count(codecs));
                mlt_properties_set(codecs, key, codec->name);
            }
        s = mlt_properties_serialise_yaml(doc);
        fprintf(stdout, "%s", s);
        free(s);
        mlt_properties_close(doc);
        error = 1;
    }

    // Check that we're not already running
    if (!error && !mlt_properties_get_int(properties, "running")) {
        // Allocate a thread
        pthread_t *thread = calloc(1, sizeof(pthread_t));

        mlt_event_block(mlt_properties_get_data(properties, "property-changed event", NULL));

        // Because Movit only reads this on the first frame,
        // we must do this after properties have been set but before first frame request.
        if (!mlt_properties_get(properties, "color_trc"))
            color_trc_from_colorspace(properties);
        if (!mlt_properties_get(properties, "color_primaries"))
            color_primaries_from_colorspace(properties);

        // Assign the thread to properties
        mlt_properties_set_data(properties, "thread", thread, sizeof(pthread_t), free, NULL);

        // Create the thread
        pthread_create(thread, NULL, consumer_thread, consumer);

        // Set the running state
        mlt_properties_set_int(properties, "running", 1);
    }
    return error;
}

/** Stop the consumer.
*/

static int consumer_stop(mlt_consumer consumer)
{
    // Get the properties
    mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
    pthread_t *thread = mlt_properties_get_data(properties, "thread", NULL);

    // Check that we're running
    if (thread) {
        // Stop the thread
        mlt_properties_set_int(properties, "running", 0);

        // Wait for termination
        pthread_join(*thread, NULL);

        mlt_properties_set_data(properties, "thread", NULL, 0, NULL, NULL);
        mlt_event_unblock(mlt_properties_get_data(properties, "property-changed event", NULL));
    }

    return 0;
}

/** Determine if the consumer is stopped.
*/

static int consumer_is_stopped(mlt_consumer consumer)
{
    // Get the properties
    mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
    return !mlt_properties_get_int(properties, "running");
}

/** Process properties as AVOptions and apply to AV context obj
*/

static void apply_properties(void *obj, mlt_properties properties, int flags)
{
    int i;
    int count = mlt_properties_count(properties);

    for (i = 0; i < count; i++) {
        const char *opt_name = mlt_properties_get_name(properties, i);
        int search_flags = AV_OPT_SEARCH_CHILDREN;
        const AVOption *opt = av_opt_find(obj, opt_name, NULL, flags, search_flags);

        // If option not found, see if it was prefixed with a or v (-vb)
        if (!opt
            && ((opt_name[0] == 'v' && (flags & AV_OPT_FLAG_VIDEO_PARAM))
                || (opt_name[0] == 'a' && (flags & AV_OPT_FLAG_AUDIO_PARAM))))
            opt = av_opt_find(obj, ++opt_name, NULL, flags, search_flags);
        // Apply option if found
        if (opt && strcmp(opt_name, "channel_layout"))
            av_opt_set(obj, opt_name, mlt_properties_get_value(properties, i), search_flags);
    }
}

static enum AVPixelFormat pick_pix_fmt(mlt_image_format img_fmt)
{
    switch (img_fmt) {
    case mlt_image_rgb:
        return AV_PIX_FMT_RGB24;
    case mlt_image_rgba:
        return AV_PIX_FMT_RGBA;
    case mlt_image_yuv420p:
        return AV_PIX_FMT_YUV420P;
    case mlt_image_yuv422p16:
        return AV_PIX_FMT_YUV422P16LE;
    case mlt_image_yuv420p10:
        return AV_PIX_FMT_YUV420P10LE;
    case mlt_image_yuv444p10:
        return AV_PIX_FMT_YUV444P10LE;
    case mlt_image_rgba64:
        return AV_PIX_FMT_RGBA64LE;
    default:
        return AV_PIX_FMT_YUYV422;
    }
}

static int get_mlt_audio_format(int av_sample_fmt)
{
    switch (av_sample_fmt) {
    case AV_SAMPLE_FMT_U8:
        return mlt_audio_u8;
    case AV_SAMPLE_FMT_S32:
        return mlt_audio_s32le;
    case AV_SAMPLE_FMT_FLT:
        return mlt_audio_f32le;
    case AV_SAMPLE_FMT_U8P:
        return mlt_audio_u8;
    case AV_SAMPLE_FMT_S32P:
        return mlt_audio_s32le;
    case AV_SAMPLE_FMT_FLTP:
        return mlt_audio_f32le;
    default:
        return mlt_audio_s16;
    }
}

static int pick_sample_fmt(mlt_properties properties, const AVCodec *codec)
{
    int sample_fmt = AV_SAMPLE_FMT_S16;
    const char *format = mlt_properties_get(properties, "mlt_audio_format");
    const int *p = codec->sample_fmts;
    const char *sample_fmt_str = mlt_properties_get(properties, "sample_fmt");

    if (sample_fmt_str)
        sample_fmt = av_get_sample_fmt(sample_fmt_str);

    // get default av_sample_fmt from mlt_audio_format
    if (format && (!sample_fmt_str || sample_fmt == AV_SAMPLE_FMT_NONE)) {
        if (!strcmp(format, "s32le"))
            sample_fmt = AV_SAMPLE_FMT_S32;
        else if (!strcmp(format, "f32le"))
            sample_fmt = AV_SAMPLE_FMT_FLT;
        else if (!strcmp(format, "u8"))
            sample_fmt = AV_SAMPLE_FMT_U8;
        else if (!strcmp(format, "s32"))
            sample_fmt = AV_SAMPLE_FMT_S32P;
        else if (!strcmp(format, "float"))
            sample_fmt = AV_SAMPLE_FMT_FLTP;
    }
    // check if codec supports our mlt_audio_format
    for (; *p != -1; p++) {
        if (*p == sample_fmt)
            return sample_fmt;
    }
    // no match - pick first one we support
    for (p = codec->sample_fmts; *p != -1; p++) {
        switch (*p) {
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_U8P:
        case AV_SAMPLE_FMT_S16P:
        case AV_SAMPLE_FMT_S32P:
        case AV_SAMPLE_FMT_FLTP:
            return *p;
        default:
            break;
        }
    }
    mlt_log_error(properties, "audio codec sample_fmt not compatible");

    return AV_SAMPLE_FMT_NONE;
}

static uint8_t *interleaved_to_planar(int samples,
                                      int channels,
                                      uint8_t *audio,
                                      int bytes_per_sample)
{
    uint8_t *buffer = mlt_pool_alloc(AUDIO_ENCODE_BUFFER_SIZE);
    uint8_t *p = buffer;
    int c;

    memset(buffer, 0, AUDIO_ENCODE_BUFFER_SIZE);
    for (c = 0; c < channels; c++) {
        uint8_t *q = audio + c * bytes_per_sample;
        int i = samples + 1;
        while (--i) {
            memcpy(p, q, bytes_per_sample);
            p += bytes_per_sample;
            q += channels * bytes_per_sample;
        }
    }
    return buffer;
}

/** Add an audio output stream
*/

static AVStream *add_audio_stream(mlt_consumer consumer,
                                  AVFormatContext *oc,
                                  const AVCodec *codec,
                                  AVCodecContext **codec_context,
                                  int channels,
#if HAVE_FFMPEG_CH_LAYOUT
                                  AVChannelLayout *channel_layout
#else
                                  int64_t channel_layout
#endif
)
{
    // Get the properties
    mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);

    // Create a new stream
    AVStream *st = avformat_new_stream(oc, codec);

    // If created, then initialise from properties
    if (st != NULL) {
        AVCodecContext *c = *codec_context = avcodec_alloc_context3(codec);
        if (!c) {
            mlt_log_fatal(MLT_CONSUMER_SERVICE(consumer),
                          "Failed to allocate the audio encoder context\n");
            return NULL;
        }

        c->codec_id = codec->id;
        c->codec_type = AVMEDIA_TYPE_AUDIO;
        c->sample_fmt = pick_sample_fmt(properties, codec);
#if HAVE_FFMPEG_CH_LAYOUT
        av_channel_layout_copy(&c->ch_layout, channel_layout);
#else
        c->channel_layout = channel_layout;
#endif

// disabled until some audio codecs are multi-threaded
#if 0
      // Setup multi-threading
		int thread_count = mlt_properties_get_int( properties, "threads" );
		if ( thread_count == 0 && getenv( "MLT_AVFORMAT_THREADS" ) )
			thread_count = atoi( getenv( "MLT_AVFORMAT_THREADS" ) );
		if ( thread_count >= 0 )
			c->thread_count = thread_count;
#endif

        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        // Allow the user to override the audio fourcc
        if (mlt_properties_get(properties, "atag")) {
            char *tail = NULL;
            char *arg = mlt_properties_get(properties, "atag");
            int tag = strtol(arg, &tail, 0);
            if (!tail || *tail)
                tag = arg[0] + (arg[1] << 8) + (arg[2] << 16) + (arg[3] << 24);
            c->codec_tag = tag;
        }

        // Process properties as AVOptions
        char *apre = mlt_properties_get(properties, "apre");
        if (apre) {
            mlt_properties p = mlt_properties_load(apre);
            apply_properties(c, p, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM);
            mlt_properties_close(p);
        }
        apply_properties(c, properties, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM);

        int audio_qscale = mlt_properties_get_int(properties, "aq");
        if (audio_qscale > QSCALE_NONE) {
            c->flags |= AV_CODEC_FLAG_QSCALE;
            c->global_quality = FF_QP2LAMBDA * audio_qscale;
        }

        // Set parameters controlled by MLT
        c->sample_rate = mlt_properties_get_int(properties, "frequency");
        st->time_base = (AVRational){1, c->sample_rate};
#if HAVE_FFMPEG_CH_LAYOUT
        c->ch_layout.nb_channels = channels;
#else
        c->channels = channels;
#endif

        if (mlt_properties_get(properties, "alang") != NULL) {
            av_dict_set(&oc->metadata, "language", mlt_properties_get(properties, "alang"), 0);
            av_dict_set(&st->metadata, "language", mlt_properties_get(properties, "alang"), 0);
        }
    } else {
        mlt_log_error(MLT_CONSUMER_SERVICE(consumer), "Could not allocate a stream for audio\n");
    }

    return st;
}

static int open_audio(mlt_properties properties,
                      AVFormatContext *oc,
                      AVStream *st,
                      const AVCodec *codec,
                      AVCodecContext *c)
{
    // We will return the audio input size from here
    int audio_input_frame_size = 0;

    // Process properties as AVOptions on the AVCodec
    if (codec && codec->priv_class) {
        char *apre = mlt_properties_get(properties, "apre");
        if (apre) {
            mlt_properties p = mlt_properties_load(apre);
            apply_properties(c->priv_data, p, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM);
            mlt_properties_close(p);
        }
        apply_properties(c->priv_data,
                         properties,
                         AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM);
    }

    // Continue if codec found and we can open it
    if (codec && avcodec_open2(c, codec, NULL) >= 0) {
#if LIBAVFORMAT_VERSION_INT < ((58 << 16) + (76 << 8) + 100)
        if (avcodec_copy_context(st->codec, c) < 0) {
            mlt_log_warning(NULL, "Failed to copy encoder parameters to output audio stream\n");
        }
#endif

        if (avcodec_parameters_from_context(st->codecpar, c) < 0) {
            mlt_log_warning(NULL, "Failed to copy encoder parameters to output audio stream\n");
            return 0;
        }

        // ugly hack for PCM codecs (will be removed ASAP with new PCM
        // support to compute the input frame size in samples
        if (c->frame_size <= 1)
            audio_input_frame_size = 1;
        else
            audio_input_frame_size = c->frame_size;

        // Some formats want stream headers to be separate (hmm)
        if (!strcmp(oc->oformat->name, "mp4") || !strcmp(oc->oformat->name, "mov")
            || !strcmp(oc->oformat->name, "3gp"))
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    } else {
        mlt_log_warning(NULL, "%s: Unable to encode audio - disabling audio output.\n", __FILE__);
        audio_input_frame_size = 0;
    }

    return audio_input_frame_size;
}

/** Add a video output stream 
*/

static AVStream *add_video_stream(mlt_consumer consumer,
                                  AVFormatContext *oc,
                                  const AVCodec *codec,
                                  AVCodecContext **codec_context)
{
    // Get the properties
    mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);

    // Create a new stream
    AVStream *st = avformat_new_stream(oc, codec);

    if (st != NULL) {
        char *pix_fmt = mlt_properties_get(properties, "pix_fmt");

        AVCodecContext *c = *codec_context = avcodec_alloc_context3(codec);
        if (!c) {
            mlt_log_fatal(MLT_CONSUMER_SERVICE(consumer),
                          "Failed to allocate the video encoder context\n");
            return NULL;
        }

        c->codec_id = codec->id;
        c->codec_type = AVMEDIA_TYPE_VIDEO;

        // Setup multi-threading
        int thread_count = mlt_properties_get_int(properties, "threads");
        if (thread_count == 0 && getenv("MLT_AVFORMAT_THREADS"))
            thread_count = atoi(getenv("MLT_AVFORMAT_THREADS"));
        if (thread_count >= 0)
            c->thread_count = thread_count;

        // Process properties as AVOptions
        char *vpre = mlt_properties_get(properties, "vpre");
        if (vpre) {
            mlt_properties p = mlt_properties_load(vpre);
#ifdef AVDATADIR
            if (mlt_properties_count(p) < 1) {
                const AVCodec *codec = avcodec_find_encoder(c->codec_id);
                if (codec) {
                    char *path = malloc(strlen(AVDATADIR) + strlen(codec->name) + strlen(vpre)
                                        + strlen(".ffpreset") + 2);
                    strcpy(path, AVDATADIR);
                    strcat(path, codec->name);
                    strcat(path, "-");
                    strcat(path, vpre);
                    strcat(path, ".ffpreset");

                    mlt_properties_close(p);
                    p = mlt_properties_load(path);
                    if (mlt_properties_count(p) > 0)
                        mlt_properties_debug(p, path, stderr);
                    free(path);
                }
            } else {
                mlt_properties_debug(p, vpre, stderr);
            }
#endif
            apply_properties(c, p, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM);
            mlt_properties_close(p);
        }
        // Temporarily convert the mlt colorspace to av colorspace before applying the properties
        const char *colorspace_str = mlt_properties_get(properties, "colorspace");
        mlt_colorspace colorspace = mlt_image_colorspace_id(colorspace_str);
        mlt_properties_clear(properties, "colorspace");
        int av_colorspace = mlt_to_av_colorspace(colorspace,
                                                 mlt_properties_get_int(properties, "height"));
        apply_properties(c, properties, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM);
        mlt_properties_set_int(properties, "colorspace", colorspace);

        // Set options controlled by MLT
        c->width = mlt_properties_get_int(properties, "width");
        c->height = mlt_properties_get_int(properties, "height");
        c->time_base.num = mlt_properties_get_int(properties, "frame_rate_den");
        c->time_base.den = mlt_properties_get_int(properties, "frame_rate_num");
        st->time_base = c->time_base;
        st->avg_frame_rate = av_inv_q(c->time_base);
        c->framerate = av_inv_q(c->time_base);

        // Default to the codec's first pix_fmt if possible.
        c->pix_fmt = pix_fmt ? av_get_pix_fmt(pix_fmt)
                     : codec ? (codec->pix_fmts ? codec->pix_fmts[0] : AV_PIX_FMT_YUV422P)
                             : AV_PIX_FMT_YUV420P;

        if (AV_PIX_FMT_VAAPI == c->pix_fmt) {
            int result = init_vaapi(properties, c);
            if (result >= 0) {
                int result = setup_hwupload_filter(properties, st, c);
                if (result < 0)
                    mlt_log_error(MLT_CONSUMER_SERVICE(consumer),
                                  "Failed to setup hwfilter: %d\n",
                                  result);
            } else {
                mlt_log_error(MLT_CONSUMER_SERVICE(consumer),
                              "Failed to initialize VA-API: %d\n",
                              result);
            }
        }

        c->colorspace = av_colorspace;

        if (mlt_properties_get(properties, "aspect")) {
            // "-aspect" on ffmpeg command line is display aspect ratio
            double ar = mlt_properties_get_double(properties, "aspect");
            c->sample_aspect_ratio = av_d2q(ar * c->height / c->width, 255);
        } else {
            c->sample_aspect_ratio.num = mlt_properties_get_int(properties, "sample_aspect_num");
            c->sample_aspect_ratio.den = mlt_properties_get_int(properties, "sample_aspect_den");
        }
        st->sample_aspect_ratio = c->sample_aspect_ratio;

        if (mlt_properties_get_double(properties, "qscale") > 0) {
            c->flags |= AV_CODEC_FLAG_QSCALE;
            c->global_quality = FF_QP2LAMBDA * mlt_properties_get_double(properties, "qscale");
        }

        // Allow the user to override the video fourcc
        if (mlt_properties_get(properties, "vtag")) {
            char *tail = NULL;
            const char *arg = mlt_properties_get(properties, "vtag");
            int tag = strtol(arg, &tail, 0);
            if (!tail || *tail)
                tag = arg[0] + (arg[1] << 8) + (arg[2] << 16) + (arg[3] << 24);
            c->codec_tag = tag;
        }

        // Some formats want stream headers to be separate
        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        // Translate these standard mlt consumer properties to ffmpeg
        if (mlt_properties_get_int(properties, "progressive") == 0
            && mlt_properties_get_int(properties, "deinterlace") == 0) {
            if (!mlt_properties_get(properties, "ildct")
                || mlt_properties_get_int(properties, "ildct"))
                c->flags |= AV_CODEC_FLAG_INTERLACED_DCT;
            if (!mlt_properties_get(properties, "ilme")
                || mlt_properties_get_int(properties, "ilme"))
                c->flags |= AV_CODEC_FLAG_INTERLACED_ME;
        }

        // parse the ratecontrol override string
        int i;
        char *rc_override = mlt_properties_get(properties, "rc_override");
        for (i = 0; rc_override; i++) {
            int start, end, q;
            int e = sscanf(rc_override, "%d,%d,%d", &start, &end, &q);
            if (e != 3)
                mlt_log_warning(MLT_CONSUMER_SERVICE(consumer), "Error parsing rc_override\n");
            c->rc_override = av_realloc(c->rc_override, sizeof(RcOverride) * (i + 1));
            c->rc_override[i].start_frame = start;
            c->rc_override[i].end_frame = end;
            if (q > 0) {
                c->rc_override[i].qscale = q;
                c->rc_override[i].quality_factor = 1.0;
            } else {
                c->rc_override[i].qscale = 0;
                c->rc_override[i].quality_factor = -q / 100.0;
            }
            rc_override = strchr(rc_override, '/');
            if (rc_override)
                rc_override++;
        }
        c->rc_override_count = i;
        if (!c->rc_initial_buffer_occupancy)
            c->rc_initial_buffer_occupancy = c->rc_buffer_size * 3 / 4;
        c->intra_dc_precision = mlt_properties_get_int(properties, "dc") - 8;

        // Setup dual-pass
        i = mlt_properties_get_int(properties, "pass");
        if (i == 1)
            c->flags |= AV_CODEC_FLAG_PASS1;
        else if (i == 2)
            c->flags |= AV_CODEC_FLAG_PASS2;
#ifdef AV_CODEC_ID_H265
        if (codec->id != AV_CODEC_ID_H265)
#endif
            if (codec->id != AV_CODEC_ID_H264
                && (c->flags & (AV_CODEC_FLAG_PASS1 | AV_CODEC_FLAG_PASS2))) {
                FILE *f;
                int size;
                char *logbuffer;
                char *filename;

                if (mlt_properties_get(properties, "passlogfile")) {
                    filename = mlt_properties_get(properties, "passlogfile");
                } else {
                    char logfilename[1024];
                    snprintf(logfilename,
                             sizeof(logfilename),
                             "%s_2pass.log",
                             mlt_properties_get(properties, "target"));
                    mlt_properties_set(properties, "_logfilename", logfilename);
                    filename = mlt_properties_get(properties, "_logfilename");
                }
                if (c->flags & AV_CODEC_FLAG_PASS1) {
                    f = mlt_fopen(filename, "w");
                    if (!f)
                        perror(filename);
                    else
                        mlt_properties_set_data(properties,
                                                "_logfile",
                                                f,
                                                0,
                                                (mlt_destructor) fclose,
                                                NULL);
                } else {
                    /* read the log file */
                    f = mlt_fopen(filename, "r");
                    if (!f) {
                        perror(filename);
                    } else {
                        fseek(f, 0, SEEK_END);
                        size = ftell(f);
                        fseek(f, 0, SEEK_SET);
                        logbuffer = av_malloc(size + 1);
                        if (!logbuffer)
                            mlt_log_fatal(MLT_CONSUMER_SERVICE(consumer),
                                          "Could not allocate log buffer\n");
                        else {
                            if (size >= 0) {
                                size = fread(logbuffer, 1, size, f);
                                logbuffer[size] = '\0';
                                c->stats_in = logbuffer;
                            }
                        }
                        fclose(f);
                    }
                }
            }
    } else {
        mlt_log_error(MLT_CONSUMER_SERVICE(consumer), "Could not allocate a stream for video\n");
    }

    return st;
}

static AVFrame *alloc_picture(int pix_fmt, int width, int height)
{
    // Allocate a frame
    AVFrame *picture = av_frame_alloc();

    if (picture) {
        int size
            = av_image_alloc(picture->data, picture->linesize, width, height, pix_fmt, IMAGE_ALIGN);
        if (size > 0) {
            picture->format = pix_fmt;
            picture->width = width;
            picture->height = height;
        } else {
            av_free(picture);
            picture = NULL;
        }
    } else {
        // Something failed - clean up what we can
        av_free(picture);
        picture = NULL;
    }

    return picture;
}

static int open_video(mlt_properties properties,
                      AVFormatContext *oc,
                      AVStream *st,
                      const AVCodec *codec,
                      AVCodecContext *video_enc)
{
    // Process properties as AVOptions on the AVCodec
    if (codec && codec->priv_class) {
        char *vpre = mlt_properties_get(properties, "vpre");
        if (vpre) {
            mlt_properties p = mlt_properties_load(vpre);
            apply_properties(video_enc->priv_data,
                             p,
                             AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM);
            mlt_properties_close(p);
        }
        apply_properties(video_enc->priv_data,
                         properties,
                         AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM);
    }

    if (codec && codec->pix_fmts) {
        const enum AVPixelFormat *p = codec->pix_fmts;
        for (; *p != -1; p++) {
            if (*p == video_enc->pix_fmt)
                break;
        }
        if (*p == -1)
            video_enc->pix_fmt = codec->pix_fmts[0];
    }

    const AVPixFmtDescriptor *srcDesc = av_pix_fmt_desc_get(video_enc->pix_fmt);
    if (srcDesc->flags & AV_PIX_FMT_FLAG_RGB) {
        video_enc->colorspace = AVCOL_SPC_RGB;
    }

    int result = codec && avcodec_open2(video_enc, codec, NULL) >= 0;

    if (result >= 0) {
#if LIBAVFORMAT_VERSION_INT < ((58 << 16) + (76 << 8) + 100)
        result = avcodec_copy_context(st->codec, video_enc);
        if (!result) {
            mlt_log_warning(NULL, "Failed to copy encoder parameters to output video stream\n");
        }
#endif
        result = avcodec_parameters_from_context(st->codecpar, video_enc) >= 0;
        if (!result) {
            mlt_log_warning(NULL, "Failed to copy encoder parameters to output video stream\n");
        }
    } else {
        mlt_log_warning(NULL, "%s: Unable to encode video - disabling video output.\n", __FILE__);
    }

    return result;
}

static inline long time_difference(struct timeval *time1)
{
    struct timeval time2;
    gettimeofday(&time2, NULL);
    return time2.tv_sec * 1000000 + time2.tv_usec - time1->tv_sec * 1000000 - time1->tv_usec;
}

typedef struct
{
    uint8_t *data;
    size_t size;
} buffer_t;

#if LIBAVFORMAT_VERSION_MAJOR >= 61
static int mlt_write(void *h, const uint8_t *buf, int size)
#else
static int mlt_write(void *h, uint8_t *buf, int size)
#endif
{
    mlt_properties properties = (mlt_properties) h;
    buffer_t buffer = {(uint8_t *) buf, size};
    mlt_events_fire(properties, "avformat-write", mlt_event_data_from_object(&buffer));
    return 0;
}

typedef struct encode_ctx_desc
{
    mlt_consumer consumer;
    int audio_outbuf_size;
    int audio_input_frame_size;
    uint8_t audio_outbuf[AUDIO_BUFFER_SIZE], audio_buf_1[AUDIO_ENCODE_BUFFER_SIZE],
        audio_buf_2[AUDIO_ENCODE_BUFFER_SIZE];

    int channels;
    int total_channels;
    int frequency;
    int sample_bytes;

    sample_fifo fifo;

    AVFormatContext *oc;
    AVStream *video_st;
    AVCodecContext *vcodec_ctx;
    AVStream *audio_st[MAX_AUDIO_STREAMS];
    AVCodecContext *acodec_ctx[MAX_AUDIO_STREAMS];
    int64_t sample_count[MAX_AUDIO_STREAMS];

    // Used to store and override codec ids
    int video_codec_id;
    int audio_codec_id;

    int error_count;
    int frame_count;

    double audio_pts;
    double video_pts;

    int terminate_on_pause;
    int terminated;
    mlt_properties properties;
    mlt_properties frame_meta_properties;

    AVFrame *audio_avframe;

    mlt_position subtitle_prev_start[MAX_SUBTITLE_STREAMS];
    AVStream *subtitle_st[MAX_SUBTITLE_STREAMS];
    AVCodecContext *sdec_ctx[MAX_AUDIO_STREAMS];
    AVCodecContext *senc_ctx[MAX_AUDIO_STREAMS];
} encode_ctx_t;

static int encode_audio(encode_ctx_t *ctx)
{
    char key[27];
    int i, j = 0, samples = ctx->audio_input_frame_size;

    int frame_length = ctx->audio_input_frame_size * ctx->channels * ctx->sample_bytes;

    // Get samples count to fetch from fifo
    if (sample_fifo_used(ctx->fifo) < frame_length) {
        samples = sample_fifo_used(ctx->fifo) / (ctx->channels * ctx->sample_bytes);
    } else if (ctx->audio_input_frame_size == 1) {
        // PCM consumes as much as possible.
        samples = FFMIN(sample_fifo_used(ctx->fifo), AUDIO_ENCODE_BUFFER_SIZE) / frame_length;
    }

    // Get the audio samples
    if (samples > 0) {
        sample_fifo_fetch(ctx->fifo, ctx->audio_buf_1, samples * ctx->sample_bytes * ctx->channels);
    } else if (ctx->audio_codec_id == AV_CODEC_ID_VORBIS && ctx->terminated) {
        // This prevents an infinite loop when some versions of vorbis do not
        // increment pts when encoding silence.
        ctx->audio_pts = ctx->video_pts;
        return 1;
    } else {
        memset(ctx->audio_buf_1, 0, AUDIO_ENCODE_BUFFER_SIZE);
    }

    // For each output stream
    for (i = 0; i < MAX_AUDIO_STREAMS && ctx->audio_st[i] && j < ctx->total_channels; i++) {
        AVStream *stream = ctx->audio_st[i];
        AVCodecContext *codec = ctx->acodec_ctx[i];
        AVPacket pkt;

        av_init_packet(&pkt);
        pkt.data = ctx->audio_outbuf;
        pkt.size = ctx->audio_outbuf_size;

        // Optimized for single track and no channel remap
        if (!ctx->audio_st[1] && !mlt_properties_count(ctx->frame_meta_properties)) {
            void *p = ctx->audio_buf_1;
            if (codec->sample_fmt == AV_SAMPLE_FMT_FLTP)
                p = interleaved_to_planar(samples, ctx->channels, p, sizeof(float));
            else if (codec->sample_fmt == AV_SAMPLE_FMT_S16P)
                p = interleaved_to_planar(samples, ctx->channels, p, sizeof(int16_t));
            else if (codec->sample_fmt == AV_SAMPLE_FMT_S32P)
                p = interleaved_to_planar(samples, ctx->channels, p, sizeof(int32_t));
            else if (codec->sample_fmt == AV_SAMPLE_FMT_U8P)
                p = interleaved_to_planar(samples, ctx->channels, p, sizeof(uint8_t));
            ctx->audio_avframe->nb_samples = FFMAX(samples, ctx->audio_input_frame_size);
            ctx->audio_avframe->pts = ctx->sample_count[i];
            ctx->sample_count[i] += ctx->audio_avframe->nb_samples;
            avcodec_fill_audio_frame(ctx->audio_avframe,
#if HAVE_FFMPEG_CH_LAYOUT
                                     codec->ch_layout.nb_channels,
#else
                                     codec->channels,
#endif
                                     codec->sample_fmt,
                                     (const uint8_t *) p,
                                     AUDIO_ENCODE_BUFFER_SIZE,
                                     0);
            int ret = avcodec_send_frame(codec, samples ? ctx->audio_avframe : NULL);
            if (ret < 0) {
                pkt.size = ret;
            } else {
                ret = avcodec_receive_packet(codec, &pkt);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    pkt.size = 0;
                else if (ret < 0)
                    pkt.size = ret;
            }
            if (p != ctx->audio_buf_1)
                mlt_pool_release(p);
        } else {
            // Extract the audio channels according to channel mapping
            int dest_offset = 0; // channel offset into interleaved dest buffer

            // Get the number of channels for this stream
            sprintf(key, "channels.%d", i);
            int current_channels = mlt_properties_get_int(ctx->properties, key);

            // Clear the destination audio buffer.
            memset(ctx->audio_buf_2, 0, AUDIO_ENCODE_BUFFER_SIZE);

            // For each output channel
            while (dest_offset < current_channels && j < ctx->total_channels) {
                int map_start = -1, map_channels = 0;
                int source_offset = 0;
                int k;

                // Look for a mapping that starts at j
                for (k = 0; k < (MAX_AUDIO_STREAMS * 2) && map_start != j; k++) {
                    sprintf(key, "%d.channels", k);
                    map_channels = mlt_properties_get_int(ctx->frame_meta_properties, key);
                    sprintf(key, "%d.start", k);
                    if (mlt_properties_get(ctx->frame_meta_properties, key))
                        map_start = mlt_properties_get_int(ctx->frame_meta_properties, key);
                    if (map_start != j)
                        source_offset += map_channels;
                }

                // If no mapping
                if (map_start != j) {
                    map_channels = current_channels;
                    source_offset = j;
                }

                // Copy samples if source offset valid
                if (source_offset < ctx->channels) {
                    // Interleave the audio buffer with the # channels for this stream/mapping.
                    for (k = 0; k < map_channels; k++, j++, source_offset++, dest_offset++) {
                        uint8_t *src = ctx->audio_buf_1 + source_offset * ctx->sample_bytes;
                        uint8_t *dest = ctx->audio_buf_2 + dest_offset * ctx->sample_bytes;
                        int s = samples + 1;

                        while (--s) {
                            memcpy(dest, src, ctx->sample_bytes);
                            dest += current_channels * ctx->sample_bytes;
                            src += ctx->channels * ctx->sample_bytes;
                        }
                    }
                }
                // Otherwise silence
                else {
                    j += current_channels;
                    dest_offset += current_channels;
                }
            }
            ctx->audio_avframe->nb_samples = FFMAX(samples, ctx->audio_input_frame_size);
            ctx->audio_avframe->pts = ctx->sample_count[i];
            ctx->sample_count[i] += ctx->audio_avframe->nb_samples;
            avcodec_fill_audio_frame(ctx->audio_avframe,
#if HAVE_FFMPEG_CH_LAYOUT
                                     codec->ch_layout.nb_channels,
#else
                                     codec->channels,
#endif
                                     codec->sample_fmt,
                                     (const uint8_t *) ctx->audio_buf_2,
                                     AUDIO_ENCODE_BUFFER_SIZE,
                                     0);
            int ret = avcodec_send_frame(codec, samples ? ctx->audio_avframe : NULL);
            if (ret < 0) {
                pkt.size = ret;
            } else {
            receive_audio_packet:
                ret = avcodec_receive_packet(codec, &pkt);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    pkt.size = 0;
                else if (ret < 0)
                    pkt.size = ret;
            }
        }

        if (pkt.size > 0) {
            // Write the compressed frame in the media file
            av_packet_rescale_ts(&pkt, codec->time_base, stream->time_base);
            pkt.stream_index = stream->index;
            if (av_interleaved_write_frame(ctx->oc, &pkt)) {
                mlt_log_fatal(MLT_CONSUMER_SERVICE(ctx->consumer), "error writing audio frame\n");
                mlt_events_fire(ctx->properties, "consumer-fatal-error", mlt_event_data_none());
                return -1;
            }
            ctx->error_count = 0;
            mlt_log_debug(MLT_CONSUMER_SERVICE(ctx->consumer),
                          "audio stream %d pkt pts %" PRId64 " frame_size %d\n",
                          stream->index,
                          pkt.pts,
                          codec->frame_size);

            goto receive_audio_packet;
        } else if (pkt.size < 0) {
            mlt_log_warning(MLT_CONSUMER_SERVICE(ctx->consumer),
                            "error with audio encode: %d (frame %d)\n",
                            pkt.size,
                            ctx->frame_count);
            if (++ctx->error_count > 2)
                return -1;
        } else if (!samples) // flushing
        {
            pkt.stream_index = stream->index;
            av_packet_rescale_ts(&pkt, codec->time_base, stream->time_base);
            av_interleaved_write_frame(ctx->oc, &pkt);
        }

        if (i == 0) {
            ctx->audio_pts = (double) ctx->sample_count[0] * av_q2d(codec->time_base);
        }
    }

    return 0;
}

static void open_subtitles(encode_ctx_t *ctx)
{
    mlt_properties properties = MLT_CONSUMER_PROPERTIES(ctx->consumer);
    memset(ctx->subtitle_st, 0, sizeof(ctx->subtitle_st));
    memset(ctx->sdec_ctx, 0, sizeof(ctx->sdec_ctx));
    memset(ctx->senc_ctx, 0, sizeof(ctx->senc_ctx));
    for (int p = 0; p < MAX_SUBTITLE_STREAMS; p++) {
        ctx->subtitle_prev_start[p] = -1;
    }

    if (!mlt_properties_get(properties, "subtitle.0.feed")
        || mlt_properties_get_int(properties, "sn")) {
        // No subtitles requested
        return;
    }

    enum AVCodecID codec_id = AV_CODEC_ID_SUBRIP;
    if (avformat_query_codec(ctx->oc->oformat, codec_id, FF_COMPLIANCE_UNOFFICIAL) != 1) {
        codec_id = AV_CODEC_ID_ASS;
        if (avformat_query_codec(ctx->oc->oformat, codec_id, FF_COMPLIANCE_UNOFFICIAL) != 1) {
            codec_id = AV_CODEC_ID_MOV_TEXT;
            if (avformat_query_codec(ctx->oc->oformat, codec_id, FF_COMPLIANCE_UNOFFICIAL) != 1) {
                mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer),
                              "Container not compatible with subtitles.\n");
                return;
            }
        }
    }

    for (int i = 0; i < MAX_SUBTITLE_STREAMS; i++) {
        char key[20];
        snprintf(key, sizeof(key), "subtitle.%d.feed", i);
        char *feed = mlt_properties_get(properties, key);
        if (!feed) {
            break;
        }
        // Set up the stream
        ctx->subtitle_st[i] = avformat_new_stream(ctx->oc, NULL);
        if (!ctx->subtitle_st[i]) {
            mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer),
                          "Failed to allocate the subtitle stream %d\n",
                          i);
            break;
        }
        ctx->subtitle_st[i]->time_base = (AVRational){1, 1000};
        ctx->subtitle_st[i]->codecpar->codec_type = AVMEDIA_TYPE_SUBTITLE;
        ctx->subtitle_st[i]->codecpar->codec_id = codec_id;
        snprintf(key, sizeof(key), "subtitle.%d.lang", i);
        if (mlt_properties_get(properties, key) != NULL)
            av_dict_set(&ctx->subtitle_st[i]->metadata,
                        "language",
                        mlt_properties_get(properties, key),
                        0);

        // Set up transcoding for non-subrip subtitle types
        if (codec_id != AV_CODEC_ID_SUBRIP) {
            // Set up the SRT decoder
            const AVCodec *sdec = avcodec_find_decoder(AV_CODEC_ID_SUBRIP);
            if (!sdec) {
                mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer),
                              "Unable to find subrip decoder\n");
                break;
            }
            ctx->sdec_ctx[i] = avcodec_alloc_context3(sdec);
            if (!ctx->sdec_ctx[i]) {
                mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer),
                              "Unable to create subrip decoder\n");
                ctx->subtitle_st[i] = NULL;
                break;
            }
            if (avcodec_open2(ctx->sdec_ctx[i], sdec, NULL) < 0) {
                mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer),
                              "Unable to open subrip decoder\n");
                ctx->subtitle_st[i] = NULL;
                break;
            }
            // Set up the subtitle encoder
            const AVCodec *senc = avcodec_find_encoder(codec_id);
            if (!senc) {
                mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer),
                              "Unable to find subtitle encoder\n");
                ctx->subtitle_st[i] = NULL;
                break;
            }
            ctx->senc_ctx[i] = avcodec_alloc_context3(senc);
            if (!ctx->senc_ctx[i]) {
                mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer),
                              "Unable to create subtitle encoder\n");
                ctx->subtitle_st[i] = NULL;
                break;
            }
            ctx->senc_ctx[i]->time_base = ctx->subtitle_st[i]->time_base;
            const char *subtitle_header
                = "[Script Info]\n"
                  "ScriptType: v4.00+\n"
                  "\n"
                  "[V4+ Styles]\n"
                  "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, "
                  "OutlineColour, "
                  "BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, "
                  "Angle, "
                  "BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, "
                  "Encoding\n"
                  "Style: Default,Cataneo BT,16,16777215,16777215,16777215,"
                  "12632256,1,0,0,0,100,100,0,0,"
                  "1,1,0,2,10,10,10,1\n"
                  "\n"
                  "[Events]\n"
                  "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, "
                  "Text\n";
            ctx->senc_ctx[i]->subtitle_header = (uint8_t *) av_strdup(subtitle_header);
            ctx->senc_ctx[i]->subtitle_header_size = strlen(subtitle_header);
            if (avcodec_open2(ctx->senc_ctx[i], senc, NULL) < 0) {
                mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer),
                              "Unable to open subtitle encoder\n");
                ctx->subtitle_st[i] = NULL;
                break;
            }
        }
    }
}

static void encode_subtitles(encode_ctx_t *ctx, mlt_frame frame)
{
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_properties subtitle_properties = mlt_properties_get_properties(frame_properties,
                                                                       "subtitles");
    if (!subtitle_properties) {
        return;
    }

    for (int i = 0; i < MAX_SUBTITLE_STREAMS; i++) {
        if (!ctx->subtitle_st[i]) {
            break;
        }
        // Get the subtitles from the feed
        char key[20];
        snprintf(key, sizeof(key), "subtitle.%d.feed", i);
        char *feed = mlt_properties_get(MLT_CONSUMER_PROPERTIES(ctx->consumer), key);
        mlt_properties feed_properties = mlt_properties_get_properties(subtitle_properties, feed);
        if (!feed_properties) {
            break;
        }
        mlt_position start_frame = mlt_properties_get_position(feed_properties, "start");
        if (ctx->subtitle_prev_start[i] >= start_frame) {
            // Skip duplicates. This is normal since subtitles are repeated on each frame for their duration.
            continue;
        }
        ctx->subtitle_prev_start[i] = start_frame;
        mlt_position end_frame = mlt_properties_get_position(feed_properties, "end");
        char *text = mlt_properties_get(feed_properties, "text");
        if (!text || !text[0]) {
            // Skip empty text.
            continue;
        }
        // Pack the SRT subtitles in a packet
        mlt_profile profile = mlt_service_profile(MLT_CONSUMER_SERVICE(ctx->consumer));
        AVPacket pkt;
        if (av_new_packet(&pkt, strlen(text))) {
            mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer), "failed to allocate packet 1\n");
            return;
        }
        strncpy((char *) pkt.data, text, pkt.size);

        // Transcode to another format if necessary
        if (ctx->subtitle_st[i]->codecpar->codec_id != AV_CODEC_ID_SUBRIP) {
            AVSubtitle avsubtitle;
            int got_sub;
            int ret = avcodec_decode_subtitle2(ctx->sdec_ctx[i], &avsubtitle, &got_sub, &pkt);
            av_packet_unref(&pkt);
            if (ret < 0) {
                mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer), "failed to decode subtitles\n");
                continue;
            }
            if (!got_sub) {
                mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer),
                              "No subtitle received from decoder\n");
                continue;
            }
            const int MAX_SUBTITLE_PACKET_SIZE = 1024 * 1024;
            if (av_new_packet(&pkt, MAX_SUBTITLE_PACKET_SIZE)) {
                mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer), "failed to allocate packet 2\n");
                return;
            }
            int subtitle_out_size
                = avcodec_encode_subtitle(ctx->senc_ctx[i], pkt.data, pkt.size, &avsubtitle);
            avsubtitle_free(&avsubtitle);
            if (subtitle_out_size < 0) {
                mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer), "Subtitle encoding failed\n");
                av_packet_unref(&pkt);
                continue;
            }
            av_shrink_packet(&pkt, subtitle_out_size);
        }
        pkt.pts = (int64_t) start_frame * 1000 * profile->frame_rate_den / profile->frame_rate_num;
        pkt.dts = AV_NOPTS_VALUE;
        pkt.duration = (int64_t) (end_frame - start_frame) * 1000 * profile->frame_rate_den
                       / profile->frame_rate_num;
        pkt.stream_index = ctx->subtitle_st[i]->index;
        // Send the packet to the output
        if (av_interleaved_write_frame(ctx->oc, &pkt)) {
            mlt_log_error(MLT_CONSUMER_SERVICE(ctx->consumer), "error writing subtitle frame\n");
            av_packet_unref(&pkt);
            break;
        }
    }
}

static int encode_video(encode_ctx_t *enc_ctx,
                        AVFrame *converted_avframe,
                        uint8_t *video_outbuf,
                        int video_outbuf_size,
                        mlt_frame frame,
                        mlt_image_format img_fmt)
{
    int ret = 0;
    mlt_properties properties = enc_ctx->properties;
    const char *dst_colorspace_str = mlt_properties_get(properties, "colorspace");
    mlt_colorspace dst_colorspace = mlt_image_colorspace_id(dst_colorspace_str);
    const char *color_range = mlt_properties_get(properties, "color_range");
    int dst_full_range = mlt_image_full_range(color_range);

    AVCodecContext *c = enc_ctx->vcodec_ctx;
    AVFrame *avframe = NULL;

    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_service service = MLT_CONSUMER_SERVICE(enc_ctx->consumer);

    if (mlt_properties_get_int(frame_properties, "rendered")) {
        uint8_t *image;
        int width = mlt_properties_get_int(properties, "width");
        int height = mlt_properties_get_int(properties, "height");
        int img_width = width;
        int img_height = height;
        AVFrame video_avframe;
        int is_interlaced_chroma_correction = 0;

        mlt_frame_get_image(frame, &image, &img_fmt, &img_width, &img_height, 0);

        // Interlaced 420 correction
        if (!mlt_properties_get_int(frame_properties, "progressive")
            && converted_avframe->format == AV_PIX_FMT_YUV420P // dst
            && img_fmt == mlt_image_yuv422 // src. It looks like rgb and 444 go as 422 too.
            && height % 4 == 0             // because reducing twice
            && width == converted_avframe->linesize[1] * 2) // if != things become too complicated
        {
            width *= 2; // substitute resolution, to appear each half-frame side-by-side
            height /= 2;
            for (int i = 0; i < 3; ++i)
                converted_avframe->linesize[i] *= 2;
            is_interlaced_chroma_correction = 1;
            mlt_log_debug(service, "interlaced chroma correction is activated\n");
        }

        mlt_image_format_planes(img_fmt,
                                width,
                                height,
                                image,
                                video_avframe.data,
                                video_avframe.linesize);

        // Do the colour space conversion
        int srcfmt = pick_pix_fmt(img_fmt);
        int flags
            = mlt_get_sws_flags(width, height, srcfmt, width, height, converted_avframe->format);
        struct SwsContext *context = sws_getContext(width,
                                                    height,
                                                    srcfmt,
                                                    width,
                                                    height,
                                                    converted_avframe->format,
                                                    flags,
                                                    NULL,
                                                    NULL,
                                                    NULL);
        const char *src_colorspace_str = mlt_properties_get(frame_properties, "colorspace");
        mlt_colorspace src_colorspace = mlt_image_colorspace_id(src_colorspace_str);
        int src_full_range = mlt_properties_get_int(frame_properties, "full_range");
        mlt_set_luma_transfer(context,
                              src_colorspace,
                              dst_colorspace,
                              src_full_range,
                              dst_full_range);
        sws_scale(context,
                  (const uint8_t *const *) video_avframe.data,
                  video_avframe.linesize,
                  0,
                  height,
                  converted_avframe->data,
                  converted_avframe->linesize);
        sws_freeContext(context);

        if (is_interlaced_chroma_correction) // restoring everything back
        {
            width /= 2;
            height *= 2;
            for (int i = 0; i < 3; ++i)
                converted_avframe->linesize[i] /= 2;
        }

        mlt_events_fire(properties, "consumer-frame-show", mlt_event_data_from_frame(frame));

        // Apply the alpha if applicable
        if (!mlt_properties_get(properties, "mlt_image_format")
            || strcmp(mlt_properties_get(properties, "mlt_image_format"), "rgba"))
            if (c->pix_fmt == AV_PIX_FMT_RGBA || c->pix_fmt == AV_PIX_FMT_ARGB
                || c->pix_fmt == AV_PIX_FMT_BGRA) {
                uint8_t *p;
                uint8_t *alpha = mlt_frame_get_alpha(frame);
                if (alpha) {
                    register int n;

                    for (int i = 0; i < height; i++) {
                        n = (width + 7) / 8;
                        p = converted_avframe->data[0] + i * converted_avframe->linesize[0] + 3;

                        switch (width % 8) {
                        case 0:
                            do {
                                *p = *alpha++;
                                p += 4;
                            case 7:
                                *p = *alpha++;
                                p += 4;
                            case 6:
                                *p = *alpha++;
                                p += 4;
                            case 5:
                                *p = *alpha++;
                                p += 4;
                            case 4:
                                *p = *alpha++;
                                p += 4;
                            case 3:
                                *p = *alpha++;
                                p += 4;
                            case 2:
                                *p = *alpha++;
                                p += 4;
                            case 1:
                                *p = *alpha++;
                                p += 4;
                            } while (--n);
                        }
                    }
                } else {
                    for (int i = 0; i < height; i++) {
                        int n = width;
                        uint8_t *p = converted_avframe->data[0] + i * converted_avframe->linesize[0]
                                     + 3;
                        while (n) {
                            *p = 255;
                            p += 4;
                            n--;
                        }
                    }
                }
            }
        if (AV_PIX_FMT_VAAPI == c->pix_fmt) {
            AVFilterContext *vfilter_in = mlt_properties_get_data(properties, "vfilter_in", NULL);
            AVFilterContext *vfilter_out = mlt_properties_get_data(properties, "vfilter_out", NULL);
            if (vfilter_in && vfilter_out) {
                if (!avframe)
                    avframe = av_frame_alloc();
                ret = av_buffersrc_add_frame(vfilter_in, converted_avframe);
                ret = av_buffersink_get_frame(vfilter_out, avframe);
                if (ret < 0) {
                    mlt_log_warning(service,
                                    "error with hwupload: %d (frame %d)\n",
                                    ret,
                                    enc_ctx->frame_count);
                    if (++enc_ctx->error_count > 2)
                        return -1;
                    ret = 0;
                }
            }
        } else {
            avframe = converted_avframe;
        }
    }

#ifdef AVFMT_RAWPICTURE
    if (enc_ctx->oc->oformat->flags & AVFMT_RAWPICTURE) {
        // raw video case. The API will change slightly in the near future for that
        AVPacket pkt;
        av_init_packet(&pkt);

        // Set frame interlace hints
        if (mlt_properties_get_int(frame_properties, "progressive"))
            c->field_order = AV_FIELD_PROGRESSIVE;
        else
            c->field_order = (mlt_properties_get_int(frame_properties, "top_field_first"))
                                 ? AV_FIELD_TB
                                 : AV_FIELD_BT;
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = enc_ctx->video_st->index;
        pkt.data = (uint8_t *) avframe;
        pkt.size = sizeof(AVPicture);

        ret = av_write_frame(enc_ctx->oc, &pkt);
    } else
#endif
    {
        AVPacket pkt;
        av_init_packet(&pkt);
        if (c->codec->id == AV_CODEC_ID_RAWVIDEO) {
            pkt.data = NULL;
            pkt.size = 0;
        } else {
            pkt.data = video_outbuf;
            pkt.size = video_outbuf_size;
        }

        // Set the quality
        avframe->quality = c->global_quality;
        avframe->pts = enc_ctx->frame_count;

        // Set frame interlace hints
#if LIBAVUTIL_VERSION_INT >= ((58 << 16) + (7 << 8) + 100)
        if (!mlt_properties_get_int(frame_properties, "progressive"))
            avframe->flags |= AV_FRAME_FLAG_INTERLACED;
        else
            avframe->flags &= ~AV_FRAME_FLAG_INTERLACED;
        const int tff = mlt_properties_get_int(frame_properties, "top_field_first");
        if (tff)
            avframe->flags |= AV_FRAME_FLAG_TOP_FIELD_FIRST;
        else
            avframe->flags &= ~AV_FRAME_FLAG_TOP_FIELD_FIRST;
#else
        avframe->interlaced_frame = !mlt_properties_get_int(frame_properties, "progressive");
        const int tff = avframe->top_field_first = mlt_properties_get_int(frame_properties,
                                                                          "top_field_first");
#endif
        if (mlt_properties_get_int(frame_properties, "progressive"))
            c->field_order = AV_FIELD_PROGRESSIVE;
        else if (c->codec_id == AV_CODEC_ID_MJPEG)
            c->field_order = tff ? AV_FIELD_TT : AV_FIELD_BB;
        else
            c->field_order = tff ? AV_FIELD_TB : AV_FIELD_BT;

        // Encode the image
        ret = avcodec_send_frame(c, avframe);
        if (ret < 0) {
            pkt.size = ret;
        } else {
        receive_video_packet:
            ret = avcodec_receive_packet(c, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                pkt.size = ret = 0;
            else if (ret < 0)
                pkt.size = ret;
        }

        // If zero size, it means the image was buffered
        if (pkt.size > 0) {
            av_packet_rescale_ts(&pkt, c->time_base, enc_ctx->video_st->time_base);
            pkt.stream_index = enc_ctx->video_st->index;

            // write the compressed frame in the media file
            ret = av_interleaved_write_frame(enc_ctx->oc, &pkt);
            mlt_log_debug(service, " frame_size %d\n", c->frame_size);

            // Dual pass logging
            if (mlt_properties_get_data(properties, "_logfile", NULL) && c->stats_out)
                fprintf(mlt_properties_get_data(properties, "_logfile", NULL), "%s", c->stats_out);

            enc_ctx->error_count = 0;

            if (!ret)
                goto receive_video_packet;
        } else if (pkt.size < 0) {
            mlt_log_warning(service,
                            "error with video encode: %d (frame %d)\n",
                            pkt.size,
                            enc_ctx->frame_count);
            if (++enc_ctx->error_count > 2)
                return -1;
            ret = 0;
        }
    }
    enc_ctx->frame_count++;
    enc_ctx->video_pts = (double) enc_ctx->frame_count * av_q2d(enc_ctx->vcodec_ctx->time_base);
    if (ret) {
        mlt_log_fatal(service, "error writing video frame: %d\n", ret);
        mlt_events_fire(properties, "consumer-fatal-error", mlt_event_data_none());
        return -1;
    }
    if (AV_PIX_FMT_VAAPI == c->pix_fmt)
        av_frame_free(&avframe);
    return 0;
}

/** The main thread - the argument is simply the consumer.
*/

static void *consumer_thread(void *arg)
{
    int i;

    // Encoding content
    encode_ctx_t *enc_ctx = mlt_pool_alloc(sizeof(encode_ctx_t));
    memset(enc_ctx, 0, sizeof(encode_ctx_t));

    // Map the argument to the object
    mlt_consumer consumer = enc_ctx->consumer = arg;

    // Get the properties
    mlt_properties properties = enc_ctx->properties = MLT_CONSUMER_PROPERTIES(consumer);

    // Get the terminate on pause property
    enc_ctx->terminate_on_pause = mlt_properties_get_int(enc_ctx->properties, "terminate_on_pause");

    // Determine if feed is slow (for realtime stuff)
    int real_time_output = mlt_properties_get_int(properties, "real_time");

    // Time structures
    struct timeval ante;

    // Get the frame rate
    double fps = mlt_properties_get_double(properties, "fps");

    // Get width and height
    int width = mlt_properties_get_int(properties, "width");
    int height = mlt_properties_get_int(properties, "height");

    // Get default audio properties
    enc_ctx->total_channels = enc_ctx->channels = mlt_properties_get_int(properties, "channels");
    enc_ctx->frequency = mlt_properties_get_int(properties, "frequency");
    void *pcm = NULL;
    int samples = 0;

    // AVFormat audio buffer and frame size
    enc_ctx->audio_outbuf_size = AUDIO_BUFFER_SIZE;

    // AVFormat video buffer and frame count
    int video_outbuf_size = VIDEO_BUFFER_SIZE;
    uint8_t *video_outbuf = av_malloc(video_outbuf_size);

    // Used for the frame properties
    mlt_frame frame = NULL;
    mlt_properties frame_properties = NULL;

    // Get the queues
    mlt_deque queue = mlt_properties_get_data(properties, "frame_queue", NULL);
    enc_ctx->fifo = mlt_properties_get_data(properties, "sample_fifo", NULL);

    AVFrame *converted_avframe = NULL;
    mlt_image_format img_fmt = mlt_image_yuv422;

    // For receiving audio samples back from the fifo
    int count = 0;

    // Frames dispatched
    long int frames = 0;
    long int total_time = 0;

    // Determine the format
    const AVOutputFormat *fmt = NULL;
    const char *filename = mlt_properties_get(properties, "target");
    char *format = mlt_properties_get(properties, "f");
    char *vcodec = mlt_properties_get(properties, "vcodec");
    char *acodec = mlt_properties_get(properties, "acodec");
    const AVCodec *audio_codec = NULL;
    const AVCodec *video_codec = NULL;

    // Misc
    char key[27];
    enc_ctx->frame_meta_properties = mlt_properties_new();
    int header_written = 0;

    // Check for user selected format first
    if (format != NULL)
        fmt = av_guess_format(format, NULL, NULL);

    // Otherwise check on the filename
    if (fmt == NULL && filename != NULL)
        fmt = av_guess_format(NULL, filename, NULL);

    // Otherwise default to mpeg
    if (fmt == NULL)
        fmt = av_guess_format("mpeg", NULL, NULL);

    // We need a filename - default to stdout?
    if (filename == NULL || !strcmp(filename, ""))
        filename = "pipe:";

    avformat_alloc_output_context2(&enc_ctx->oc, fmt, format, filename);

    // Get the codec ids selected
    enc_ctx->audio_codec_id = fmt->audio_codec;
    enc_ctx->video_codec_id = fmt->video_codec;

    // Check for audio codec overrides
    if ((acodec && strcmp(acodec, "none") == 0) || mlt_properties_get_int(properties, "an"))
        enc_ctx->audio_codec_id = AV_CODEC_ID_NONE;
    else if (acodec) {
        audio_codec = avcodec_find_encoder_by_name(acodec);
        if (audio_codec) {
            enc_ctx->audio_codec_id = audio_codec->id;
            if (enc_ctx->audio_codec_id == AV_CODEC_ID_AC3
                && avcodec_find_encoder_by_name("ac3_fixed")) {
                mlt_properties_set(enc_ctx->properties, "_acodec", "ac3_fixed");
                acodec = mlt_properties_get(enc_ctx->properties, "_acodec");
                audio_codec = avcodec_find_encoder_by_name(acodec);
            } else if (!strcmp(acodec, "aac") || !strcmp(acodec, "vorbis")) {
                mlt_properties_set(enc_ctx->properties, "astrict", "experimental");
            }
        } else {
            enc_ctx->audio_codec_id = AV_CODEC_ID_NONE;
            mlt_log_warning(MLT_CONSUMER_SERVICE(consumer),
                            "audio codec %s unrecognised - ignoring\n",
                            acodec);
        }
    } else {
        audio_codec = avcodec_find_encoder(enc_ctx->audio_codec_id);
    }

    // Check for video codec overrides
    if ((vcodec && strcmp(vcodec, "none") == 0) || mlt_properties_get_int(properties, "vn"))
        enc_ctx->video_codec_id = AV_CODEC_ID_NONE;
    else if (vcodec) {
        video_codec = avcodec_find_encoder_by_name(vcodec);
        if (video_codec) {
            enc_ctx->video_codec_id = video_codec->id;
        } else {
            enc_ctx->video_codec_id = AV_CODEC_ID_NONE;
            mlt_log_warning(MLT_CONSUMER_SERVICE(consumer),
                            "video codec %s unrecognised - ignoring\n",
                            vcodec);
        }
    } else {
        video_codec = avcodec_find_encoder(enc_ctx->video_codec_id);
    }

    // Write metadata
    for (i = 0; i < mlt_properties_count(properties); i++) {
        char *name = mlt_properties_get_name(properties, i);
        if (name && !strncmp(name, "meta.attr.", 10)) {
            char *key = strdup(name + 10);
            char *markup = strrchr(key, '.');
            if (markup && !strcmp(markup, ".markup")) {
                markup[0] = '\0';
                if (!strstr(key, ".stream."))
                    av_dict_set(&enc_ctx->oc->metadata,
                                key,
                                mlt_properties_get_value(properties, i),
                                0);
            }
            free(key);
        }
    }

    // Add audio and video streams
    if (enc_ctx->video_codec_id != AV_CODEC_ID_NONE) {
        if ((enc_ctx->video_st
             = add_video_stream(consumer, enc_ctx->oc, video_codec, &enc_ctx->vcodec_ctx))) {
            const char *img_fmt_name = mlt_properties_get(properties, "mlt_image_format");
            if (img_fmt_name) {
                // Set the mlt_image_format from explicit property.
                mlt_image_format f = mlt_image_format_id(img_fmt_name);
                if (mlt_image_invalid != f)
                    img_fmt = f;
            } else {
                // Set the mlt_image_format from the selected pix_fmt.
                const char *pix_fmt_name = av_get_pix_fmt_name(enc_ctx->vcodec_ctx->pix_fmt);
                if (!strcmp(pix_fmt_name, "rgba") || !strcmp(pix_fmt_name, "argb")
                    || !strcmp(pix_fmt_name, "bgra")) {
                    mlt_properties_set(properties, "mlt_image_format", "rgba");
                    img_fmt = mlt_image_rgba;
                } else if (strstr(pix_fmt_name, "rgb") || strstr(pix_fmt_name, "bgr")) {
                    mlt_properties_set(properties, "mlt_image_format", "rgb");
                    img_fmt = mlt_image_rgb;
                } else if (strstr(pix_fmt_name, "yuv420p10le")) {
                    mlt_properties_set(properties, "mlt_image_format", "yuv420p10");
                    img_fmt = mlt_image_yuv420p10;
                } else if (strstr(pix_fmt_name, "yuv444p10le")) {
                    mlt_properties_set(properties, "mlt_image_format", "yuv444p10");
                    img_fmt = mlt_image_yuv444p10;
                } else if (strstr(pix_fmt_name, "rgba64le")) {
                    mlt_properties_set(properties, "mlt_image_format", "rgba64");
                    img_fmt = mlt_image_rgba64;
                }
            }
        }
    }
    if (enc_ctx->audio_codec_id != AV_CODEC_ID_NONE) {
        int is_multi = 0;
#if HAVE_FFMPEG_CH_LAYOUT
        AVChannelLayout ch_layout;
#else
        int64_t ch_layout;
#endif

        enc_ctx->total_channels = 0;
        // multitrack audio
        for (i = 0; i < MAX_AUDIO_STREAMS; i++) {
            sprintf(key, "channels.%d", i);
            int j = mlt_properties_get_int(properties, key);
            if (j) {
                is_multi = 1;
                enc_ctx->total_channels += j;
#if HAVE_FFMPEG_CH_LAYOUT
                av_channel_layout_default(&ch_layout, j);
                enc_ctx->audio_st[i] = add_audio_stream(consumer,
                                                        enc_ctx->oc,
                                                        audio_codec,
                                                        &enc_ctx->acodec_ctx[i],
                                                        j,
                                                        &ch_layout);
                av_channel_layout_uninit(&ch_layout);
#else
                ch_layout = av_get_default_channel_layout(j);
                enc_ctx->audio_st[i] = add_audio_stream(consumer,
                                                        enc_ctx->oc,
                                                        audio_codec,
                                                        &enc_ctx->acodec_ctx[i],
                                                        j,
                                                        ch_layout);
#endif
            }
        }
        // single track
        if (!is_multi) {
            mlt_channel_layout layout = mlt_audio_channel_layout_id(
                mlt_properties_get(properties, "channel_layout"));
            if (layout == mlt_channel_auto || layout == mlt_channel_independent
                || mlt_audio_channel_layout_channels(layout) != enc_ctx->channels) {
                layout = mlt_audio_channel_layout_default(enc_ctx->channels);
            }
#if HAVE_FFMPEG_CH_LAYOUT
            av_channel_layout_from_mask(&ch_layout, mlt_to_av_channel_layout(layout));
            enc_ctx->audio_st[0] = add_audio_stream(consumer,
                                                    enc_ctx->oc,
                                                    audio_codec,
                                                    &enc_ctx->acodec_ctx[0],
                                                    enc_ctx->channels,
                                                    &ch_layout);
            av_channel_layout_uninit(&ch_layout);
#else
            ch_layout = mlt_to_av_channel_layout(layout);
            enc_ctx->audio_st[0] = add_audio_stream(consumer,
                                                    enc_ctx->oc,
                                                    audio_codec,
                                                    &enc_ctx->acodec_ctx[0],
                                                    enc_ctx->channels,
                                                    ch_layout);
#endif
            enc_ctx->total_channels = enc_ctx->channels;
        }
    }
    mlt_properties_set_int(properties, "channels", enc_ctx->total_channels);

    // Audio format is determined when adding the audio stream
    mlt_audio_format aud_fmt = mlt_audio_none;
    if (enc_ctx->audio_st[0])
        aud_fmt = get_mlt_audio_format(enc_ctx->acodec_ctx[0]->sample_fmt);
    enc_ctx->sample_bytes = mlt_audio_format_size(aud_fmt, 1, 1);
    enc_ctx->sample_bytes = enc_ctx->sample_bytes ? enc_ctx->sample_bytes
                                                  : 1; // prevent divide by zero

    // Set the parameters (even though we have none...)
    {
        if (mlt_properties_get(properties, "muxpreload")
            && !mlt_properties_get(properties, "preload"))
            mlt_properties_set_double(properties,
                                      "preload",
                                      mlt_properties_get_double(properties, "muxpreload"));
        enc_ctx->oc->max_delay = (int) (mlt_properties_get_double(properties, "muxdelay")
                                        * AV_TIME_BASE);

        // Process properties as AVOptions
        char *fpre = mlt_properties_get(properties, "fpre");
        if (fpre) {
            mlt_properties p = mlt_properties_load(fpre);
            apply_properties(enc_ctx->oc, p, AV_OPT_FLAG_ENCODING_PARAM);
            if (enc_ctx->oc->oformat && enc_ctx->oc->oformat->priv_class && enc_ctx->oc->priv_data)
                apply_properties(enc_ctx->oc->priv_data, p, AV_OPT_FLAG_ENCODING_PARAM);
            mlt_properties_close(p);
        }
        apply_properties(enc_ctx->oc, properties, AV_OPT_FLAG_ENCODING_PARAM);
        if (enc_ctx->oc->oformat && enc_ctx->oc->oformat->priv_class && enc_ctx->oc->priv_data)
            apply_properties(enc_ctx->oc->priv_data, properties, AV_OPT_FLAG_ENCODING_PARAM);

        if (enc_ctx->video_st
            && !open_video(properties,
                           enc_ctx->oc,
                           enc_ctx->video_st,
                           video_codec,
                           enc_ctx->vcodec_ctx))
            enc_ctx->video_st = NULL;
        for (i = 0; i < MAX_AUDIO_STREAMS && enc_ctx->audio_st[i]; i++) {
            enc_ctx->audio_input_frame_size = open_audio(properties,
                                                         enc_ctx->oc,
                                                         enc_ctx->audio_st[i],
                                                         audio_codec,
                                                         enc_ctx->acodec_ctx[i]);
            if (!enc_ctx->audio_input_frame_size) {
                // Remove the audio stream from the output context
                unsigned int j;
                for (j = 0; j < enc_ctx->oc->nb_streams; j++) {
                    if (enc_ctx->oc->streams[j] == enc_ctx->audio_st[i])
                        av_freep(&enc_ctx->oc->streams[j]);
                }
                --enc_ctx->oc->nb_streams;
                enc_ctx->audio_st[i] = NULL;
            }
        }

        open_subtitles(enc_ctx);

        // Setup custom I/O if redirecting
        if (mlt_properties_get_int(properties, "redirect")) {
            int buffer_size = 32768;
            unsigned char *buffer = av_malloc(buffer_size);
            AVIOContext *io
                = avio_alloc_context(buffer, buffer_size, 1, properties, NULL, mlt_write, NULL);
            if (buffer && io) {
                enc_ctx->oc->pb = io;
                enc_ctx->oc->flags |= AVFMT_FLAG_CUSTOM_IO;
                mlt_properties_set_data(properties,
                                        "avio_buffer",
                                        buffer,
                                        buffer_size,
                                        av_free,
                                        NULL);
                mlt_properties_set_data(properties, "avio_context", io, 0, av_free, NULL);
                mlt_events_register(properties, "avformat-write");
            } else {
                av_free(buffer);
                mlt_log_error(MLT_CONSUMER_SERVICE(consumer),
                              "failed to setup output redirection\n");
            }
        }
        // Open the output file, if needed
        else if (!(fmt->flags & AVFMT_NOFILE)) {
            if (avio_open(&enc_ctx->oc->pb, filename, AVIO_FLAG_WRITE) < 0) {
                mlt_log_error(MLT_CONSUMER_SERVICE(consumer), "Could not open '%s'\n", filename);
                mlt_events_fire(properties, "consumer-fatal-error", mlt_event_data_none());
                goto on_fatal_error;
            }
        }
    }

    // Last check - need at least one stream
    if (!enc_ctx->audio_st[0] && !enc_ctx->video_st) {
        mlt_events_fire(properties, "consumer-fatal-error", mlt_event_data_none());
        goto on_fatal_error;
    }

    // Allocate picture
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
    if (enc_ctx->video_st) {
        pix_fmt = enc_ctx->vcodec_ctx->pix_fmt == AV_PIX_FMT_VAAPI ? AV_PIX_FMT_NV12
                                                                   : enc_ctx->vcodec_ctx->pix_fmt;
        converted_avframe = alloc_picture(pix_fmt, width, height);
        if (!converted_avframe) {
            mlt_log_error(MLT_CONSUMER_SERVICE(consumer), "failed to allocate video AVFrame\n");
            mlt_events_fire(properties, "consumer-fatal-error", mlt_event_data_none());
            goto on_fatal_error;
        }
    }

    // Allocate audio AVFrame
    if (enc_ctx->audio_st[0]) {
        enc_ctx->audio_avframe = av_frame_alloc();
        if (enc_ctx->audio_avframe) {
            AVCodecContext *c = enc_ctx->acodec_ctx[0];
            enc_ctx->audio_avframe->format = c->sample_fmt;
            enc_ctx->audio_avframe->nb_samples = enc_ctx->audio_input_frame_size;
#if HAVE_FFMPEG_CH_LAYOUT
            av_channel_layout_copy(&enc_ctx->audio_avframe->ch_layout, &c->ch_layout);
#else
            enc_ctx->audio_avframe->channel_layout = c->channel_layout;
            enc_ctx->audio_avframe->channels = c->channels;
#endif
        } else {
            mlt_log_error(MLT_CONSUMER_SERVICE(consumer), "failed to allocate audio AVFrame\n");
            mlt_events_fire(properties, "consumer-fatal-error", mlt_event_data_none());
            goto on_fatal_error;
        }
    }

    // Get the starting time (can ignore the times above)
    gettimeofday(&ante, NULL);

    // Loop while running
    while (mlt_properties_get_int(properties, "running")
           && (!enc_ctx->terminated || (enc_ctx->video_st && mlt_deque_count(queue)))) {
        if (!frame)
            frame = mlt_consumer_rt_frame(consumer);

        // Check that we have a frame to work with
        if (frame != NULL) {
            // Default audio args
            frame_properties = MLT_FRAME_PROPERTIES(frame);

            // Write the stream header.
            if (!header_written) {
                // set timecode from first frame if not been set from metadata
                if (!mlt_properties_get(properties, "timecode")) {
                    char *vitc = mlt_properties_get(frame_properties, "meta.attr.vitc.markup");
                    if (vitc && vitc[0]) {
                        mlt_log_debug(MLT_CONSUMER_SERVICE(consumer), "timecode=[%s]\n", vitc);
                        av_dict_set(&enc_ctx->oc->metadata, "timecode", vitc, 0);

                        if (enc_ctx->video_st)
                            av_dict_set(&enc_ctx->video_st->metadata, "timecode", vitc, 0);
                    };
                };

                if (avformat_write_header(enc_ctx->oc, NULL) < 0) {
                    mlt_log_error(MLT_CONSUMER_SERVICE(consumer),
                                  "Could not write header '%s'\n",
                                  filename);
                    mlt_events_fire(properties, "consumer-fatal-error", mlt_event_data_none());
                    goto on_fatal_error;
                }

                header_written = 1;
            }

            // Increment frames dispatched
            frames++;

            // Check for the terminated condition
            enc_ctx->terminated = enc_ctx->terminate_on_pause
                                  && mlt_properties_get_double(frame_properties, "_speed") == 0.0;

            // Encode subtitles
            if (!enc_ctx->terminated && enc_ctx->subtitle_st[0]) {
                encode_subtitles(enc_ctx, frame);
            }

            // Get audio and append to the fifo
            if (!enc_ctx->terminated && enc_ctx->audio_st[0]) {
                samples = mlt_audio_calculate_frame_samples(fps, enc_ctx->frequency, count++);
                enc_ctx->channels = enc_ctx->total_channels;
                mlt_frame_get_audio(frame,
                                    &pcm,
                                    &aud_fmt,
                                    &enc_ctx->frequency,
                                    &enc_ctx->channels,
                                    &samples);

                // Save the audio channel remap properties for later
                mlt_properties_pass(enc_ctx->frame_meta_properties,
                                    frame_properties,
                                    "meta.map.audio.");

                // Create the fifo if we don't have one
                if (enc_ctx->fifo == NULL) {
                    enc_ctx->fifo = sample_fifo_init(enc_ctx->frequency, enc_ctx->channels);
                    mlt_properties_set_data(properties,
                                            "sample_fifo",
                                            enc_ctx->fifo,
                                            0,
                                            (mlt_destructor) sample_fifo_close,
                                            NULL);
                }
                if (pcm) {
                    // Silence if not normal forward speed
                    if (mlt_properties_get_double(frame_properties, "_speed") != 1.0)
                        memset(pcm, 0, samples * enc_ctx->channels * enc_ctx->sample_bytes);

                    // Append the samples
                    sample_fifo_append(enc_ctx->fifo,
                                       pcm,
                                       samples * enc_ctx->channels * enc_ctx->sample_bytes);
                    total_time += (samples * 1000000) / enc_ctx->frequency;
                }
                if (!enc_ctx->video_st) {
                    mlt_events_fire(properties,
                                    "consumer-frame-show",
                                    mlt_event_data_from_frame(frame));
                }
            }

            // Encode the image
            if (!enc_ctx->terminated && enc_ctx->video_st)
                mlt_deque_push_back(queue, frame);
            else
                mlt_frame_close(frame);
            frame = NULL;
        }

        // While we have stuff to process, process...
        while (1) {
            // Write interleaved audio and video frames
            if (!enc_ctx->video_st
                || (enc_ctx->video_st && enc_ctx->audio_st[0]
                    && enc_ctx->audio_pts < enc_ctx->video_pts)) {
                // Write audio
                int fifo_frames = sample_fifo_used(enc_ctx->fifo)
                                  / (enc_ctx->audio_input_frame_size * enc_ctx->channels
                                     * enc_ctx->sample_bytes);
                if ((enc_ctx->video_st && enc_ctx->terminated) || fifo_frames) {
                    int r = encode_audio(enc_ctx);

                    if (r > 0)
                        break;
                    else if (r < 0)
                        goto on_fatal_error;
                } else {
                    break;
                }
            } else if (enc_ctx->video_st) {
                // Write video
                if (mlt_deque_count(queue)) {
                    frame = mlt_deque_pop_front(queue);
                    if (encode_video(enc_ctx,
                                     converted_avframe,
                                     video_outbuf,
                                     video_outbuf_size,
                                     frame,
                                     img_fmt)
                        < 0)
                        goto on_fatal_error;
                    mlt_frame_close(frame);
                    frame = NULL;
                } else {
                    break;
                }
            }
            if (enc_ctx->audio_st[0])
                mlt_log_debug(MLT_CONSUMER_SERVICE(consumer), "audio pts %f ", enc_ctx->audio_pts);
            if (enc_ctx->video_st)
                mlt_log_debug(MLT_CONSUMER_SERVICE(consumer), "video pts %f ", enc_ctx->video_pts);
            mlt_log_debug(MLT_CONSUMER_SERVICE(consumer), "\n");
        }

        if (real_time_output == 1 && frames % 2 == 0) {
            long passed = time_difference(&ante);
            if (enc_ctx->fifo != NULL) {
                long pending = (((long) sample_fifo_used(enc_ctx->fifo) / enc_ctx->sample_bytes
                                 * 1000)
                                / enc_ctx->frequency)
                               * 1000;
                passed -= pending;
            }
            if (passed < total_time) {
                long total = (total_time - passed);
                struct timespec t = {total / 1000000, (total % 1000000) * 1000};
                nanosleep(&t, NULL);
            }
        }
    }

    // Flush the encoder buffers
    if (real_time_output <= 0) {
        // Flush audio fifo
        // TODO: flush all audio streams
        if (enc_ctx->fifo && enc_ctx->audio_st[0])
            for (;;) {
                int sz = sample_fifo_used(enc_ctx->fifo);
                int ret = encode_audio(enc_ctx);

                mlt_log_debug(MLT_CONSUMER_SERVICE(consumer),
                              "flushing audio: sz=%d, ret=%d\n",
                              sz,
                              ret);

                if (!sz || ret < 0)
                    break;
            }

            // Flush video
#ifdef AVFMT_RAWPICTURE
        if (enc_ctx->video_st && !(enc_ctx->oc->oformat->flags & AVFMT_RAWPICTURE))
            for (;;)
#else
        if (enc_ctx->video_st)
            for (;;)
#endif
            {
                AVCodecContext *c = enc_ctx->vcodec_ctx;
                AVPacket pkt;
                av_init_packet(&pkt);
                if (c->codec->id == AV_CODEC_ID_RAWVIDEO) {
                    pkt.data = NULL;
                    pkt.size = 0;
                } else {
                    pkt.data = video_outbuf;
                    pkt.size = video_outbuf_size;
                }

                // Encode the image
                int ret;
                while ((ret = avcodec_receive_packet(c, &pkt)) == AVERROR(EAGAIN)) {
                    ret = avcodec_send_frame(c, NULL);
                    if (ret < 0) {
                        mlt_log_warning(MLT_CONSUMER_SERVICE(consumer),
                                        "error with video encode: %d\n",
                                        ret);
                        break;
                    }
                }
                mlt_log_debug(MLT_CONSUMER_SERVICE(consumer), "flushing video size %d\n", pkt.size);
                if (pkt.size < 0)
                    break;
                // Dual pass logging
                if (mlt_properties_get_data(properties, "_logfile", NULL) && c->stats_out)
                    fprintf(mlt_properties_get_data(properties, "_logfile", NULL),
                            "%s",
                            c->stats_out);
                if (!pkt.size)
                    break;

                av_packet_rescale_ts(&pkt, c->time_base, enc_ctx->video_st->time_base);
                pkt.stream_index = enc_ctx->video_st->index;

                // write the compressed frame in the media file
                if (av_interleaved_write_frame(enc_ctx->oc, &pkt) != 0) {
                    mlt_log_fatal(MLT_CONSUMER_SERVICE(consumer),
                                  "error writing flushed video frame\n");
                    mlt_events_fire(properties, "consumer-fatal-error", mlt_event_data_none());
                    goto on_fatal_error;
                }
            }
    }

on_fatal_error:

    if (frame)
        mlt_frame_close(frame);

    // Write the trailer, if any
    if (frames)
        av_write_trailer(enc_ctx->oc);

    // Clean up input and output frames
    if (converted_avframe)
        av_free(converted_avframe->data[0]);
    av_free(converted_avframe);
    av_free(video_outbuf);
    av_free(enc_ctx->audio_avframe);

    // Close subtitle transcoding codecs
    for (i = 0; i < MAX_SUBTITLE_STREAMS; i++) {
        avcodec_free_context(&enc_ctx->sdec_ctx[i]);
        avcodec_free_context(&enc_ctx->senc_ctx[i]);
    }

    // close each codec
    avcodec_free_context(&enc_ctx->vcodec_ctx);
    for (i = 0; i < MAX_AUDIO_STREAMS; i++)
        avcodec_free_context(&enc_ctx->acodec_ctx[i]);

    // Free the streams
    for (unsigned int i = 0; i < enc_ctx->oc->nb_streams; i++)
        av_freep(&enc_ctx->oc->streams[i]);

    // Close the output file
    if (!(fmt->flags & AVFMT_NOFILE) && !mlt_properties_get_int(properties, "redirect")) {
        if (enc_ctx->oc->pb)
            avio_close(enc_ctx->oc->pb);
    }

    // Free the stream
    av_free(enc_ctx->oc);

    // Just in case we terminated on pause
    mlt_consumer_stopped(consumer);
    mlt_properties_close(enc_ctx->frame_meta_properties);

    if (mlt_properties_get_int(properties, "pass") > 1) {
        // Remove the dual pass log file
        if (mlt_properties_get(properties, "_logfilename"))
            remove(mlt_properties_get(properties, "_logfilename"));

        // Remove the x264 dual pass logs
        char *cwd = getcwd(NULL, 0);
        const char *file = "x264_2pass.log";
        char *full = malloc(strlen(cwd) + strlen(file) + 2);
        sprintf(full, "%s/%s", cwd, file);
        remove(full);
        free(full);
        file = "x264_2pass.log.temp";
        full = malloc(strlen(cwd) + strlen(file) + 2);
        sprintf(full, "%s/%s", cwd, file);
        remove(full);
        free(full);
        file = "x264_2pass.log.mbtree";
        full = malloc(strlen(cwd) + strlen(file) + 2);
        sprintf(full, "%s/%s", cwd, file);
        remove(full);
        free(full);
        free(cwd);
        remove("x264_2pass.log.temp");

        // Recent versions of libavcodec/x264 support passlogfile and need cleanup if specified.
        if (!mlt_properties_get(properties, "_logfilename")
            && mlt_properties_get(properties, "passlogfile")) {
            mlt_properties_get(properties, "passlogfile");
            file = mlt_properties_get(properties, "passlogfile");
            remove(file);
            full = malloc(strlen(file) + strlen(".mbtree") + 1);
            sprintf(full, "%s.mbtree", file);
            remove(full);
            free(full);
        }
    }

    while ((frame = mlt_deque_pop_back(queue)))
        mlt_frame_close(frame);

    mlt_pool_release(enc_ctx);

    return NULL;
}

/** Close the consumer.
*/

static void consumer_close(mlt_consumer consumer)
{
    // Stop the consumer
    mlt_consumer_stop(consumer);

    // Close the parent
    mlt_consumer_close(consumer);

    // Free the memory
    free(consumer);
}
