/**
 * \file mlt_factory.c
 * \brief the factory method interfaces
 *
 * Copyright (C) 2003-2022 Meltytech, LLC
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

#include "mlt.h"
#include "mlt_repository.h"

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** the default subdirectory of the datadir for holding presets */
#define PRESETS_DIR "/presets"

#ifdef _WIN32
#include <windows.h>
#ifdef PREFIX_LIB
#undef PREFIX_LIB
#endif
#ifdef PREFIX_DATA
#undef PREFIX_DATA
#endif
/** the default subdirectory of the libdir for holding modules (plugins) */
#define PREFIX_LIB "\\lib\\mlt"
/** the default subdirectory of the install prefix for holding module (plugin) data */
#define PREFIX_DATA "\\share\\mlt"

#elif defined(RELOCATABLE)
#ifdef PREFIX_LIB
#undef PREFIX_LIB
#endif
#ifdef PREFIX_DATA
#undef PREFIX_DATA
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
/** the default subdirectory of the libdir for holding modules (plugins) */
#define PREFIX_LIB "/PlugIns/mlt"
/** the default subdirectory of the install prefix for holding module (plugin) data */
#define PREFIX_DATA "/Resources/mlt"
#else
#include <unistd.h>
#define PREFIX_LIB "/lib/mlt-7"
/** the default subdirectory of the install prefix for holding module (plugin) data */
#define PREFIX_DATA "/share/mlt-7"
#endif
#endif

/** holds the full path to the modules directory - initialized and retained for the entire session */
static char *mlt_directory = NULL;
/** a global properties list for holding environment config data and things needing session-oriented cleanup */
static mlt_properties global_properties = NULL;
/** the global repository singleton */
static mlt_repository repository = NULL;
/** the events object for the factory events */
static mlt_properties event_object = NULL;
/** for tracking the unique_id set on each constructed service */
static int unique_id = 0;

#if defined(_WIN32) || defined(RELOCATABLE)
// Replacement for buggy dirname() on some systems.
// https://github.com/mltframework/mlt/issues/285
static char *mlt_dirname(char *path)
{
    if (path && strlen(path)) {
        char *dirsep = strrchr(path, '/');
        // Handle back slash on Windows.
        if (!dirsep)
            dirsep = strrchr(path, '\\');
        // Handle trailing slash.
        if (dirsep == &path[strlen(path) - 1]) {
            dirsep[0] = '\0';
            dirsep = strrchr(path, '/');
            if (!dirsep)
                dirsep = strrchr(path, '\\');
        }
        // Truncate string at last directory separator.
        if (dirsep)
            dirsep[0] = '\0';
    }
    return path;
}
#endif

/** Construct the repository and factories.
 *
 * \param directory an optional full path to a directory containing the modules that overrides the default and
 * the MLT_REPOSITORY environment variable
 * \return the repository
 */

