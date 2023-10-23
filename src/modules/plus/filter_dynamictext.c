/*
 * filter_dynamictext.c -- dynamic text overlay filter
 * Copyright (C) 2011-2020 Meltytech, LLC
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

#include <libgen.h> // for basename()
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  // for stat()
#include <sys/types.h> // for stat()
#include <time.h>      // for strftime() and gtime()
#include <unistd.h>    // for stat()

#define MAX_TEXT_LEN 512

/** Get the next token and indicate whether it is enclosed in "# #".
*/
static int get_next_token(char *str, int *pos, char *token, int *is_keyword)
{
    int token_pos = 0;
    int str_len = strlen(str);

    if ((*pos) >= str_len || str[*pos] == '\0') {
        return 0;
    }

    if (str[*pos] == '#') {
        *is_keyword = 1;
        (*pos)++;
    } else {
        *is_keyword = 0;
    }

    while (*pos < str_len && token_pos < MAX_TEXT_LEN - 1) {
        if (str[*pos] == '\\' && str[(*pos) + 1] == '#') {
            // Escape Sequence - "#" preceded by "\" - copy the # into the token.
            token[token_pos] = '#';
            token_pos++;
            (*pos)++; // skip "\"
            (*pos)++; // skip "#"
        } else if (str[*pos] == '#') {
            if (*is_keyword) {
                // Found the end of the keyword
                (*pos)++;
            }
            break;
        } else {
            token[token_pos] = str[*pos];
            token_pos++;
            (*pos)++;
        }
    }

    token[token_pos] = '\0';

    return 1;
}

static void get_timecode_str(mlt_filter filter,
                             mlt_frame frame,
                             char *text,
                             mlt_time_format time_format)
{
    mlt_position frames = mlt_frame_get_position(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    char *s = mlt_properties_frames_to_time(properties, frames, time_format);
    if (s)
        strncat(text, s, MAX_TEXT_LEN - strlen(text) - 1);
}

static void get_frame_str(mlt_filter filter, mlt_frame frame, char *text)
{
    int pos = mlt_frame_get_position(frame);
    char s[12];
    snprintf(s, sizeof(s) - 1, "%d", pos);
    strncat(text, s, MAX_TEXT_LEN - strlen(text) - 1);
}

static void get_filedate_str(const char *keyword, mlt_filter filter, mlt_frame frame, char *text)
{
    mlt_producer producer = mlt_producer_cut_parent(mlt_frame_get_original_producer(frame));
    mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);
    char *filename = mlt_properties_get(producer_properties, "resource");
    struct stat file_info;

    if (!stat(filename, &file_info)) {
        const char *format = "%Y/%m/%d";
        int n = strlen("filedate") + 1;
        struct tm *time_info = gmtime(&(file_info.st_mtime));
        char *date = calloc(1, MAX_TEXT_LEN);

        if (strlen(keyword) > n)
            format = &keyword[n];
        strftime(date, MAX_TEXT_LEN, format, time_info);
        strncat(text, date, MAX_TEXT_LEN - strlen(text) - 1);
        free(date);
    }
}

static void get_localfiledate_str(const char *keyword,
                                  mlt_filter filter,
                                  mlt_frame frame,
                                  char *text)
{
    mlt_producer producer = mlt_producer_cut_parent(mlt_frame_get_original_producer(frame));
    mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);
    char *filename = mlt_properties_get(producer_properties, "resource");
    struct stat file_info;

    if (!stat(filename, &file_info)) {
        const char *format = "%Y/%m/%d";
        int n = strlen("localfiledate") + 1;
        struct tm *time_info = localtime(&(file_info.st_mtime));
        char *date = calloc(1, MAX_TEXT_LEN);

        if (strlen(keyword) > n)
            format = &keyword[n];
        strftime(date, MAX_TEXT_LEN, format, time_info);
        strncat(text, date, MAX_TEXT_LEN - strlen(text) - 1);
        free(date);
    }
}

static void get_localtime_str(const char *keyword, char *text)
{
    const char *format = "%Y/%m/%d %H:%M:%S";
    int n = strlen("localtime") + 1;
    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);
    char *date = calloc(1, MAX_TEXT_LEN);

    if (strlen(keyword) > n)
        format = &keyword[n];
    strftime(date, MAX_TEXT_LEN, format, time_info);
    strncat(text, date, MAX_TEXT_LEN - strlen(text) - 1);
    free(date);
}

static void get_resource_str(mlt_filter filter, mlt_frame frame, char *text)
{
    mlt_producer producer = mlt_producer_cut_parent(mlt_frame_get_original_producer(frame));
    mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);
    strncat(text,
            mlt_properties_get(producer_properties, "resource"),
            MAX_TEXT_LEN - strlen(text) - 1);
}

static void get_filename_str(mlt_filter filter, mlt_frame frame, char *text)
{
    mlt_producer producer = mlt_producer_cut_parent(mlt_frame_get_original_producer(frame));
    mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);
    char *filename = mlt_properties_get(producer_properties, "resource");
    if (access(filename, F_OK) == 0) {
        strncat(text, basename(filename), MAX_TEXT_LEN - strlen(text) - 1);
    }
}

static void get_basename_str(mlt_filter filter, mlt_frame frame, char *text)
{
    mlt_producer producer = mlt_producer_cut_parent(mlt_frame_get_original_producer(frame));
    mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);
    char *filename = strdup(mlt_properties_get(producer_properties, "resource"));
    if (access(filename, F_OK) == 0) {
        char *bname = basename(filename);
        char *ext = strrchr(bname, '.');
        if (ext) {
            *ext = '\0';
        }
        strncat(text, bname, MAX_TEXT_LEN - strlen(text) - 1);
    }
    free(filename);
}

