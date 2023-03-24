/**
 * \file mlt_events.h
 * \brief event handling
 * \see mlt_events_struct
 *
 * Copyright (C) 2004-2021 Meltytech, LLC
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

#ifndef MLT_EVENTS_H
#define MLT_EVENTS_H

#include "mlt_types.h"

/** A container for data that may be supplied with an event */
typedef struct
{
    union {
        int i;
        void *p;
    } u;
} mlt_event_data;

/** An event data structure to convey thread parameters */
typedef struct
{
    void **thread; /**< a pointer to a thread object or handle as determined by you */
    int *priority; /**< a priority level for the thread */
    mlt_thread_function_t function; /**< a pointer to the function that thread will run */
    void *data;                     /**< an opaque data pointer to pass along */
} mlt_event_data_thread;

/** event handler when receiving an event message
 * \param the properties object on which the event was registered
 * \param an opaque pointer to the listener's data
 * \param an event data object
 */
typedef void (*mlt_listener)(mlt_properties, void *, mlt_event_data);

extern void mlt_events_init(mlt_properties self);
extern int mlt_events_register(mlt_properties self, const char *id);
extern int mlt_events_fire(mlt_properties self, const char *id, mlt_event_data);
extern mlt_event mlt_events_listen(mlt_properties self,
                                   void *listener_data,
                                   const char *id,
                                   mlt_listener listener);
extern void mlt_events_block(mlt_properties self, void *listener_data);
extern void mlt_events_unblock(mlt_properties self, void *listener_data);
extern void mlt_events_disconnect(mlt_properties self, void *listener_data);

extern mlt_event mlt_events_setup_wait_for(mlt_properties self, const char *id);
extern void mlt_events_wait_for(mlt_properties self, mlt_event event);
extern void mlt_events_close_wait_for(mlt_properties self, mlt_event event);

extern void mlt_event_inc_ref(mlt_event self);
extern void mlt_event_block(mlt_event self);
extern void mlt_event_unblock(mlt_event self);
extern void mlt_event_close(mlt_event self);

extern mlt_event_data mlt_event_data_none();
extern mlt_event_data mlt_event_data_from_int(int value);
extern int mlt_event_data_to_int(mlt_event_data);
extern mlt_event_data mlt_event_data_from_string(const char *value);
extern const char *mlt_event_data_to_string(mlt_event_data);
extern mlt_event_data mlt_event_data_from_frame(mlt_frame);
extern mlt_frame mlt_event_data_to_frame(mlt_event_data);
extern mlt_event_data mlt_event_data_from_object(void *);
extern void *mlt_event_data_to_object(mlt_event_data);

#endif
