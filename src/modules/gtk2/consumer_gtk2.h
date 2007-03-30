/*
 * consumer_gtk2.h -- A consumer for GTK2 apps
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _CONSUMER_GTK2_PREVIEW_H
#define _CONSUMER_GTK2_PREVIEW_H

#include <framework/mlt_consumer.h>
#include <gtk/gtk.h>

extern mlt_consumer consumer_gtk2_preview_init( GtkWidget *widget );

#endif
