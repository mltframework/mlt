/*
 * consumer_decklink.cpp -- output through Blackmagic Design DeckLink
 * Copyright (C) 2010-2025 Meltytech, LLC
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
 * License along with consumer library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define __STDC_FORMAT_MACROS /* see inttypes.h */
#include "common.h"
#include <framework/mlt.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define SWAB_SLICED_ALIGN_POW 5
static int swab_sliced(int id, int idx, int jobs, void *cookie)
{
    unsigned char **args = (unsigned char **) cookie;
    ssize_t sz = (ssize_t) args[2];
    ssize_t bsz = ((sz / jobs + (1 << SWAB_SLICED_ALIGN_POW) - 1) >> SWAB_SLICED_ALIGN_POW)
                  << SWAB_SLICED_ALIGN_POW;
    ssize_t offset = bsz * idx;

    if (offset < sz) {
        if ((offset + bsz) > sz)
            bsz = sz - offset;

        swab2(args[0] + offset, args[1] + offset, bsz);
    }

    return 0;
};

static const unsigned PREROLL_MINIMUM = 3;

enum { OP_NONE = 0, OP_OPEN, OP_START, OP_STOP, OP_EXIT };
enum { EOTF_SDR = 0, EOTF_HDR = 1, EOTF_PQ = 2, EOTF_HLG = 3 }; ///< CEA 861.3

class DeckLinkConsumer : public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback
{
private:
    mlt_consumer_s m_consumer;
    IDeckLink *m_deckLink;
    IDeckLinkOutput *m_deckLinkOutput;
    IDeckLinkDisplayMode *m_displayMode;
    int m_width;
    int m_height;
    BMDTimeValue m_duration;
    BMDTimeScale m_timescale;
    double m_fps;
    uint64_t m_count;
    int m_outChannels;
    int m_inChannels;
    bool m_fix5p1{false};
    bool m_isAudio;
    int m_isKeyer;
    IDeckLinkKeyer *m_deckLinkKeyer;
    bool m_terminate_on_pause;
    uint32_t m_preroll;
    uint32_t m_reprio;

    mlt_deque m_aqueue;
    pthread_mutex_t m_aqueue_lock;
    mlt_deque m_frames;

    pthread_mutex_t m_op_lock;
    pthread_mutex_t m_op_arg_mutex;
    pthread_cond_t m_op_arg_cond;
    int m_op_id;
    int m_op_res;
    int m_op_arg;
    pthread_t m_op_thread;
    bool m_sliced_swab;
    uint8_t *m_buffer;
    bool m_running{false};
    pthread_cond_t m_refresh_cond;
    pthread_mutex_t m_refresh_mutex;
    int m_refresh{0};
    bool m_purge{false};
    int m_previousSpeed{0};
    int m_currentSpeed{0};

    IDeckLinkDisplayMode *getDisplayMode()
    {
        mlt_profile profile = mlt_service_profile(MLT_CONSUMER_SERVICE(getConsumer()));
        IDeckLinkDisplayModeIterator *iter = nullptr;
        IDeckLinkDisplayMode *mode = nullptr;
        IDeckLinkDisplayMode *result = 0;

        if (m_deckLinkOutput->GetDisplayModeIterator(&iter) == S_OK) {
            while (!result && iter->Next(&mode) == S_OK) {
                m_width = mode->GetWidth();
                m_height = mode->GetHeight();
                mode->GetFrameRate(&m_duration, &m_timescale);
                m_fps = (double) m_timescale / m_duration;
                int p = mode->GetFieldDominance() == bmdProgressiveFrame;
                mlt_log_verbose(getConsumer(),
                                "BMD mode %dx%d %.3f fps prog %d\n",
                                m_width,
                                m_height,
                                m_fps,
                                p);

                if (m_width == profile->width && p == profile->progressive
                    && (int) m_fps == (int) mlt_profile_fps(profile)
                    && (m_height == profile->height || (m_height == 486 && profile->height == 480)))
                    result = mode;
                else
                    SAFE_RELEASE(mode);
            }
            SAFE_RELEASE(iter);
        }

        return result;
    }

public:
    mlt_consumer getConsumer() { return &m_consumer; }
    bool isRunning() const { return m_running; }

