/**
 * \file mlt_chain.h
 * \brief chain service class
 * \see mlt_chain_s
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

#ifndef MLT_CHAIN_H
#define MLT_CHAIN_H

#include "mlt_link.h"
#include "mlt_producer.h"

/** \brief Chain class
 *
 * The chain is a producer class that that can connect multiple link producers in a sequence.
 *
 * \extends mlt_producer_s
 */

struct mlt_chain_s
{
    struct mlt_producer_s parent;
    void *local; /**< \private instance object */
};

#define MLT_CHAIN_PRODUCER(chain) (&(chain)->parent)
#define MLT_CHAIN_SERVICE(chain) MLT_PRODUCER_SERVICE(MLT_CHAIN_PRODUCER(chain))
#define MLT_CHAIN_PROPERTIES(chain) MLT_SERVICE_PROPERTIES(MLT_CHAIN_SERVICE(chain))

extern mlt_chain mlt_chain_init(mlt_profile);
extern void mlt_chain_set_source(mlt_chain self, mlt_producer source);
extern mlt_producer mlt_chain_get_source(mlt_chain self);
extern int mlt_chain_attach(mlt_chain self, mlt_link link);
extern int mlt_chain_detach(mlt_chain self, mlt_link link);
extern int mlt_chain_link_count(mlt_chain self);
extern int mlt_chain_move_link(mlt_chain self, int from, int to);
extern mlt_link mlt_chain_link(mlt_chain self, int index);
extern void mlt_chain_close(mlt_chain self);
extern void mlt_chain_attach_normalizers(mlt_chain self);

#endif
