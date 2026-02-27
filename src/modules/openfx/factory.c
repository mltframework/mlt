/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2025-2026 Meltytech, LLC
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mlt_openfx.h"
#include <glib.h>
#include <stdbool.h>
extern OfxHost MltOfxHost;
static OfxSetHostFn ofx_set_host;
static OfxGetPluginFn ofx_get_plugin;
static OfxGetNumberOfPluginsFn ofx_get_number_of_plugins;
static int NumberOfPlugins;
mlt_properties mltofx_context;
mlt_properties mltofx_dl;

#if defined(__linux__) || defined(__FreeBSD__)

#define OFX_DIRLIST_SEP_CHARS ":;"
#define OFX_DIRSEP "/"
#include <dirent.h>

static const char *getArchStr()
{
    if (sizeof(void *) == 4) {
#if defined(__linux__)
        return "Linux-x86";
#else
        return "FreeBSD-x86";
#endif
    } else {
#if defined(__linux__)
        return "Linux-x86-64";
#else
        return "FreeBSD-x86-64";
#endif
    }
}

#define OFX_ARCHSTR getArchStr()

#elif defined(__APPLE__)

#define OFX_DIRLIST_SEP_CHARS ";:"
#if defined(__x86_64) || defined(__x86_64__)
#define OFX_ARCHSTR "MacOS-x86-64"
#else
#define OFX_ARCHSTR "MacOS"
#endif
#define OFX_DIRSEP "/"
#include <dirent.h>

#elif defined(WINDOWS) || defined(WIN32) || defined(_WIN32) || defined(__MINGW32__) \
    || defined(__MINGW64__)
#define OFX_DIRLIST_SEP_CHARS ";"
#if defined(_WIN64) || defined(__MINGW64__)
#define OFX_ARCHSTR "Win64"
#else
#define OFX_ARCHSTR "Win32"
#endif
#define OFX_DIRSEP "\\"

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <dirent.h>
#endif

#include "shlobj.h"
#include "tchar.h"
#endif

extern mlt_filter filter_openfx_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *arg);

static void plugin_mgr_destroy(mlt_properties p)
{
    int cN = mlt_properties_count(mltofx_context);

    for (int j = 0; j < cN; ++j) {
        char *id = mlt_properties_get_name(mltofx_context, j);
        mlt_properties pb = (mlt_properties) mlt_properties_get_data(mltofx_context, id, NULL);

        char *dli = mlt_properties_get(pb, "dli");
        dli[0] = 'g';
        int index = mlt_properties_get_int(pb, "index");

        OfxGetPluginFn get_plugin = mlt_properties_get_data(mltofx_dl, dli, NULL);

        if (get_plugin != NULL) {
            OfxPlugin *pt = get_plugin(index);
            if (pt == NULL)
                continue;
            pt->mainEntry(kOfxActionUnload, NULL, NULL, NULL);
        }
    }

    int N = mlt_properties_get_int(p, "N");

    for (int i = 0; i < N; ++i) {
        char tstr[12] = {
            '\0',
        };
        sprintf(tstr, "%d", i);
        void *current_dlhandle = mlt_properties_get_data(mltofx_dl, tstr, NULL);
        dlclose(current_dlhandle);
    }
}

static mlt_properties metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    snprintf(file, PATH_MAX, "%s/openfx/%s", mlt_environment("MLT_DATA"), (char *) data);
    mlt_properties result = mlt_properties_parse_yaml(file);

    mlt_properties pb = (mlt_properties) mlt_properties_get_data(mltofx_context, id, NULL);

    char *dli = mlt_properties_get(pb, "dli");
    int index = mlt_properties_get_int(pb, "index");

    OfxGetPluginFn get_plugin = mlt_properties_get_data(mltofx_dl, dli, NULL);
    OfxPlugin *pt = get_plugin(index);

mlt_properties_debug(pb, "plugin properties", stderr);

    dli[0] = 's';
    OfxSetHostFn set_host = mlt_properties_get_data(mltofx_dl, dli, NULL);
    if (set_host != NULL)
        set_host(&MltOfxHost);

    mlt_properties_set(result, "identifier", id);
    mlt_properties_set(result, "title", id);

    // parameters
    mlt_properties params = mlt_properties_new();
    mlt_properties_set_data(result,
                            "parameters",
                            params,
                            0,
                            (mlt_destructor) mlt_properties_close,
                            NULL);

    mltofx_fetch_params(pt, params, result);
    return result;
}