mlt_repository mlt_factory_init(const char *directory)
{
    if (!global_properties)
        global_properties = mlt_properties_new();

    // Allow property refresh on a subsequent initialisation
    if (global_properties) {
        mlt_properties_set_or_default(global_properties,
                                      "MLT_NORMALISATION",
                                      getenv("MLT_NORMALISATION"),
                                      "PAL");
        mlt_properties_set_or_default(global_properties,
                                      "MLT_PRODUCER",
                                      getenv("MLT_PRODUCER"),
                                      "loader");
        mlt_properties_set_or_default(global_properties,
                                      "MLT_CONSUMER",
                                      getenv("MLT_CONSUMER"),
                                      "sdl2");
        mlt_properties_set(global_properties, "MLT_TEST_CARD", getenv("MLT_TEST_CARD"));
        mlt_properties_set_or_default(global_properties,
                                      "MLT_PROFILE",
                                      getenv("MLT_PROFILE"),
                                      "dv_pal");
        mlt_properties_set_or_default(global_properties,
                                      "MLT_DATA",
                                      getenv("MLT_DATA"),
                                      PREFIX_DATA);

#if defined(_WIN32)
        char path[1024];
        DWORD size = sizeof(path);
        GetModuleFileName(NULL, path, size);
#ifndef NODEPLOY
        char *appdir = mlt_dirname(strdup(path));
#else
        char *appdir = mlt_dirname(mlt_dirname(strdup(path)));
#endif
        mlt_properties_set(global_properties, "MLT_APPDIR", appdir);
        free(appdir);
#elif defined(RELOCATABLE)
        char path[1024];
        uint32_t size = sizeof(path);
#ifdef __APPLE__
        _NSGetExecutablePath(path, &size);
#else
        readlink("/proc/self/exe", path, size);
#endif
        char *appdir = mlt_dirname(mlt_dirname(strdup(path)));
        mlt_properties_set(global_properties, "MLT_APPDIR", appdir);
        free(appdir);
#endif
    }

    // Only initialise once
    if (mlt_directory == NULL) {
#if !defined(_WIN32) && !defined(RELOCATABLE)
        // Allow user overrides
        if (directory == NULL || !strcmp(directory, ""))
            directory = getenv("MLT_REPOSITORY");
        // If no directory is specified, default to install directory
        if (directory == NULL)
            directory = PREFIX_LIB;
        // Store the prefix for later retrieval
        mlt_directory = strdup(directory);
#else
        if (directory) {
            mlt_directory = strdup(directory);
        } else {
            char *exedir = mlt_environment("MLT_APPDIR");
            size_t size = strlen(exedir);
            if (global_properties && !getenv("MLT_DATA")) {
                mlt_directory = calloc(1, size + strlen(PREFIX_DATA) + 1);
                strcpy(mlt_directory, exedir);
                strcat(mlt_directory, PREFIX_DATA);
                mlt_properties_set(global_properties, "MLT_DATA", mlt_directory);
                free(mlt_directory);
            }
            mlt_directory = calloc(1, size + strlen(PREFIX_LIB) + 1);
            strcpy(mlt_directory, exedir);
            strcat(mlt_directory, PREFIX_LIB);
        }
#endif

        // Initialise the pool
        mlt_pool_init();

        // Create and set up the events object
        event_object = mlt_properties_new();
        mlt_events_init(event_object);
        mlt_events_register(event_object, "producer-create-request");
        mlt_events_register(event_object, "producer-create-done");
        mlt_events_register(event_object, "filter-create-request");
        mlt_events_register(event_object, "filter-create-done");
        mlt_events_register(event_object, "transition-create-request");
        mlt_events_register(event_object, "transition-create-done");
        mlt_events_register(event_object, "consumer-create-request");
        mlt_events_register(event_object, "consumer-create-done");

        // Create the repository of services
        repository = mlt_repository_init(mlt_directory);

        // Force a clean up when app closes
        atexit(mlt_factory_close);
    }

    if (global_properties) {
        char *path = getenv("MLT_PRESETS_PATH");
        if (path) {
            mlt_properties_set(global_properties, "MLT_PRESETS_PATH", path);
        } else {
            path = malloc(strlen(mlt_environment("MLT_DATA")) + strlen(PRESETS_DIR) + 1);
            strcpy(path, mlt_environment("MLT_DATA"));
            strcat(path, PRESETS_DIR);
            mlt_properties_set(global_properties, "MLT_PRESETS_PATH", path);
            free(path);
        }
    }

    return repository;
}

/** Fetch the repository.
 *
 * \return the global repository object
 */

mlt_repository mlt_factory_repository()
{
    return repository;
}

/** Fetch the events object.
 *
 * \return the global factory event object
 */

mlt_properties mlt_factory_event_object()
{
    return event_object;
}

/** Fetch the module directory used in this instance.
 *
 * \return the full path to the module directory that this session is using
 */

const char *mlt_factory_directory()
{
    return mlt_directory;
}

/** Get a value from the environment.
 *
 * \param name the name of a MLT (runtime configuration) environment variable
 * \return the value of the variable
 */

char *mlt_environment(const char *name)
{
    if (global_properties)
        return mlt_properties_get(global_properties, name);
    else
        return NULL;
}

/** Set a value in the environment.
 *
 * \param name the name of a MLT environment variable
 * \param value the value of the variable
 * \return true on error
 */

