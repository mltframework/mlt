/*
 * JACK Rack
 *
 * Original:
 * Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
 *
 * Modifications for MLT:
 * Copyright (C) 2004-2024 Meltytech, LLC
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

#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <ladspa.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "framework/mlt_factory.h"
#include "framework/mlt_log.h"
#include "plugin_desc.h"
#include "plugin_mgr.h"

#ifdef WITH_LV2

#include <lv2.h>

/* lv2 extenstions */
#include "lv2/buf-size/buf-size.h"
#include "lv2/parameters/parameters.h"
#include <lv2/atom/atom.h>
#include <lv2/midi/midi.h>
#include <lv2/options/options.h>
#include <lv2/port-groups/port-groups.h>
#include <lv2/port-props/port-props.h>
#include <lv2/presets/presets.h>
#include <lv2/resize-port/resize-port.h>
#include <lv2/ui/ui.h>
#include <lv2/worker/worker.h>
#include <lv2/state/state.h>

#include "lv2_urid_helper.h"

LilvNode *lv2_input_class;
LilvNode *lv2_output_class;
LilvNode *lv2_audio_class;
LilvNode *lv2_control_class;
LilvNode *lv2_atom_class;
LilvNode *lv2_integer_property;
LilvNode *lv2_logarithmic_property;
LilvNode *lv2_toggled_property;
LilvNode *lv2_enumeration_property;

static LV2_URID urid_atom_Float;
static LV2_URID urid_atom_Int;
static LV2_URID urid_bufsz_minBlockLength;
static LV2_URID urid_bufsz_maxBlockLength;
static LV2_URID urid_param_sampleRate;
static LV2_URID urid_bufsz_sequenceSize;
static LV2_URID urid_ui_updateRate;
static LV2_URID urid_ui_scaleFactor;

static URITable uri_table;

static float lv2opt_sample_rate = 48000.000000;
static uint32_t lv2opt_block_length = 4096;
static size_t lv2opt_midi_buf_size = 32768;
static float lv2opt_ui_update_hz = 60.000000;
static float lv2opt_ui_scale_factor = 1.000000;

static LV2_URID_Map map = {&uri_table, uri_table_map};
static LV2_Feature map_feature = {LV2_URID_MAP_URI, &map};
static LV2_URID_Unmap unmap = {&uri_table, uri_table_unmap};
static LV2_Feature unmap_feature = {LV2_URID_UNMAP_URI, &unmap};
static LV2_Feature boundedBlockLength_feature = {LV2_BUF_SIZE__boundedBlockLength, NULL};
static LV2_Feature fixedBlockLength_feature = {LV2_BUF_SIZE__fixedBlockLength, NULL};
static LV2_Feature powerOf2BlockLength_feature = {LV2_BUF_SIZE__powerOf2BlockLength, NULL};
static LV2_Feature loadDefaultState_feature = {LV2_STATE__loadDefaultState, NULL};
static LV2_Options_Option lv2_options_features[7];
static LV2_Feature options_feature = {LV2_OPTIONS__options, (void *) lv2_options_features};

const LV2_Feature *features[] =
  {
    &map_feature,
    &unmap_feature,
    &options_feature,
    &boundedBlockLength_feature,
    &fixedBlockLength_feature,
    &powerOf2BlockLength_feature,
    &loadDefaultState_feature,
    NULL
  };

#endif

#ifdef WITH_VST2

#include "vestige.h"

#define audioMasterGetOutputSpeakerArrangement audioMasterGetSpeakerArrangement
#define effFlagsProgramChunks (1 << 5)
#define effSetProgramName 4
#define effGetParamLabel 6
#define effGetParamDisplay 7
#define effGetVu 9
#define effEditDraw 16
#define effEditMouse 17
#define effEditKey 18
#define effEditSleep 21
#define effIdentify 22
#define effGetChunk 23
#define effSetChunk 24
#define effCanBeAutomated 26
#define effString2Parameter 27
#define effGetNumProgramCategories 28
#define effGetProgramNameIndexed 29
#define effCopyProgram 30
#define effConnectInput 31
#define effConnectOutput 32
#define effGetInputProperties 33
#define effGetOutputProperties 34
#define effGetCurrentPosition 36
#define effGetDestinationBuffer 37
#define effOfflineNotify 38
#define effOfflinePrepare 39
#define effOfflineRun 40
#define effProcessVarIo 41
#define effSetSpeakerArrangement 42
#define effSetBlockSizeAndSampleRate 43
#define effSetBypass 44
#define effGetErrorText 46
#define effVendorSpecific 50
#define effGetTailSize 52
#define effGetIcon 54
#define effSetViewPosition 55
#define effKeysRequired 57
#define effEditKeyDown 59
#define effEditKeyUp 60
#define effSetEditKnobMode 61
#define effGetMidiProgramName 62
#define effGetCurrentMidiProgram 63
#define effGetMidiProgramCategory 64
#define effHasMidiProgramsChanged 65
#define effGetMidiKeyName 66
#define effGetSpeakerArrangement 69
#define effSetTotalSampleToProcess 73
#define effSetPanLaw 74
#define effBeginLoadBank 75
#define effBeginLoadProgram 76
#define effSetProcessPrecision 77
#define effGetNumMidiInputChannels 78
#define effGetNumMidiOutputChannels 79
#define kVstAutomationOff 1
#define kVstAutomationReadWrite 4
#define kVstProcessLevelUnknown 0
#define kVstProcessLevelUser 1
#define kVstProcessLevelRealtime 2
#define kVstProcessLevelOffline 4
#define kVstProcessPrecision32 0
#define kVstVersion 2400

