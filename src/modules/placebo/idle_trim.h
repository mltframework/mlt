/*
 * idle_trim.h -- release cached GPU objects that have not been used for a while
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

#ifndef IDLE_TRIM_H
#define IDLE_TRIM_H

#include <libplacebo/gpu.h>

#include <stdint.h>

/* The texture pool and the renderer cache keep objects between frames so each
   frame does not allocate video memory. When frames stop, one background thread
   asks each cache to release what has been idle for too long, so a stopped
   player does not keep it. */

/* A cache's trim function. It runs with the render lock held and returns how
   many objects it released. */
typedef int (*placebo_trim_fn)(pl_gpu gpu);

/* Register a trim function, once, and start the thread if it is not running.
   A cache calls this when it creates an object. */
void placebo_idle_trim_start(placebo_trim_fn trim);

/* Stop the thread. Call it without the render lock, before the GPU is released,
   because the thread takes the render lock to do its work. */
void placebo_idle_trim_stop(void);

/* How long an object may be idle, in milliseconds. The environment variable
   MLT_PLACEBO_IDLE_MS overrides the default, for tests. */
int placebo_idle_ms(void);

/* Milliseconds from a clock that does not follow changes to the system time. */
int64_t placebo_now_ms(void);

#endif /* IDLE_TRIM_H */
