/*
 * producer_xml.c -- a libxml2 parser of mlt service networks
 * Copyright (C) 2003-2023 Meltytech, LLC
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

// TODO: destroy unreferenced producers (they are currently destroyed
//       when the returned producer is closed).

#include "common.h"

#include <ctype.h>
#include <framework/mlt.h>
#include <framework/mlt_log.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h> // for xmlCreateFileParserCtxt
#include <libxml/tree.h>

#define BRANCH_SIG_LEN 4000

#define _x (const xmlChar *)
#define _s (const char *)

#undef DEBUG
#ifdef DEBUG
extern xmlDocPtr xml_make_doc(mlt_service service);
#endif

enum service_type {
    mlt_invalid_type,
    mlt_unknown_type,
    mlt_producer_type,
    mlt_playlist_type,
    mlt_entry_type,
    mlt_tractor_type,
    mlt_multitrack_type,
    mlt_filter_type,
    mlt_transition_type,
    mlt_consumer_type,
    mlt_field_type,
    mlt_services_type,
    mlt_dummy_filter_type,
    mlt_dummy_transition_type,
    mlt_dummy_producer_type,
    mlt_dummy_consumer_type,
    mlt_chain_type,
    mlt_link_type,
};

struct deserialise_context_s
{
    mlt_deque stack_types;
    mlt_deque stack_service;
    mlt_deque stack_properties;
    mlt_properties producer_map;
    mlt_properties destructors;
    char *property;
    int is_value;
    xmlDocPtr value_doc;
    mlt_deque stack_node;
    xmlDocPtr entity_doc;
    int entity_is_replace;
    mlt_deque stack_branch;
    const xmlChar *publicId;
    const xmlChar *systemId;
    mlt_properties params;
    mlt_profile profile;
    mlt_profile consumer_profile;
    int pass;
    char *lc_numeric;
    mlt_consumer consumer;
    int multi_consumer;
    int consumer_count;
    int seekable;
    mlt_consumer qglsl;
};
typedef struct deserialise_context_s *deserialise_context;

/** Trim the leading and trailing whitespace from a string in-place.
*/
static char *trim(char *s)
{
    int n;
    if (s && (n = strlen(s))) {
        int i = 0;
        while (i < n && isspace(s[i]))
            i++;
        while (--n && isspace(s[n]))
            ;
        n = n - i + 1;
        if (n > 0)
            memmove(s, s + i, n);
        s[n] = 0;
    }
    return s;
}

/** Convert the numerical current branch address to a dot-delimited string.
*/
static char *serialise_branch(deserialise_context context, char *s)
{
    int i, n = mlt_deque_count(context->stack_branch);

    s[0] = 0;
    for (i = 0; i < n - 1; i++) {
        int len = strlen(s);
        snprintf(s + len,
                 BRANCH_SIG_LEN - len,
                 "%" PRIu64 ".",
                 (uint64_t) mlt_deque_peek(context->stack_branch, i));
    }
    return s;
}

/** Push a service.
*/

static void context_push_service(deserialise_context context,
                                 mlt_service that,
                                 enum service_type type)
{
    mlt_deque_push_back(context->stack_service, that);
    mlt_deque_push_back_int(context->stack_types, type);

    // Record the tree branch on which this service lives
    if (that != NULL && mlt_properties_get(MLT_SERVICE_PROPERTIES(that), "_xml_branch") == NULL) {
        char s[BRANCH_SIG_LEN];
        mlt_properties_set_string(MLT_SERVICE_PROPERTIES(that),
                                  "_xml_branch",
                                  serialise_branch(context, s));
    }
}

/** Pop a service.
*/

static mlt_service context_pop_service(deserialise_context context, enum service_type *type)
{
    mlt_service result = NULL;

    if (type)
        *type = mlt_invalid_type;
    if (mlt_deque_count(context->stack_service) > 0) {
        result = mlt_deque_pop_back(context->stack_service);
        if (type != NULL)
            *type = mlt_deque_pop_back_int(context->stack_types);
        // Set the service's profile and locale so mlt_property time-to-position conversions can get fps
        if (result) {
            mlt_properties_set_data(MLT_SERVICE_PROPERTIES(result),
                                    "_profile",
                                    context->profile,
                                    0,
                                    NULL,
                                    NULL);
            mlt_properties_set_lcnumeric(MLT_SERVICE_PROPERTIES(result), context->lc_numeric);
        }
    }
    return result;
}

/** Push a node.
*/

static void context_push_node(deserialise_context context, xmlNodePtr node)
{
    mlt_deque_push_back(context->stack_node, node);
}

/** Pop a node.
*/

static xmlNodePtr context_pop_node(deserialise_context context)
{
    return mlt_deque_pop_back(context->stack_node);
}

// Get the current properties object to add each property to.
// The object could be a service, or it could be a properties instance
// nested under a service.
static mlt_properties current_properties(deserialise_context context)
{
    mlt_properties properties = NULL;
    if (mlt_deque_count(context->stack_properties)) {
        // Nested properties are being parsed
        properties = mlt_deque_peek_back(context->stack_properties);
    } else if (mlt_deque_count(context->stack_service)) {
        // Properties belong to the service being parsed
        mlt_service service = mlt_deque_peek_back(context->stack_service);
        properties = MLT_SERVICE_PROPERTIES(service);
    }

    if (properties) {
        mlt_properties_set_data(properties, "_profile", context->profile, 0, NULL, NULL);
        mlt_properties_set_lcnumeric(properties, context->lc_numeric);
    }
    return properties;
}

// Set the destructor on a new service
static void track_service(mlt_properties properties, void *service, mlt_destructor destructor)
{
    int registered = mlt_properties_get_int(properties, "registered");
    char *key = mlt_properties_get(properties, "registered");
    mlt_properties_set_data(properties, key, service, 0, destructor, NULL);
    mlt_properties_set_int(properties, "registered", ++registered);
}

static inline int is_known_prefix(const char *resource)
{
    char *prefix = strchr(resource, ':');
    if (prefix) {
        const char *whitelist[]
            = {"alsa",  "avfoundation", "dshow",  "fbdev",        "gdigrab",    "jack",    "lavfi",
               "oss",   "pulse",        "sndio",  "video4linux2", "v4l2",       "x11grab", "async",
               "cache", "concat",       "crypto", "data",         "ffrtmphttp", "file",    "ftp",
               "fs",    "gopher",       "hls",    "http",         "httpproxy",  "mmsh",    "mmst",
               "pipe",  "rtmp",         "rtmpt",  "rtp",          "srtp",       "subfile", "tcp",
               "udp",   "udplite",      "unix",   "color",        "colour",     "consumer"};
        size_t i, n = prefix - resource;
        for (i = 0; i < sizeof(whitelist) / sizeof(whitelist[0]); ++i) {
            if (!strncmp(whitelist[i], resource, n))
                return 1;
        }
    }
    return 0;
}

// Prepend the property value with the document root
static inline void qualify_property(deserialise_context context,
                                    mlt_properties properties,
                                    const char *name)
{
    const char *resource_orig = mlt_properties_get(properties, name);
    char *resource = mlt_properties_get(properties, name);
    if (resource != NULL && resource[0]) {
        char *root = mlt_properties_get(context->producer_map, "root");
        int n = strlen(root) + strlen(resource) + 2;
        size_t prefix_size = mlt_xml_prefix_size(properties, name, resource);

        // Strip off prefix.
        if (prefix_size)
            resource += prefix_size;

        // Qualify file name properties
        if (root != NULL && strcmp(root, "")) {
            char *full_resource = calloc(1, n);
            int drive_letter = strlen(resource) > 3 && resource[1] == ':'
                               && (resource[2] == '/' || resource[2] == '\\');
            if (resource[0] != '/' && resource[0] != '\\' && !drive_letter
                && !is_known_prefix(resource)) {
                if (prefix_size)
                    strncat(full_resource, resource_orig, prefix_size);
                strcat(full_resource, root);
                strcat(full_resource, "/");
                strcat(full_resource, resource);
            } else {
                strcpy(full_resource, resource_orig);
            }
            mlt_properties_set_string(properties, name, full_resource);
            free(full_resource);
        }
    }
}

/** This function adds a producer to a playlist or multitrack when
    there is no entry or track element.
*/

static int add_producer(deserialise_context context,
                        mlt_service service,
                        mlt_position in,
                        mlt_position out)
{
    // Return value (0 = service remains top of stack, 1 means it can be removed)
    int result = 0;

    // Get the parent producer
    enum service_type type = mlt_invalid_type;
    mlt_service container = context_pop_service(context, &type);
    int contained = 0;

    if (service != NULL && container != NULL) {
        char *container_branch = mlt_properties_get(MLT_SERVICE_PROPERTIES(container),
                                                    "_xml_branch");
        char *service_branch = mlt_properties_get(MLT_SERVICE_PROPERTIES(service), "_xml_branch");
        contained = !strncmp(container_branch, service_branch, strlen(container_branch));
    }

    if (contained) {
        mlt_properties properties = MLT_SERVICE_PROPERTIES(service);
        char *hide_s = mlt_properties_get(properties, "hide");

        // Indicate that this service is no longer top of stack
        result = 1;

        switch (type) {
        case mlt_tractor_type: {
            mlt_multitrack multitrack = mlt_tractor_multitrack(MLT_TRACTOR(container));
            mlt_multitrack_connect(multitrack,
                                   MLT_PRODUCER(service),
                                   mlt_multitrack_count(multitrack));
        } break;
        case mlt_multitrack_type: {
            mlt_multitrack_connect(MLT_MULTITRACK(container),
                                   MLT_PRODUCER(service),
                                   mlt_multitrack_count(MLT_MULTITRACK(container)));
        } break;
        case mlt_playlist_type: {
            mlt_playlist_append_io(MLT_PLAYLIST(container), MLT_PRODUCER(service), in, out);
        } break;
        default:
            result = 0;
            mlt_log_warning(
                NULL, "[producer_xml] Producer defined inside something that isn't a container\n");
            break;
        };

        // Set the hide state of the track producer
        if (hide_s != NULL) {
            if (strcmp(hide_s, "video") == 0)
                mlt_properties_set_int(properties, "hide", 1);
            else if (strcmp(hide_s, "audio") == 0)
                mlt_properties_set_int(properties, "hide", 2);
            else if (strcmp(hide_s, "both") == 0)
                mlt_properties_set_int(properties, "hide", 3);
        }
    }

    // Put the parent producer back
    if (container != NULL)
        context_push_service(context, container, type);

    return result;
}

