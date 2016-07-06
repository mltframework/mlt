/*
 * consumer_gtk2.c -- A consumer for GTK2 apps
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include <stdlib.h>
#include <framework/mlt_consumer.h>
#include <framework/mlt_factory.h>
#include <gdk/gdk.h>
#ifdef _WIN32
#include <gdk/gdkwin32.h>
#else
#include <gdk/gdkx.h>
#endif
#include <gtk/gtk.h>

mlt_consumer consumer_gtk2_preview_init( mlt_profile profile, GtkWidget *widget )
{
	// Create an sdl preview consumer
	mlt_consumer consumer = NULL;

	// This is a nasty little hack which is required by SDL
	if ( widget != NULL )
	{
#ifdef _WIN32
		HWND xwin = GDK_WINDOW_HWND( widget->window );
#else
		Window xwin = GDK_WINDOW_XWINDOW( widget->window );
#endif
        char windowhack[ 32 ];
        sprintf( windowhack, "%ld", (long) xwin );
        setenv( "SDL_WINDOWID", windowhack, 1 );
	}

	// Create an sdl preview consumer
	consumer = mlt_factory_consumer( profile, "sdl_preview", NULL );

	// Now assign the lock/unlock callbacks
	if ( consumer != NULL )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
		mlt_properties_set_int( properties, "app_locked", 1 );
		mlt_properties_set_data( properties, "app_lock", gdk_threads_enter, 0, NULL, NULL );
		mlt_properties_set_data( properties, "app_unlock", gdk_threads_leave, 0, NULL, NULL );
	}

	return consumer;
}