int mlt_environment_set(const char *name, const char *value)
{
    if (global_properties)
        return mlt_properties_set(global_properties, name, value);
    else
        return -1;
}

/** Set some properties common to all services.
 *
 * This sets _unique_id, \p mlt_type, \p mlt_service (unless _mlt_service_hidden), and _profile.
 *
 * \param properties a service's properties list
 * \param profile the \p mlt_profile supplied to the factory function
 * \param type the MLT service class
 * \param service the name of the service
 */

static void set_common_properties(mlt_properties properties,
                                  mlt_profile profile,
                                  const char *type,
                                  const char *service)
{
    mlt_properties_set_int(properties, "_unique_id", ++unique_id);
    mlt_properties_set(properties, "mlt_type", type);
    if (mlt_properties_get_int(properties, "_mlt_service_hidden") == 0)
        mlt_properties_set(properties, "mlt_service", service);
    if (profile != NULL)
        mlt_properties_set_data(properties, "_profile", profile, 0, NULL, NULL);
}

/** Fetch a producer from the repository.
 *
 * If you give NULL to \p service, then it will use core module's special
 * "loader"producer to load \p resource. One can override this default producer
 * by setting the environment variable MLT_PRODUCER.
 *
 * \param profile the \p mlt_profile to use
 * \param service the name of the producer (optional, defaults to MLT_PRODUCER)
 * \param resource an optional argument to the producer constructor, typically a string
 * \return a new producer
 */

mlt_producer mlt_factory_producer(mlt_profile profile, const char *service, const void *resource)
{
    mlt_producer obj = NULL;

    // Pick up the default normalizing producer if necessary
    if (service == NULL)
        service = mlt_environment("MLT_PRODUCER");

    // Offer the application the chance to 'create'
    mlt_factory_event_data data = {.name = service, .input = resource, .service = &obj};
    mlt_events_fire(event_object, "producer-create-request", mlt_event_data_from_object(&data));

    // Try to instantiate via the specified service
    if (obj == NULL) {
        obj = mlt_repository_create(repository,
                                    profile,
                                    mlt_service_producer_type,
                                    service,
                                    resource);
        mlt_events_fire(event_object, "producer-create-done", mlt_event_data_from_object(&data));
        if (obj != NULL) {
            mlt_properties properties = MLT_PRODUCER_PROPERTIES(obj);
            if (mlt_service_identify(MLT_PRODUCER_SERVICE(obj)) == mlt_service_chain_type) {
                // XML producer may return a chain
                set_common_properties(properties, profile, "chain", service);
            } else {
                set_common_properties(properties, profile, "producer", service);
            }
        }
    }
    return obj;
}

/** Fetch a filter from the repository.
 *
 * \param profile the \p mlt_profile to use
 * \param service the name of the filter
 * \param input an optional argument to the filter constructor, typically a string
 * \return a new filter
 */

mlt_filter mlt_factory_filter(mlt_profile profile, const char *service, const void *input)
{
    mlt_filter obj = NULL;

    // Offer the application the chance to 'create'
    mlt_factory_event_data data = {.name = service, .input = input, .service = &obj};
    mlt_events_fire(event_object, "filter-create-request", mlt_event_data_from_object(&data));

    if (obj == NULL) {
        obj = mlt_repository_create(repository, profile, mlt_service_filter_type, service, input);
        mlt_events_fire(event_object, "filter-create-done", mlt_event_data_from_object(&data));
    }

    if (obj != NULL) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(obj);
        set_common_properties(properties, profile, "filter", service);
    }
    return obj;
}

/** Fetch a link from the repository.
 *
 * \param service the name of the link
 * \param input an optional argument to the link constructor, typically a string
 * \return a new link
 */