/** Attach filters defined on that to this.
*/

static void attach_filters(mlt_service service, mlt_service that)
{
    if (that != NULL) {
        int i = 0;
        mlt_filter filter = NULL;
        for (i = 0; (filter = mlt_service_filter(that, i)) != NULL; i++) {
            mlt_service_attach(service, filter);
            attach_filters(MLT_FILTER_SERVICE(filter), MLT_FILTER_SERVICE(filter));
        }
    }
}

static void on_start_profile(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    mlt_profile p = context->profile;
    for (; atts != NULL && *atts != NULL; atts += 2) {
        if (xmlStrcmp(atts[0], _x("name")) == 0 || xmlStrcmp(atts[0], _x("profile")) == 0) {
            mlt_profile my_profile = mlt_profile_init(_s(atts[1]));
            if (my_profile) {
                p->description = strdup(my_profile->description);
                p->display_aspect_den = my_profile->display_aspect_den;
                p->display_aspect_num = my_profile->display_aspect_num;
                p->frame_rate_den = my_profile->frame_rate_den;
                p->frame_rate_num = my_profile->frame_rate_num;
                p->width = my_profile->width;
                p->height = my_profile->height;
                p->progressive = my_profile->progressive;
                p->sample_aspect_den = my_profile->sample_aspect_den;
                p->sample_aspect_num = my_profile->sample_aspect_num;
                p->colorspace = my_profile->colorspace;
                p->is_explicit = 1;
                mlt_profile_close(my_profile);
            }
        } else if (xmlStrcmp(atts[0], _x("description")) == 0) {
            free(p->description);
            p->description = strdup(_s(atts[1]));
            p->is_explicit = 1;
        } else if (xmlStrcmp(atts[0], _x("display_aspect_den")) == 0)
            p->display_aspect_den = strtol(_s(atts[1]), NULL, 0);
        else if (xmlStrcmp(atts[0], _x("display_aspect_num")) == 0)
            p->display_aspect_num = strtol(_s(atts[1]), NULL, 0);
        else if (xmlStrcmp(atts[0], _x("sample_aspect_num")) == 0)
            p->sample_aspect_num = strtol(_s(atts[1]), NULL, 0);
        else if (xmlStrcmp(atts[0], _x("sample_aspect_den")) == 0)
            p->sample_aspect_den = strtol(_s(atts[1]), NULL, 0);
        else if (xmlStrcmp(atts[0], _x("width")) == 0)
            p->width = strtol(_s(atts[1]), NULL, 0);
        else if (xmlStrcmp(atts[0], _x("height")) == 0)
            p->height = strtol(_s(atts[1]), NULL, 0);
        else if (xmlStrcmp(atts[0], _x("progressive")) == 0)
            p->progressive = strtol(_s(atts[1]), NULL, 0);
        else if (xmlStrcmp(atts[0], _x("frame_rate_num")) == 0)
            p->frame_rate_num = strtol(_s(atts[1]), NULL, 0);
        else if (xmlStrcmp(atts[0], _x("frame_rate_den")) == 0)
            p->frame_rate_den = strtol(_s(atts[1]), NULL, 0);
        else if (xmlStrcmp(atts[0], _x("colorspace")) == 0)
            p->colorspace = strtol(_s(atts[1]), NULL, 0);
    }
}

static void on_start_tractor(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    mlt_tractor tractor = mlt_tractor_new();
    mlt_service service = MLT_TRACTOR_SERVICE(tractor);
    mlt_properties properties = MLT_SERVICE_PROPERTIES(service);

    track_service(context->destructors, service, (mlt_destructor) mlt_tractor_close);
    mlt_properties_set_lcnumeric(MLT_SERVICE_PROPERTIES(service), context->lc_numeric);

    for (; atts != NULL && *atts != NULL; atts += 2)
        mlt_properties_set_string(MLT_SERVICE_PROPERTIES(service),
                                  (const char *) atts[0],
                                  atts[1] == NULL ? "" : (const char *) atts[1]);

    if (mlt_properties_get(properties, "id") != NULL)
        mlt_properties_set_data(context->producer_map,
                                mlt_properties_get(properties, "id"),
                                service,
                                0,
                                NULL,
                                NULL);

    context_push_service(context, service, mlt_tractor_type);
}

static void on_end_tractor(deserialise_context context, const xmlChar *name)
{
    // Get the tractor
    enum service_type type;
    mlt_service tractor = context_pop_service(context, &type);

    if (tractor != NULL && type == mlt_tractor_type) {
        // See if the tractor should be added to a playlist or multitrack
        if (add_producer(context, tractor, 0, mlt_producer_get_out(MLT_PRODUCER(tractor))) == 0)
            context_push_service(context, tractor, type);
    } else {
        mlt_log_error(NULL, "[producer_xml] Invalid state for tractor\n");
    }
}

static void on_start_multitrack(deserialise_context context,
                                const xmlChar *name,
                                const xmlChar **atts)
{
    enum service_type type;
    mlt_service parent = context_pop_service(context, &type);

    // If we don't have a parent, then create one now, providing we're in a state where we can
    if (parent == NULL || (type == mlt_playlist_type || type == mlt_multitrack_type)) {
        mlt_tractor tractor = NULL;
        // Push the parent back
        if (parent != NULL)
            context_push_service(context, parent, type);

        // Create a tractor to contain the multitrack
        tractor = mlt_tractor_new();
        parent = MLT_TRACTOR_SERVICE(tractor);
        track_service(context->destructors, parent, (mlt_destructor) mlt_tractor_close);
        mlt_properties_set_lcnumeric(MLT_SERVICE_PROPERTIES(parent), context->lc_numeric);
        type = mlt_tractor_type;

        // Flag it as a synthesised tractor for clean up later
        mlt_properties_set_int(MLT_SERVICE_PROPERTIES(parent), "loader_synth", 1);
    }

    if (type == mlt_tractor_type) {
        mlt_service service = MLT_SERVICE(mlt_tractor_multitrack(MLT_TRACTOR(parent)));
        mlt_properties properties = MLT_SERVICE_PROPERTIES(service);
        for (; atts != NULL && *atts != NULL; atts += 2)
            mlt_properties_set_string(properties,
                                      (const char *) atts[0],
                                      atts[1] == NULL ? "" : (const char *) atts[1]);

        if (mlt_properties_get(properties, "id") != NULL)
            mlt_properties_set_data(context->producer_map,
                                    mlt_properties_get(properties, "id"),
                                    service,
                                    0,
                                    NULL,
                                    NULL);

        context_push_service(context, parent, type);
        context_push_service(context, service, mlt_multitrack_type);
    } else {
        mlt_log_error(NULL, "[producer_xml] Invalid multitrack position\n");
    }
}

static void on_end_multitrack(deserialise_context context, const xmlChar *name)
{
    // Get the multitrack from the stack
    enum service_type type;
    mlt_service service = context_pop_service(context, &type);

    if (service == NULL || type != mlt_multitrack_type)
        mlt_log_error(NULL, "[producer_xml] End multitrack in the wrong state...\n");
}

static void on_start_playlist(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    mlt_playlist playlist = mlt_playlist_new(context->profile);
    mlt_service service = MLT_PLAYLIST_SERVICE(playlist);
    mlt_properties properties = MLT_SERVICE_PROPERTIES(service);

    track_service(context->destructors, service, (mlt_destructor) mlt_playlist_close);

    for (; atts != NULL && *atts != NULL; atts += 2) {
        mlt_properties_set_string(properties,
                                  (const char *) atts[0],
                                  atts[1] == NULL ? "" : (const char *) atts[1]);

        // Out will be overwritten later as we append, so we need to save it
        if (xmlStrcmp(atts[0], _x("out")) == 0)
            mlt_properties_set_string(properties, "_xml.out", (const char *) atts[1]);
    }

    if (mlt_properties_get(properties, "id") != NULL)
        mlt_properties_set_data(context->producer_map,
                                mlt_properties_get(properties, "id"),
                                service,
                                0,
                                NULL,
                                NULL);

    context_push_service(context, service, mlt_playlist_type);
}

static void on_end_playlist(deserialise_context context, const xmlChar *name)
{
    // Get the playlist from the stack
    enum service_type type;
    mlt_service service = context_pop_service(context, &type);

    if (service != NULL && type == mlt_playlist_type) {
        mlt_properties properties = MLT_SERVICE_PROPERTIES(service);
        mlt_position in = -1;
        mlt_position out = -1;

        if (mlt_properties_get(properties, "in"))
            in = mlt_properties_get_position(properties, "in");
        if (mlt_properties_get(properties, "out"))
            out = mlt_properties_get_position(properties, "out");

        // See if the playlist should be added to a playlist or multitrack
        if (add_producer(context, service, in, out) == 0)
            context_push_service(context, service, type);
    } else {
        mlt_log_error(NULL, "[producer_xml] Invalid state of playlist end %d\n", type);
    }
}

static void on_start_chain(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    mlt_chain chain = mlt_chain_init(context->profile);
    mlt_service service = MLT_CHAIN_SERVICE(chain);
    mlt_properties properties = MLT_SERVICE_PROPERTIES(service);

    track_service(context->destructors, service, (mlt_destructor) mlt_chain_close);

    for (; atts != NULL && *atts != NULL; atts += 2) {
        mlt_properties_set_string(properties,
                                  (const char *) atts[0],
                                  atts[1] == NULL ? "" : (const char *) atts[1]);

        // Out will be overwritten later as we append, so we need to save it
        if (xmlStrcmp(atts[0], _x("out")) == 0)
            mlt_properties_set_string(properties, "_xml.out", (const char *) atts[1]);
    }

    if (mlt_properties_get(properties, "id") != NULL)
        mlt_properties_set_data(context->producer_map,
                                mlt_properties_get(properties, "id"),
                                service,
                                0,
                                NULL,
                                NULL);

    context_push_service(context, service, mlt_chain_type);
}