    DeckLinkConsumer()
    {
        pthread_mutexattr_t mta;

        m_displayMode = nullptr;
        m_deckLinkKeyer = nullptr;
        m_deckLinkOutput = nullptr;
        m_deckLink = nullptr;
        m_aqueue = mlt_deque_init();
        m_frames = mlt_deque_init();
        m_buffer = nullptr;

        // operation locks
        m_op_id = OP_NONE;
        m_op_arg = 0;
        pthread_mutexattr_init(&mta);
        pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_op_lock, &mta);
        pthread_mutex_init(&m_op_arg_mutex, &mta);
        pthread_mutex_init(&m_aqueue_lock, &mta);
        pthread_mutexattr_destroy(&mta);
        pthread_cond_init(&m_op_arg_cond, nullptr);
        pthread_create(&m_op_thread, nullptr, op_main, this);
        pthread_cond_init(&m_refresh_cond, nullptr);
        pthread_mutex_init(&m_refresh_mutex, nullptr);
    }

    virtual ~DeckLinkConsumer()
    {
        mlt_log_debug(getConsumer(), "%s: entering\n", __FUNCTION__);

        SAFE_RELEASE(m_displayMode);
        SAFE_RELEASE(m_deckLinkKeyer);
        SAFE_RELEASE(m_deckLinkOutput);
        SAFE_RELEASE(m_deckLink);

        mlt_deque_close(m_aqueue);
        mlt_deque_close(m_frames);

        op(OP_EXIT, 0);
        mlt_log_debug(getConsumer(), "%s: waiting for op thread\n", __FUNCTION__);
        pthread_join(m_op_thread, nullptr);
        mlt_log_debug(getConsumer(), "%s: finished op thread\n", __FUNCTION__);

        pthread_mutex_destroy(&m_aqueue_lock);
        pthread_mutex_destroy(&m_op_lock);
        pthread_mutex_destroy(&m_op_arg_mutex);
        pthread_cond_destroy(&m_op_arg_cond);

        pthread_mutex_lock(&m_refresh_mutex);
        if (m_refresh < 2)
            m_refresh = m_refresh <= 0 ? 1 : m_refresh + 1;
        pthread_cond_broadcast(&m_refresh_cond);
        pthread_mutex_unlock(&m_refresh_mutex);
        pthread_mutex_destroy(&m_refresh_mutex);
        pthread_cond_destroy(&m_refresh_cond);

        mlt_log_debug(getConsumer(), "%s: exiting\n", __FUNCTION__);
    }

    int op(int op_id, int arg)
    {
        int r;

        // lock operation mutex
        pthread_mutex_lock(&m_op_lock);

        mlt_log_debug(getConsumer(), "%s: op_id=%d\n", __FUNCTION__, op_id);

        // notify op id
        pthread_mutex_lock(&m_op_arg_mutex);
        m_op_id = op_id;
        m_op_arg = arg;
        pthread_cond_signal(&m_op_arg_cond);
        pthread_mutex_unlock(&m_op_arg_mutex);

        // wait op done
        pthread_mutex_lock(&m_op_arg_mutex);
        while (OP_NONE != m_op_id)
            pthread_cond_wait(&m_op_arg_cond, &m_op_arg_mutex);
        pthread_mutex_unlock(&m_op_arg_mutex);

        // save result
        r = m_op_res;

        mlt_log_debug(getConsumer(), "%s: r=%d\n", __FUNCTION__, r);

        // unlock operation mutex
        pthread_mutex_unlock(&m_op_lock);

        return r;
    }

    void refresh()
    {
        pthread_mutex_lock(&m_refresh_mutex);
        m_refresh = CLAMP(m_refresh + 1, 0, 2);
        pthread_cond_broadcast(&m_refresh_cond);
        pthread_mutex_unlock(&m_refresh_mutex);
    }

    void purge()
    {
        if (!m_running)
            return;

        pthread_mutex_lock(&m_aqueue_lock);
        // When playing rewind or fast forward then we need to keep one
        // frame in the queue to prevent playback stalling.
        int n = (m_currentSpeed == 0 || m_currentSpeed == 1) ? 0 : 1;
        while (mlt_deque_count(m_aqueue) > n)
            mlt_frame_close(mlt_frame(mlt_deque_pop_back(m_aqueue)));
        m_purge = true;
        m_deckLinkOutput->FlushBufferedAudioSamples();
        pthread_mutex_unlock(&m_aqueue_lock);
    }