#if defined(CARLA_OS_WIN) && defined(__cdecl)
# define VSTCALLBACK __cdecl
#else
# define VSTCALLBACK
#endif

typedef AEffect* (*VST_Function)(audioMasterCallback);

#define kVstVersion 2400

#endif

static gboolean plugin_is_valid(const LADSPA_Descriptor *descriptor)
{
    unsigned long i;
    unsigned long icount = 0;
    unsigned long ocount = 0;

    for (i = 0; i < descriptor->PortCount; i++) {
        if (!LADSPA_IS_PORT_AUDIO(descriptor->PortDescriptors[i]))
            continue;

        if (LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[i]))
            icount++;
        else
            ocount++;
    }

    if (ocount == 0)
        return FALSE;

    return TRUE;
}

static void plugin_mgr_get_object_file_plugins(plugin_mgr_t *plugin_mgr, const char *filename)
{
    const char *dlerr;
    void *dl_handle;
    LADSPA_Descriptor_Function get_descriptor;
    const LADSPA_Descriptor *descriptor;
    unsigned long plugin_index;
    plugin_desc_t *desc, *other_desc = NULL;
    GSList *list;
    gboolean exists;
    int err;

    /* open the object file */
    dl_handle = dlopen(filename, RTLD_LAZY);
    if (!dl_handle) {
        mlt_log_info(NULL,
                     "%s: error opening shared object file '%s': %s\n",
                     __FUNCTION__,
                     filename,
                     dlerror());
        return;
    }

    /* get the get_descriptor function */
    dlerror(); /* clear the error report */

    get_descriptor = (LADSPA_Descriptor_Function) dlsym(dl_handle, "ladspa_descriptor");

    dlerr = dlerror();
    if (dlerr) {
        mlt_log_info(NULL,
                     "%s: error finding ladspa_descriptor symbol in object file '%s': %s\n",
                     __FUNCTION__,
                     filename,
                     dlerr);
        dlclose(dl_handle);
        return;
    }

#ifdef __APPLE__
    if (!get_descriptor(0)) {
        void (*constructor)(void) = dlsym(dl_handle, "_init");
        if (constructor)
            constructor();
    }
#endif

    plugin_index = 0;
    while ((descriptor = get_descriptor(plugin_index))) {
        if (!plugin_is_valid(descriptor)) {
            plugin_index++;
            continue;
        }

        /* check it doesn't already exist */
        exists = FALSE;
        for (list = plugin_mgr->all_plugins; list; list = g_slist_next(list)) {
            other_desc = (plugin_desc_t *) list->data;

            if (other_desc->id == descriptor->UniqueID) {
                exists = TRUE;
                break;
            }
        }

        if (exists) {
            mlt_log_info(NULL,
                         "Plugin %ld exists in both '%s' and '%s'; using version in '%s'\n",
                         descriptor->UniqueID,
                         other_desc->object_file,
                         filename,
                         other_desc->object_file);
            plugin_index++;
            continue;
        }

        desc = plugin_desc_new_with_descriptor(filename, plugin_index, descriptor);
        plugin_mgr->all_plugins = g_slist_append(plugin_mgr->all_plugins, desc);
        plugin_index++;
        plugin_mgr->plugin_count++;

        /* print in the splash screen */
        /* mlt_log_verbose( NULL, "Loaded plugin '%s'\n", desc->name); */
    }

    err = dlclose(dl_handle);
    if (err) {
        mlt_log_warning(NULL,
                        "%s: error closing object file '%s': %s\n",
                        __FUNCTION__,
                        filename,
                        dlerror());
    }
}

