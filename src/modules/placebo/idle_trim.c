/*
 * idle_trim.c -- release cached GPU objects that have not been used for a while
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

#include "idle_trim.h"

#include "gpu_context.h"

#include <framework/mlt_factory.h>
#include <framework/mlt_log.h>
#include <framework/mlt_properties.h>

#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define INTERVAL_SECONDS 1
#define DEFAULT_IDLE_MS 5000
#define MAX_TRIMMERS 4

/* s_mutex guards everything below and is never held while waiting for the
   render lock. */
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_wake = PTHREAD_COND_INITIALIZER;
static pthread_t s_thread;
static int s_running;
static int s_stop;
static placebo_trim_fn s_trimmers[MAX_TRIMMERS];
static int s_trimmer_count;

int placebo_idle_ms(void)
{
    static int idle_ms;

    if (!idle_ms) {
        const char *env = getenv("MLT_PLACEBO_IDLE_MS");
        idle_ms = env && atoi(env) > 0 ? atoi(env) : DEFAULT_IDLE_MS;
    }
    return idle_ms;
}

int64_t placebo_now_ms(void)
{
#ifdef _WIN32
    return (int64_t) GetTickCount64();
#else
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

/* Run every trim function, and add what they released to the global property
   "placebo.trimmed", so a test can see that idle objects were released. The GPU
   is read under the render lock, which placebo_gpu_release holds while it tears
   the caches down, so it cannot be freed in between. */
static void trim_all(void)
{
    placebo_trim_fn trimmers[MAX_TRIMMERS];
    int count;
    int released = 0;

    pthread_mutex_lock(&s_mutex);
    count = s_trimmer_count;
    for (int i = 0; i < count; i++)
        trimmers[i] = s_trimmers[i];
    pthread_mutex_unlock(&s_mutex);

    placebo_render_lock();
    pl_gpu gpu = placebo_gpu_peek();
    for (int i = 0; gpu && i < count; i++)
        released += trimmers[i](gpu);
    placebo_render_unlock();
    if (released) {
        mlt_properties global = mlt_global_properties();
        mlt_properties_set_int(global,
                               "placebo.trimmed",
                               mlt_properties_get_int(global, "placebo.trimmed") + released);
    }
}

/* The wait uses the system time, like every timed wait in MLT: winpthreads and
   macOS cannot give a condition variable another clock. A change of the system
   time only moves one tick, and the ages it checks use a monotonic clock. */
static void *trim_main(void *arg)
{
    (void) arg;
    pthread_mutex_lock(&s_mutex);
    while (!s_stop) {
        struct timeval now;
        struct timespec until;

        gettimeofday(&now, NULL);
        until.tv_sec = now.tv_sec + INTERVAL_SECONDS;
        until.tv_nsec = now.tv_usec * 1000;
        pthread_cond_timedwait(&s_wake, &s_mutex, &until);
        if (s_stop)
            break;
        pthread_mutex_unlock(&s_mutex);
        trim_all();
        pthread_mutex_lock(&s_mutex);
    }
    pthread_mutex_unlock(&s_mutex);
    return NULL;
}

void placebo_idle_trim_start(placebo_trim_fn trim)
{
    int known = 0;

    pthread_mutex_lock(&s_mutex);
    for (int i = 0; i < s_trimmer_count; i++)
        known |= s_trimmers[i] == trim;
    if (!known) {
        static int warned;

        if (s_trimmer_count < MAX_TRIMMERS)
            s_trimmers[s_trimmer_count++] = trim;
        else if (!warned++)
            mlt_log_error(NULL, "[placebo] Too many idle trim functions; one is never run\n");
    }
    if (!s_running) {
        s_stop = 0;
        s_running = pthread_create(&s_thread, NULL, trim_main, NULL) == 0;
    }
    pthread_mutex_unlock(&s_mutex);
}

void placebo_idle_trim_stop(void)
{
    pthread_mutex_lock(&s_mutex);
    int running = s_running;
    s_stop = 1;
    pthread_cond_signal(&s_wake);
    pthread_mutex_unlock(&s_mutex);

    if (running) {
        pthread_join(s_thread, NULL);
        pthread_mutex_lock(&s_mutex);
        s_running = 0;
        pthread_mutex_unlock(&s_mutex);
    }
}