static void on_end_chain(deserialise_context context, const xmlChar *name)
{
    // Get the chain from the stack
    enum service_type type;
    mlt_service service = context_pop_service(context, &type);

    if (service != NULL && type == mlt_chain_type) {
        mlt_chain chain = MLT_CHAIN(service);
        mlt_properties properties = MLT_SERVICE_PROPERTIES(service);
        mlt_position in = -1;
        mlt_position out = -1;
        mlt_producer source = NULL;

        qualify_property(context, properties, "resource");
        char *resource = mlt_properties_get(properties, "resource");

        // Let Kino-SMIL src be a synonym for resource
        if (resource == NULL) {
            qualify_property(context, properties, "src");
            resource = mlt_properties_get(properties, "src");
        }

        // Instantiate the producer
        if (mlt_properties_get(properties, "mlt_service") != NULL) {
            char *service_name = trim(mlt_properties_get(properties, "mlt_service"));
            if (resource) {
                // If a document was saved as +INVALID.txt (see below), then ignore the mlt_service and
                // try to load it just from the resource. This is an attempt to recover the failed
                // producer in case, for example, a file returns.
                if (!strcmp("qtext", service_name)) {
                    const char *text = mlt_properties_get(properties, "text");
                    if (text && !strcmp("INVALID", text)) {
                        service_name = NULL;
                    }
                } else if (!strcmp("pango", service_name)) {
                    const char *markup = mlt_properties_get(properties, "markup");
                    if (markup && !strcmp("INVALID", markup)) {
                        service_name = NULL;
                    }
                }
                if (service_name) {
                    char *temp = calloc(1, strlen(service_name) + strlen(resource) + 2);
                    strcat(temp, service_name);
                    strcat(temp, ":");
                    strcat(temp, resource);
                    source = mlt_factory_producer(context->profile, NULL, temp);
                    free(temp);
                }
            } else {
                source = mlt_factory_producer(context->profile, NULL, service_name);
            }
        }

        // Just in case the plugin requested doesn't exist...
        if (!source && resource)
            source = mlt_factory_producer(context->profile, NULL, resource);
        if (!source) {
            mlt_log_error(NULL, "[producer_xml] failed to load chain \"%s\"\n", resource);
            source = mlt_factory_producer(context->profile, NULL, "+INVALID.txt");
            if (source) {
                // Save the original mlt_service for the consumer to serialize it as original.
                mlt_properties_set_string(properties,
                                          "_xml_mlt_service",
                                          mlt_properties_get(properties, "mlt_service"));
            }
        }
        if (!source)
            source = mlt_factory_producer(context->profile, NULL, "colour:red");
        // Propagate properties to the source
        mlt_properties_inherit(MLT_PRODUCER_PROPERTIES(source), properties);
        // Add the source producer to the chain
        mlt_chain_set_source(chain, source);
        mlt_producer_close(source);
        mlt_chain_attach_normalizers(chain);

        // See if the chain should be added to a playlist or multitrack
        if (mlt_properties_get(properties, "in"))
            in = mlt_properties_get_position(properties, "in");
        if (mlt_properties_get(properties, "out"))
            out = mlt_properties_get_position(properties, "out");
        if (add_producer(context, service, in, out) == 0)
            context_push_service(context, service, type);
    } else {
        mlt_log_error(NULL, "[producer_xml] Invalid state of chain end %d\n", type);
    }
}

static void on_start_link(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    // Store properties until the service type is known
    mlt_service service = calloc(1, sizeof(struct mlt_service_s));
    mlt_service_init(service, NULL);
    mlt_properties properties = MLT_SERVICE_PROPERTIES(service);
    context_push_service(context, service, mlt_link_type);

    for (; atts != NULL && *atts != NULL; atts += 2)
        mlt_properties_set_string(properties,
                                  (const char *) atts[0],
                                  atts[1] == NULL ? "" : (const char *) atts[1]);
}

static void on_end_link(deserialise_context context, const xmlChar *name)
{
    enum service_type type;
    mlt_service service = context_pop_service(context, &type);
    mlt_properties properties = MLT_SERVICE_PROPERTIES(service);

    enum service_type parent_type = mlt_invalid_type;
    mlt_service parent = context_pop_service(context, &parent_type);

    if (service != NULL && type == mlt_link_type) {
        char *id = trim(mlt_properties_get(properties, "mlt_service"));
        mlt_service link = MLT_SERVICE(mlt_factory_link(id, NULL));
        mlt_properties link_props = MLT_SERVICE_PROPERTIES(link);

        if (!link) {
            mlt_log_error(NULL, "[producer_xml] failed to load link \"%s\"\n", id);
            if (parent)
                context_push_service(context, parent, parent_type);
            mlt_service_close(service);
            free(service);
            return;
        }

        track_service(context->destructors, link, (mlt_destructor) mlt_link_close);
        mlt_properties_set_lcnumeric(MLT_SERVICE_PROPERTIES(link), context->lc_numeric);

        // Do not let XML overwrite these important properties set by mlt_factory.
        mlt_properties_set_string(properties, "mlt_type", NULL);
        mlt_properties_set_string(properties, "mlt_service", NULL);

        // Propagate the properties
        mlt_properties_inherit(link_props, properties);

        // Attach the link to the chain
        if (parent != NULL) {
            if (parent_type == mlt_chain_type) {
                mlt_chain_attach(MLT_CHAIN(parent), MLT_LINK(link));
            } else {
                mlt_log_error(NULL, "[producer_xml] link can only be added to a chain...\n");
            }

            // Put the parent back on the stack
            context_push_service(context, parent, parent_type);
        } else {
            mlt_log_error(NULL, "[producer_xml] link closed with invalid parent...\n");
        }
    } else {
        mlt_log_error(NULL, "[producer_xml] Invalid top of stack on link close\n");
    }

    if (service) {
        mlt_service_close(service);
        free(service);
    }
}

static void on_start_producer(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    // use a dummy service to hold properties to allow arbitrary nesting
    mlt_service service = calloc(1, sizeof(struct mlt_service_s));
    mlt_service_init(service, NULL);

    mlt_properties properties = MLT_SERVICE_PROPERTIES(service);

    context_push_service(context, service, mlt_dummy_producer_type);

    for (; atts != NULL && *atts != NULL; atts += 2)
        mlt_properties_set_string(properties,
                                  (const char *) atts[0],
                                  atts[1] == NULL ? "" : (const char *) atts[1]);
}

static void on_end_producer(deserialise_context context, const xmlChar *name)
{
    enum service_type type;
    mlt_service service = context_pop_service(context, &type);
    mlt_properties properties = MLT_SERVICE_PROPERTIES(service);

    if (service != NULL && type == mlt_dummy_producer_type) {
        mlt_service producer = NULL;

        qualify_property(context, properties, "resource");
        char *resource = mlt_properties_get(properties, "resource");

        // Let Kino-SMIL src be a synonym for resource
        if (resource == NULL) {
            qualify_property(context, properties, "src");
            resource = mlt_properties_get(properties, "src");
        }

        // Instantiate the producer
        if (mlt_properties_get(properties, "mlt_service") != NULL) {
            char *service_name = trim(mlt_properties_get(properties, "mlt_service"));
            if (resource) {
                // If a document was saved as +INVALID.txt (see below), then ignore the mlt_service and
                // try to load it just from the resource. This is an attempt to recover the failed
                // producer in case, for example, a file returns.
                if (!strcmp("qtext", service_name)) {
                    const char *text = mlt_properties_get(properties, "text");
                    if (text && !strcmp("INVALID", text)) {
                        service_name = NULL;
                    }
                } else if (!strcmp("pango", service_name)) {
                    const char *markup = mlt_properties_get(properties, "markup");
                    if (markup && !strcmp("INVALID", markup)) {
                        service_name = NULL;
                    }
                }
                if (service_name) {
                    char *temp = calloc(1, strlen(service_name) + strlen(resource) + 2);
                    strcat(temp, service_name);
                    strcat(temp, ":");
                    strcat(temp, resource);
                    producer = MLT_SERVICE(mlt_factory_producer(context->profile, NULL, temp));
                    free(temp);
                }
            } else {
                producer = MLT_SERVICE(mlt_factory_producer(context->profile, NULL, service_name));
            }
        }

        // Just in case the plugin requested doesn't exist...
        if (!producer && resource)
            producer = MLT_SERVICE(mlt_factory_producer(context->profile, NULL, resource));
        if (!producer) {
            mlt_log_error(NULL, "[producer_xml] failed to load producer \"%s\"\n", resource);
            producer = MLT_SERVICE(mlt_factory_producer(context->profile, NULL, "+INVALID.txt"));
            if (producer) {
                // Save the original mlt_service for the consumer to serialize it as original.
                mlt_properties_set_string(MLT_SERVICE_PROPERTIES(producer),
                                          "_xml_mlt_service",
                                          mlt_properties_get(properties, "mlt_service"));
            }
        }
        if (!producer)
            producer = MLT_SERVICE(mlt_factory_producer(context->profile, NULL, "colour:red"));
        if (!producer) {
            mlt_service_close(service);
            free(service);
            return;
        }

        // Track this producer
        track_service(context->destructors, producer, (mlt_destructor) mlt_producer_close);
        mlt_properties producer_props = MLT_SERVICE_PROPERTIES(producer);
        mlt_properties_set_lcnumeric(producer_props, context->lc_numeric);
        if (mlt_properties_get(producer_props, "seekable"))
            context->seekable &= mlt_properties_get_int(producer_props, "seekable");

        // Propagate the properties
        qualify_property(context, properties, "resource");
        qualify_property(context, properties, "luma");
        qualify_property(context, properties, "luma.resource");
        qualify_property(context, properties, "composite.luma");
        qualify_property(context, properties, "producer.resource");
        qualify_property(context, properties, "argument"); // timewarp producer

        // Handle in/out properties separately
        mlt_position in = -1;
        mlt_position out = -1;

        // Get in
        if (mlt_properties_get(properties, "in"))
            in = mlt_properties_get_position(properties, "in");
        // Let Kino-SMIL clipBegin be a synonym for in
        else if (mlt_properties_get(properties, "clipBegin"))
            in = mlt_properties_get_position(properties, "clipBegin");
        // Get out
        if (mlt_properties_get(properties, "out"))
            out = mlt_properties_get_position(properties, "out");
        // Let Kino-SMIL clipEnd be a synonym for out
        else if (mlt_properties_get(properties, "clipEnd"))
            out = mlt_properties_get_position(properties, "clipEnd");
        // Remove in and out
        mlt_properties_clear(properties, "in");
        mlt_properties_clear(properties, "out");

        // Let a child XML length extend the length of a parent XML clip.
        if (mlt_properties_get(producer_props, "mlt_service")
            && (!strcmp("xml", mlt_properties_get(producer_props, "mlt_service"))
                || !strcmp("consumer", mlt_properties_get(producer_props, "mlt_service")))
            && mlt_properties_get_position(producer_props, "length")
                   > mlt_properties_get_position(properties, "length")) {
            mlt_properties_set_position(properties,
                                        "length",
                                        mlt_properties_get_position(producer_props, "length"));
            in = mlt_properties_get_position(producer_props, "in");
            out = mlt_properties_get_position(producer_props, "out");
        }

        // Do not let XML overwrite these important properties set by mlt_factory.
        mlt_properties_set_string(properties, "mlt_type", NULL);
        mlt_properties_set_string(properties, "mlt_service", NULL);

        // Inherit the properties
        mlt_properties_inherit(producer_props, properties);

        // Attach all filters from service onto producer
        attach_filters(producer, service);

        // Add the producer to the producer map
        if (mlt_properties_get(properties, "id") != NULL)
            mlt_properties_set_data(context->producer_map,
                                    mlt_properties_get(properties, "id"),
                                    producer,
                                    0,
                                    NULL,
                                    NULL);

        // See if the producer should be added to a playlist or multitrack
        if (add_producer(context, producer, in, out) == 0) {
            // Otherwise, set in and out on...
            if (in != -1 || out != -1) {
                // Get the parent service
                enum service_type type;
                mlt_service parent = context_pop_service(context, &type);
                if (parent != NULL) {
                    // Get the parent properties
                    properties = MLT_SERVICE_PROPERTIES(parent);

                    char *resource = mlt_properties_get(properties, "resource");

                    // Put the parent producer back
                    context_push_service(context, parent, type);

                    // If the parent is a track or entry
                    if (resource && (strcmp(resource, "<entry>") == 0)) {
                        if (in > -1)
                            mlt_properties_set_position(properties, "in", in);
                        if (out > -1)
                            mlt_properties_set_position(properties, "out", out);
                    } else {
                        // Otherwise, set in and out on producer directly
                        mlt_producer_set_in_and_out(MLT_PRODUCER(producer), in, out);
                    }
                } else {
                    // Otherwise, set in and out on producer directly
                    mlt_producer_set_in_and_out(MLT_PRODUCER(producer), in, out);
                }
            }

            // Push the producer onto the stack
            context_push_service(context, producer, mlt_producer_type);
        }
    }

    if (service) {
        mlt_service_close(service);
        free(service);
    }
}

