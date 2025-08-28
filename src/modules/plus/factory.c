/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2024 Meltytech, LLC
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

#include <framework/mlt.h>
#include <limits.h>
#include <string.h>
#include "mltplus_export.h"
extern mlt_consumer consumer_blipflash_init(mlt_profile profile,
                                            mlt_service_type type,
                                            const char *id,
                                            char *arg);
extern mlt_filter filter_affine_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *arg);
extern mlt_filter filter_charcoal_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg);
extern mlt_filter filter_chroma_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *arg);
extern mlt_filter filter_chroma_hold_init(mlt_profile profile,
                                          mlt_service_type type,
                                          const char *id,
                                          char *arg);
extern mlt_filter filter_dynamictext_init(mlt_profile profile,
                                          mlt_service_type type,
                                          const char *id,
                                          char *arg);
extern mlt_filter filter_dynamic_loudness_init(mlt_profile profile,
                                               mlt_service_type type,
                                               const char *id,
                                               char *arg);
extern mlt_filter filter_gradientmap_init(mlt_profile profile,
                                          mlt_service_type type,
                                          const char *id,
                                          char *arg);
extern mlt_filter filter_hslprimaries_init(mlt_profile profile,
                                           mlt_service_type type,
                                           const char *id,
                                           char *arg);
extern mlt_filter filter_hslrange_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg);
extern mlt_filter filter_invert_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *arg);
extern mlt_filter filter_lift_gamma_gain_init(mlt_profile profile,
                                              mlt_service_type type,
                                              const char *id,
                                              char *arg);
extern mlt_filter filter_loudness_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg);
extern mlt_filter filter_loudness_meter_init(mlt_profile profile,
                                             mlt_service_type type,
                                             const char *id,
                                             char *arg);
extern mlt_filter filter_lumakey_init(mlt_profile profile,
                                      mlt_service_type type,
                                      const char *id,
                                      char *arg);
extern mlt_filter filter_rgblut_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *arg);
extern mlt_filter filter_sepia_init(mlt_profile profile,
                                    mlt_service_type type,
                                    const char *id,
                                    char *arg);
extern mlt_filter filter_shape_init(mlt_profile profile,
                                    mlt_service_type type,
                                    const char *id,
                                    char *arg);
extern mlt_filter filter_spot_remover_init(mlt_profile profile,
                                           mlt_service_type type,
                                           const char *id,
                                           char *arg);
extern mlt_filter filter_strobe_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *arg);
extern mlt_filter filter_subtitle_feed_init(mlt_profile profile,
                                            mlt_service_type type,
                                            const char *id,
                                            char *arg);
extern mlt_filter filter_subtitle_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg);
extern mlt_filter filter_text_init(mlt_profile profile,
                                   mlt_service_type type,
                                   const char *id,
                                   char *arg);
extern mlt_filter filter_threshold_init(mlt_profile profile,
                                        mlt_service_type type,
                                        const char *id,
                                        char *arg);
extern mlt_filter filter_timer_init(mlt_profile profile,
                                    mlt_service_type type,
                                    const char *id,
                                    char *arg);
extern mlt_producer producer_blipflash_init(mlt_profile profile,
                                            mlt_service_type type,
                                            const char *id,
                                            char *arg);
extern mlt_producer producer_count_init(mlt_profile profile,
                                        mlt_service_type type,
                                        const char *id,
                                        char *arg);
extern mlt_producer producer_pgm_init(mlt_profile profile,
                                      mlt_service_type type,
                                      const char *id,
                                      char *arg);
extern mlt_producer producer_subtitle_init(mlt_profile profile,
                                           mlt_service_type type,
                                           const char *id,
                                           char *arg);
extern mlt_transition transition_affine_init(mlt_profile profile,
                                             mlt_service_type type,
                                             const char *id,
                                             char *arg);

#ifdef USE_FFTW
extern mlt_filter filter_dance_init(mlt_profile profile,
                                    mlt_service_type type,
                                    const char *id,
                                    char *arg);
extern mlt_filter filter_fft_init(mlt_profile profile,
                                  mlt_service_type type,
                                  const char *id,
                                  char *arg);
#endif

static mlt_properties metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    snprintf(file, PATH_MAX, "%s/plus/%s", mlt_environment("MLT_DATA"), (char *) data);
    return mlt_properties_parse_yaml(file);
}