static void plugin_mgr_get_dir_plugins(plugin_mgr_t *plugin_mgr, const char *dir)
{
    DIR *dir_stream;
    struct dirent *dir_entry;
    char *file_name;
    int err;
    size_t dirlen;

    dir_stream = opendir(dir);
    if (!dir_stream) {
        /*      mlt_log_warning( NULL, "%s: error opening directory '%s': %s\n",
               __FUNCTION__, dir, strerror (errno)); */
        return;
    }

    dirlen = strlen(dir);

    while ((dir_entry = readdir(dir_stream))) {
        struct stat info;

        if (strcmp(dir_entry->d_name, ".") == 0
            || mlt_properties_get(plugin_mgr->blacklist, dir_entry->d_name)
            || strcmp(dir_entry->d_name, "..") == 0)
            continue;

        file_name = g_malloc(dirlen + 1 + strlen(dir_entry->d_name) + 1);

        strcpy(file_name, dir);
        if (file_name[dirlen - 1] == '/')
            strcpy(file_name + dirlen, dir_entry->d_name);
        else {
            file_name[dirlen] = '/';
            strcpy(file_name + dirlen + 1, dir_entry->d_name);
        }

        stat(file_name, &info);
        if (S_ISDIR(info.st_mode))
            plugin_mgr_get_dir_plugins(plugin_mgr, file_name);
        else {
            char *ext = strrchr(file_name, '.');
            if (ext
                && (strcmp(ext, ".so") == 0 || strcmp(ext, ".dll") == 0
                    || strcmp(ext, ".dylib") == 0)) {
                plugin_mgr_get_object_file_plugins(plugin_mgr, file_name);
            }
        }

        g_free(file_name);
    }

    err = closedir(dir_stream);
    if (err)
        mlt_log_warning(NULL,
                        "%s: error closing directory '%s': %s\n",
                        __FUNCTION__,
                        dir,
                        strerror(errno));
}

static void plugin_mgr_get_path_plugins(plugin_mgr_t *plugin_mgr)
{
    char *ladspa_path, *dir;

    ladspa_path = g_strdup(getenv("LADSPA_PATH"));
#ifdef _WIN32
    if (!ladspa_path) {
        ladspa_path = malloc(strlen(mlt_environment("MLT_APPDIR")) + strlen("\\lib\\ladspa") + 1);
        strcpy(ladspa_path, mlt_environment("MLT_APPDIR"));
        strcat(ladspa_path, "\\lib\\ladspa");
    }
#elif defined(RELOCATABLE)
#ifdef __APPLE__
#define LADSPA_SUBDIR "/PlugIns/ladspa"
#else
#define LADSPA_SUBDIR "/lib/ladspa"
#endif
    {
        ladspa_path = malloc(strlen(mlt_environment("MLT_APPDIR")) + strlen(LADSPA_SUBDIR) + 1);
        strcpy(ladspa_path, mlt_environment("MLT_APPDIR"));
        strcat(ladspa_path, LADSPA_SUBDIR);
    }
#else
    if (!ladspa_path)
        ladspa_path = g_strdup("/usr/local/lib/ladspa:/usr/lib/ladspa:/usr/lib64/ladspa");
#endif

    for (dir = strtok(ladspa_path, MLT_DIRLIST_DELIMITER); dir;
         dir = strtok(NULL, MLT_DIRLIST_DELIMITER))
        plugin_mgr_get_dir_plugins(plugin_mgr, dir);

    g_free(ladspa_path);
}

static gint plugin_mgr_sort(gconstpointer a, gconstpointer b)
{
    const plugin_desc_t *da;
    const plugin_desc_t *db;
    da = (const plugin_desc_t *) a;
    db = (const plugin_desc_t *) b;

    return strcasecmp(da->name, db->name);
}

plugin_mgr_t *plugin_mgr_new()
{
    plugin_mgr_t *pm;
    char dirname[PATH_MAX];

    pm = g_malloc(sizeof(plugin_mgr_t));
    pm->all_plugins = NULL;
    pm->plugins = NULL;
    pm->plugin_count = 0;

    snprintf(dirname, PATH_MAX, "%s/jackrack/blacklist.txt", mlt_environment("MLT_DATA"));
    pm->blacklist = mlt_properties_load(dirname);
    plugin_mgr_get_path_plugins(pm);

    if (!pm->all_plugins)
        mlt_log_warning(
            NULL, "No LADSPA plugins were found!\n\nCheck your LADSPA_PATH environment variable.\n");
    else
        pm->all_plugins = g_slist_sort(pm->all_plugins, plugin_mgr_sort);

    return pm;
}

void plugin_mgr_destroy(plugin_mgr_t *plugin_mgr)
{
    GSList *list;

    for (list = plugin_mgr->all_plugins; list; list = g_slist_next(list))
        plugin_desc_destroy((plugin_desc_t *) list->data);

    g_slist_free(plugin_mgr->plugins);
    g_slist_free(plugin_mgr->all_plugins);
    mlt_properties_close(plugin_mgr->blacklist);
    free(plugin_mgr);
}

