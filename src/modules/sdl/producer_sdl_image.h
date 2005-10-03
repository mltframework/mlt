/*
 * producer_sdl_image.h -- Image loader which wraps SDL_image
 * Copyright (C) 2005 Visual Media FX
 * Author: Charles Yates <charles.yates@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef PRODUCER_SDL_IMAGE_H_
#define PRODUCER_SDL_IMAGE_H_

#include <framework/mlt_producer.h>

extern mlt_producer producer_sdl_image_init( char *filename );

#endif