static void on_start_blank(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    // Get the playlist from the stack
    enum service_type type;
    mlt_service service = context_pop_service(context, &type);

    if (type == mlt_playlist_type && service != NULL) {
        // Look for the length attribute
        for (; atts != NULL && *atts != NULL; atts += 2) {
            if (xmlStrcmp(atts[0], _x("length")) == 0) {
                // Append a blank to the playlist
                mlt_playlist_blank_time(MLT_PLAYLIST(service), _s(atts[1]));
                break;
            }
        }

        // Push the playlist back onto the stack
        context_push_service(context, service, type);
    } else {
        mlt_log_error(NULL, "[producer_xml] blank without a playlist - a definite no no\n");
    }
}

static void on_start_entry(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    mlt_producer entry = NULL;
    mlt_properties temp = mlt_properties_new();
    mlt_properties_set_data(temp, "_profile", context->profile, 0, NULL, NULL);
    mlt_properties_set_lcnumeric(temp, context->lc_numeric);

    for (; atts != NULL && *atts != NULL; atts += 2) {
        mlt_properties_set_string(temp,
                                  (const char *) atts[0],
                                  atts[1] == NULL ? "" : (const char *) atts[1]);

        // Look for the producer attribute
        if (xmlStrcmp(atts[0], _x("producer")) == 0) {
            mlt_producer producer = mlt_properties_get_data(context->producer_map,
                                                            (const char *) atts[1],
                                                            NULL);
            if (producer != NULL)
                mlt_properties_set_data(temp, "producer", producer, 0, NULL, NULL);
        }
    }

    // If we have a valid entry
    if (mlt_properties_get_data(temp, "producer", NULL) != NULL) {
        mlt_playlist_clip_info info;
        enum service_type parent_type = mlt_invalid_type;
        mlt_service parent = context_pop_service(context, &parent_type);
        mlt_producer producer = mlt_properties_get_data(temp, "producer", NULL);

        if (parent_type == mlt_playlist_type) {
            // Append the producer to the playlist
            mlt_position in = -1;
            mlt_position out = -1;
            if (mlt_properties_get(temp, "in"))
                in = mlt_properties_get_position(temp, "in");
            if (mlt_properties_get(temp, "out"))
                out = mlt_properties_get_position(temp, "out");
            mlt_playlist_append_io(MLT_PLAYLIST(parent), producer, in, out);

            // Handle the repeat property
            if (mlt_properties_get_int(temp, "repeat") > 0) {
                mlt_playlist_repeat_clip(MLT_PLAYLIST(parent),
                                         mlt_playlist_count(MLT_PLAYLIST(parent)) - 1,
                                         mlt_properties_get_int(temp, "repeat"));
            }

            mlt_playlist_get_clip_info(MLT_PLAYLIST(parent),
                                       &info,
                                       mlt_playlist_count(MLT_PLAYLIST(parent)) - 1);
            entry = info.cut;
        } else {
            mlt_log_error(NULL, "[producer_xml] Entry not part of a playlist...\n");
        }

        context_push_service(context, parent, parent_type);
    }

    // Push the cut onto the stack
    context_push_service(context, MLT_PRODUCER_SERVICE(entry), mlt_entry_type);

    mlt_properties_close(temp);
}

static void on_end_entry(deserialise_context context, const xmlChar *name)
{
    // Get the entry from the stack
    enum service_type entry_type = mlt_invalid_type;
    mlt_service entry = context_pop_service(context, &entry_type);

    if (entry == NULL && entry_type != mlt_entry_type) {
        mlt_log_error(NULL, "[producer_xml] Invalid state at end of entry\n");
    }
}

static void on_start_track(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    // use a dummy service to hold properties to allow arbitrary nesting
    mlt_service service = calloc(1, sizeof(struct mlt_service_s));
    mlt_service_init(service, NULL);

    // Push the dummy service onto the stack
    context_push_service(context, service, mlt_entry_type);

    mlt_properties_set_string(MLT_SERVICE_PROPERTIES(service), "resource", "<track>");

    for (; atts != NULL && *atts != NULL; atts += 2) {
        mlt_properties_set_string(MLT_SERVICE_PROPERTIES(service),
                                  (const char *) atts[0],
                                  atts[1] == NULL ? "" : (const char *) atts[1]);

        // Look for the producer attribute
        if (xmlStrcmp(atts[0], _x("producer")) == 0) {
            mlt_producer producer = mlt_properties_get_data(context->producer_map,
                                                            (const char *) atts[1],
                                                            NULL);
            if (producer != NULL)
                mlt_properties_set_data(MLT_SERVICE_PROPERTIES(service),
                                        "producer",
                                        producer,
                                        0,
                                        NULL,
                                        NULL);
        }
    }
}

static void on_end_track(deserialise_context context, const xmlChar *name)
{
    // Get the track from the stack
    enum service_type track_type;
    mlt_service track = context_pop_service(context, &track_type);

    if (track != NULL && track_type == mlt_entry_type) {
        mlt_properties track_props = MLT_SERVICE_PROPERTIES(track);
        enum service_type parent_type = mlt_invalid_type;
        mlt_service parent = context_pop_service(context, &parent_type);
        mlt_multitrack multitrack = NULL;

        mlt_producer producer = mlt_properties_get_data(track_props, "producer", NULL);
        mlt_properties producer_props = MLT_PRODUCER_PROPERTIES(producer);

        if (parent_type == mlt_tractor_type)
            multitrack = mlt_tractor_multitrack(MLT_TRACTOR(parent));
        else if (parent_type == mlt_multitrack_type)
            multitrack = MLT_MULTITRACK(parent);
        else
            mlt_log_error(NULL, "[producer_xml] track contained in an invalid container\n");

        if (multitrack != NULL) {
            // Set producer i/o if specified
            if (mlt_properties_get(track_props, "in") != NULL
                || mlt_properties_get(track_props, "out") != NULL) {
                mlt_position in = -1;
                mlt_position out = -1;
                if (mlt_properties_get(track_props, "in"))
                    in = mlt_properties_get_position(track_props, "in");
                if (mlt_properties_get(track_props, "out"))
                    out = mlt_properties_get_position(track_props, "out");
                mlt_producer cut = mlt_producer_cut(MLT_PRODUCER(producer), in, out);
                mlt_multitrack_connect(multitrack, cut, mlt_multitrack_count(multitrack));
                mlt_properties_inherit(MLT_PRODUCER_PROPERTIES(cut), track_props);
                track_props = MLT_PRODUCER_PROPERTIES(cut);
                mlt_producer_close(cut);
            } else {
                mlt_multitrack_connect(multitrack, producer, mlt_multitrack_count(multitrack));
            }

            // Set the hide state of the track producer
            char *hide_s = mlt_properties_get(track_props, "hide");
            if (hide_s != NULL) {
                if (strcmp(hide_s, "video") == 0)
                    mlt_properties_set_int(producer_props, "hide", 1);
                else if (strcmp(hide_s, "audio") == 0)
                    mlt_properties_set_int(producer_props, "hide", 2);
                else if (strcmp(hide_s, "both") == 0)
                    mlt_properties_set_int(producer_props, "hide", 3);
            }
        }

        if (parent != NULL)
            context_push_service(context, parent, parent_type);
    } else {
        mlt_log_error(NULL, "[producer_xml] Invalid state at end of track\n");
    }

    if (track) {
        mlt_service_close(track);
        free(track);
    }
}

