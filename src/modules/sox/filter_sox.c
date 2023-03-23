/*
 * filter_sox.c -- apply any number of SOX effects using libst
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_tokeniser.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: does not support multiple effects with SoX v14.1.0+

#ifdef SOX14
#include <sox.h>
#define ST_EOF SOX_EOF
#define ST_SUCCESS SOX_SUCCESS
#define st_sample_t sox_sample_t
#define eff_t sox_effect_t *
#define ST_LIB_VERSION_CODE SOX_LIB_VERSION_CODE
#define ST_LIB_VERSION SOX_LIB_VERSION
#if (ST_LIB_VERSION_CODE >= ST_LIB_VERSION(14, 2, 0))
#define st_size_t size_t
#else
#define st_size_t sox_size_t
#endif
#define ST_SIGNED_WORD_TO_SAMPLE(d, clips) SOX_SIGNED_16BIT_TO_SAMPLE(d, clips)
#if (ST_LIB_VERSION_CODE >= ST_LIB_VERSION(14, 1, 0))
#define ST_SSIZE_MIN SOX_SAMPLE_MIN
#else
#define ST_SSIZE_MIN SOX_SSIZE_MIN
#endif
#define ST_SAMPLE_TO_SIGNED_WORD(d, clips) SOX_SAMPLE_TO_SIGNED_16BIT(d, clips)
#else
#include <st.h>
#endif

#define BUFFER_LEN 8192
#define AMPLITUDE_NORM 0.2511886431509580 /* -12dBFS */
#define AMPLITUDE_MIN 0.00001
#define DBFSTOAMP(x) pow(10, (x) / 20.0)

/** Compute the mean of a set of doubles skipping unset values flagged as -1
*/
static inline double mean(double *buf, int count)
{
    double mean = 0;
    int i;
    int j = 0;

    for (i = 0; i < count; i++) {
        if (buf[i] != -1.0) {
            mean += buf[i];
            j++;
        }
    }
    if (j > 0)
        mean /= j;

    return mean;
}

#if (ST_LIB_VERSION_CODE >= ST_LIB_VERSION(14, 1, 0))
static void delete_effect(eff_t effp)
{
    free(effp->priv);
    free((void *) effp->in_encoding);
    free(effp);
}
#endif

/** Create an effect state instance for a channels
*/
static int create_effect(mlt_filter this, char *value, int count, int channel, int frequency)
{
    mlt_tokeniser tokeniser = mlt_tokeniser_init();
    char id[256];
    int error = 1;

    // Tokenise the effect specification
    mlt_tokeniser_parse_new(tokeniser, value, " ");
    if (tokeniser->count < 1) {
        mlt_tokeniser_close(tokeniser);
        return error;
    }

    // Locate the effect
    mlt_destructor effect_destructor = mlt_pool_release;
#ifdef SOX14
    //fprintf(stderr, "%s: effect %s count %d\n", __FUNCTION__, tokeniser->tokens[0], tokeniser->count );
#if (ST_LIB_VERSION_CODE >= ST_LIB_VERSION(14, 1, 0))
    sox_effect_handler_t const *eff_handle = sox_find_effect(tokeniser->tokens[0]);
    if (eff_handle == NULL)
        return error;
    eff_t eff = sox_create_effect(eff_handle);
    effect_destructor = (mlt_destructor) delete_effect;
    sox_encodinginfo_t *enc = calloc(1, sizeof(sox_encodinginfo_t));
    enc->encoding = SOX_ENCODING_SIGN2;
    enc->bits_per_sample = 16;
    eff->in_encoding = eff->out_encoding = enc;
#else
    eff_t eff = mlt_pool_alloc(sizeof(sox_effect_t));
    sox_create_effect(eff, sox_find_effect(tokeniser->tokens[0]));
#endif
    int opt_count = tokeniser->count - 1;
#else
    eff_t eff = mlt_pool_alloc(sizeof(struct st_effect));
    int opt_count = st_geteffect_opt(eff, tokeniser->count, tokeniser->tokens);
#endif

    // If valid effect
    if (opt_count != ST_EOF) {
        // Supply the effect parameters
#ifdef SOX14
#if (ST_LIB_VERSION_CODE >= ST_LIB_VERSION(14, 2, 0))
        if (sox_effect_options(eff, opt_count, &tokeniser->tokens[tokeniser->count > 1 ? 1 : 0])
            == ST_SUCCESS)
#else
        if ((*eff->handler.getopts)(eff, opt_count, &tokeniser->tokens[tokeniser->count > 1 ? 1 : 0])
            == ST_SUCCESS)
#endif
#else
        if ((*eff->h->getopts)(eff, opt_count, &tokeniser->tokens[tokeniser->count - opt_count])
            == ST_SUCCESS)
#endif
        {
            // Set the sox signal parameters
#if (ST_LIB_VERSION_CODE >= ST_LIB_VERSION(14, 1, 0))
            eff->in_signal.rate = frequency;
            eff->out_signal.rate = frequency;
            eff->in_signal.channels = 1;
            eff->out_signal.channels = 1;
            eff->in_signal.precision = 16;
            eff->out_signal.precision = 16;
            eff->in_signal.length = 0;
            eff->out_signal.length = 0;
#else
            eff->ininfo.rate = frequency;
            eff->outinfo.rate = frequency;
            eff->ininfo.channels = 1;
            eff->outinfo.channels = 1;
#endif

            // Start the effect
#ifdef SOX14
            if ((*eff->handler.start)(eff) == ST_SUCCESS)
#else
            if ((*eff->h->start)(eff) == ST_SUCCESS)
#endif
            {
                // Construct id
                sprintf(id, "_effect_%d_%d", count, channel);

                // Save the effect state
                mlt_properties_set_data(MLT_FILTER_PROPERTIES(this),
                                        id,
                                        eff,
                                        0,
                                        effect_destructor,
                                        NULL);
                error = 0;
            }
        }
    }
    // Some error occurred so delete the temp effect state
    if (error == 1)
        effect_destructor(eff);

    mlt_tokeniser_close(tokeniser);

    return error;
}