void plugin_mgr_set_plugins(plugin_mgr_t *plugin_mgr, unsigned long rack_channels)
{
    GSList *list;
    plugin_desc_t *desc;

    /* clear the current plugins */
    g_slist_free(plugin_mgr->plugins);
    plugin_mgr->plugins = NULL;

    for (list = plugin_mgr->all_plugins; list; list = g_slist_next(list)) {
        desc = (plugin_desc_t *) list->data;

        if (plugin_desc_get_copies(desc, rack_channels) != 0)
            plugin_mgr->plugins = g_slist_append(plugin_mgr->plugins, desc);
    }
}

static plugin_desc_t *plugin_mgr_find_desc(plugin_mgr_t *plugin_mgr,
                                           GSList *plugins,
                                           unsigned long id)
{
    GSList *list;
    plugin_desc_t *desc;

    for (list = plugins; list; list = g_slist_next(list)) {
        desc = (plugin_desc_t *) list->data;

        if (desc->id == id)
            return desc;
    }

    return NULL;
}

plugin_desc_t *plugin_mgr_get_desc(plugin_mgr_t *plugin_mgr, unsigned long id)
{
    return plugin_mgr_find_desc(plugin_mgr, plugin_mgr->plugins, id);
}

plugin_desc_t *plugin_mgr_get_any_desc(plugin_mgr_t *plugin_mgr, unsigned long id)
{
    return plugin_mgr_find_desc(plugin_mgr, plugin_mgr->all_plugins, id);
}

#ifdef WITH_LV2

static gint lv2_mgr_sort(gconstpointer a, gconstpointer b)
{
    const lv2_plugin_desc_t *da;
    const lv2_plugin_desc_t *db;
    da = (const lv2_plugin_desc_t *) a;
    db = (const lv2_plugin_desc_t *) b;

    return strcasecmp(da->name, db->name);
}

static void lv2_mgr_get_uri_plugins(lv2_mgr_t *plugin_mgr, const LilvPlugin *plugin)
{
    unsigned long plugin_index = 0;
    lv2_plugin_desc_t *desc;

    desc = lv2_plugin_desc_new_with_descriptor(lilv_node_as_uri(lilv_plugin_get_uri(plugin)),
                                               plugin_index,
                                               plugin);
    plugin_mgr->all_plugins = g_slist_append(plugin_mgr->all_plugins, desc);
    plugin_index++;
    plugin_mgr->plugin_count++;
}

static void lv2_mgr_get_dir_plugins(lv2_mgr_t *plugin_mgr)
{
    LILV_FOREACH(plugins, i, plugin_mgr->plugin_list)
    {
        const LilvPlugin *p = lilv_plugins_get(plugin_mgr->plugin_list, i);
        lv2_mgr_get_uri_plugins(plugin_mgr, p);
    }
}

static void lv2_mgr_get_path_plugins(lv2_mgr_t *plugin_mgr)
{
    lv2_mgr_get_dir_plugins(plugin_mgr);
}

