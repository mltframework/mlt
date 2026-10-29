/*
 * conceal.h -- last good picture for avformat error concealment
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

#ifndef AVFORMAT_CONCEAL_H
#define AVFORMAT_CONCEAL_H

#include <framework/mlt.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The last good picture is either an owned frame or a cache key, never both
 * as the source of a fetch. Once the picture has been detached, owned is set
 * and fetch clones that frame. Otherwise owned is NULL and fetch loads
 * cache_position from the image cache. source_position is only the watermark
 * that tells record() whether a newer success may replace the picture.
 * While the producer is open, record and detach are the only writers of the
 * picture. fetch does not modify it. close releases the owner. reset_source
 * moves only the watermark.
 */
typedef struct avformat_conceal_s
{
    mlt_frame owned;
    mlt_position cache_position;
    int source_position;
} avformat_conceal;

void avformat_conceal_init(avformat_conceal *self);
void avformat_conceal_close(avformat_conceal *self);

/* Remember a successful frame. A source_position older than the watermark
 * does not replace the picture. With a cache, only the cache key is kept.
 * Without one, a deep copy is owned.
 */
void avformat_conceal_record(avformat_conceal *self,
                             mlt_frame frame,
                             mlt_cache cache,
                             int source_position,
                             mlt_position cache_position);

/* Copy the picture out of the cache before a put or a cache reset can drop it. */
void avformat_conceal_detach(avformat_conceal *self, mlt_cache cache);

/* Detach when this frame is older than the watermark, so caching it can evict. */
void avformat_conceal_detach_if_older(avformat_conceal *self, mlt_cache cache, int source_position);

/* A new frame the caller closes, or NULL. Prefers the owned frame after detach. */
mlt_frame avformat_conceal_fetch(avformat_conceal *self, mlt_cache cache);

/* Allow the next record() to replace the picture without dropping the one held now. */
void avformat_conceal_reset_source(avformat_conceal *self);

#ifdef __cplusplus
}
#endif

#endif