protected:
    static void *op_main(void *thisptr)
    {
        DeckLinkConsumer *d = static_cast<DeckLinkConsumer *>(thisptr);

        mlt_log_debug(d->getConsumer(), "%s: entering\n", __FUNCTION__);

        for (;;) {
            int o, r = 0;

            // wait op command
            pthread_mutex_lock(&d->m_op_arg_mutex);
            while (OP_NONE == d->m_op_id)
                pthread_cond_wait(&d->m_op_arg_cond, &d->m_op_arg_mutex);
            pthread_mutex_unlock(&d->m_op_arg_mutex);
            o = d->m_op_id;

            mlt_log_debug(d->getConsumer(),
                          "%s:%d d->m_op_id=%d\n",
                          __FUNCTION__,
                          __LINE__,
                          d->m_op_id);

            switch (d->m_op_id) {
            case OP_OPEN:
                r = d->m_op_res = d->open(d->m_op_arg);
                break;

            case OP_START:
                r = d->m_op_res = d->start(d->m_op_arg);
                break;

            case OP_STOP:
                r = d->m_op_res = d->stop();
                break;
            };

            // notify op done
            pthread_mutex_lock(&d->m_op_arg_mutex);
            d->m_op_id = OP_NONE;
            pthread_cond_signal(&d->m_op_arg_cond);
            pthread_mutex_unlock(&d->m_op_arg_mutex);

            // post for async
            if (OP_START == o && r)
                d->preroll();

            if (OP_EXIT == o) {
                mlt_log_debug(d->getConsumer(), "%s: exiting\n", __FUNCTION__);
                return nullptr;
            }
        };

        return nullptr;
    }

    bool open(unsigned card = 0)
    {
        unsigned i = 0;
#ifdef _WIN32
        IDeckLinkIterator *deckLinkIterator = nullptr;
        HRESULT result = CoInitialize(nullptr);
        if (FAILED(result)) {
            mlt_log_error(getConsumer(), "COM initialization failed\n");
            return false;
        }
        result = CoCreateInstance(CLSID_CDeckLinkIterator,
                                  nullptr,
                                  CLSCTX_ALL,
                                  IID_IDeckLinkIterator,
                                  (void **) &deckLinkIterator);
        if (FAILED(result)) {
            mlt_log_warning(getConsumer(), "The DeckLink drivers not installed.\n");
            return false;
        }
#else
        IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();

        if (!deckLinkIterator) {
            mlt_log_warning(getConsumer(), "The DeckLink drivers not installed.\n");
            return false;
        }
#endif

        // Connect to the Nth DeckLink instance
        for (i = 0; deckLinkIterator->Next(&m_deckLink) == S_OK; i++) {
            if (i == card)
                break;
            else
                SAFE_RELEASE(m_deckLink);
        }
        SAFE_RELEASE(deckLinkIterator);
        if (!m_deckLink) {
            mlt_log_error(getConsumer(), "DeckLink card not found\n");
            return false;
        }

        // Obtain the audio/video output interface (IDeckLinkOutput)
        if (m_deckLink->QueryInterface(IID_IDeckLinkOutput, (void **) &m_deckLinkOutput) != S_OK) {
            mlt_log_error(getConsumer(), "No DeckLink cards support output\n");
            SAFE_RELEASE(m_deckLink);
            return false;
        }

        // Get the keyer interface
        IDeckLinkProfileAttributes *deckLinkAttributes = 0;
        if (m_deckLink->QueryInterface(IID_IDeckLinkProfileAttributes, (void **) &deckLinkAttributes)
            == S_OK) {
#ifdef _WIN32
            BOOL flag = FALSE;
#else
            bool flag = false;
#endif
            if (deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInternalKeying, &flag) == S_OK
                && flag) {
                if (m_deckLink->QueryInterface(IID_IDeckLinkKeyer, (void **) &m_deckLinkKeyer)
                    != S_OK) {
                    mlt_log_error(getConsumer(), "Failed to get keyer\n");
                    SAFE_RELEASE(m_deckLinkOutput);
                    SAFE_RELEASE(m_deckLink);
                    return false;
                }
            }
            SAFE_RELEASE(deckLinkAttributes);
        }

        // Provide this class as a delegate to the audio and video output interfaces
        m_deckLinkOutput->SetScheduledFrameCompletionCallback(this);
        m_deckLinkOutput->SetAudioCallback(this);

        return true;
    }

    int preroll()
    {
        mlt_log_debug(getConsumer(), "%s: starting\n", __FUNCTION__);

        if (!m_running)
            return 0;

        mlt_log_debug(getConsumer(), "preroll %u frames\n", m_preroll);

        // preroll frames
        for (unsigned i = 0; i < m_preroll; i++)
            ScheduleNextFrame(true);

        // start audio preroll
        if (m_isAudio)
            m_deckLinkOutput->BeginAudioPreroll();
        else
            m_deckLinkOutput->StartScheduledPlayback(0, m_timescale, 1.0);

        mlt_log_debug(getConsumer(), "%s: exiting\n", __FUNCTION__);

        return 0;
    }

    bool start(unsigned preroll)
    {
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(getConsumer());

        // Initialize members
        m_count = 0;
        m_buffer = nullptr;
        preroll = preroll < PREROLL_MINIMUM ? PREROLL_MINIMUM : preroll;
        m_inChannels = mlt_properties_get_int(properties, "channels");
        if (m_inChannels <= 2) {
            m_outChannels = 2;
        } else if (m_inChannels <= 8) {
            m_outChannels = 8;
        } else {
            m_outChannels = 16;
        }
        auto channelLayout = mlt_audio_channel_layout_id(
            mlt_properties_get(properties, "channel_layout"));
        m_fix5p1 = (channelLayout == mlt_channel_auto && m_inChannels == 6)
                   || channelLayout == mlt_channel_5p1 || channelLayout == mlt_channel_5p1_back;
        m_isAudio = !mlt_properties_get_int(properties, "audio_off");
        m_terminate_on_pause = mlt_properties_get_int(properties, "terminate_on_pause");

        m_displayMode = getDisplayMode();
        if (!m_displayMode) {
            mlt_log_error(getConsumer(), "Profile is not compatible with decklink.\n");
            return false;
        }
        mlt_properties_set_int(properties,
                               "top_field_first",
                               m_displayMode->GetFieldDominance() == bmdUpperFieldFirst);

        // Set the keyer
        if (m_deckLinkKeyer && (m_isKeyer = mlt_properties_get_int(properties, "keyer"))) {
            bool external = (m_isKeyer == 2);
            double level = mlt_properties_get_double(properties, "keyer_level");

            if (m_deckLinkKeyer->Enable(external) != S_OK)
                mlt_log_error(getConsumer(),
                              "Failed to enable %s keyer\n",
                              external ? "external" : "internal");
            m_deckLinkKeyer->SetLevel(level <= 1 ? (level > 0 ? 255 * level : 255) : 255);
        } else if (m_deckLinkKeyer) {
            m_deckLinkKeyer->Disable();
        }

        // Set the video output mode
        if (S_OK
            != m_deckLinkOutput->EnableVideoOutput(m_displayMode->GetDisplayMode(),
                                                   (BMDVideoOutputFlags) (bmdVideoOutputFlagDefault
                                                                          | bmdVideoOutputRP188
                                                                          | bmdVideoOutputVITC))) {
            mlt_log_error(getConsumer(), "Failed to enable video output\n");
            return false;
        }

        // Set the audio output mode
        if (m_isAudio
            && S_OK
                   != m_deckLinkOutput->EnableAudioOutput(bmdAudioSampleRate48kHz,
                                                          bmdAudioSampleType16bitInteger,
                                                          m_outChannels,
                                                          bmdAudioOutputStreamContinuous)) {
            mlt_log_error(getConsumer(), "Failed to enable audio output\n");
            stop();
            return false;
        }

        m_preroll = preroll;
        m_reprio = 2;

        for (unsigned i = 0; i < (m_preroll + 2); i++) {
            IDeckLinkMutableVideoFrame *frame;

            // Generate a DeckLink video frame
            if (S_OK
                != m_deckLinkOutput->CreateVideoFrame(m_width,
                                                      m_height,
                                                      m_width * (m_isKeyer ? 4 : 2),
                                                      m_isKeyer ? bmdFormat8BitARGB
                                                                : bmdFormat8BitYUV,
                                                      bmdFrameFlagDefault,
                                                      &frame)) {
                mlt_log_error(getConsumer(), "%s: CreateVideoFrame (%d) failed\n", __FUNCTION__, i);
                return false;
            }

            mlt_deque_push_back(m_frames, frame);
        }

        pthread_mutex_lock(&m_refresh_mutex);
        m_refresh = 0;
        pthread_mutex_unlock(&m_refresh_mutex);

        // Set the running state
        m_running = true;

        return true;
    }

    bool stop()
    {
        if (!m_running)
            return false;
        m_running = false;

        // Unlatch the consumer thread
        pthread_mutex_lock(&m_refresh_mutex);
        pthread_cond_broadcast(&m_refresh_cond);
        pthread_mutex_unlock(&m_refresh_mutex);

        mlt_log_debug(getConsumer(), "%s: starting\n", __FUNCTION__);

        // Stop the audio and video output streams immediately
        if (m_deckLinkOutput) {
            m_deckLinkOutput->StopScheduledPlayback(0, 0, 0);
            m_deckLinkOutput->DisableAudioOutput();
            m_deckLinkOutput->DisableVideoOutput();
        }

        pthread_mutex_lock(&m_aqueue_lock);
        while (mlt_frame frame = (mlt_frame) mlt_deque_pop_back(m_aqueue))
            mlt_frame_close(frame);
        pthread_mutex_unlock(&m_aqueue_lock);

        m_buffer = nullptr;
        while (IDeckLinkMutableVideoFrame *frame
               = (IDeckLinkMutableVideoFrame *) mlt_deque_pop_back(m_frames))
            SAFE_RELEASE(frame);

        mlt_consumer_stopped(getConsumer());

        mlt_log_debug(getConsumer(), "%s: exiting\n", __FUNCTION__);

        return true;
    }

    void renderAudio(mlt_frame frame)
    {
        mlt_properties properties;
        properties = MLT_FRAME_PROPERTIES(frame);
        mlt_properties_set_int64(properties, "m_count", m_count);
        mlt_properties_inc_ref(properties);
        pthread_mutex_lock(&m_aqueue_lock);
        mlt_deque_push_back(m_aqueue, frame);
        mlt_log_debug(getConsumer(),
                      "%s:%d frame=%p, len=%d\n",
                      __FUNCTION__,
                      __LINE__,
                      frame,
                      mlt_deque_count(m_aqueue));
        pthread_mutex_unlock(&m_aqueue_lock);
    }

    void renderVideo(mlt_frame frame)
    {
        HRESULT hr;
        mlt_image_format format = m_isKeyer ? mlt_image_rgba : mlt_image_yuv422;
        uint8_t *image = 0;
        int rendered = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "rendered");
        mlt_properties consumer_properties = MLT_CONSUMER_PROPERTIES(getConsumer());
        int stride = m_width * (m_isKeyer ? 4 : 2);
        int height = m_height;
        IDeckLinkMutableVideoFrame *decklinkFrame = static_cast<IDeckLinkMutableVideoFrame *>(
            mlt_deque_pop_front(m_frames));

        mlt_log_debug(getConsumer(), "%s: entering\n", __FUNCTION__);

        m_sliced_swab = mlt_properties_get_int(consumer_properties, "sliced_swab");

        if (rendered && !mlt_frame_get_image(frame, &image, &format, &m_width, &height, 0)) {
            if (decklinkFrame) {
                IDeckLinkVideoBuffer *videoBuffer = nullptr;
                if (decklinkFrame->QueryInterface(IID_IDeckLinkVideoBuffer, (void **) &videoBuffer)
                    == S_OK) {
                    if (videoBuffer->StartAccess(bmdBufferAccessWrite) == S_OK) {
                        videoBuffer->GetBytes((void **) &m_buffer);
                        videoBuffer->EndAccess(bmdBufferAccessWrite);
                    }
                    videoBuffer->Release();
                    videoBuffer = nullptr;
                }
            }

            if (m_buffer) {
                // NTSC SDI is always 486 lines
                if (m_height == 486 && height == 480) {
                    // blank first 6 lines
                    if (m_isKeyer) {
                        memset(m_buffer, 0, stride * 6);
                        m_buffer += stride * 6;
                    } else
                        for (int i = 0; i < m_width * 6; i++) {
                            *m_buffer++ = 128;
                            *m_buffer++ = 16;
                        }
                }
                if (!m_isKeyer) {
                    unsigned char *arg[3] = {image, m_buffer};
                    ssize_t size = stride * height;

                    // Normal non-keyer playout - needs byte swapping
                    if (!m_sliced_swab)
                        swab2(arg[0], arg[1], size);
                    else {
                        arg[2] = (unsigned char *) size;
                        mlt_slices_run_fifo(0, swab_sliced, arg);
                    }
                } else if (!mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "test_image")) {
                    // Normal keyer output
                    int y = height + 1;
                    uint32_t *s = (uint32_t *) image;
                    uint32_t *d = (uint32_t *) m_buffer;

                    // Need to relocate alpha channel RGBA => ARGB
                    while (--y) {
                        int x = m_width + 1;
                        while (--x) {
                            *d++ = (*s << 8) | (*s >> 24);
                            s++;
                        }
                    }
                } else {
                    // Keying blank frames - nullify alpha
                    memset(m_buffer, 0, stride * height);
                }
            }
        } else if (decklinkFrame) {
            uint8_t *buffer = nullptr;
            IDeckLinkVideoBuffer *videoBuffer = nullptr;
            if (decklinkFrame->QueryInterface(IID_IDeckLinkVideoBuffer, (void **) &videoBuffer)
                == S_OK) {
                if (videoBuffer->StartAccess(bmdBufferAccessWrite) == S_OK) {
                    videoBuffer->GetBytes((void **) &buffer);
                    if (buffer)
                        memcpy(buffer, m_buffer, stride * height);
                    videoBuffer->EndAccess(bmdBufferAccessWrite);
                }
                videoBuffer->Release();
                videoBuffer = nullptr;
            }
        }
        if (decklinkFrame) {
            char *vitc;

            // set timecode
            vitc = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "meta.attr.vitc.markup");
            if (vitc) {
                int h, m, s, f;
                if (4 == sscanf(vitc, "%d:%d:%d:%d", &h, &m, &s, &f))
                    decklinkFrame->SetTimecodeFromComponents(bmdTimecodeVITC,
                                                             h,
                                                             m,
                                                             s,
                                                             f,
                                                             bmdTimecodeFlagDefault);
            }

            // set userbits
            vitc = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "meta.attr.vitc.userbits");
            if (vitc)
                decklinkFrame
                    ->SetTimecodeUserBits(bmdTimecodeVITC,
                                          mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame),
                                                                 "meta.attr.vitc.userbits"));

            // Set color space
            IDeckLinkVideoFrameMutableMetadataExtensions *frameMeta = nullptr;
            if (decklinkFrame->QueryInterface(IID_IDeckLinkVideoFrameMutableMetadataExtensions,
                                              (void **) &frameMeta)
                == S_OK) {
                auto colorspace = bmdColorspaceRec709;
                switch (mlt_properties_get_int(consumer_properties, "colorspace")) {
                case 601:
                    colorspace = bmdColorspaceRec601;
                    break;
                case 2020:
                    colorspace = bmdColorspaceRec2020;
                    break;
                }
                frameMeta->SetInt(bmdDeckLinkFrameMetadataColorspace, colorspace);

                // Set HDR a transfer function
                if (mlt_properties_exists(consumer_properties, "color_trc")) {
                    if (!strcmp("arib-std-b67",
                                mlt_properties_get(consumer_properties, "color_trc"))) {
                        frameMeta->SetInt(bmdDeckLinkFrameMetadataHDRElectroOpticalTransferFunc,
                                          EOTF_HLG);
                        decklinkFrame->SetFlags(decklinkFrame->GetFlags()
                                                & ~bmdFrameContainsHDRMetadata);
                    } else if (!strcmp("smpte2084",
                                       mlt_properties_get(consumer_properties, "color_trc"))) {
                        frameMeta->SetInt(bmdDeckLinkFrameMetadataHDRElectroOpticalTransferFunc,
                                          EOTF_PQ);
                        // CEA/SMPTE HDR metadata
                        // TODO: document and provide defaults for these
                        decklinkFrame->SetFlags(decklinkFrame->GetFlags()
                                                | bmdFrameContainsHDRMetadata);
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedX,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr_red_x"));
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedY,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr_red_y"));
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenX,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr_green_x"));
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenY,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr.green_y"));
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueX,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr_blue_x"));
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueY,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr_blue_y"));
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRWhitePointX,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr_white_x"));
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRWhitePointY,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr_white_y"));
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRMaxDisplayMasteringLuminance,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr_max_luminance"));
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRMinDisplayMasteringLuminance,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr_min_luminance"));
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRMaximumContentLightLevel,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr_max_cll"));
                        frameMeta->SetFloat(bmdDeckLinkFrameMetadataHDRMaximumFrameAverageLightLevel,
                                            mlt_properties_get_double(consumer_properties,
                                                                      "hdr_max_fall"));
                    }
                }
            }

            hr = m_deckLinkOutput->ScheduleVideoFrame(decklinkFrame,
                                                      m_count * m_duration,
                                                      m_duration,
                                                      m_timescale);
            if (S_OK != hr)
                mlt_log_error(getConsumer(),
                              "%s:%d: ScheduleVideoFrame failed, hr=%.8X \n",
                              __FUNCTION__,
                              __LINE__,
                              unsigned(hr));
            else
                mlt_log_debug(getConsumer(), "%s: ScheduleVideoFrame SUCCESS\n", __FUNCTION__);
        }
    }

    HRESULT render(mlt_frame frame)
    {
        HRESULT result = S_OK;

        // Get the audio
        auto scrub = mlt_properties_get_int(MLT_CONSUMER_PROPERTIES(getConsumer()), "scrub_audio");

        if (m_isAudio && (m_currentSpeed == 1 || (scrub && m_currentSpeed == 0)))
            renderAudio(frame);

        // Get the video
        renderVideo(frame);
        ++m_count;

        return result;
    }

    void reprio(int target)
    {
        int r;
        pthread_t thread;
        pthread_attr_t tattr;
        struct sched_param param;
        mlt_properties properties;

        if (m_reprio & target)
            return;

        m_reprio |= target;

        properties = MLT_CONSUMER_PROPERTIES(getConsumer());

        if (!mlt_properties_get(properties, "priority"))
            return;

        pthread_attr_init(&tattr);
        pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);

        if (!strcmp("max", mlt_properties_get(properties, "priority")))
            param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
        else if (!strcmp("min", mlt_properties_get(properties, "priority")))
            param.sched_priority = sched_get_priority_min(SCHED_FIFO) + 1;
        else
            param.sched_priority = mlt_properties_get_int(properties, "priority");

        pthread_attr_setschedparam(&tattr, &param);

        thread = pthread_self();

        r = pthread_setschedparam(thread, SCHED_FIFO, &param);
        if (r)
            mlt_log_error(getConsumer(),
                          "%s: [%d] pthread_setschedparam returned %d\n",
                          __FUNCTION__,
                          target,
                          r);
        else
            mlt_log_verbose(getConsumer(),
                            "%s: [%d] param.sched_priority=%d\n",
                            __FUNCTION__,
                            target,
                            param.sched_priority);
    }

    // *** DeckLink API implementation of IDeckLinkVideoOutputCallback IDeckLinkAudioOutputCallback *** //

    // IUnknown needs only a dummy implementation
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv)
    {
        return E_NOINTERFACE;
    }
    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        return 1;
    }
    virtual ULONG STDMETHODCALLTYPE Release()
    {
        return 1;
    }

    /************************* DeckLink API Delegate Methods *****************************/