lv2_mgr_t *lv2_mgr_new()
{
    lv2_mgr_t *pm;
    char dirname[PATH_MAX];

    pm = g_malloc(sizeof(lv2_mgr_t));
    pm->all_plugins = NULL;
    pm->plugins = NULL;
    pm->plugin_count = 0;

    pm->lv2_world = lilv_world_new();
    lilv_world_load_all(pm->lv2_world);
    pm->plugin_list = (LilvPlugins *) lilv_world_get_all_plugins(pm->lv2_world);

    lv2_input_class = lilv_new_uri(pm->lv2_world, LILV_URI_INPUT_PORT);
    lv2_output_class = lilv_new_uri(pm->lv2_world, LILV_URI_OUTPUT_PORT);
    lv2_audio_class = lilv_new_uri(pm->lv2_world, LILV_URI_AUDIO_PORT);
    lv2_control_class = lilv_new_uri(pm->lv2_world, LILV_URI_CONTROL_PORT);
    lv2_atom_class = lilv_new_uri(pm->lv2_world, LV2_ATOM__AtomPort);
    lv2_integer_property = lilv_new_uri(pm->lv2_world, LV2_CORE__integer);
    lv2_logarithmic_property = lilv_new_uri(pm->lv2_world, LV2_PORT_PROPS__logarithmic);
    lv2_toggled_property = lilv_new_uri(pm->lv2_world, LV2_CORE__toggled);
    lv2_enumeration_property = lilv_new_uri(pm->lv2_world, LV2_CORE__enumeration);

    uri_table_init(&uri_table);

    urid_atom_Float = uri_table_map(&uri_table, LV2_ATOM__Float);
    urid_atom_Int = uri_table_map(&uri_table, LV2_ATOM__Int);
    urid_bufsz_minBlockLength = uri_table_map(&uri_table, LV2_BUF_SIZE__minBlockLength);
    urid_bufsz_maxBlockLength = uri_table_map(&uri_table, LV2_BUF_SIZE__maxBlockLength);
    urid_bufsz_sequenceSize = uri_table_map(&uri_table, LV2_BUF_SIZE__sequenceSize);
    urid_ui_updateRate = uri_table_map(&uri_table, LV2_UI__updateRate);
    urid_ui_scaleFactor = uri_table_map(&uri_table, LV2_UI__scaleFactor);
    urid_param_sampleRate = uri_table_map(&uri_table, LV2_PARAMETERS__sampleRate);

    lv2_options_features[0].context = LV2_OPTIONS_INSTANCE;
    lv2_options_features[0].subject = 0;
    lv2_options_features[0].key = urid_param_sampleRate;
    lv2_options_features[0].size = sizeof(float);
    lv2_options_features[0].type = urid_atom_Float;
    lv2_options_features[0].value = &lv2opt_sample_rate;

    lv2_options_features[1].context = LV2_OPTIONS_INSTANCE;
    lv2_options_features[1].subject = 0;
    lv2_options_features[1].key = urid_bufsz_minBlockLength;
    lv2_options_features[1].size = sizeof(int32_t);
    lv2_options_features[1].type = urid_atom_Int;
    lv2_options_features[1].value = &lv2opt_block_length;

    lv2_options_features[2].context = LV2_OPTIONS_INSTANCE;
    lv2_options_features[2].subject = 0;
    lv2_options_features[2].key = urid_bufsz_maxBlockLength;
    lv2_options_features[2].size = sizeof(int32_t);
    lv2_options_features[2].type = urid_atom_Int;
    lv2_options_features[2].value = &lv2opt_block_length;

    lv2_options_features[3].context = LV2_OPTIONS_INSTANCE;
    lv2_options_features[3].subject = 0;
    lv2_options_features[3].key = urid_bufsz_sequenceSize;
    lv2_options_features[3].size = sizeof(int32_t);
    lv2_options_features[3].type = urid_atom_Int;
    lv2_options_features[3].value = &lv2opt_midi_buf_size;

    lv2_options_features[4].context = LV2_OPTIONS_INSTANCE;
    lv2_options_features[4].subject = 0;
    lv2_options_features[4].key = urid_ui_updateRate;
    lv2_options_features[4].size = sizeof(float);
    lv2_options_features[4].type = urid_atom_Float;
    lv2_options_features[4].value = &lv2opt_ui_update_hz;

    lv2_options_features[5].context = LV2_OPTIONS_INSTANCE;
    lv2_options_features[5].subject = 0;
    lv2_options_features[5].key = urid_ui_scaleFactor;
    lv2_options_features[5].size = sizeof(float);
    lv2_options_features[5].type = urid_atom_Float;
    lv2_options_features[5].value = &lv2opt_ui_scale_factor;

    lv2_options_features[6].context = LV2_OPTIONS_INSTANCE;
    lv2_options_features[6].subject = 0;
    lv2_options_features[6].key = 0;
    lv2_options_features[6].size = 0;
    lv2_options_features[6].type = 0;
    lv2_options_features[6].value = NULL;

    snprintf(dirname, PATH_MAX, "%s/jackrack/lv2blacklist.txt", mlt_environment("MLT_DATA"));
    pm->blacklist = mlt_properties_load(dirname);
    lv2_mgr_get_path_plugins(pm);

    if (!pm->all_plugins)
        mlt_log_warning(
            NULL, "No LV2 plugins were found!\n\nCheck your LV2_PATH environment variable.\n");
    else
        pm->all_plugins = g_slist_sort(pm->all_plugins, lv2_mgr_sort);

    return pm;
}

void lv2_mgr_destroy(lv2_mgr_t *plugin_mgr)
{
    GSList *list;

    for (list = plugin_mgr->all_plugins; list; list = g_slist_next(list))
        lv2_plugin_desc_destroy((lv2_plugin_desc_t *) list->data);

    g_slist_free(plugin_mgr->plugins);
    g_slist_free(plugin_mgr->all_plugins);
    mlt_properties_close(plugin_mgr->blacklist);

    lilv_node_free(lv2_input_class);
    lilv_node_free(lv2_output_class);
    lilv_node_free(lv2_audio_class);
    lilv_node_free(lv2_control_class);
    lilv_node_free(lv2_atom_class);
    lilv_node_free(lv2_integer_property);
    lilv_node_free(lv2_logarithmic_property);
    lilv_node_free(lv2_toggled_property);
    lilv_node_free(lv2_enumeration_property);

    lilv_world_free(plugin_mgr->lv2_world);
    free(plugin_mgr);
}

