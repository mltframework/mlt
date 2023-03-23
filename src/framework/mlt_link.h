/**
 * \file mlt_link.h
 * \brief link service class
 * \see mlt_link_s
 *
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

#ifndef MLT_LINK_H
#define MLT_LINK_H

#include "mlt_producer.h"

/** \brief Link class
 *
 * The link is a producer class that that can be connected to other link producers in a Chain.
 *
 * \extends mlt_producer_s
 * \properties \em next holds a reference to the next producer in the chain
 */

struct mlt_link_s
{
    /** \publicsection */
    struct mlt_producer_s parent;

    /** \protectedsection */

    /** Get a frame of data (virtual function).
	 *
	 * \param mlt_link a link
	 * \param mlt_frame_ptr a frame pointer by reference
	 * \param int an index
	 * \return true if there was an error
	 */
    int (*get_frame)(mlt_link, mlt_frame_ptr, int);

    /** Configure the link (virtual function).
	 *
	 * \param mlt_link a link
	 * \param mlt_profile a default profile to use
	 */
    void (*configure)(mlt_link, mlt_profile);

    /** Virtual close function */
    void (*close)(mlt_link);

    /** \privatesection */
    mlt_producer next;
    /** the object of a subclass */
    void *child;
};

#define MLT_LINK_PRODUCER(link) (&(link)->parent)
#define MLT_LINK_SERVICE(link) MLT_PRODUCER_SERVICE(MLT_LINK_PRODUCER(link))
#define MLT_LINK_PROPERTIES(link) MLT_SERVICE_PROPERTIES(MLT_LINK_SERVICE(link))

extern mlt_link mlt_link_init();
extern int mlt_link_connect_next(mlt_link self, mlt_producer next, mlt_profile chain_profile);
extern void mlt_link_close(mlt_link self);

// Link filter wrapper functions
extern mlt_link mlt_link_filter_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *arg);
extern mlt_properties mlt_link_filter_metadata(mlt_service_type type, const char *id, void *data);

#endif