#ifdef _WIN32
    virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples(BOOL preroll)
#else
    virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples(bool preroll)
#endif
    {
        pthread_mutex_lock(&m_aqueue_lock);
        mlt_log_debug(getConsumer(),
                      "%s: ENTERING preroll=%d, len=%d\n",
                      __FUNCTION__,
                      (int) preroll,
                      mlt_deque_count(m_aqueue));
        mlt_frame frame = (mlt_frame) mlt_deque_pop_front(m_aqueue);
        pthread_mutex_unlock(&m_aqueue_lock);

        reprio(2);

        if (frame) {
            mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
            uint64_t m_count = mlt_properties_get_int64(properties, "m_count");
            mlt_audio_format format = mlt_audio_s16;
            int frequency = bmdAudioSampleRate48kHz;
            int samples = mlt_audio_calculate_frame_samples(m_fps, frequency, m_count);
            int16_t *pcm = 0;

            if (!mlt_frame_get_audio(frame,
                                     (void **) &pcm,
                                     &format,
                                     &frequency,
                                     &m_inChannels,
                                     &samples)) {
                HRESULT hr;
                int16_t *outBuff = nullptr;
                mlt_log_debug(getConsumer(),
                              "%s:%d, samples=%d, channels=%d, freq=%d\n",
                              __FUNCTION__,
                              __LINE__,
                              samples,
                              m_inChannels,
                              frequency);

                if (m_inChannels != m_outChannels) {
                    int size = mlt_audio_format_size(format, samples, m_outChannels);
                    int16_t *src = pcm;
                    int16_t *dst = outBuff = (int16_t *) mlt_pool_alloc(size);
                    for (int s = 0; s < samples; s++) {
                        for (int c = 0; c < m_outChannels; c++) {
                            int cOut = m_fix5p1 ? c == 2 ? 3 : c == 3 ? 2 : c : c;
                            // Fill silence if there are more out channels than in channels.
                            dst[s * m_outChannels + cOut] = (c < m_inChannels)
                                                                ? src[s * m_inChannels + c]
                                                                : 0;
                        }
                    }
                    pcm = outBuff;
                }

#ifdef _WIN32
#define DECKLINK_UNSIGNED_FORMAT "%lu"
                unsigned long written = 0;
#else
#define DECKLINK_UNSIGNED_FORMAT "%u"
                uint32_t written = 0;
#endif
                BMDTimeValue streamTime = m_count * frequency * m_duration / m_timescale;
#ifdef _WIN32
                hr = m_deckLinkOutput->ScheduleAudioSamples(pcm,
                                                            samples,
                                                            streamTime,
                                                            frequency,
                                                            (unsigned int *) &written);
#else
                hr = m_deckLinkOutput->ScheduleAudioSamples(pcm,
                                                            samples,
                                                            streamTime,
                                                            frequency,
                                                            &written);
#endif
                if (S_OK != hr)
                    mlt_log_error(getConsumer(),
                                  "%s:%d ScheduleAudioSamples failed, hr=%.8X \n",
                                  __FUNCTION__,
                                  __LINE__,
                                  unsigned(hr));
                else
                    mlt_log_debug(getConsumer(),
                                  "%s:%d ScheduleAudioSamples success " DECKLINK_UNSIGNED_FORMAT
                                  " samples\n",
                                  __FUNCTION__,
                                  __LINE__,
                                  written);
                if (written != (uint32_t) samples)
                    mlt_log_verbose(getConsumer(),
                                    "renderAudio: samples=%d, written=" DECKLINK_UNSIGNED_FORMAT
                                    "\n",
                                    samples,
                                    written);

                mlt_pool_release(outBuff);
            } else
                mlt_log_error(getConsumer(),
                              "%s:%d mlt_frame_get_audio failed\n",
                              __FUNCTION__,
                              __LINE__);

            mlt_frame_close(frame);

            if (!preroll)
                RenderAudioSamples(preroll);
        }

        if (preroll)
            m_deckLinkOutput->StartScheduledPlayback(0, m_timescale, 1.0);

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted(
        IDeckLinkVideoFrame *completedFrame, BMDOutputFrameCompletionResult completed)
    {
        mlt_log_debug(getConsumer(), "%s: ENTERING\n", __FUNCTION__);

        mlt_deque_push_back(m_frames, completedFrame);

        //  change priority of video callback thread
        reprio(1);

        // When a video frame has been released by the API, schedule another video frame to be output

        // ignore handler if frame was flushed
        if (bmdOutputFrameFlushed == completed)
            return S_OK;

        // schedule next frame
        ScheduleNextFrame(false);

        // step forward frames counter if underrun
        if (bmdOutputFrameDisplayedLate == completed) {
            mlt_log_debug(getConsumer(), "ScheduledFrameCompleted: bmdOutputFrameDisplayedLate\n");
        }
        if (bmdOutputFrameDropped == completed) {
            mlt_log_debug(getConsumer(), "ScheduledFrameCompleted: bmdOutputFrameDropped\n");
            m_count++;
            ScheduleNextFrame(false);
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped()
    {
        return mlt_consumer_is_stopped(getConsumer()) ? S_FALSE : S_OK;
    }

    void ScheduleNextFrame(bool preroll)
    {
        // get the consumer
        mlt_consumer consumer = getConsumer();

        // Get the properties
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);

        // Frame and size
        mlt_frame frame = nullptr;

        mlt_log_debug(getConsumer(), "%s:%d: preroll=%d\n", __FUNCTION__, __LINE__, preroll);

        while (!frame && (m_running || preroll)) {
            mlt_log_timings_begin();
            frame = mlt_consumer_rt_frame(consumer);
            mlt_log_timings_end(nullptr, "mlt_consumer_rt_frame");
            if (frame) {
                m_currentSpeed = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "_speed");

                if (m_running) {
                    if (m_purge && m_currentSpeed == 1.0) {
                        mlt_frame_close(frame);
                        frame = nullptr;
                        m_purge = false;
                        continue;
                    } else {
                        mlt_log_timings_begin();

                        render(frame);

                        mlt_log_timings_end(nullptr, "render");
                        mlt_events_fire(properties,
                                        "consumer-frame-show",
                                        mlt_event_data_from_frame(frame));
                    }
                    if (m_currentSpeed || preroll) {
                        if (!preroll && m_previousSpeed != 1 && m_currentSpeed == 1) {
                            // Resume
                            mlt_log_verbose(getConsumer(), "Resuming foward 1x playback\n");
                            m_deckLinkOutput->StopScheduledPlayback(0, nullptr, 0);
                            m_count = 0;
                            if (m_isAudio)
                                m_deckLinkOutput->BeginAudioPreroll();
                            else
                                m_deckLinkOutput->StartScheduledPlayback(0, m_timescale, 1.0);
                        }
                    } else {
                        pthread_mutex_lock(&m_refresh_mutex);
                        if (--m_refresh <= 0)
                            pthread_cond_wait(&m_refresh_cond, &m_refresh_mutex);
                        pthread_mutex_unlock(&m_refresh_mutex);

                        mlt_consumer_purge(consumer);
                    }
                }

                // terminate on pause
                if (m_terminate_on_pause && !m_currentSpeed)
                    stop();

                mlt_frame_close(frame);
                m_previousSpeed = m_currentSpeed;
            } else
                mlt_log_warning(getConsumer(),
                                "%s: mlt_consumer_rt_frame return nullptr\n",
                                __FUNCTION__);
        }
    }
};