static void get_createdate_str(const char *keyword, mlt_filter filter, mlt_frame frame, char *text)
{
    time_t creation_date
        = (time_t) (mlt_producer_get_creation_time(mlt_frame_get_original_producer(frame)) / 1000);
    const char *format = "%Y/%m/%d";
    int n = strlen("createdate") + 1;
    if (strlen(keyword) > n)
        format = &keyword[n];
    int text_length = strlen(text);
    strftime(text + text_length, MAX_TEXT_LEN - text_length - 1, format, localtime(&creation_date));
}

/** Perform substitution for keywords that are enclosed in "# #".
*/
static void substitute_keywords(mlt_filter filter, char *result, char *value, mlt_frame frame)
{
    char keyword[MAX_TEXT_LEN] = "";
    int pos = 0;
    int is_keyword = 0;

    while (get_next_token(value, &pos, keyword, &is_keyword)) {
        if (!is_keyword) {
            strncat(result, keyword, MAX_TEXT_LEN - strlen(result) - 1);
        } else if (!strcmp(keyword, "timecode") || !strcmp(keyword, "smpte_df")) {
            get_timecode_str(filter, frame, result, mlt_time_smpte_df);
        } else if (!strcmp(keyword, "smpte_ndf")) {
            get_timecode_str(filter, frame, result, mlt_time_smpte_ndf);
        } else if (!strcmp(keyword, "frame")) {
            get_frame_str(filter, frame, result);
        } else if (!strncmp(keyword, "filedate", 8)) {
            get_filedate_str(keyword, filter, frame, result);
        } else if (!strncmp(keyword, "localfiledate", 13)) {
            get_localfiledate_str(keyword, filter, frame, result);
        } else if (!strncmp(keyword, "localtime", 9)) {
            get_localtime_str(keyword, result);
        } else if (!strcmp(keyword, "resource")) {
            get_resource_str(filter, frame, result);
        } else if (!strcmp(keyword, "filename")) {
            get_filename_str(filter, frame, result);
        } else if (!strcmp(keyword, "basename")) {
            get_basename_str(filter, frame, result);
        } else if (!strncmp(keyword, "createdate", 10)) {
            get_createdate_str(keyword, filter, frame, result);
        } else {
            // replace keyword with property value from this frame
            mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
            char *frame_value = mlt_properties_get(frame_properties, keyword);
            if (frame_value) {
                strncat(result, frame_value, MAX_TEXT_LEN - strlen(result) - 1);
            }
        }
    }
}

/** Filter processing.
*/
static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    char *dynamic_text = mlt_properties_get(properties, "argument");
    if (!dynamic_text || !strcmp("", dynamic_text))
        return frame;

    mlt_filter text_filter = mlt_properties_get_data(properties, "_text_filter", NULL);
    mlt_properties text_filter_properties
        = mlt_frame_unique_properties(frame, MLT_FILTER_SERVICE(text_filter));

    // Apply keyword substitution before passing the text to the filter.
    char *result = calloc(1, MAX_TEXT_LEN);
    substitute_keywords(filter, result, dynamic_text, frame);
    mlt_properties_set_string(text_filter_properties, "argument", result);
    free(result);
    mlt_properties_pass_list(text_filter_properties,
                             properties,
                             "geometry family size weight style fgcolour bgcolour olcolour pad "
                             "halign valign outline opacity");
    mlt_filter_set_in_and_out(text_filter, mlt_filter_get_in(filter), mlt_filter_get_out(filter));
    return mlt_filter_process(text_filter, frame);
}

/** Constructor for the filter.
*/
mlt_filter filter_dynamictext_init(mlt_profile profile,
                                   mlt_service_type type,
                                   const char *id,
                                   char *arg)
{
    mlt_filter filter = mlt_filter_new();
    mlt_filter text_filter = mlt_factory_filter(profile, "qtext", NULL);

    if (!text_filter)
        text_filter = mlt_factory_filter(profile, "text", NULL);

    if (!text_filter)
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "Unable to create text filter.\n");

    if (filter && text_filter) {
        mlt_properties my_properties = MLT_FILTER_PROPERTIES(filter);

        // Register the text filter for reuse/destruction
        mlt_properties_set_data(my_properties,
                                "_text_filter",
                                text_filter,
                                0,
                                (mlt_destructor) mlt_filter_close,
                                NULL);

        // Assign default values
        mlt_properties_set_string(my_properties, "argument", arg ? arg : "#timecode#");
        mlt_properties_set_string(my_properties, "geometry", "0%/0%:100%x100%:100%");
        mlt_properties_set_string(my_properties, "family", "Sans");
        mlt_properties_set_string(my_properties, "size", "48");
        mlt_properties_set_string(my_properties, "weight", "400");
        mlt_properties_set_string(my_properties, "style", "normal");
        mlt_properties_set_string(my_properties, "fgcolour", "0x000000ff");
        mlt_properties_set_string(my_properties, "bgcolour", "0x00000020");
        mlt_properties_set_string(my_properties, "olcolour", "0x00000000");
        mlt_properties_set_string(my_properties, "pad", "0");
        mlt_properties_set_string(my_properties, "halign", "left");
        mlt_properties_set_string(my_properties, "valign", "top");
        mlt_properties_set_string(my_properties, "outline", "0");
        mlt_properties_set_string(my_properties, "opacity", "1.0");
        mlt_properties_set_int(my_properties, "_filter_private", 1);

        filter->process = filter_process;
    } else {
        if (filter) {
            mlt_filter_close(filter);
        }

        if (text_filter) {
            mlt_filter_close(text_filter);
        }

        filter = NULL;
    }
    return filter;
}
