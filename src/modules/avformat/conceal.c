/*
 * conceal.c -- last good picture for avformat error concealment
 * Copyright (C) 2026 Meltytech, LLC
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

#include "conceal.h"

/* Not a valid image-cache key. Zero is a valid key, so the empty state cannot be 0. */
#define CONCEAL_NO_CACHE_POSITION ((mlt_position) -1)

void avformat_conceal_init(avformat_conceal *self)
{
    self->owned = NULL;
    self->cache_position = CONCEAL_NO_CACHE_POSITION;
    /* Same initial watermark as the previous zeroed int: position 0 is recorded. */
    self->source_position = 0;
}

void avformat_conceal_close(avformat_conceal *self)
{
    if (self->owned)
        mlt_frame_close(self->owned);
    self->owned = NULL;
    self->cache_position = CONCEAL_NO_CACHE_POSITION;
}

void avformat_conceal_record(avformat_conceal *self,
                             mlt_frame frame,
                             mlt_cache cache,
                             int source_position,
                             mlt_position cache_position)
{
    if (source_position < self->source_position)
        return;

    self->source_position = source_position;
    if (self->owned)
        mlt_frame_close(self->owned);
    if (cache) {
        self->owned = NULL;
        self->cache_position = cache_position;
    } else {
        self->owned = mlt_frame_clone(frame, 1);
        self->cache_position = CONCEAL_NO_CACHE_POSITION;
    }
}

void avformat_conceal_detach(avformat_conceal *self, mlt_cache cache)
{
    if (!self->owned && cache && self->cache_position != CONCEAL_NO_CACHE_POSITION)
        self->owned = mlt_cache_get_frame(cache, self->cache_position);
}

void avformat_conceal_detach_if_older(avformat_conceal *self, mlt_cache cache, int source_position)
{
    if (source_position < self->source_position)
        avformat_conceal_detach(self, cache);
}

mlt_frame avformat_conceal_fetch(avformat_conceal *self, mlt_cache cache)
{
    if (self->owned)
        return mlt_frame_clone(self->owned, 1);
    if (cache && self->cache_position != CONCEAL_NO_CACHE_POSITION)
        return mlt_cache_get_frame(cache, self->cache_position);
    return NULL;
}

void avformat_conceal_reset_source(avformat_conceal *self)
{
    self->source_position = -1;
}