mlt_link mlt_factory_link(const char *service, const void *input)
{
    mlt_link obj = NULL;

    // Offer the application the chance to 'create'
    mlt_factory_event_data data = {.name = service, .input = input, .service = &obj};
    mlt_events_fire(event_object, "link-create-request", mlt_event_data_from_object(&data));

    if (obj == NULL) {
        obj = mlt_repository_create(repository, NULL, mlt_service_link_type, service, input);
        mlt_events_fire(event_object, "link-create-done", mlt_event_data_from_object(&data));
    }

    if (obj != NULL) {
        mlt_properties properties = MLT_LINK_PROPERTIES(obj);
        set_common_properties(properties, NULL, "link", service);
    }
    return obj;
}

/** Fetch a transition from the repository.
 *
 * \param profile the \p mlt_profile to use
 * \param service the name of the transition
 * \param input an optional argument to the transition constructor, typically a string
 * \return a new transition
 */

mlt_transition mlt_factory_transition(mlt_profile profile, const char *service, const void *input)
{
    mlt_transition obj = NULL;

    // Offer the application the chance to 'create'
    mlt_factory_event_data data = {.name = service, .input = input, .service = &obj};
    mlt_events_fire(event_object, "transition-create-request", mlt_event_data_from_object(&data));

    if (obj == NULL) {
        obj = mlt_repository_create(repository, profile, mlt_service_transition_type, service, input);
        mlt_events_fire(event_object, "transition-create-done", mlt_event_data_from_object(&data));
    }

    if (obj != NULL) {
        mlt_properties properties = MLT_TRANSITION_PROPERTIES(obj);
        set_common_properties(properties, profile, "transition", service);
    }
    return obj;
}

/** Fetch a consumer from the repository.
 *
 * \param profile the \p mlt_profile to use
 * \param service the name of the consumer (optional, defaults to MLT_CONSUMER)
 * \param input an optional argument to the consumer constructor, typically a string
 * \return a new consumer
 */

mlt_consumer mlt_factory_consumer(mlt_profile profile, const char *service, const void *input)
{
    mlt_consumer obj = NULL;

    if (service == NULL)
        service = mlt_environment("MLT_CONSUMER");

    // Offer the application the chance to 'create'
    mlt_factory_event_data data = {.name = service, .input = input, .service = &obj};
    mlt_events_fire(event_object, "consumer-create-request", mlt_event_data_from_object(&data));

    if (obj == NULL) {
        obj = mlt_repository_create(repository, profile, mlt_service_consumer_type, service, input);
    }

    if (obj == NULL) {
        if (!strcmp(service, "sdl2")) {
            service = "sdl";
            obj = mlt_repository_create(repository,
                                        profile,
                                        mlt_service_consumer_type,
                                        service,
                                        input);
        } else if (!strcmp(service, "sdl_audio")) {
            service = "sdl2_audio";
            obj = mlt_repository_create(repository,
                                        profile,
                                        mlt_service_consumer_type,
                                        service,
                                        input);
        }
    }

    if (obj != NULL) {
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(obj);
        mlt_events_fire(event_object, "consumer-create-done", mlt_event_data_from_object(&data));
        set_common_properties(properties, profile, "consumer", service);
    }
    return obj;
}

/** Register an object for clean up.
 *
 * \param ptr an opaque pointer to anything allocated on the heap
 * \param destructor the function pointer of the deallocation subroutine (e.g., free or \p mlt_pool_release)
 */

void mlt_factory_register_for_clean_up(void *ptr, mlt_destructor destructor)
{
    char unique[256];
    sprintf(unique, "%08d", mlt_properties_count(global_properties));
    mlt_properties_set_data(global_properties, unique, ptr, 0, destructor, NULL);
}

/** Close the factory.
 *
 * Cleanup all resources for the session.
 */

void mlt_factory_close()
{
    if (mlt_directory != NULL) {
        mlt_properties_close(event_object);
        event_object = NULL;
#if !defined(_WIN32)
        // XXX something in here is causing Shotcut/Win32 to not exit completely
        // under certain conditions: e.g. play a playlist.
        mlt_properties_close(global_properties);
        global_properties = NULL;
#endif
        if (repository) {
            mlt_repository_close(repository);
            repository = NULL;
        }
        free(mlt_directory);
        mlt_directory = NULL;
        mlt_pool_close();
    }
}

mlt_properties mlt_global_properties()
{
    return global_properties;
}