MLTPLUS_EXPORT MLT_REPOSITORY
{
    MLT_REGISTER(mlt_service_consumer_type, "blipflash", consumer_blipflash_init);
    MLT_REGISTER(mlt_service_filter_type, "affine", filter_affine_init);
    MLT_REGISTER(mlt_service_filter_type, "charcoal", filter_charcoal_init);
    MLT_REGISTER(mlt_service_filter_type, "chroma", filter_chroma_init);
    MLT_REGISTER(mlt_service_filter_type, "chroma_hold", filter_chroma_hold_init);
    MLT_REGISTER(mlt_service_filter_type, "dynamictext", filter_dynamictext_init);
    MLT_REGISTER(mlt_service_filter_type, "dynamic_loudness", filter_dynamic_loudness_init);
    MLT_REGISTER(mlt_service_filter_type, "gradientmap", filter_gradientmap_init);
    MLT_REGISTER(mlt_service_filter_type, "hslprimaries", filter_hslprimaries_init);
    MLT_REGISTER(mlt_service_filter_type, "hslrange", filter_hslrange_init);
    MLT_REGISTER(mlt_service_filter_type, "invert", filter_invert_init);
    MLT_REGISTER(mlt_service_filter_type, "lift_gamma_gain", filter_lift_gamma_gain_init);
    MLT_REGISTER(mlt_service_filter_type, "loudness", filter_loudness_init);
    MLT_REGISTER(mlt_service_filter_type, "loudness_meter", filter_loudness_meter_init);
    MLT_REGISTER(mlt_service_filter_type, "lumakey", filter_lumakey_init);
    MLT_REGISTER(mlt_service_filter_type, "rgblut", filter_rgblut_init);
    MLT_REGISTER(mlt_service_filter_type, "sepia", filter_sepia_init);
    MLT_REGISTER(mlt_service_filter_type, "shape", filter_shape_init);
    MLT_REGISTER(mlt_service_filter_type, "spot_remover", filter_spot_remover_init);
    MLT_REGISTER(mlt_service_filter_type, "strobe", filter_strobe_init);
    MLT_REGISTER(mlt_service_filter_type, "subtitle_feed", filter_subtitle_feed_init);
    MLT_REGISTER(mlt_service_filter_type, "subtitle", filter_subtitle_init);
    MLT_REGISTER(mlt_service_filter_type, "text", filter_text_init);
    MLT_REGISTER(mlt_service_filter_type, "threshold", filter_threshold_init);
    MLT_REGISTER(mlt_service_filter_type, "timer", filter_timer_init);
    MLT_REGISTER(mlt_service_producer_type, "blipflash", producer_blipflash_init);
    MLT_REGISTER(mlt_service_producer_type, "count", producer_count_init);
    MLT_REGISTER(mlt_service_producer_type, "pgm", producer_pgm_init);
    MLT_REGISTER(mlt_service_producer_type, "subtitle", producer_subtitle_init);
    MLT_REGISTER(mlt_service_transition_type, "affine", transition_affine_init);
#ifdef USE_FFTW
    MLT_REGISTER(mlt_service_filter_type, "dance", filter_dance_init);
    MLT_REGISTER(mlt_service_filter_type, "fft", filter_fft_init);
#endif

    MLT_REGISTER_METADATA(mlt_service_consumer_type,
                          "blipflash",
                          metadata,
                          "consumer_blipflash.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "affine", metadata, "filter_affine.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "charcoal", metadata, "filter_charcoal.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "chroma", metadata, "filter_chroma.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "chroma_hold",
                          metadata,
                          "filter_chroma_hold.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "dynamictext",
                          metadata,
                          "filter_dynamictext.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "dynamic_loudness",
                          metadata,
                          "filter_dynamic_loudness.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "gradientmap",
                          metadata,
                          "filter_gradientmap.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "hslprimaries",
                          metadata,
                          "filter_hslprimaries.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "hslrange", metadata, "filter_hslrange.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "invert", metadata, "filter_invert.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "lift_gamma_gain",
                          metadata,
                          "filter_lift_gamma_gain.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "loudness", metadata, "filter_loudness.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "loudness_meter",
                          metadata,
                          "filter_loudness_meter.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "lumakey", metadata, "filter_lumakey.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "rgblut", metadata, "filter_rgblut.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "sepia", metadata, "filter_sepia.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "shape", metadata, "filter_shape.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "spot_remover",
                          metadata,
                          "filter_spot_remover.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "strobe", metadata, "filter_strobe.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type,
                          "subtitle_feed",
                          metadata,
                          "filter_subtitle_feed.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "subtitle", metadata, "filter_subtitle.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "text", metadata, "filter_text.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "threshold", metadata, "filter_threshold.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "timer", metadata, "filter_timer.yml");
    MLT_REGISTER_METADATA(mlt_service_producer_type,
                          "blipflash",
                          metadata,
                          "producer_blipflash.yml");
    MLT_REGISTER_METADATA(mlt_service_producer_type, "count", metadata, "producer_count.yml");
    MLT_REGISTER_METADATA(mlt_service_producer_type, "pgm", metadata, "producer_pgm.yml");
    MLT_REGISTER_METADATA(mlt_service_producer_type, "subtitle", metadata, "producer_subtitle.yml");
    MLT_REGISTER_METADATA(mlt_service_transition_type, "affine", metadata, "transition_affine.yml");
#ifdef USE_FFTW
    MLT_REGISTER_METADATA(mlt_service_filter_type, "dance", metadata, "filter_dance.yml");
    MLT_REGISTER_METADATA(mlt_service_filter_type, "fft", metadata, "filter_fft.yml");
#endif
}