void lv2_mgr_set_plugins(lv2_mgr_t *plugin_mgr, unsigned long rack_channels)
{
    GSList *list;
    lv2_plugin_desc_t *desc;

    /* clear the current plugins */
    g_slist_free(plugin_mgr->plugins);
    plugin_mgr->plugins = NULL;

    for (list = plugin_mgr->all_plugins; list; list = g_slist_next(list)) {
        desc = (lv2_plugin_desc_t *) list->data;

        if (desc->channels > 0 && lv2_plugin_desc_get_copies(desc, rack_channels) != 0)
            plugin_mgr->plugins = g_slist_append(plugin_mgr->plugins, desc);
    }
}

static lv2_plugin_desc_t *lv2_mgr_find_desc(lv2_mgr_t *plugin_mgr, GSList *plugins, char *id)
{
    GSList *list;
    lv2_plugin_desc_t *desc;

    for (list = plugins; list; list = g_slist_next(list)) {
        desc = (lv2_plugin_desc_t *) list->data;

        if (!strcmp(desc->uri, id))
            return desc;
    }

    return NULL;
}

lv2_plugin_desc_t *lv2_mgr_get_desc(lv2_mgr_t *plugin_mgr, char *id)
{
    return lv2_mgr_find_desc(plugin_mgr, plugin_mgr->plugins, id);
}

lv2_plugin_desc_t *lv2_mgr_get_any_desc(lv2_mgr_t *plugin_mgr, char *id)
{
    return lv2_mgr_find_desc(plugin_mgr, plugin_mgr->all_plugins, id);
}
#endif

#ifdef WITH_VST2

static gboolean vst2_is_valid(const AEffect *effect)
{
    /* unsigned long icount = 0; */
    unsigned long ocount = 0;

    /* icount = effect->numInputs; */
    ocount = effect->numOutputs;

    if (effect->magic == kEffectMagic)
        return FALSE;

    if (ocount == 0)
        return FALSE;

    return TRUE;
}

static intptr_t mlt_vst_hostCanDo(const char* const feature)
{
  mlt_log_info(NULL, "mlt_vst_hostCanDo(\"%s\")", feature);
      
  if (strcmp(feature, "supplyIdle") == 0)
    return 1;
  if (strcmp(feature, "sendVstEvents") == 0)
    return 1;
  if (strcmp(feature, "sendVstMidiEvent") == 0)
    return 1;
  if (strcmp(feature, "sendVstMidiEventFlagIsRealtime") == 0)
    return 1;
  if (strcmp(feature, "sendVstTimeInfo") == 0)
    return 1;
  if (strcmp(feature, "receiveVstEvents") == 0)
    return 1;
  if (strcmp(feature, "receiveVstMidiEvent") == 0)
    return 1;
  if (strcmp(feature, "receiveVstTimeInfo") == 0)
    return -1;
  if (strcmp(feature, "reportConnectionChanges") == 0)
    return -1;
  if (strcmp(feature, "acceptIOChanges") == 0)
    return 1;
  if (strcmp(feature, "sizeWindow") == 0)
    return 1;
  if (strcmp(feature, "offline") == 0)
    return -1;
  if (strcmp(feature, "openFileSelector") == 0)
    return -1;
  if (strcmp(feature, "closeFileSelector") == 0)
    return -1;
  if (strcmp(feature, "startStopProcess") == 0)
    return 1;
  if (strcmp(feature, "supportShell") == 0)
    return 1;
  if (strcmp(feature, "shellCategory") == 0)
    return 1;
  if (strcmp(feature, "NIMKPIVendorSpecificCallbacks") == 0)
    return -1;
      
  // unimplemented
  mlt_log_error(NULL, "mlt_vst_hostCanDo(\"%s\") - unknown feature", feature);
  return 0;
}
      
static intptr_t VSTCALLBACK
mlt_vst_audioMasterCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
  switch (opcode) 
    {
    case audioMasterAutomate:
      return 1;
      
    case audioMasterVersion:
      return kVstVersion;
      
    case audioMasterCurrentId:
      return 0;
      
    case audioMasterGetVendorString:
      strcpy((char*)ptr, "MRF");
      return 1;
      
    case audioMasterGetProductString:
      strcpy((char*)ptr, "No Organization");
      return 1;
      
    case audioMasterGetVendorVersion:
      return 0x01;
      
    case audioMasterCanDo:
      return mlt_vst_hostCanDo((const char*)ptr);
      
    case audioMasterGetLanguage:
      return kVstLangEnglish;
    }
      
  return 0;
}