MLT_REPOSITORY
{
    MltOfxHost.host = (OfxPropertySetHandle) mlt_properties_new();
    mltofx_init_host_properties(MltOfxHost.host);

    // Quiet warnings from plugins that use OCIO
    char *ocio = getenv("OCIO");
    if (!ocio || !strcmp(ocio, "")) {
        // If the OCIO environment variable is not set, set it to a built-in default config
        setenv("OCIO", "ocio://ocio://default", 1);
    }

    char *dir;
    char *openfx_path = getenv("OFX_PLUGIN_PATH");
    size_t archstr_len = strlen(OFX_ARCHSTR);

    mltofx_context = mlt_properties_new();
    mltofx_dl = mlt_properties_new();

    if (openfx_path) {
        int dli = 0;
        char *saveptr;

        for (char *strptr = openfx_path;; strptr = NULL) {
            dir = strtok_r(strptr, MLT_DIRLIST_DELIMITER, &saveptr);
            if (dir == NULL)
                break;
            size_t dir_len = strlen(dir);

            DIR *d = opendir(dir);
            if (!d)
                continue;

            struct dirent *de = readdir(d);
            while (de) {
                char *name = de->d_name;

                char *bni = NULL;
                if ((bni = strstr(name, ".ofx.bundle")) != NULL && bni[11] == '\0') {
                    char *barename = g_strndup(name, (int) (bni - name) + 4);
                    size_t name_len = (size_t) (bni - name) + 4 + 7;
                    // 12b sizeof `Contents` word, 1 sizeof null byte
                    char *binpath = malloc(dir_len + name_len + 12 + (name_len - 7) + archstr_len
                                           + 1);
                    sprintf(binpath, "%s/%s/Contents/%s/%s", dir, name, OFX_ARCHSTR, barename);

                    void *dlhandle = dlopen(binpath, RTLD_LOCAL | RTLD_LAZY);

                    ofx_set_host = dlsym(dlhandle, "OfxSetHost");

                    ofx_get_plugin = dlsym(dlhandle, "OfxGetPlugin");
                    ofx_get_number_of_plugins = dlsym(dlhandle, "OfxGetNumberOfPlugins");
                    if (!ofx_get_plugin || !ofx_get_number_of_plugins)
                        goto parse_error;
                    NumberOfPlugins = ofx_get_number_of_plugins();

                    char dl_n[16] = {
                        '\0',
                    };
                    sprintf(dl_n, "%d", dli);

                    mlt_properties_set_data(mltofx_dl, dl_n, dlhandle, 0, NULL, NULL);
                    dl_n[0] = '\0';
                    sprintf(dl_n, "gn%d", dli);
                    mlt_properties_set_data(mltofx_dl,
                                            dl_n,
                                            ofx_get_number_of_plugins,
                                            0,
                                            (mlt_destructor) dlclose,
                                            NULL);
                    dl_n[0] = '\0';
                    sprintf(dl_n, "s%d", dli);
                    mlt_properties_set_data(mltofx_dl,
                                            dl_n,
                                            ofx_set_host,
                                            0,
                                            (mlt_destructor) dlclose,
                                            NULL);

                    dl_n[0] = '\0';
                    sprintf(dl_n, "g%d", dli);
                    mlt_properties_set_data(mltofx_dl,
                                            dl_n,
                                            ofx_get_plugin,
                                            0,
                                            (mlt_destructor) dlclose,
                                            NULL);

                    dli++;

                    if (ofx_get_plugin == NULL)
                        goto parse_error;

                    for (int i = 0; i < NumberOfPlugins; ++i) {
                        OfxPlugin *plugin_ptr = ofx_get_plugin(i);
                        if (!plugin_ptr)
                            goto parse_error;

                        int detected = mltofx_detect_plugin(plugin_ptr);

                        if (!detected)
                            continue;

                        char *s = NULL;
                        size_t pluginIdentifier_len = strlen(plugin_ptr->pluginIdentifier);
                        s = malloc(pluginIdentifier_len + 8);
                        sprintf(s, "openfx.%s", plugin_ptr->pluginIdentifier);

                        // if colon `:` exists in plugin identifier
                        // change it to accent sign `^` because `:`
                        // can cause issues with mlt if put in filter
                        // name
                        char *str_ptr = strchr(s, ':');
                        while (str_ptr != NULL) {
                            *str_ptr++ = '^';
                            str_ptr = strchr(str_ptr, ':');
                        }

                        mlt_properties p;
                        p = mlt_properties_new();
                        mlt_properties_set_data(mltofx_context,
                                                s,
                                                p,
                                                0,
                                                (mlt_destructor) mlt_properties_close,
                                                NULL);

                        mlt_properties_set(p, "dli", dl_n);
                        mlt_properties_set_int(p, "index", i);

                        MLT_REGISTER(mlt_service_filter_type, s, filter_openfx_init);
                        MLT_REGISTER_METADATA(mlt_service_filter_type,
                                              s,
                                              metadata,
                                              "filter_openfx.yml");
                        free(s);
                    }

                parse_error:

                    free(binpath);
                    free(barename);
                }

                de = readdir(d);
            }

            closedir(d);
        }

        mlt_properties_set_int(mltofx_dl, "N", dli);
        mlt_factory_register_for_clean_up(mltofx_dl, (mlt_destructor) plugin_mgr_destroy);
    }
}