static void on_start_filter(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    // use a dummy service to hold properties to allow arbitrary nesting
    mlt_service service = calloc(1, sizeof(struct mlt_service_s));
    mlt_service_init(service, NULL);

    mlt_properties properties = MLT_SERVICE_PROPERTIES(service);

    context_push_service(context, service, mlt_dummy_filter_type);

    // Set the properties
    for (; atts != NULL && *atts != NULL; atts += 2)
        mlt_properties_set_string(properties, (const char *) atts[0], (const char *) atts[1]);
}

static void on_end_filter(deserialise_context context, const xmlChar *name)
{
    enum service_type type;
    mlt_service service = context_pop_service(context, &type);
    mlt_properties properties = MLT_SERVICE_PROPERTIES(service);

    enum service_type parent_type = mlt_invalid_type;
    mlt_service parent = context_pop_service(context, &parent_type);

    if (service != NULL && type == mlt_dummy_filter_type) {
        char *id = trim(mlt_properties_get(properties, "mlt_service"));
        mlt_service filter = MLT_SERVICE(mlt_factory_filter(context->profile, id, NULL));
        mlt_properties filter_props = MLT_SERVICE_PROPERTIES(filter);

        if (!filter) {
            mlt_log_error(NULL, "[producer_xml] failed to load filter \"%s\"\n", id);
            if (parent)
                context_push_service(context, parent, parent_type);
            mlt_service_close(service);
            free(service);
            return;
        }

        track_service(context->destructors, filter, (mlt_destructor) mlt_filter_close);
        mlt_properties_set_lcnumeric(MLT_SERVICE_PROPERTIES(filter), context->lc_numeric);

        // Do not let XML overwrite these important properties set by mlt_factory.
        mlt_properties_set_string(properties, "mlt_type", NULL);
        mlt_properties_set_string(properties, "mlt_service", NULL);

        // Propagate the properties
        qualify_property(context, properties, "resource");
        qualify_property(context, properties, "luma");
        qualify_property(context, properties, "luma.resource");
        qualify_property(context, properties, "composite.luma");
        qualify_property(context, properties, "producer.resource");
        qualify_property(context, properties, "filename");
        qualify_property(context, properties, "av.file");
        qualify_property(context, properties, "av.filename");
        qualify_property(context, properties, "filter.resource");
        mlt_properties_inherit(filter_props, properties);

        // Attach all filters from service onto filter
        attach_filters(filter, service);

        // Associate the filter with the parent
        if (parent != NULL) {
            if (parent_type == mlt_tractor_type && mlt_properties_get(properties, "track")) {
                mlt_field field = mlt_tractor_field(MLT_TRACTOR(parent));
                mlt_field_plant_filter(field,
                                       MLT_FILTER(filter),
                                       mlt_properties_get_int(properties, "track"));
                mlt_filter_set_in_and_out(MLT_FILTER(filter),
                                          mlt_properties_get_int(properties, "in"),
                                          mlt_properties_get_int(properties, "out"));
            } else {
                mlt_service_attach(parent, MLT_FILTER(filter));
            }

            // Put the parent back on the stack
            context_push_service(context, parent, parent_type);
        } else {
            mlt_log_error(NULL, "[producer_xml] filter closed with invalid parent...\n");
        }
    } else {
        mlt_log_error(NULL, "[producer_xml] Invalid top of stack on filter close\n");
    }

    if (service) {
        mlt_service_close(service);
        free(service);
    }
}

static void on_start_transition(deserialise_context context,
                                const xmlChar *name,
                                const xmlChar **atts)
{
    // use a dummy service to hold properties to allow arbitrary nesting
    mlt_service service = calloc(1, sizeof(struct mlt_service_s));
    mlt_service_init(service, NULL);

    mlt_properties properties = MLT_SERVICE_PROPERTIES(service);

    context_push_service(context, service, mlt_dummy_transition_type);

    // Set the properties
    for (; atts != NULL && *atts != NULL; atts += 2)
        mlt_properties_set_string(properties, (const char *) atts[0], (const char *) atts[1]);
}

static void on_end_transition(deserialise_context context, const xmlChar *name)
{
    enum service_type type;
    mlt_service service = context_pop_service(context, &type);
    mlt_properties properties = MLT_SERVICE_PROPERTIES(service);

    enum service_type parent_type = mlt_invalid_type;
    mlt_service parent = context_pop_service(context, &parent_type);

    if (service != NULL && type == mlt_dummy_transition_type) {
        char *id = trim(mlt_properties_get(properties, "mlt_service"));
        mlt_service effect = MLT_SERVICE(mlt_factory_transition(context->profile, id, NULL));
        mlt_properties effect_props = MLT_SERVICE_PROPERTIES(effect);

        if (!effect) {
            mlt_log_error(NULL, "[producer_xml] failed to load transition \"%s\"\n", id);
            if (parent)
                context_push_service(context, parent, parent_type);
            mlt_service_close(service);
            free(service);
            return;
        }
        track_service(context->destructors, effect, (mlt_destructor) mlt_transition_close);
        mlt_properties_set_lcnumeric(MLT_SERVICE_PROPERTIES(effect), context->lc_numeric);

        // Do not let XML overwrite these important properties set by mlt_factory.
        mlt_properties_set_string(properties, "mlt_type", NULL);
        mlt_properties_set_string(properties, "mlt_service", NULL);

        // Propagate the properties
        qualify_property(context, properties, "resource");
        qualify_property(context, properties, "luma");
        qualify_property(context, properties, "luma.resource");
        qualify_property(context, properties, "composite.luma");
        qualify_property(context, properties, "producer.resource");
        mlt_properties_inherit(effect_props, properties);

        // Attach all filters from service onto effect
        attach_filters(effect, service);

        // Associate the filter with the parent
        if (parent != NULL) {
            if (parent_type == mlt_tractor_type) {
                mlt_field field = mlt_tractor_field(MLT_TRACTOR(parent));
                mlt_field_plant_transition(field,
                                           MLT_TRANSITION(effect),
                                           mlt_properties_get_int(properties, "a_track"),
                                           mlt_properties_get_int(properties, "b_track"));
                mlt_transition_set_in_and_out(MLT_TRANSITION(effect),
                                              mlt_properties_get_int(properties, "in"),
                                              mlt_properties_get_int(properties, "out"));
            } else {
                mlt_log_warning(NULL, "[producer_xml] Misplaced transition - ignoring\n");
            }

            // Put the parent back on the stack
            context_push_service(context, parent, parent_type);
        } else {
            mlt_log_error(NULL, "[producer_xml] transition closed with invalid parent...\n");
        }

    } else {
        mlt_log_error(NULL, "[producer_xml] Invalid top of stack on transition close\n");
    }

    if (service) {
        mlt_service_close(service);
        free(service);
    }
}

static void on_start_consumer(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    if (context->pass == 1) {
        mlt_properties properties = mlt_properties_new();

        mlt_properties_set_lcnumeric(properties, context->lc_numeric);
        context_push_service(context, (mlt_service) properties, mlt_dummy_consumer_type);

        // Set the properties from attributes
        for (; atts != NULL && *atts != NULL; atts += 2)
            mlt_properties_set_string(properties, (const char *) atts[0], (const char *) atts[1]);
    }
}

static void set_preview_scale(mlt_profile *consumer_profile, mlt_profile *profile, double scale)
{
    *consumer_profile = mlt_profile_clone(*profile);
    if (*consumer_profile) {
        (*consumer_profile)->width *= scale;
        (*consumer_profile)->width -= (*consumer_profile)->width % 2;
        (*consumer_profile)->height *= scale;
        (*consumer_profile)->height -= (*consumer_profile)->height % 2;
    }
}