/** Start the consumer.
 */

static int start(mlt_consumer consumer)
{
    // Get the properties
    mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
    DeckLinkConsumer *decklink = (DeckLinkConsumer *) consumer->child;
    return decklink->op(OP_START, mlt_properties_get_int(properties, "preroll")) ? 0 : 1;
}

/** Stop the consumer.
 */

static int stop(mlt_consumer consumer)
{
    int r;

    mlt_log_debug(MLT_CONSUMER_SERVICE(consumer), "%s: entering\n", __FUNCTION__);

    // Get the properties
    DeckLinkConsumer *decklink = (DeckLinkConsumer *) consumer->child;
    r = decklink->op(OP_STOP, 0);

    mlt_log_debug(MLT_CONSUMER_SERVICE(consumer), "%s: exiting\n", __FUNCTION__);

    return r;
}

/** Determine if the consumer is stopped.
 */

static int is_stopped(mlt_consumer consumer)
{
    auto *decklink = (DeckLinkConsumer *) consumer->child;
    return !decklink->isRunning();
}

/** Close the consumer.
 */

static void close(mlt_consumer consumer)
{
    mlt_log_debug(MLT_CONSUMER_SERVICE(consumer), "%s: entering\n", __FUNCTION__);

    // Stop the consumer
    mlt_consumer_stop(consumer);

    // Close the parent
    consumer->close = nullptr;
    mlt_consumer_close(consumer);

    // Free the memory
    delete (DeckLinkConsumer *) consumer->child;

    mlt_log_debug(MLT_CONSUMER_SERVICE(consumer), "%s: exiting\n", __FUNCTION__);
}

