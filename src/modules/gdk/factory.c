/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2020 Meltytech, LLC
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

#include <framework/mlt.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_PIXBUF
extern mlt_producer producer_pixbuf_init(char *filename);
extern mlt_filter filter_rescale_init(mlt_profile profile, char *arg);
#endif

#ifdef USE_PANGO
extern mlt_producer producer_pango_init(const char *filename);
#endif

static void initialise()
{
    static int init = 0;
    if (init == 0) {
        init = 1;

#if !GLIB_CHECK_VERSION(2, 35, 0)
        g_type_init();
#endif
        if (getenv("MLT_PIXBUF_PRODUCER_CACHE")) {
            int n = atoi(getenv("MLT_PIXBUF_PRODUCER_CACHE"));
            mlt_service_cache_set_size(NULL, "pixbuf.image", n);
            mlt_service_cache_set_size(NULL, "pixbuf.alpha", n);
            mlt_service_cache_set_size(NULL, "pixbuf.pixbuf", n);
        }
        if (getenv("MLT_PANGO_PRODUCER_CACHE")) {
            int n = atoi(getenv("MLT_PANGO_PRODUCER_CACHE"));
            mlt_service_cache_set_size(NULL, "pango.image", n);
        }
    }
}

void *create_service(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    initialise();

#ifdef USE_PIXBUF
    if (!strcmp(id, "pixbuf"))
        return producer_pixbuf_init(arg);
#endif

#ifdef USE_PANGO
    if (!strcmp(id, "pango"))
        return producer_pango_init(arg);
#endif

#ifdef USE_PIXBUF
    if (!strcmp(id, "gtkrescale"))
        return filter_rescale_init(profile, arg);
#endif

    return NULL;
}

static mlt_properties metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    snprintf(file, PATH_MAX, "%s/gdk/%s", mlt_environment("MLT_DATA"), (char *) data);
    return mlt_properties_parse_yaml(file);
}

MLT_REPOSITORY
{
    MLT_REGISTER(mlt_service_filter_type, "gtkrescale", create_service);
    MLT_REGISTER(mlt_service_link_type, "gtkrescale", mlt_link_filter_init);
    MLT_REGISTER(mlt_service_producer_type, "pango", create_service);
    MLT_REGISTER(mlt_service_producer_type, "pixbuf", create_service);

    MLT_REGISTER_METADATA(mlt_service_filter_type, "gtkrescale", metadata, "filter_rescale.yml");
    MLT_REGISTER_METADATA(mlt_service_link_type, "gtkrescale", mlt_link_filter_metadata, NULL);
    MLT_REGISTER_METADATA(mlt_service_producer_type, "pango", metadata, "producer_pango.yml");
    MLT_REGISTER_METADATA(mlt_service_producer_type, "pixbuf", metadata, "producer_pixbuf.yml");
}
