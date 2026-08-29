/*
 * common.h
 * Copyright (C) 2018-2026 Meltytech, LLC
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

#ifndef COMMON_H
#define COMMON_H

#include <SDL.h>

#define SDL_AUDIO_BUFFER_MIN_BYTES (2048 * 2 * (int) sizeof(float) * 2)

SDL_AudioDeviceID sdl2_open_audio(const SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
int sdl2_ensure_buffer_capacity(uint8_t **buffer, int *capacity, int minimum);

#endif // COMMON_H