extern "C" {

// Listen for the list_devices property to be set
static void on_property_changed(void *, mlt_consumer consumer, mlt_event_data event_data)
{
    const char *name = mlt_event_data_to_string(event_data);
    auto properties = MLT_CONSUMER_PROPERTIES(consumer);
    IDeckLinkIterator *decklinkIterator = nullptr;
    IDeckLink *decklink = nullptr;
    IDeckLinkInput *decklinkOutput = nullptr;
    int i = 0;

    if (name) {
        if (!strcmp(name, "list_devices")) {
            mlt_event_block(
                (mlt_event) mlt_properties_get_data(properties, "list-devices-event", nullptr));
        } else if (!strcmp(name, "refresh")) {
            auto *decklink = (DeckLinkConsumer *) consumer->child;
            decklink->refresh();
            return;
        } else {
            return;
        }
    }

#ifdef _WIN32
    if (FAILED(CoInitialize(nullptr)))
        return;
    if (FAILED(CoCreateInstance(CLSID_CDeckLinkIterator,
                                nullptr,
                                CLSCTX_ALL,
                                IID_IDeckLinkIterator,
                                (void **) &decklinkIterator)))
        return;
#else
    if (!(decklinkIterator = CreateDeckLinkIteratorInstance()))
        return;
#endif
    for (; decklinkIterator->Next(&decklink) == S_OK; i++) {
        if (decklink->QueryInterface(IID_IDeckLinkOutput, (void **) &decklinkOutput) == S_OK) {
            DLString name = nullptr;
            if (decklink->GetModelName(&name) == S_OK) {
                char *name_cstr = getCString(name);
                const char *format = "device.%d";
                char *key = (char *) calloc(1, strlen(format) + 10);

                sprintf(key, format, i);
                mlt_properties_set(properties, key, name_cstr);
                free(key);
                freeDLString(name);
                freeCString(name_cstr);
            }
            SAFE_RELEASE(decklinkOutput);
        }
        SAFE_RELEASE(decklink);
    }
    SAFE_RELEASE(decklinkIterator);
    mlt_properties_set_int(properties, "devices", i);
}

void purge(mlt_consumer consumer)
{
    DeckLinkConsumer *decklink = (DeckLinkConsumer *) consumer->child;
    decklink->purge();
}
/** Initialise the consumer.
 */

mlt_consumer consumer_decklink_init(mlt_profile profile,
                                    mlt_service_type type,
                                    const char *id,
                                    char *arg)
{
    // Allocate the consumer
    DeckLinkConsumer *decklink = new DeckLinkConsumer();
    mlt_consumer consumer = nullptr;

    // If allocated
    if (!mlt_consumer_init(decklink->getConsumer(), decklink, profile)) {
        // If initialises without error
        if (decklink->op(OP_OPEN, arg ? atoi(arg) : 0)) {
            consumer = decklink->getConsumer();
            mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);

            // Setup callbacks
            consumer->close = close;
            consumer->start = start;
            consumer->stop = stop;
            consumer->is_stopped = is_stopped;
            consumer->purge = purge;
            mlt_properties_set(properties, "consumer.deinterlacer", "onefield");

            mlt_event event = mlt_events_listen(properties,
                                                consumer,
                                                "property-changed",
                                                (mlt_listener) on_property_changed);
            mlt_properties_set_data(properties, "list-devices-event", event, 0, nullptr, nullptr);
        }
    }

    // Return consumer
    return consumer;
}

extern mlt_producer producer_decklink_init(mlt_profile profile,
                                           mlt_service_type type,
                                           const char *id,
                                           char *arg);

static mlt_properties metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    const char *service_type = nullptr;
    switch (type) {
    case mlt_service_consumer_type:
        service_type = "consumer";
        break;
    case mlt_service_producer_type:
        service_type = "producer";
        break;
    default:
        return nullptr;
    }
    snprintf(file, PATH_MAX, "%s/decklink/%s_%s.yml", mlt_environment("MLT_DATA"), service_type, id);
    return mlt_properties_parse_yaml(file);
}

MLT_REPOSITORY
{
    MLT_REGISTER(mlt_service_consumer_type, "decklink", consumer_decklink_init);
    MLT_REGISTER(mlt_service_producer_type, "decklink", producer_decklink_init);
    MLT_REGISTER_METADATA(mlt_service_consumer_type, "decklink", metadata, nullptr);
    MLT_REGISTER_METADATA(mlt_service_producer_type, "decklink", metadata, nullptr);
}

} // extern C