static void on_end_consumer(deserialise_context context, const xmlChar *name)
{
    if (context->pass == 1) {
        // Get the consumer from the stack
        enum service_type type;
        mlt_properties properties = (mlt_properties) context_pop_service(context, &type);

        if (properties && type == mlt_dummy_consumer_type) {
            qualify_property(context, properties, "resource");
            qualify_property(context, properties, "target");
            char *resource = mlt_properties_get(properties, "resource");

            if (context->multi_consumer > 1 || context->qglsl
                || mlt_properties_get_int(context->params, "multi")) {
                // Instantiate the multi consumer
                if (!context->consumer) {
                    if (context->qglsl)
                        context->consumer = context->qglsl;
                    else
                        context->consumer = mlt_factory_consumer(context->profile, "multi", NULL);
                    if (context->consumer) {
                        // Track this consumer
                        track_service(context->destructors,
                                      MLT_CONSUMER_SERVICE(context->consumer),
                                      (mlt_destructor) mlt_consumer_close);
                        mlt_properties_set_lcnumeric(MLT_CONSUMER_PROPERTIES(context->consumer),
                                                     context->lc_numeric);
                    }
                }
                if (context->consumer) {
                    // Set this properties object on multi consumer
                    mlt_properties consumer_properties = MLT_CONSUMER_PROPERTIES(context->consumer);
                    char key[20];
                    snprintf(key, sizeof(key), "%d", context->consumer_count++);
                    mlt_properties_inc_ref(properties);
                    mlt_properties_set_data(consumer_properties,
                                            key,
                                            properties,
                                            0,
                                            (mlt_destructor) mlt_properties_close,
                                            NULL);

                    // Pass in / out if provided
                    mlt_properties_pass_list(consumer_properties, properties, "in, out");

                    // Pass along quality and performance properties to the multi consumer and its render thread(s).
                    if (!context->qglsl) {
                        mlt_properties_pass_list(
                            consumer_properties,
                            properties,
                            "real_time, deinterlacer, deinterlace_method, rescale, progressive, "
                            "top_field_first, channels, channel_layout");

                        // We only really know how to optimize real_time for the avformat consumer.
                        const char *service_name = mlt_properties_get(properties, "mlt_service");
                        if (service_name && !strcmp("avformat", service_name))
                            mlt_properties_set_int(properties, "real_time", -1);
                    }
                }
            } else {
                double scale = mlt_properties_get_double(properties, "scale");
                if (scale > 0.0) {
                    set_preview_scale(&context->consumer_profile, &context->profile, scale);
                }
                // Instantiate the consumer
                char *id = trim(mlt_properties_get(properties, "mlt_service"));
                mlt_profile profile = context->consumer_profile ? context->consumer_profile
                                                                : context->profile;
                context->consumer = mlt_factory_consumer(profile, id, resource);
                if (context->consumer) {
                    // Track this consumer
                    track_service(context->destructors,
                                  MLT_CONSUMER_SERVICE(context->consumer),
                                  (mlt_destructor) mlt_consumer_close);
                    mlt_properties_set_lcnumeric(MLT_CONSUMER_PROPERTIES(context->consumer),
                                                 context->lc_numeric);
                    if (context->consumer_profile) {
                        mlt_properties_set_data(MLT_CONSUMER_PROPERTIES(context->consumer),
                                                "_profile",
                                                context->consumer_profile,
                                                sizeof(*context->consumer_profile),
                                                (mlt_destructor) mlt_profile_close,
                                                NULL);
                    }

                    // Do not let XML overwrite these important properties set by mlt_factory.
                    mlt_properties_set_string(properties, "mlt_type", NULL);
                    mlt_properties_set_string(properties, "mlt_service", NULL);

                    // Inherit the properties
                    mlt_properties_inherit(MLT_CONSUMER_PROPERTIES(context->consumer), properties);
                }
            }
        }
        // Close the dummy
        if (properties)
            mlt_properties_close(properties);
    }
}

static void on_start_property(deserialise_context context, const xmlChar *name, const xmlChar **atts)
{
    mlt_properties properties = current_properties(context);
    const char *value = NULL;

    if (properties != NULL) {
        // Set the properties
        for (; atts != NULL && *atts != NULL; atts += 2) {
            if (xmlStrcmp(atts[0], _x("name")) == 0)
                context->property = strdup(_s(atts[1]));
            else if (xmlStrcmp(atts[0], _x("value")) == 0)
                value = _s(atts[1]);
        }

        if (context->property != NULL)
            mlt_properties_set_string(properties, context->property, value == NULL ? "" : value);

        // Tell parser to collect any further nodes for serialisation
        context->is_value = 1;
    } else {
        mlt_log_error(NULL, "[producer_xml] Property without a parent '%s'?\n", (const char *) name);
    }
}

static void on_end_property(deserialise_context context, const xmlChar *name)
{
    mlt_properties properties = current_properties(context);

    if (properties != NULL) {
        // Tell parser to stop building a tree
        context->is_value = 0;

        // See if there is a xml tree for the value
        if (context->property != NULL && context->value_doc != NULL) {
            xmlChar *value;
            int size;

            // Serialise the tree to get value
            xmlDocDumpMemory(context->value_doc, &value, &size);
            mlt_properties_set_string(properties, context->property, _s(value));
#ifdef _WIN32
            xmlFreeFunc xmlFree = NULL;
            xmlMemGet(&xmlFree, NULL, NULL, NULL);
#endif
            xmlFree(value);
            xmlFreeDoc(context->value_doc);
            context->value_doc = NULL;
        }

        // Close this property handling
        free(context->property);
        context->property = NULL;
    } else {
        mlt_log_error(NULL,
                      "[producer_xml] Property without a parent '%s'??\n",
                      (const char *) name);
    }
}

static void on_start_properties(deserialise_context context,
                                const xmlChar *name,
                                const xmlChar **atts)
{
    mlt_properties parent_properties = current_properties(context);
    if (parent_properties != NULL) {
        mlt_properties properties = NULL;

        // Get the name
        for (; atts != NULL && *atts != NULL; atts += 2) {
            if (xmlStrcmp(atts[0], _x("name")) == 0) {
                properties = mlt_properties_new();
                mlt_properties_set_properties(parent_properties, _s(atts[1]), properties);
                mlt_properties_dec_ref(properties);
            } else {
                mlt_log_warning(NULL,
                                "[producer_xml] Invalid attribute for properties '%s=%s'\n",
                                (const char *) atts[0],
                                (const char *) atts[1]);
            }
        }

        if (properties) {
            mlt_deque_push_back(context->stack_properties, properties);
        } else {
            mlt_log_error(NULL,
                          "[producer_xml] Properties without a name '%s'?\n",
                          (const char *) name);
        }
    } else {
        mlt_log_error(NULL,
                      "[producer_xml] Properties without a parent '%s'?\n",
                      (const char *) name);
    }
}

static void on_end_properties(deserialise_context context, const xmlChar *name)
{
    if (mlt_deque_count(context->stack_properties)) {
        mlt_deque_pop_back(context->stack_properties);
    } else {
        mlt_log_error(NULL,
                      "[producer_xml] Properties end missing properties '%s'.\n",
                      (const char *) name);
    }
}

static void on_start_element(void *ctx, const xmlChar *name, const xmlChar **atts)
{
    struct _xmlParserCtxt *xmlcontext = (struct _xmlParserCtxt *) ctx;
    deserialise_context context = (deserialise_context) (xmlcontext->_private);

    if (context->pass == 0) {
        if (xmlStrcmp(name, _x("mlt")) == 0 || xmlStrcmp(name, _x("profile")) == 0
            || xmlStrcmp(name, _x("profileinfo")) == 0)
            on_start_profile(context, name, atts);
        if (xmlStrcmp(name, _x("consumer")) == 0)
            context->multi_consumer++;

        // Check for a service beginning with glsl. or movit.
        for (; atts != NULL && *atts != NULL; atts += 2) {
            if (!xmlStrncmp(atts[1], _x("glsl."), 5) || !xmlStrncmp(atts[1], _x("movit."), 6)) {
                mlt_properties_set_int(context->params, "qglsl", 1);
                break;
            }
        }
        return;
    }
    mlt_deque_push_back_int(context->stack_branch,
                            mlt_deque_pop_back_int(context->stack_branch) + 1);
    mlt_deque_push_back_int(context->stack_branch, 0);

    // Build a tree from nodes within a property value
    if (context->is_value == 1 && context->pass == 1) {
        xmlNodePtr node = xmlNewNode(NULL, name);

        if (context->value_doc == NULL) {
            // Start a new tree
            context->value_doc = xmlNewDoc(_x("1.0"));
            xmlDocSetRootElement(context->value_doc, node);
        } else {
            // Append child to tree
            xmlAddChild(mlt_deque_peek_back(context->stack_node), node);
        }
        context_push_node(context, node);

        // Set the attributes
        for (; atts != NULL && *atts != NULL; atts += 2)
            xmlSetProp(node, atts[0], atts[1]);
    } else if (xmlStrcmp(name, _x("tractor")) == 0)
        on_start_tractor(context, name, atts);
    else if (xmlStrcmp(name, _x("multitrack")) == 0)
        on_start_multitrack(context, name, atts);
    else if (xmlStrcmp(name, _x("playlist")) == 0 || xmlStrcmp(name, _x("seq")) == 0
             || xmlStrcmp(name, _x("smil")) == 0)
        on_start_playlist(context, name, atts);
    else if (xmlStrcmp(name, _x("producer")) == 0 || xmlStrcmp(name, _x("video")) == 0)
        on_start_producer(context, name, atts);
    else if (xmlStrcmp(name, _x("blank")) == 0)
        on_start_blank(context, name, atts);
    else if (xmlStrcmp(name, _x("entry")) == 0)
        on_start_entry(context, name, atts);
    else if (xmlStrcmp(name, _x("track")) == 0)
        on_start_track(context, name, atts);
    else if (xmlStrcmp(name, _x("filter")) == 0)
        on_start_filter(context, name, atts);
    else if (xmlStrcmp(name, _x("transition")) == 0)
        on_start_transition(context, name, atts);
    else if (xmlStrcmp(name, _x("chain")) == 0)
        on_start_chain(context, name, atts);
    else if (xmlStrcmp(name, _x("link")) == 0)
        on_start_link(context, name, atts);
    else if (xmlStrcmp(name, _x("property")) == 0)
        on_start_property(context, name, atts);
    else if (xmlStrcmp(name, _x("properties")) == 0)
        on_start_properties(context, name, atts);
    else if (xmlStrcmp(name, _x("consumer")) == 0)
        on_start_consumer(context, name, atts);
    else if (xmlStrcmp(name, _x("westley")) == 0 || xmlStrcmp(name, _x("mlt")) == 0) {
        for (; atts != NULL && *atts != NULL; atts += 2) {
            if (xmlStrcmp(atts[0], _x("LC_NUMERIC")))
                mlt_properties_set_string(context->producer_map, _s(atts[0]), _s(atts[1]));
            else if (!context->lc_numeric)
                context->lc_numeric = strdup(_s(atts[1]));
        }
    }
}

