/*
 * JACK Rack
 *
 * Original:
 * Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
 *
 * Modification for MLT:
 * Copyright (C) 2004-2014 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "lock_free_fifo.h"

/** initialise a lock free fifo */

void
lff_init (lff_t * lff, unsigned int size, size_t object_size)
{
  lff->size = size;
  lff->object_size = object_size;
  lff->read_index = 0;
  lff->write_index = 0;
  lff->data = g_malloc (object_size * size);
}

lff_t *
lff_new (unsigned int size, size_t object_size)
{
  lff_t * lff;
  
  lff = g_malloc (sizeof (lff_t));
  
  lff_init (lff, size, object_size);
  
  return lff;
}

void
lff_free (lff_t * lff)
{
  g_free (lff->data);
}

void
lff_destroy (lff_t * lff)
{
  lff_free (lff);
  g_free (lff);
}

/** read an element from the fifo into data.
returns 0 on success, non-zero if there were no elements to read */
int lff_read (lff_t * lff, void * data) {
  if (lff->read_index == lff->write_index) {
    return -1;
  } else {
    memcpy (data, ((char *)lff->data) + (lff->read_index * lff->object_size),
            lff->object_size);
    lff->read_index++;
    if (lff->read_index >= lff->size) {
      lff->read_index = 0;
    }
    return 0;
  }
}

/** write an element from data to the fifo.
returns 0 on success, non-zero if there was no space */
int lff_write (lff_t * lff, void * data) {
  static unsigned int ri;
  
  /* got to read read_index only once for safety */
  ri = lff->read_index;

  /* lots of logic for when we're allowed to write to the fifo which basically
     boils down to "don't write if we're one element behind the read index" */  
  if ((ri > lff->write_index && ri - lff->write_index > 1) ||
      (lff->write_index >= ri && lff->write_index != lff->size - 1) ||
      (lff->write_index >= ri && lff->write_index == lff->size - 1 && ri != 0)) { 

/*  if ((ri > lff->write_index && ri - lff->write_index > 1) ||
      (lff->write_index >= ri && (lff->write_index != lff->size - 1 || ri != 0))) { */

    memcpy (((char *)lff->data) + (lff->write_index * lff->object_size),
            data, lff->object_size);

    /* FIXME: is this safe? */
    lff->write_index++;
    if (lff->write_index >= lff->size) {
      lff->write_index = 0;
    }

    return 0;
  } else {
    return -1;
  }
}
