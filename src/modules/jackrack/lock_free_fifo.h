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

#ifndef __JLH_LOCK_FREE_FIFO_H__
#define __JLH_LOCK_FREE_FIFO_H__

/** lock free fifo ring buffer structure */
typedef struct lock_free_fifo {
  /** Size of the ringbuffer (in elements) */
  unsigned int size;
  /** the memory containing the ringbuffer */
  void * data;
  /** the size of an element */
  size_t object_size;
  /** the current position of the reader */
  unsigned int read_index;
  /** the current position of the writer */
  unsigned int write_index;
} lff_t;

void lff_init (lff_t * lff, unsigned int size, size_t object_size);
void lff_free (lff_t * lff);

lff_t * lff_new     (unsigned int size, size_t object_size);
void    lff_destroy (lff_t * lock_free_fifo);
               
int lff_read (lff_t * lock_free_fifo, void * data);
int lff_write (lff_t * lock_free_fifo, void * data);


#endif /* __JLH_LOCK_FREE_FIFO_H__ */