static void on_end_element(void *ctx, const xmlChar *name)
{
    struct _xmlParserCtxt *xmlcontext = (struct _xmlParserCtxt *) ctx;
    deserialise_context context = (deserialise_context) (xmlcontext->_private);

    if (context->is_value == 1 && context->pass == 1 && xmlStrcmp(name, _x("property")) != 0)
        context_pop_node(context);
    else if (xmlStrcmp(name, _x("multitrack")) == 0)
        on_end_multitrack(context, name);
    else if (xmlStrcmp(name, _x("playlist")) == 0 || xmlStrcmp(name, _x("seq")) == 0
             || xmlStrcmp(name, _x("smil")) == 0)
        on_end_playlist(context, name);
    else if (xmlStrcmp(name, _x("track")) == 0)
        on_end_track(context, name);
    else if (xmlStrcmp(name, _x("entry")) == 0)
        on_end_entry(context, name);
    else if (xmlStrcmp(name, _x("tractor")) == 0)
        on_end_tractor(context, name);
    else if (xmlStrcmp(name, _x("property")) == 0)
        on_end_property(context, name);
    else if (xmlStrcmp(name, _x("properties")) == 0)
        on_end_properties(context, name);
    else if (xmlStrcmp(name, _x("producer")) == 0 || xmlStrcmp(name, _x("video")) == 0)
        on_end_producer(context, name);
    else if (xmlStrcmp(name, _x("filter")) == 0)
        on_end_filter(context, name);
    else if (xmlStrcmp(name, _x("transition")) == 0)
        on_end_transition(context, name);
    else if (xmlStrcmp(name, _x("chain")) == 0)
        on_end_chain(context, name);
    else if (xmlStrcmp(name, _x("link")) == 0)
        on_end_link(context, name);
    else if (xmlStrcmp(name, _x("consumer")) == 0)
        on_end_consumer(context, name);

    mlt_deque_pop_back_int(context->stack_branch);
}

static void on_characters(void *ctx, const xmlChar *ch, int len)
{
    struct _xmlParserCtxt *xmlcontext = (struct _xmlParserCtxt *) ctx;
    deserialise_context context = (deserialise_context) (xmlcontext->_private);
    char *value = calloc(1, len + 1);
    mlt_properties properties = current_properties(context);

    value[len] = 0;
    strncpy(value, (const char *) ch, len);

    if (mlt_deque_count(context->stack_node))
        xmlNodeAddContent(mlt_deque_peek_back(context->stack_node), (xmlChar *) value);

    // libxml2 generates an on_characters immediately after a get_entity within
    // an element value, and we ignore it because it is called again during
    // actual substitution.
    else if (context->property != NULL && context->entity_is_replace == 0) {
        char *s = mlt_properties_get(properties, context->property);
        if (s != NULL) {
            // Append new text to existing content
            char *new = calloc(1, strlen(s) + len + 1);
            strcat(new, s);
            strcat(new, value);
            mlt_properties_set_string(properties, context->property, new);
            free(new);
        } else
            mlt_properties_set_string(properties, context->property, value);
    }
    context->entity_is_replace = 0;

    // Check for a service beginning with glsl. or movit.
    if (!strncmp(value, "glsl.", 5) || !strncmp(value, "movit.", 6))
        mlt_properties_set_int(context->params, "qglsl", 1);

    free(value);
}

/** Convert parameters parsed from resource into entity declarations.
*/
static void params_to_entities(deserialise_context context)
{
    if (context->params != NULL) {
        int i;

        // Add our params as entity declarations
        for (i = 0; i < mlt_properties_count(context->params); i++) {
            xmlChar *name = (xmlChar *) mlt_properties_get_name(context->params, i);
            xmlAddDocEntity(context->entity_doc,
                            name,
                            XML_INTERNAL_GENERAL_ENTITY,
                            context->publicId,
                            context->systemId,
                            (xmlChar *) mlt_properties_get(context->params, _s(name)));
        }

        // Flag completion
        mlt_properties_close(context->params);
        context->params = NULL;
    }
}

// The following 3 facilitate entity substitution in the SAX parser
static void on_internal_subset(void *ctx,
                               const xmlChar *name,
                               const xmlChar *publicId,
                               const xmlChar *systemId)
{
    struct _xmlParserCtxt *xmlcontext = (struct _xmlParserCtxt *) ctx;
    deserialise_context context = (deserialise_context) (xmlcontext->_private);

    context->publicId = publicId;
    context->systemId = systemId;
    xmlCreateIntSubset(context->entity_doc, name, publicId, systemId);

    // Override default entities with our parameters
    params_to_entities(context);
}

// TODO: Check this with Dan... I think this is for parameterisation
// but it's breaking standard escaped entities (like &lt; etc).
static void on_entity_declaration(void *ctx,
                                  const xmlChar *name,
                                  int type,
                                  const xmlChar *publicId,
                                  const xmlChar *systemId,
                                  xmlChar *content)
{
    struct _xmlParserCtxt *xmlcontext = (struct _xmlParserCtxt *) ctx;
    deserialise_context context = (deserialise_context) (xmlcontext->_private);

    xmlAddDocEntity(context->entity_doc, name, type, publicId, systemId, content);
}

// TODO: Check this functionality (see on_entity_declaration)
static xmlEntityPtr on_get_entity(void *ctx, const xmlChar *name)
{
    struct _xmlParserCtxt *xmlcontext = (struct _xmlParserCtxt *) ctx;
    deserialise_context context = (deserialise_context) (xmlcontext->_private);
    xmlEntityPtr e = NULL;

    // Setup for entity declarations if not ready
    if (xmlGetIntSubset(context->entity_doc) == NULL) {
        xmlCreateIntSubset(context->entity_doc, _x("mlt"), _x(""), _x(""));
        context->publicId = _x("");
        context->systemId = _x("");
    }

    // Add our parameters if not already
    params_to_entities(context);

    e = xmlGetPredefinedEntity(name);

    // Send signal to on_characters that an entity substitution is pending
    if (e == NULL) {
        e = xmlGetDocEntity(context->entity_doc, name);
        if (e != NULL)
            context->entity_is_replace = 1;
    }

    return e;
}

static void on_error(void *ctx, const char *msg, ...)
{
    struct _xmlError *err_ptr = xmlCtxtGetLastError(ctx);

    switch (err_ptr->level) {
    case XML_ERR_WARNING:
        mlt_log_warning(NULL,
                        "[producer_xml] parse warning: %s\trow: %d\tcol: %d\n",
                        err_ptr->message,
                        err_ptr->line,
                        err_ptr->int2);
        break;
    case XML_ERR_ERROR:
        mlt_log_error(NULL,
                      "[producer_xml] parse error: %s\trow: %d\tcol: %d\n",
                      err_ptr->message,
                      err_ptr->line,
                      err_ptr->int2);
        break;
    default:
    case XML_ERR_FATAL:
        mlt_log_fatal(NULL,
                      "[producer_xml] parse fatal: %s\trow: %d\tcol: %d\n",
                      err_ptr->message,
                      err_ptr->line,
                      err_ptr->int2);
        break;
    }
}

/** Convert a hexadecimal character to its value.
*/
static int tohex(char p)
{
    return isdigit(p) ? p - '0' : tolower(p) - 'a' + 10;
}

/** Decode a url-encoded string containing hexadecimal character sequences.
*/
static char *url_decode(char *dest, char *src)
{
    char *p = dest;

    while (*src) {
        if (*src == '%') {
            *p++ = (tohex(*(src + 1)) << 4) | tohex(*(src + 2));
            src += 3;
        } else {
            *p++ = *src++;
        }
    }

    *p = *src;
    return dest;
}

/** Extract the filename from a URL attaching parameters to a properties list.
*/
static void parse_url(mlt_properties properties, char *url)
{
    int i;
    int n = strlen(url);
    char *name = NULL;
    char *value = NULL;
    int is_query = 0;

    for (i = 0; i < n; i++) {
        switch (url[i]) {
        case '?':
            url[i++] = '\0';
            name = &url[i];
            is_query = 1;
            break;

        case ':':
#ifdef _WIN32
            if (url[i + 1] != '/' && url[i + 1] != '\\')
#endif
            case '=':
                if (is_query) {
                    url[i++] = '\0';
                    value = &url[i];
                }
            break;

        case '&':
            if (is_query) {
                url[i++] = '\0';
                if (name != NULL && value != NULL)
                    mlt_properties_set_string(properties, name, value);
                name = &url[i];
                value = NULL;
            }
            break;
        }
    }
    if (name != NULL && value != NULL)
        mlt_properties_set_string(properties, name, value);
}

// Quick workaround to avoid unnecessary libxml2 warnings
static int file_exists(char *name)
{
    int exists = 0;
    if (name != NULL) {
        FILE *f = mlt_fopen(name, "r");
        exists = f != NULL;
        if (exists)
            fclose(f);
    }
    return exists;
}

// This function will add remaining services in the context service stack marked
// with a "xml_retain" property to a property named "xml_retain" on the returned
// service. The property is a mlt_properties data property.

static void retain_services(struct deserialise_context_s *context, mlt_service service)
{
    mlt_properties retain_list = mlt_properties_new();
    enum service_type type;
    mlt_service retain_service = context_pop_service(context, &type);

    while (retain_service) {
        mlt_properties retain_properties = MLT_SERVICE_PROPERTIES(retain_service);

        if (mlt_properties_get_int(retain_properties, "xml_retain")) {
            // Remove the retained service from the destructors list.
            int i;
            for (i = mlt_properties_count(context->destructors) - 1; i >= 1; i--) {
                const char *name = mlt_properties_get_name(context->destructors, i);
                if (mlt_properties_get_data_at(context->destructors, i, NULL) == retain_service) {
                    mlt_properties_set_data(context->destructors,
                                            name,
                                            retain_service,
                                            0,
                                            NULL,
                                            NULL);
                    break;
                }
            }
            const char *name = mlt_properties_get(retain_properties, "id");
            if (name)
                mlt_properties_set_data(retain_list,
                                        name,
                                        retain_service,
                                        0,
                                        (mlt_destructor) mlt_service_close,
                                        NULL);
        }
        retain_service = context_pop_service(context, &type);
    }
    if (mlt_properties_count(retain_list) > 0) {
        mlt_properties_set_data(MLT_SERVICE_PROPERTIES(service),
                                "xml_retain",
                                retain_list,
                                0,
                                (mlt_destructor) mlt_properties_close,
                                NULL);
    } else {
        mlt_properties_close(retain_list);
    }
}

