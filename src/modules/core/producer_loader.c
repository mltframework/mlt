/*
 * producer_loader.c -- auto-load producer by file name extension
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

#include <assert.h>
#include <ctype.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <framework/mlt.h>

static mlt_properties dictionary = NULL;
static mlt_properties normalizers = NULL;

static mlt_producer create_from(mlt_profile profile, char *file, char *services)
{
    mlt_producer producer = NULL;
    char *temp = strdup(services);
    char *service = temp;
    do {
        char *p = strchr(service, ',');
        if (p != NULL)
            *p++ = '\0';

        // If  the service name has a colon as field delimiter, then treat the
        // second field as a prefix for the file/url.
        char *prefix = strchr(service, ':');
        if (prefix) {
            *prefix++ = '\0';
            char *prefix_file = calloc(1, strlen(file) + strlen(prefix) + 1);
            strcpy(prefix_file, prefix);
            strcat(prefix_file, file);
            producer = mlt_factory_producer(profile, service, prefix_file);
            free(prefix_file);
        } else {
            producer = mlt_factory_producer(profile, service, file);
        }
        service = p;
    } while (producer == NULL && service != NULL);
    free(temp);
    return producer;
}

static mlt_producer create_producer(mlt_profile profile, char *file)
{
    mlt_producer result = NULL;

    // 1st Line - check for service:resource handling
    // And ignore drive letters on Win32 - no single char services supported.
    if (strchr(file, ':') > file + 1) {
        char *temp = strdup(file);
        char *service = temp;
        char *resource = strchr(temp, ':');
        *resource++ = '\0';
        result = mlt_factory_producer(profile, service, resource);
        free(temp);
    }

    // 2nd Line preferences
    if (result == NULL) {
        int i = 0;
        char *lookup = strdup(file);
        char *p = lookup;

        // Make backup of profile for determining if we need to use 'consumer' producer.
        mlt_profile backup_profile = mlt_profile_clone(profile);

        // We only need to load the dictionary once
        if (dictionary == NULL) {
            char temp[PATH_MAX];
            snprintf(temp, sizeof(temp), "%s/core/loader.dict", mlt_environment("MLT_DATA"));
            dictionary = mlt_properties_load(temp);
            mlt_factory_register_for_clean_up(dictionary, (mlt_destructor) mlt_properties_close);
        }

        // Convert the lookup string to lower case
        while (*p) {
            *p = tolower(*p);
            p++;
        }

        // Chop off the query string
        p = strrchr(lookup, '?');
        if (p && p > lookup && p[-1] == '\\')
            p[-1] = '\0';

        // Strip file:// prefix
        p = lookup;
        if (strncmp(lookup, "file://", 7) == 0)
            p += 7;

        // Iterate through the dictionary
        for (i = 0; result == NULL && i < mlt_properties_count(dictionary); i++) {
            char *name = mlt_properties_get_name(dictionary, i);
            if (fnmatch(name, p, 0) == 0)
                result = create_from(profile, file, mlt_properties_get_value(dictionary, i));
        }

        // Check if the producer changed the profile - xml does this.
        if (result && backup_profile && backup_profile->is_explicit
            && (profile->width != backup_profile->width || profile->height != backup_profile->height
                || profile->sample_aspect_num != backup_profile->sample_aspect_num
                || profile->sample_aspect_den != backup_profile->sample_aspect_den
                || profile->frame_rate_num != backup_profile->frame_rate_num
                || profile->frame_rate_den != backup_profile->frame_rate_den
                || profile->colorspace != backup_profile->colorspace)) {
            // Restore the original profile attributes.
            profile->display_aspect_den = backup_profile->display_aspect_den;
            profile->display_aspect_num = backup_profile->display_aspect_num;
            profile->frame_rate_den = backup_profile->frame_rate_den;
            profile->frame_rate_num = backup_profile->frame_rate_num;
            profile->height = backup_profile->height;
            profile->progressive = backup_profile->progressive;
            profile->sample_aspect_den = backup_profile->sample_aspect_den;
            profile->sample_aspect_num = backup_profile->sample_aspect_num;
            profile->width = backup_profile->width;
            profile->colorspace = backup_profile->colorspace;
            free(profile->description);
            profile->description = strdup(backup_profile->description);

            // Use the 'consumer' producer.
            mlt_producer_close(result);
            result = mlt_factory_producer(profile, "consumer", file);
        }

        mlt_profile_close(backup_profile);
        free(lookup);
    }

    // Finally, try just loading as service
    if (result == NULL)
        result = mlt_factory_producer(profile, file, NULL);

    return result;
}

static void create_filter(mlt_profile profile,
                          mlt_producer producer,
                          const char *effect,
                          int *created)
{
    mlt_filter filter = NULL;
    int i = 0;
    int exists = 0;
    char *id = strdup(effect);
    char *arg = strchr(id, ':');
    if (arg != NULL)
        *arg++ = '\0';

    for (i = 0; (filter = mlt_service_filter(MLT_PRODUCER_SERVICE(producer), i)) != NULL; i++) {
        // Check if this filter already exists
        char *filter_id = mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "mlt_service");
        if (filter_id && strcmp(id, filter_id) == 0) {
            exists = 1;
            *created = 1;
            break;
        } else if (mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "_loader") == 0) {
            // Stop at the first non-loader filter. This will be the insertion point for the new filter.
            break;
        }
    }

    if (!exists) {
        filter = mlt_factory_filter(profile, id, arg);
        if (filter) {
            mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "_loader", 1);
            mlt_producer_attach(producer, filter);
            int last_filter_index = mlt_service_filter_count(MLT_PRODUCER_SERVICE(producer)) - 1;
            if (i != last_filter_index) {
                // Move the filter to be before any non-loader filters;
                mlt_service_move_filter(MLT_PRODUCER_SERVICE(producer), last_filter_index, i);
            }
            mlt_filter_close(filter);
            *created = 1;
        }
    }
    free(id);
}

static void attach_normalizers(mlt_profile profile, mlt_producer producer, int nogl)
{
    // Loop variable
    int i;

    // Tokeniser
    mlt_tokeniser tokeniser = mlt_tokeniser_init();

    // We only need to load the normalizing properties once
    if (normalizers == NULL) {
        char temp[PATH_MAX];
        snprintf(temp, sizeof(temp), "%s/core/loader.ini", mlt_environment("MLT_DATA"));
        normalizers = mlt_properties_load(temp);
        mlt_factory_register_for_clean_up(normalizers, (mlt_destructor) mlt_properties_close);
    }

    // Apply normalizers
    for (i = 0; i < mlt_properties_count(normalizers); i++) {
        int j = 0;
        int created = 0;
        char *value = mlt_properties_get_value(normalizers, i);
        mlt_tokeniser_parse_new(tokeniser, value, ",");
        for (j = 0; !created && j < mlt_tokeniser_count(tokeniser); j++) {
            const char *filter_name = mlt_tokeniser_get_string(tokeniser, j);
            if (!nogl || (filter_name && strncmp(filter_name, "movit.", 6)))
                create_filter(profile, producer, filter_name, &created);
        }
    }

    // Close the tokeniser
    mlt_tokeniser_close(tokeniser);
}

mlt_producer producer_loader_init(mlt_profile profile,
                                  mlt_service_type type,
                                  const char *id,
                                  char *arg)
{
    // Create the producer
    mlt_producer producer = NULL;
    mlt_properties properties = NULL;
    int nogl = !strcmp(id, "loader-nogl");

    if (arg != NULL)
        producer = create_producer(profile, arg);

    if (producer != NULL)
        properties = MLT_PRODUCER_PROPERTIES(producer);

    // Attach filters if we have a producer and it isn't already xml'd :-)
    if (producer && strcmp(id, "abnormal") && strncmp(arg, "abnormal:", 9)
        && mlt_properties_get(properties, "xml") == NULL
        && mlt_properties_get(properties, "_xml") == NULL
        && mlt_service_identify(MLT_PRODUCER_SERVICE(producer)) != mlt_service_chain_type
        && mlt_properties_get(properties, "loader_normalized") == NULL)
        attach_normalizers(profile, producer, nogl);

    if (producer && mlt_service_identify(MLT_PRODUCER_SERVICE(producer)) != mlt_service_chain_type) {
        // Always let the image and audio be converted
        int created = 0;
        // movit.convert skips setting the frame->convert_image pointer if GLSL cannot be used.
        if (!nogl)
            create_filter(profile, producer, "movit.convert", &created);
        // avcolor_space and imageconvert only set frame->convert_image if it has not been set.
        create_filter(profile, producer, "avcolor_space", &created);
        if (!created)
            create_filter(profile, producer, "imageconvert", &created);
        create_filter(profile, producer, "audioconvert", &created);
    }

    // Now make sure we don't lose our identity
    if (properties != NULL)
        mlt_properties_set_int(properties, "_mlt_service_hidden", 1);

    // Return the producer
    return producer;
}