static void vst2_mgr_get_object_file_plugins(vst2_mgr_t *vst2_mgr, const char *filename)
{
    const char *dlerr;
    void *dl_handle;
    VST_Function vstFn = NULL;
    AEffect *effect = NULL;
    unsigned long vst2_index;
    vst2_plugin_desc_t *desc, *other_desc = NULL;
    GSList *list;
    gboolean exists;
    /* int err; */

    /* open the object file */
    dl_handle = dlopen(filename, RTLD_LAZY);
    if (!dl_handle) {
        mlt_log_info(NULL,
                     "%s: error opening shared object file '%s': %s\n",
                     __FUNCTION__,
                     filename,
                     dlerror());
        return;
    }

    /* get the get_descriptor function */
    dlerror(); /* clear the error report */

    vstFn = (VST_Function) dlsym (dl_handle, "VSTPluginMain");       
    if (vstFn == NULL)
      vstFn = (VST_Function) dlsym (dl_handle, "main_macho");
    if (vstFn == NULL)
      vstFn = (VST_Function) dlsym (dl_handle, "main");

    if (vstFn == NULL)
      return;

    effect = vstFn(mlt_vst_audioMasterCallback);

    dlerr = dlerror();
    if (dlerr) {
        mlt_log_info(NULL,
                     "%s: error finding {VSTPluginMain, main_macho, main} symbol in object file '%s': %s\n",
                     __FUNCTION__,
                     filename,
                     dlerr);
        dlclose(dl_handle);
        return;
    }


    vst2_index = 0;
    /* while ((descriptor = get_descriptor(vst2_index))) */
    if (effect != NULL)
    {
      if (!vst2_is_valid(effect)) {
	vst2_index++;
	//continue;
      }

      /* check it doesn't already exist */
      exists = FALSE;
      for (list = vst2_mgr->all_plugins; list; list = g_slist_next(list)) {
	other_desc = (vst2_plugin_desc_t *) list->data;
       
	if (other_desc->id == effect->uniqueID) {
	  exists = TRUE;
	  break;
	}
      }
       
      if (exists) {
	mlt_log_info(NULL,
		     "Plugin %d exists in both '%s' and '%s'; using version in '%s'\n",
		     effect->uniqueID,
		     other_desc->object_file,
		     filename,
		     other_desc->object_file);
	vst2_index++;
	//continue;
      }

      desc = vst2_plugin_desc_new_with_descriptor(filename, vst2_index, effect);
      vst2_mgr->all_plugins = g_slist_append(vst2_mgr->all_plugins, desc);
      vst2_index++;
      vst2_mgr->plugin_count++;
     
      /* print in the splash screen */
      /* mlt_log_verbose( NULL, "Loaded plugin '%s'\n", desc->name); */
    }

    /* WIP temporarily disabled */
    /* err = dlclose(dl_handle); */
    /* if (err) {
              mlt_log_warning(NULL,
                              "%s: error closing object file '%s': %s\n",
                              __FUNCTION__,
                              filename,
                              dlerror());
          } */
}


static gint vst2_mgr_sort(gconstpointer a, gconstpointer b)
{
    const vst2_plugin_desc_t *da;
    const vst2_plugin_desc_t *db;
    da = (const vst2_plugin_desc_t *) a;
    db = (const vst2_plugin_desc_t *) b;

    return strcasecmp(da->name, db->name);
}

static void vst2_mgr_get_dir_plugins(vst2_mgr_t *vst2_mgr, const char *dir)
{
    DIR *dir_stream;
    struct dirent *dir_entry;
    char *file_name;
    int err;
    size_t dirlen;

    dir_stream = opendir(dir);
    if (!dir_stream) {
        /*      mlt_log_warning( NULL, "%s: error opening directory '%s': %s\n",
               __FUNCTION__, dir, strerror (errno)); */
        return;
    }

    dirlen = strlen(dir);

    while ((dir_entry = readdir(dir_stream))) {
        struct stat info;

        if (strcmp(dir_entry->d_name, ".") == 0
            || mlt_properties_get(vst2_mgr->blacklist, dir_entry->d_name)
            || strcmp(dir_entry->d_name, "..") == 0)
            continue;

        file_name = g_malloc(dirlen + 1 + strlen(dir_entry->d_name) + 1);

        strcpy(file_name, dir);
        if (file_name[dirlen - 1] == '/')
            strcpy(file_name + dirlen, dir_entry->d_name);
        else {
            file_name[dirlen] = '/';
            strcpy(file_name + dirlen + 1, dir_entry->d_name);
        }

        stat(file_name, &info);
        if (S_ISDIR(info.st_mode))
	  {
	    vst2_mgr_get_dir_plugins(vst2_mgr, file_name);
	  }
        else {
            char *ext = strrchr(file_name, '.');
            if (ext
                && (strcmp(ext, ".so") == 0 || strcasecmp(ext, ".dll") == 0
                    || strcmp(ext, ".dylib") == 0 || strcasecmp(ext, ".vst") == 0)) {
                vst2_mgr_get_object_file_plugins(vst2_mgr, file_name);
            }
        }

        g_free(file_name);
    }

    err = closedir(dir_stream);
    if (err)
        mlt_log_warning(NULL,
                        "%s: error closing directory '%s': %s\n",
                        __FUNCTION__,
                        dir,
                        strerror(errno));
}

