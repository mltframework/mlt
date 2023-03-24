/**
 * MltAudio.h - MLT Wrapper
 * Copyright (C) 2020 Meltytech, LLC
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

#ifndef MLTPP_AUDIO_H
#define MLTPP_AUDIO_H

#include "MltConfig.h"

#include <framework/mlt.h>

namespace Mlt {
class MLTPP_DECLSPEC Audio
{
private:
    mlt_audio instance;

public:
    Audio();
    Audio(mlt_audio audio);
    virtual ~Audio();
    void *data();
    void set_data(void *data);
    int frequency();
    void set_frequency(int frequency);
    mlt_audio_format format();
    void set_format(mlt_audio_format format);
    int samples();
    void set_samples(int samples);
    int channels();
    void set_channels(int channels);
    mlt_channel_layout layout();
    void set_layout(mlt_channel_layout layout);
};
} // namespace Mlt

#endif
