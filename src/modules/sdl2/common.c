/*
 * common.h
 * Copyright (C) 2018 Meltytech, LLC
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

#include <framework/mlt_log.h>

SDL_AudioDeviceID sdl2_open_audio(const SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
    SDL_AudioDeviceID dev = 0;

    // First try to open using default/user requested driver.
    dev = SDL_OpenAudioDevice(NULL, 0, desired, obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);

    if (dev == 0) {
        mlt_log_info(NULL, "Failed to open audio device: %s\n", SDL_GetError());

        // Try alternative drivers.
        int i = 0;
        int driver_count = SDL_GetNumAudioDrivers();

        for (i = 0; i < driver_count; i++) {
            const char *driver = SDL_GetAudioDriver(i);
            if (strcmp(driver, "disk") == 0 || strcmp(driver, "dummy") == 0) {
                continue;
            }
            if (SDL_AudioInit(driver) != 0) {
                continue;
            }
            mlt_log_info(NULL, "[sdl2] Try alternative driver: %s\n", driver);
            dev = SDL_OpenAudioDevice(NULL, 0, desired, obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
            if (dev != 0) {
                break;
            } else {
                mlt_log_info(NULL, "[sdl2] Open failed: %s\n", SDL_GetError());
            }
        }
    }

    if (dev == 0 && desired->channels > 2) {
        // All drivers have failed to open with the provided spec.
        // Try stereo channels since all drivers support that.
        mlt_log_info(NULL, "Failed to open surround device. Try stereo instead\n");
        SDL_AudioSpec desired_copy = *desired;
        desired_copy.channels = 2;
        SDL_AudioInit(NULL);
        dev = sdl2_open_audio(&desired_copy, obtained);
    }

    return dev;
}