static void vst2_mgr_get_path_plugins(vst2_mgr_t *vst2_mgr)
{
    char *vst_path, *dir;

    vst_path = g_strdup(getenv("VST_PATH"));
#ifdef _WIN32
    if (!vst_path) {
        vst_path = malloc(strlen(mlt_environment("MLT_APPDIR")) + strlen("\\lib\\vst") + 1);
        strcpy(vst_path, mlt_environment("MLT_APPDIR"));
        strcat(vst_path, "\\lib\\vst");
    }
#elif defined(RELOCATABLE)
#ifdef __APPLE__
#define VST2_SUBDIR "/PlugIns/vst"
#else
#define VST2_SUBDIR "/lib/vst"
#endif
    {
        vst_path = malloc(strlen(mlt_environment("MLT_APPDIR")) + strlen(VST2_SUBDIR) + 1);
        strcpy(vst_path, mlt_environment("MLT_APPDIR"));
        strcat(vst_path, VST2_SUBDIR);
    }
#else
    if (!vst_path)
        vst_path = g_strdup("/usr/local/lib/vst:/usr/lib/vst:/usr/lib64/vst");
#endif

    for (dir = strtok(vst_path, MLT_DIRLIST_DELIMITER); dir;
         dir = strtok(NULL, MLT_DIRLIST_DELIMITER))
      {
	vst2_mgr_get_dir_plugins(vst2_mgr, dir);
      }

    g_free(vst_path);
}

vst2_mgr_t *vst2_mgr_new()
{
    vst2_mgr_t *pm;
    char dirname[PATH_MAX];

    pm = g_malloc(sizeof(vst2_mgr_t));
    pm->all_plugins = NULL;
    pm->plugins = NULL;
    pm->plugin_count = 0;

    snprintf(dirname, PATH_MAX, "%s/jackrack/blacklist.txt", mlt_environment("MLT_DATA"));
    pm->blacklist = mlt_properties_load(dirname);
    vst2_mgr_get_path_plugins(pm);

    if (!pm->all_plugins)
        mlt_log_info(
            NULL, "No VST2 plugins were found! Check your VST_PATH environment variable.\n");
    else
        pm->all_plugins = g_slist_sort(pm->all_plugins, vst2_mgr_sort);

    return pm;
}

void vst2_mgr_destroy(vst2_mgr_t *vst2_mgr)
{
    GSList *list;

    for (list = vst2_mgr->all_plugins; list; list = g_slist_next(list))
              vst2_plugin_desc_destroy((vst2_plugin_desc_t *) list->data);

    g_slist_free(vst2_mgr->plugins);
    g_slist_free(vst2_mgr->all_plugins);
    mlt_properties_close(vst2_mgr->blacklist);
    free(vst2_mgr);
}

void vst2_mgr_set_plugins(vst2_mgr_t *vst2_mgr, unsigned long rack_channels)
{
    GSList *list;
    vst2_plugin_desc_t *desc;

    /* clear the current plugins */
    g_slist_free(vst2_mgr->plugins);
    vst2_mgr->plugins = NULL;
    for (list = vst2_mgr->all_plugins; list; list = g_slist_next(list)) {
        desc = (vst2_plugin_desc_t *) list->data;

        if (vst2_plugin_desc_get_copies(desc, rack_channels) != 0)
            vst2_mgr->plugins = g_slist_append(vst2_mgr->plugins, desc);
    }
}

static vst2_plugin_desc_t *vst2_mgr_find_desc(vst2_mgr_t *vst2_mgr,
                                           GSList *plugins,
                                           unsigned long id)
{
    GSList *list;
    vst2_plugin_desc_t *desc;

    for (list = plugins; list; list = g_slist_next(list)) {
        desc = (vst2_plugin_desc_t *) list->data;

        if (desc->id == id)
            return desc;
    }

    return NULL;
}

vst2_plugin_desc_t *vst2_mgr_get_desc(vst2_mgr_t *vst2_mgr, unsigned long id)
{
    return vst2_mgr_find_desc(vst2_mgr, vst2_mgr->plugins, id);
}

vst2_plugin_desc_t *vst2_mgr_get_any_desc(vst2_mgr_t *vst2_mgr, unsigned long id)
{
    return vst2_mgr_find_desc(vst2_mgr, vst2_mgr->all_plugins, id);
}

#endif

/* EOF */