static deserialise_context context_new(mlt_profile profile)
{
    deserialise_context context = calloc(1, sizeof(struct deserialise_context_s));
    if (context) {
        context->producer_map = mlt_properties_new();
        context->destructors = mlt_properties_new();
        context->params = mlt_properties_new();
        context->profile = profile;
        context->seekable = 1;
        context->stack_service = mlt_deque_init();
        context->stack_types = mlt_deque_init();
        context->stack_properties = mlt_deque_init();
        context->stack_node = mlt_deque_init();
        context->stack_branch = mlt_deque_init();
        mlt_deque_push_back_int(context->stack_branch, 0);
    }
    return context;
}

static void context_close(deserialise_context context)
{
    mlt_properties_close(context->producer_map);
    mlt_properties_close(context->destructors);
    mlt_properties_close(context->params);
    mlt_deque_close(context->stack_service);
    mlt_deque_close(context->stack_types);
    mlt_deque_close(context->stack_properties);
    mlt_deque_close(context->stack_node);
    mlt_deque_close(context->stack_branch);
    xmlFreeDoc(context->entity_doc);
    free(context->lc_numeric);
    free(context);
}

mlt_producer producer_xml_init(mlt_profile profile,
                               mlt_service_type servtype,
                               const char *id,
                               char *data)
{
    xmlSAXHandler *sax, *sax_orig;
    deserialise_context context;
    mlt_properties properties = NULL;
    int i = 0;
    struct _xmlParserCtxt *xmlcontext;
    int well_formed = 0;
    char *filename = NULL;
    int is_filename = strcmp(id, "xml-string");

    // Strip file:// prefix
    if (data && strlen(data) >= 7 && strncmp(data, "file://", 7) == 0)
        data += 7;

    if (data == NULL || !strcmp(data, ""))
        return NULL;

    context = context_new(profile);
    if (context == NULL)
        return NULL;

    // Decode URL and parse parameters
    mlt_properties_set_string(context->producer_map, "root", "");
    if (is_filename) {
        mlt_properties_set_string(context->params, "_mlt_xml_resource", data);
        filename = mlt_properties_get(context->params, "_mlt_xml_resource");
        parse_url(context->params, url_decode(filename, data));

        // We need the directory prefix which was used for the xml
        if (strchr(filename, '/') || strchr(filename, '\\')) {
            char *root = NULL;
            mlt_properties_set_string(context->producer_map, "root", filename);
            root = mlt_properties_get(context->producer_map, "root");
            if (strchr(root, '/'))
                *(strrchr(root, '/')) = '\0';
            else if (strchr(root, '\\'))
                *(strrchr(root, '\\')) = '\0';

            // If we don't have an absolute path here, we're heading for disaster...
            if (root[0] != '/' && !strchr(root, ':')) {
                char *cwd = getcwd(NULL, 0);
                char *real = malloc(strlen(cwd) + strlen(root) + 2);
                sprintf(real, "%s/%s", cwd, root);
                mlt_properties_set_string(context->producer_map, "root", real);
                free(real);
                free(cwd);
            }
        }

        if (!file_exists(filename)) {
            // Try the un-converted text encoding as a fallback.
            // Fixes launching melt as child process from Shotcut on Windows
            // when there are extended characters in the path.
            filename = mlt_properties_get(context->params, "_mlt_xml_resource");
        }

        if (!file_exists(filename)) {
            context_close(context);
            return NULL;
        }
    }

    // We need to track the number of registered filters
    mlt_properties_set_int(context->destructors, "registered", 0);

    // Setup SAX callbacks for first pass
    sax = calloc(1, sizeof(xmlSAXHandler));
    sax->startElement = on_start_element;
    sax->characters = on_characters;
    sax->warning = on_error;
    sax->error = on_error;
    sax->fatalError = on_error;

    // Setup libxml2 SAX parsing
    xmlInitParser();
    xmlSubstituteEntitiesDefault(1);
    // This is used to facilitate entity substitution in the SAX parser
    context->entity_doc = xmlNewDoc(_x("1.0"));
    if (is_filename)
        xmlcontext = xmlCreateFileParserCtxt(filename);
    else
        xmlcontext = xmlCreateMemoryParserCtxt(data, strlen(data));

    // Invalid context - clean up and return NULL
    if (xmlcontext == NULL) {
        context_close(context);
        free(sax);
        return NULL;
    }

    // Parse
    sax_orig = xmlcontext->sax;
    xmlcontext->sax = sax;
    xmlcontext->_private = (void *) context;
    xmlParseDocument(xmlcontext);
    well_formed = xmlcontext->wellFormed;

    // Cleanup after parsing
    xmlcontext->sax = sax_orig;
    xmlcontext->_private = NULL;
    if (xmlcontext->myDoc)
        xmlFreeDoc(xmlcontext->myDoc);
    xmlFreeParserCtxt(xmlcontext);

    // Bad xml - clean up and return NULL
    if (!well_formed) {
        context_close(context);
        free(sax);
        return NULL;
    }

    // Setup the second pass
    context->pass++;
    if (is_filename)
        xmlcontext = xmlCreateFileParserCtxt(filename);
    else
        xmlcontext = xmlCreateMemoryParserCtxt(data, strlen(data));

    // Invalid context - clean up and return NULL
    if (xmlcontext == NULL) {
        context_close(context);
        free(sax);
        return NULL;
    }

    // Reset the stack.
    mlt_deque_close(context->stack_service);
    mlt_deque_close(context->stack_types);
    mlt_deque_close(context->stack_properties);
    mlt_deque_close(context->stack_node);
    context->stack_service = mlt_deque_init();
    context->stack_types = mlt_deque_init();
    context->stack_properties = mlt_deque_init();
    context->stack_node = mlt_deque_init();

    // Create the qglsl consumer now, if requested, so that glsl.manager
    // may exist when trying to load glsl. or movit. services.
    // The "if requested" part can come from query string qglsl=1 or when
    // a service beginning with glsl. or movit. appears in the XML.
    if (mlt_properties_get_int(context->params, "qglsl")
        && strcmp(id, "xml-nogl")
        // Only if glslManager does not yet exist.
        && !mlt_properties_get_data(mlt_global_properties(), "glslManager", NULL))
        context->qglsl = mlt_factory_consumer(profile, "qglsl", NULL);

    // Setup SAX callbacks for second pass
    sax->endElement = on_end_element;
    sax->cdataBlock = on_characters;
    sax->internalSubset = on_internal_subset;
    sax->entityDecl = on_entity_declaration;
    sax->getEntity = on_get_entity;

    // Parse
    sax_orig = xmlcontext->sax;
    xmlcontext->sax = sax;
    xmlcontext->_private = (void *) context;
    xmlParseDocument(xmlcontext);
    well_formed = xmlcontext->wellFormed;

    // Cleanup after parsing
    xmlFreeDoc(context->entity_doc);
    context->entity_doc = NULL;
    free(sax);
    xmlMemoryDump(); // for debugging
    xmlcontext->sax = sax_orig;
    xmlcontext->_private = NULL;
    if (xmlcontext->myDoc)
        xmlFreeDoc(xmlcontext->myDoc);
    xmlFreeParserCtxt(xmlcontext);

    // Get the last producer on the stack
    enum service_type type;
    mlt_service service = context_pop_service(context, &type);
    if (well_formed && service != NULL) {
        // Verify it is a producer service (mlt_type="mlt_producer")
        // (producer, chain, playlist, multitrack)
        char *type = mlt_properties_get(MLT_SERVICE_PROPERTIES(service), "mlt_type");
        if (type == NULL
            || (strcmp(type, "mlt_producer") != 0 && strcmp(type, "producer") != 0
                && strcmp(type, "chain") != 0))
            service = NULL;
    }

#ifdef DEBUG
    xmlDocPtr doc = xml_make_doc(service);
    xmlDocFormatDump(stdout, doc, 1);
    xmlFreeDoc(doc);
    service = NULL;
#endif

    if (well_formed && service != NULL) {
        char *title = mlt_properties_get(context->producer_map, "title");

        // Need the complete producer list for various reasons
        properties = context->destructors;

        // Now make sure we don't have a reference to the service in the properties
        for (i = mlt_properties_count(properties) - 1; i >= 1; i--) {
            char *name = mlt_properties_get_name(properties, i);
            if (mlt_properties_get_data_at(properties, i, NULL) == service) {
                mlt_properties_set_data(properties, name, service, 0, NULL, NULL);
                break;
            }
        }

        // We are done referencing destructor property list
        // Set this var to service properties for convenience
        properties = MLT_SERVICE_PROPERTIES(service);

        // Assign the title
        mlt_properties_set_string(properties, "title", title);

        // Optimise for overlapping producers
        mlt_producer_optimise(MLT_PRODUCER(service));

        // Handle deep copies
        if (getenv("MLT_XML_DEEP") == NULL) {
            // Now assign additional properties
            if (is_filename
                && (mlt_service_identify(service) == mlt_service_tractor_type
                    || mlt_service_identify(service) == mlt_service_playlist_type
                    || mlt_service_identify(service) == mlt_service_multitrack_type)) {
                mlt_properties_set_int(properties, "_original_type", mlt_service_identify(service));
                mlt_properties_set_string(properties,
                                          "_original_resource",
                                          mlt_properties_get(properties, "resource"));
                mlt_properties_set_string(properties, "resource", data);
            }

            // This tells consumer_xml not to deep copy
            mlt_properties_set_string(properties, "xml", "was here");
        } else {
            // Allow the project to be edited
            mlt_properties_set_string(properties, "_xml", "was here");
            mlt_properties_set_int(properties, "_mlt_service_hidden", 1);
        }

        // Make consumer available
        mlt_properties_inc_ref(MLT_CONSUMER_PROPERTIES(context->consumer));
        mlt_properties_set_data(properties,
                                "consumer",
                                context->consumer,
                                0,
                                (mlt_destructor) mlt_consumer_close,
                                NULL);

        mlt_properties_set_int(properties, "seekable", context->seekable);

        retain_services(context, service);
    } else {
        // Return null if not well formed
        service = NULL;
    }

    // Clean up
    if (context->qglsl && context->consumer != context->qglsl)
        mlt_consumer_close(context->qglsl);
    context_close(context);

    return MLT_PRODUCER(service);
}