/** Get the audio.
*/

static int filter_get_audio(mlt_frame frame,
                            void **buffer,
                            mlt_audio_format *format,
                            int *frequency,
                            int *channels,
                            int *samples)
{
#if (ST_LIB_VERSION_CODE >= ST_LIB_VERSION(14, 3, 0))
    SOX_SAMPLE_LOCALS;
#endif
    // Get the filter service
    mlt_filter filter = mlt_frame_pop_audio(frame);

    // Get the filter properties
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);

    mlt_service_lock(MLT_FILTER_SERVICE(filter));

    // Get the properties
    st_sample_t *input_buffer; // = mlt_properties_get_data( filter_properties, "input_buffer", NULL );
    st_sample_t *output_buffer = mlt_properties_get_data(filter_properties, "output_buffer", NULL);
    int i; // channel
    int count = mlt_properties_get_int(filter_properties, "_effect_count");
    int analysis = mlt_properties_get(filter_properties, "effect")
                   && !strcmp(mlt_properties_get(filter_properties, "effect"), "analysis");

    // Get the producer's audio
    *format = mlt_audio_s32;
    mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);

    // Even though some effects are multi-channel aware, it is not reliable
    // We must maintain a separate effect state for each channel
    for (i = 0; i < *channels; i++) {
        char id[256];
        sprintf(id, "_effect_0_%d", i);

        // Get an existing effect state
        eff_t e = mlt_properties_get_data(filter_properties, id, NULL);

        // Validate the existing effect state
#if (ST_LIB_VERSION_CODE >= ST_LIB_VERSION(14, 1, 0))
        if (e != NULL && (e->in_signal.rate != *frequency || e->out_signal.rate != *frequency))
#else
        if (e != NULL && (e->ininfo.rate != *frequency || e->outinfo.rate != *frequency))
#endif
            e = NULL;

        // (Re)Create the effect state
        if (e == NULL) {
            int j = 0;

            // Reset the count
            count = 0;

            // Loop over all properties
            for (j = 0; j < mlt_properties_count(filter_properties); j++) {
                // Get the name of this property
                char *name = mlt_properties_get_name(filter_properties, j);

                // If the name does not contain a . and matches effect
                if (!strncmp(name, "effect", 6)) {
                    // Get the effect specification
                    char *value = mlt_properties_get_value(filter_properties, j);

                    // Create an instance
                    if (create_effect(filter, value, count, i, *frequency) == 0)
                        count++;
                }
            }

            // Save the number of filters
            mlt_properties_set_int(filter_properties, "_effect_count", count);
        }
        if (*samples > 0 && (count > 0 || analysis)) {
            input_buffer = (st_sample_t *) *buffer + i * *samples;
            st_sample_t *p = input_buffer;
            st_size_t isamp = *samples;
            st_size_t osamp = *samples;
            int j = *samples + 1;
            int normalize = mlt_properties_get_int(filter_properties, "normalize")
                            || mlt_properties_get_int(filter_properties, "normalise");
            double normalized_gain = 1.0;

            if (analysis) {
                // Run analysis to compute a gain level to normalize the audio across entire filter duration
                double max_power = mlt_properties_get_double(filter_properties, "_max_power");
                double peak = mlt_properties_get_double(filter_properties, "_max_peak");
                double use_peak = mlt_properties_get_int(filter_properties, "use_peak");
                double power = 0;
                int n = *samples + 1;

                // Compute power level of samples in this channel of this frame
                while (--n) {
                    double s = abs(*p++);
                    // Track peak
                    if (s > peak) {
                        peak = s;
                        mlt_properties_set_double(filter_properties, "_max_peak", peak);
                    }
                    power += s * s;
                }
                power /= *samples;
                // Track maximum power
                if (power > max_power) {
                    max_power = power;
                    mlt_properties_set_double(filter_properties, "_max_power", max_power);
                }

                // Complete analysis the last channel of the last frame.
                if (i + 1 == *channels
                    && mlt_filter_get_position(filter, frame) + 1
                           == mlt_filter_get_length2(filter, frame)) {
                    double rms = sqrt(max_power / ST_SSIZE_MIN / ST_SSIZE_MIN);
                    char effect[32];

                    // Convert RMS or peak to gain
                    if (use_peak)
                        normalized_gain = ST_SSIZE_MIN / -peak;
                    else {
                        double gain = DBFSTOAMP(-12); // default -12 dBFS
                        char *p = mlt_properties_get(filter_properties, "analysis_level");
                        if (p) {
                            gain = mlt_properties_get_double(filter_properties, "analysis_level");
                            if (strstr(p, "dB"))
                                gain = DBFSTOAMP(gain);
                        }
                        normalized_gain = gain / rms;
                    }

                    // Set properties for serialization
                    snprintf(effect, sizeof(effect), "vol %f", normalized_gain);
                    effect[31] = 0;
                    mlt_properties_set(filter_properties, "effect", effect);
                    mlt_properties_set(filter_properties, "analyze", NULL);

                    // Show output comparable to normalize --no-adjust --fractions
                    mlt_properties_set_double(filter_properties, "level", rms);
                    mlt_properties_set_double(filter_properties, "gain", normalized_gain);
                    mlt_properties_set_double(filter_properties, "peak", -peak / ST_SSIZE_MIN);
                }

                // restore some variables
                p = input_buffer;
            }

            if (normalize) {
                int window = mlt_properties_get_int(filter_properties, "window");
                double *smooth_buffer = mlt_properties_get_data(filter_properties,
                                                                "smooth_buffer",
                                                                NULL);
                double max_gain = mlt_properties_get_double(filter_properties, "max_gain");
                double rms = 0;

                // Default the maximum gain factor to 20dBFS
                if (max_gain == 0)
                    max_gain = 10.0;

                // Compute rms amplitude
                while (--j) {
                    rms += (double) *p * (double) *p;
                    p++;
                }
                rms = sqrt(rms / *samples / ST_SSIZE_MIN / ST_SSIZE_MIN);

                // The smoothing buffer prevents radical shifts in the gain level
                if (window > 0 && smooth_buffer != NULL) {
                    int smooth_index = mlt_properties_get_int(filter_properties, "_smooth_index");
                    smooth_buffer[smooth_index] = rms;

                    // Ignore very small values that adversely affect the mean
                    if (rms > AMPLITUDE_MIN)
                        mlt_properties_set_int(filter_properties,
                                               "_smooth_index",
                                               (smooth_index + 1) % window);

                    // Smoothing is really just a mean over the past N values
                    normalized_gain = AMPLITUDE_NORM / mean(smooth_buffer, window);
                } else if (rms > 0) {
                    // Determine gain to apply as current amplitude
                    normalized_gain = AMPLITUDE_NORM / rms;
                }

                //printf("filter_sox: rms %.3f gain %.3f\n", rms, normalized_gain );

                // Govern the maximum gain
                if (normalized_gain > max_gain)
                    normalized_gain = max_gain;
            }

            // For each effect
            for (j = 0; j < count; j++) {
                sprintf(id, "_effect_%d_%d", j, i);
                e = mlt_properties_get_data(filter_properties, id, NULL);

                // We better have this guy
                if (e != NULL) {
                    float saved_gain = 1.0;

                    // XXX: hack to apply the normalized gain level to the vol effect
#ifdef SOX14
                    if (normalize && strcmp(e->handler.name, "vol") == 0)
#else
                    if (normalize && strcmp(e->name, "vol") == 0)
#endif
                    {
                        float *f = (float *) (e->priv);
                        saved_gain = *f;
                        *f = saved_gain * normalized_gain;
                    }

                    // Apply the effect
#ifdef SOX14
                    if ((*e->handler.flow)(e, input_buffer, output_buffer, &isamp, &osamp)
                        != ST_SUCCESS)
#else
                    if ((*e->h->flow)(e, input_buffer, output_buffer, &isamp, &osamp) != ST_SUCCESS)
#endif
                    {
                        mlt_log_warning(MLT_FILTER_SERVICE(filter), "effect processing failed\n");
                    }

                    // XXX: hack to restore the original vol gain to prevent accumulation
#ifdef SOX14
                    if (normalize && strcmp(e->handler.name, "vol") == 0)
#else
                    if (normalize && strcmp(e->name, "vol") == 0)
#endif
                    {
                        float *f = (float *) (e->priv);
                        *f = saved_gain;
                    }
                }
            }

            // Write back
            memcpy(input_buffer, output_buffer, *samples * sizeof(st_sample_t));
        }
    }

    mlt_service_unlock(MLT_FILTER_SERVICE(filter));

    return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter this, mlt_frame frame)
{
    if (mlt_frame_is_test_audio(frame) == 0) {
        // Add the filter to the frame
        mlt_frame_push_audio(frame, this);
        mlt_frame_push_audio(frame, filter_get_audio);

        // Parse the window property and allocate smoothing buffer if needed
        mlt_properties properties = MLT_FILTER_PROPERTIES(this);
        int window = mlt_properties_get_int(properties, "window");
        if (mlt_properties_get(properties, "smooth_buffer") == NULL && window > 1) {
            // Create a smoothing buffer for the calculated "max power" of frame of audio used in normalization
            double *smooth_buffer = (double *) calloc(window, sizeof(double));
            int i;
            for (i = 0; i < window; i++)
                smooth_buffer[i] = -1.0;
            mlt_properties_set_data(properties, "smooth_buffer", smooth_buffer, 0, free, NULL);
        }
    }

    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_sox_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter this = mlt_filter_new();
    if (this != NULL) {
        void *input_buffer = mlt_pool_alloc(BUFFER_LEN);
        void *output_buffer = mlt_pool_alloc(BUFFER_LEN);
        mlt_properties properties = MLT_FILTER_PROPERTIES(this);

        this->process = filter_process;

        if (!strncmp(id, "sox.", 4)) {
            char *s = malloc(strlen(id) + (arg ? strlen(arg) + 2 : 1));
            strcpy(s, id + 4);
            if (arg) {
                strcat(s, " ");
                strcat(s, arg);
            }
            mlt_properties_set(properties, "effect", s);
            free(s);
        } else if (arg)
            mlt_properties_set(properties, "effect", arg);
        mlt_properties_set_data(properties,
                                "input_buffer",
                                input_buffer,
                                BUFFER_LEN,
                                mlt_pool_release,
                                NULL);
        mlt_properties_set_data(properties,
                                "output_buffer",
                                output_buffer,
                                BUFFER_LEN,
                                mlt_pool_release,
                                NULL);
        mlt_properties_set_int(properties, "window", 75);
        mlt_properties_set(properties, "version", sox_version());
    }
    return this;
}

// What to do when a libst internal failure occurs
void cleanup(void) {}
