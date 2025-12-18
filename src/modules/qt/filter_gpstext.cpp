/*
 * filter_gpstext.c -- overlays GPS data as text on video
 * Copyright (C) 2011-2025 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "gps_parser.h"

#include <QMutex>
static QMutex f_mutex;

#define MAX_TEXT_LEN 1024

namespace {

typedef struct
{
    gps_point_raw *gps_points_r;  //raw gps data from file
    gps_point_proc *gps_points_p; //processed gps data
    int gps_points_size;
    int last_smooth_lvl;
    int last_searched_index; //used to optimize repeated searches
    int64_t first_gps_time;
    int64_t last_gps_time;
    int64_t gps_offset;
    int64_t gps_proc_start_t; //process only points after this time (epoch miliseconds)
    double speed_multiplier;
    double updates_per_second;
    char last_filename[256]; //gps file fullpath
    char interpolated;
    int swap_180;
} private_data;

} // namespace

// Sets the private data to default values and frees gps points array
static void default_priv_data(private_data *pdata)
{
    if (pdata) {
        if (pdata->gps_points_r)
            free(pdata->gps_points_r);
        if (pdata->gps_points_p)
            free(pdata->gps_points_p);
        memset(pdata, 0, sizeof(private_data));
        pdata->speed_multiplier = 1;
        pdata->updates_per_second = 1;
    }
}

// Collects often used filter private data into a single variable
static gps_private_data filter_to_gps_data(mlt_filter filter)
{
    private_data *pdata = (private_data *) filter->child;
    gps_private_data ret;

    ret.gps_points_r = pdata->gps_points_r;
    ret.gps_points_p = pdata->gps_points_p;
    ret.ptr_to_gps_points_r = &pdata->gps_points_r;
    ret.ptr_to_gps_points_p = &pdata->gps_points_p;
    ret.gps_points_size = &pdata->gps_points_size;

    ret.last_searched_index = &pdata->last_searched_index;
    ret.first_gps_time = &pdata->first_gps_time;
    ret.last_gps_time = &pdata->last_gps_time;
    ret.interpolated = &pdata->interpolated;
    ret.swap180 = &pdata->swap_180;

    ret.gps_proc_start_t = pdata->gps_proc_start_t;
    ret.last_smooth_lvl = pdata->last_smooth_lvl;
    ret.last_filename = pdata->last_filename;
    ret.filter = filter;
    return ret;
}

// Get the next token and indicate whether it is enclosed in "# #".
static int get_next_token(char *str, int *pos, char *token, int *is_keyword)
{
    int token_pos = 0;
    int str_len = strlen(str);

    if ((*pos) >= str_len || str[*pos] == '\0')
        return 0;

    if (str[*pos] == '#') {
        *is_keyword = 1;
        (*pos)++;
    } else
        *is_keyword = 0;

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

/* Processes the offset (seconds) in a time keyword and removes it
 * #gps_datetime_now +3600# -> add 1hour
 * returns value in miliesconds
*/
static int64_t extract_offset_time_ms_keyword(char *keyword)
{
    //mlt_log_info(NULL, "extract_offset_time_keyword: in keyword: %s\n", keyword);
    char *end = NULL;
    if (keyword == NULL)
        return 0;

    int val = strtol(keyword, &end, 0);

    //if found, remove the processed number
    if (val != 0) {
        if (strlen(end) == 0)
            *keyword = '\0';
        else
            memmove(keyword, end, strlen(end) + 1);
    }
    return val * 1000;
}

/** Replaces the GPS keywords with actual values, also parses any keyword extra format.
 *  Returns "--" for keywords with no valid return.
*/
static void gps_point_to_output(mlt_filter filter,
                                char *keyword,
                                char *result_gps_text,
                                int i_now,
                                int64_t req_time,
                                gps_point_proc now_gps)
{
    private_data *pdata = (private_data *) filter->child;
    char *format = NULL;
    char gps_text[MAX_TEXT_LEN];
    strcpy(gps_text, "--");

    if (i_now == -1 || pdata->gps_points_r == NULL) {
        strncat(result_gps_text, gps_text, MAX_TEXT_LEN - strlen(result_gps_text) - 1);
        return;
    }

    //assign the raw or processed values to a tmp point (depending on smoothing value)
    gps_point_proc crt_point = uninit_gps_proc_point;
    gps_point_raw raw = pdata->gps_points_r[i_now];
    if (pdata->last_smooth_lvl == 0) {
        crt_point.lat = raw.lat;
        crt_point.lat_projected = raw.lat_projected;
        crt_point.lon = raw.lon;
        crt_point.speed = raw.speed;
        crt_point.total_dist = raw.total_dist;
        crt_point.ele = raw.ele;
        crt_point.bearing = raw.bearing;
        crt_point.hr = raw.hr;
        crt_point.cad = raw.cad;
        crt_point.atemp = raw.atemp;
        crt_point.power = raw.power;
    } else {
        if (pdata->gps_points_p == NULL)
            return;
        //now_gps is already interpolated to frame time + updates_per_second + speed
        if (pdata->updates_per_second != 0)
            crt_point = now_gps;
        else
            crt_point = pdata->gps_points_p[i_now];
    }

    //check for the generic "decimals" extra-keyword and if present read a single digit after it (+whitespace)
    char *ptr = NULL;
    int use_decimals = -1;
    if ((ptr = strstr(keyword, "decimals"))) {
        ptr += strlen("decimals");
        while (ptr && isspace(*ptr))
            ptr++;
        if (ptr && isdigit(*ptr))
            use_decimals = *ptr - '0';
    }
    //check for generic "RAW" extra keyword
    bool use_raw = 0;
    if (strstr(keyword, "RAW"))
        use_raw = 1;

    /* for every keyword: we first check if the "RAW" keyword is used and if so, we print the value read from file (or --)
       then we check if there's a format keyword and apply the necessary conversion/format.
       The format must appear after the main keyword, RAW and format order is not important (strstr is used)
    */
    if (!strncmp(keyword, "gps_lat", strlen("gps_lat")) && crt_point.lat != GPS_UNINIT) {
        double val = use_raw ? raw.lat : crt_point.lat;
        if (val == GPS_UNINIT)
            return;
        snprintf(gps_text, 15, "%3.*f", (use_decimals == -1 ? 6 : use_decimals), val);
    } else if (!strncmp(keyword, "gps_lon", strlen("gps_lon")) && crt_point.lon != GPS_UNINIT) {
        double val = swap_180_if_needed(use_raw ? raw.lon : crt_point.lon);
        if (val == GPS_UNINIT)
            return;
        snprintf(gps_text, 15, "%3.*f", (use_decimals == -1 ? 6 : use_decimals), val);
    } else if (!strncmp(keyword, "gps_elev", strlen("gps_elev")) && crt_point.ele != GPS_UNINIT) {
        if (strlen(keyword) > strlen("gps_elev"))
            format = keyword + strlen("gps_elev");
        double val = convert_distance_to_format((use_raw ? raw.ele : crt_point.ele), format);
        if (val == GPS_UNINIT)
            return;
        snprintf(gps_text, 15, "%.*f", decimals_needed(val, use_decimals), val);
    } else if (!strncmp(keyword, "gps_speed", strlen("gps_speed"))
               && crt_point.speed != GPS_UNINIT) {
        if (strlen(keyword) > strlen("gps_speed"))
            format = keyword + strlen("gps_speed");
        double val = use_raw ? raw.speed : crt_point.speed;
        if (!use_raw && strstr(keyword, "vertical"))
            val = crt_point.speed_vertical;
        else if (!use_raw && strstr(keyword, "3d"))
            val = crt_point.speed_3d;
        if (val == GPS_UNINIT)
            return;
        val = convert_speed_to_format(val, format);
        snprintf(gps_text, 15, "%.*f", decimals_needed(val, use_decimals), val);
    } else if (!strncmp(keyword, "gps_hr", strlen("gps_hr")) && crt_point.hr != GPS_UNINIT) {
        double val = use_raw ? raw.hr : crt_point.hr;
        if (val == GPS_UNINIT)
            return;
        snprintf(gps_text, 15, "%.*f", (use_decimals == -1 ? 0 : use_decimals), val);
    } else if (!strncmp(keyword, "gps_bearing", strlen("gps_bearing"))
               && crt_point.bearing != GPS_UNINIT) {
        double val = use_raw ? raw.bearing : crt_point.bearing;
        if (val == GPS_UNINIT)
            return;
        snprintf(gps_text, 15, "%.*f", (use_decimals == -1 ? 0 : use_decimals), val);
    } else if (!strncmp(keyword, "gps_compass", strlen("gps_compass"))
               && crt_point.bearing != GPS_UNINIT) {
        const char *val = (const char *) (bearing_to_compass(use_raw ? raw.bearing
                                                                     : crt_point.bearing));
        snprintf(gps_text, 4, "%s", val);
    } else if (!strncmp(keyword, "gps_cadence", strlen("gps_cadence"))
               && crt_point.cad != GPS_UNINIT) {
        double val = use_raw ? raw.cad : crt_point.cad;
        if (val == GPS_UNINIT)
            return;
        snprintf(gps_text, 15, "%.*f", (use_decimals == -1 ? 0 : use_decimals), crt_point.cad);
    } else if (!strncmp(keyword, "gps_temperature", strlen("gps_temperature"))
               && crt_point.atemp != GPS_UNINIT) {
        double val = use_raw ? raw.atemp : crt_point.atemp;
        if (val == GPS_UNINIT)
            return;
        if (strstr(keyword, "F"))
            val = val * 1.8 + 32;
        else if (strstr(keyword, "K"))
            val = val + 273.15;
        snprintf(gps_text, 15, "%.*f", (use_decimals == -1 ? 0 : use_decimals), val);
    } else if (!strncmp(keyword, "gps_power", strlen("gps_power"))
               && crt_point.power != GPS_UNINIT) {
        double val = use_raw ? raw.power : crt_point.power;
        if (val == GPS_UNINIT)
            return;
        snprintf(gps_text, 15, "%.*f", (use_decimals == -1 ? 0 : use_decimals), val);
    } else if (!strncmp(keyword, "gps_vdist_up", strlen("gps_vdist_up"))
               && crt_point.elev_up != GPS_UNINIT) {
        if (strlen(keyword) > strlen("gps_vdist_up"))
            format = keyword + strlen("gps_vdist_up");
        double val = convert_distance_to_format(fabs(crt_point.elev_up), format);
        snprintf(gps_text, 15, "%.*f", decimals_needed(val, use_decimals), val);
    } else if (!strncmp(keyword, "gps_vdist_down", strlen("gps_vdist_down"))
               && crt_point.elev_down != GPS_UNINIT) {
        if (strlen(keyword) > strlen("gps_vdist_down"))
            format = keyword + strlen("gps_vdist_down");
        double val = convert_distance_to_format(fabs(crt_point.elev_down), format);
        snprintf(gps_text, 15, "%.*f", decimals_needed(val, use_decimals), val);
    } else if (!strncmp(keyword, "gps_dist_uphill", strlen("gps_dist_uphill"))
               && crt_point.dist_up != GPS_UNINIT) {
        if (strlen(keyword) > strlen("gps_dist_uphill"))
            format = keyword + strlen("gps_dist_uphill");
        double val = convert_distance_to_format(crt_point.dist_up, format);
        snprintf(gps_text, 15, "%.*f", decimals_needed(val, use_decimals), val);
    } else if (!strncmp(keyword, "gps_dist_downhill", strlen("gps_dist_downhill"))
               && crt_point.dist_down != GPS_UNINIT) {
        if (strlen(keyword) > strlen("gps_dist_downhill"))
            format = keyword + strlen("gps_dist_downhill");
        double val = convert_distance_to_format(crt_point.dist_down, format);
        snprintf(gps_text, 15, "%.*f", decimals_needed(val, use_decimals), val);
    } else if (!strncmp(keyword, "gps_dist_flat", strlen("gps_dist_flat"))
               && crt_point.dist_flat != GPS_UNINIT) {
        if (strlen(keyword) > strlen("gps_dist_flat"))
            format = keyword + strlen("gps_dist_flat");
        double val = convert_distance_to_format(crt_point.dist_flat, format);
        snprintf(gps_text, 15, "%.*f", decimals_needed(val, use_decimals), val);
    }
    //NOTE: gps_dist must be below gps_dist_up/down/flat or it'll match them
    else if (!strncmp(keyword, "gps_dist", strlen("gps_dist"))
             && crt_point.total_dist != GPS_UNINIT) {
        if (strlen(keyword) > strlen("gps_dist"))
            format = keyword + strlen("gps_dist");
        double val = convert_distance_to_format((use_raw ? raw.total_dist : crt_point.total_dist),
                                                format);
        if (val == GPS_UNINIT)
            return;
        snprintf(gps_text, 15, "%.*f", decimals_needed(val, use_decimals), val);
    } else if (!strncmp(keyword, "gps_grade_percentage", strlen("gps_grade_percentage"))
               && crt_point.grade_p != GPS_UNINIT) {
        double val = crt_point.grade_p;
        snprintf(gps_text, 15, "%+.*f", (use_decimals == -1 ? 0 : use_decimals), val);
    } else if (!strncmp(keyword, "gps_grade_degrees", strlen("gps_grade_degrees"))
               && crt_point.grade_p != GPS_UNINIT) {
        double val = to_deg(atan(crt_point.grade_p / 100.0));
        snprintf(gps_text, 15, "%+.*f", (use_decimals == -1 ? 0 : use_decimals), val);
    } else if (!strncmp(keyword, "gps_datetime_now", strlen("gps_datetime_now"))
               && raw.time != GPS_UNINIT) {
        int64_t val = 0;
        char *offset = NULL;
        if ((offset = strstr(keyword, "+")) != NULL || (offset = strstr(keyword, "-")) != NULL)
            val = extract_offset_time_ms_keyword(offset);
        if (strlen(keyword) > strlen("gps_datetime_now"))
            format = keyword + strlen("gps_datetime_now");
        mseconds_to_timestring(raw.time + val, format, gps_text);
    }
    strncat(result_gps_text, gps_text, MAX_TEXT_LEN - strlen(result_gps_text) - 1);
    //	mlt_log_info(NULL, "filter_gps.c gps_point_to_output, keyword=%s, result_gps_text=%s\n", keyword, result_gps_text);
}

// Returns the unix time (miliseconds) of "Media Created" metadata, or fallbacks to "Modified Time" from OS
static int64_t get_original_video_file_time_mseconds(mlt_frame frame)
{
    mlt_producer producer = mlt_producer_cut_parent(mlt_frame_get_original_producer(frame));
    return mlt_producer_get_creation_time(producer);
}

//Restricts how many updates per second are done (the searched gps - frame time is altered)
static int64_t restrict_updates(int64_t fr, double upd_per_sec)
{
    if (upd_per_sec == 0) // = disabled
        return fr;
    int64_t rez = fr - fr % (int) (1000.0 / upd_per_sec);
    //mlt_log_info(NULL, "_time restrict: %d [%f x] -> %d\n", fr%100000, upd_per_sec, rez%100000);
    return rez;
}

/** Returns absolute* current frame time in miliseconds
 *  (original file creation + current timecode)
 *  *also applies updates_per_second and speed_multiplier
 */
static int64_t get_current_frame_time_ms(mlt_filter filter, mlt_frame frame)
{
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    private_data *pdata = (private_data *) filter->child;
    int64_t file_time = 0, fr_time = 0;

    file_time = get_original_video_file_time_mseconds(frame);
    mlt_position frame_position = mlt_frame_original_position(frame);

    f_mutex.lock();
    char *s = mlt_properties_frames_to_time(properties, frame_position, mlt_time_clock);
    if (s) {
        int h = 0, m = 0, sec = 0, msec = 0;
        sscanf(s, "%d:%d:%d.%d", &h, &m, &sec, &msec);
        fr_time = (h * 3600 + m * 60 + sec) * 1000 + msec;
    } else
        mlt_log_warning(filter,
                        "get_current_frame_time_ms time string null, giving up "
                        "[mlt_frame_original_position()=%d], retry result:%s\n",
                        frame_position,
                        mlt_properties_frames_to_time(properties, frame_position, mlt_time_clock));
    f_mutex.unlock();

    return file_time
           + restrict_updates(fr_time, pdata->updates_per_second) * pdata->speed_multiplier;
}

/** Reads and updates all necessary filter properties, and processes the gps data if needed
 */
static void process_filter_properties(mlt_filter filter, mlt_frame frame)
{
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    private_data *pdata = (private_data *) filter->child;
    char do_smoothing = 0, do_processing = 0;

    //read properties
    int read_video_offset1 = mlt_properties_get_int(properties, "time_offset");
    int read_smooth_val = mlt_properties_get_int(properties, "smoothing_value");
    char *read_gps_processing_start_time = mlt_properties_get(properties,
                                                              "gps_processing_start_time");
    double read_speed_multiplier = mlt_properties_get_double(properties, "speed_multiplier");
    double read_updates_per_second = mlt_properties_get_double(properties, "updates_per_second");

    int64_t original_video_time = get_original_video_file_time_mseconds(frame);
    // mlt_log_info(filter, "process_filter_properties - read values: offset1=%d, smooth=%d, gps_start_time=%s, speed=%f, updates=%f\n",
    //  	read_video_offset1, read_smooth_val, read_gps_processing_start_time, read_speed_multiplier, read_updates_per_second);

    //process properties
    pdata->gps_offset = (int64_t) read_video_offset1 * 1000;
    pdata->speed_multiplier = (read_speed_multiplier ? read_speed_multiplier : 1);
    pdata->updates_per_second = read_updates_per_second;

    if (pdata->last_smooth_lvl != read_smooth_val) {
        pdata->last_smooth_lvl = read_smooth_val;
        do_smoothing = 1;
    }

    if (read_gps_processing_start_time != NULL) {
        int64_t gps_proc_t = 0;
        if (strlen(read_gps_processing_start_time) != 0
            && strcmp(read_gps_processing_start_time, "yyyy-MM-dd hh:mm:ss"))
            gps_proc_t = datetimeXMLstring_to_mseconds(read_gps_processing_start_time,
                                                       (char *) "yyyy-MM-dd hh:mm:ss");
        if (gps_proc_t != pdata->gps_proc_start_t) {
            pdata->gps_proc_start_t = gps_proc_t;
            do_processing = 1;
        }
    } else if (pdata->gps_proc_start_t != 0) {
        pdata->gps_proc_start_t = 0;
        do_processing = 1;
    }

    char video_start_text[255], gps_start_text[255];
    mseconds_to_timestring(original_video_time, NULL, video_start_text);
    mseconds_to_timestring(pdata->first_gps_time, NULL, gps_start_text);

    if (do_smoothing)
        process_gps_smoothing(filter_to_gps_data(filter), 1);
    else if (do_processing) //smoothing also does processing
        recalculate_gps_data(filter_to_gps_data(filter));

    char gps_processing_start_now[255];
    int64_t gps_now = get_current_frame_time_ms(filter, frame) + pdata->gps_offset;
    mseconds_to_timestring(gps_now, NULL, gps_processing_start_now);

    //write properties
    mlt_properties_set(properties, "gps_start_text", gps_start_text);
    mlt_properties_set(properties, "video_start_text", video_start_text);
    mlt_properties_set_int(properties,
                           "auto_gps_offset_start",
                           (pdata->first_gps_time - original_video_time) / 1000);
    mlt_properties_set_int(properties,
                           "auto_gps_offset_now",
                           (pdata->first_gps_time - get_current_frame_time_ms(filter, frame))
                               / 1000);
    mlt_properties_set(properties, "auto_gps_processing_start_now", gps_processing_start_now);
}

/** Replaces file_datetime_now with absolute time-date string (video created + current timecode)
 *  (time includes speed_multiplier and updates per second) */
static void get_current_frame_time_str(char *keyword,
                                       mlt_filter filter,
                                       mlt_frame frame,
                                       char *result)
{
    int64_t val = 0;
    char *offset = NULL, *format = NULL;

    //check for seconds offset
    if ((offset = strstr(keyword, "+")) != NULL || (offset = strstr(keyword, "-")) != NULL)
        val = extract_offset_time_ms_keyword(offset);

    //check for time format
    char text[MAX_TEXT_LEN];
    if (strlen(keyword) > strlen("file_datetime_now"))
        format = keyword + strlen("file_datetime_now");

    mseconds_to_timestring(val + get_current_frame_time_ms(filter, frame), format, text);
    strncat(result, text, MAX_TEXT_LEN - strlen(result) - 1);
}

/** Perform substitution for keywords that are enclosed in "# #".
 *  Also prepares [current] gps point
*/
static void substitute_keywords(mlt_filter filter, char *result, char *value, mlt_frame frame)
{
    private_data *pdata = (private_data *) filter->child;
    char keyword[MAX_TEXT_LEN] = "";
    int pos = 0, is_keyword = 0;

    //prepare current gps point here so it is reused for all keywords
    int64_t video_time_synced = get_current_frame_time_ms(filter, frame) + pdata->gps_offset;
    int i_now = binary_search_gps(filter_to_gps_data(filter), video_time_synced);
    int max_gps_diff_ms = get_max_gps_diff_ms(filter_to_gps_data(filter));
    gps_point_proc crt_point = uninit_gps_proc_point;
    if (i_now != -1 && pdata->gps_points_p
        && time_val_between_indices_proc(video_time_synced,
                                         pdata->gps_points_p,
                                         i_now,
                                         pdata->gps_points_size - 1,
                                         max_gps_diff_ms,
                                         false))
        crt_point = weighted_middle_point_proc(&pdata->gps_points_p[i_now],
                                               &pdata->gps_points_p[i_now + 1],
                                               video_time_synced,
                                               max_gps_diff_ms);

    while (get_next_token(value, &pos, keyword, &is_keyword)) {
        if (!is_keyword) {
            strncat(result, keyword, MAX_TEXT_LEN - strlen(result) - 1);
        } else if (!strncmp(keyword, "gps_", strlen("gps_"))) {
            gps_point_to_output(filter, keyword, result, i_now, video_time_synced, crt_point);
        } else if (!strncmp(keyword, "file_datetime_now", strlen("file_datetime_now"))) {
            get_current_frame_time_str(keyword, filter, frame, result);
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

//checks if a new file is selected and if so, does private_data cleanup and calls xml_parse_file
static void process_file(mlt_filter filter, mlt_frame frame)
{
    private_data *pdata = (private_data *) filter->child;
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    char *filename = mlt_properties_get(properties, "resource");
    if (filename == NULL)
        filename = mlt_properties_get(properties,
                                      "gps.file"); /* for backwards compatibility with v1 */

    //if there's no file selected just return
    if (!filename || !strcmp(filename, ""))
        return;

    //check if the file has been changed, if not, current data is ok, do nothing
    if (strcmp(pdata->last_filename, filename)) {
        // mlt_log_info(filter, "Reading new file: last_filename (%s) != entered_filename (%s), swap_180 = %d\n", pdata->last_filename, filename, swap);
        default_priv_data(pdata);
        strcpy(pdata->last_filename, filename);

        if (qxml_parse_file(filter_to_gps_data(filter)) == 1) {
            get_first_gps_time(filter_to_gps_data(filter));
            get_last_gps_time(filter_to_gps_data(filter));

            //assume smooth is 5 (default) so we can guarantee *gps_points_p and save some time
            pdata->last_smooth_lvl = 5;
            process_gps_smoothing(filter_to_gps_data(filter), 1);
        } else {
            default_priv_data(pdata);
            //store name in pdata or it'll retry reading every frame if bad file
            strcpy(pdata->last_filename, filename);
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

    mlt_filter text_filter = (mlt_filter) mlt_properties_get_data(properties, "_text_filter", NULL);
    mlt_properties text_filter_properties
        = mlt_frame_unique_properties(frame, MLT_FILTER_SERVICE(text_filter));

    //read and process file if needed
    process_file(filter, frame);
    process_filter_properties(filter, frame);

    // Apply keyword substitution before passing the text to the filter.
    char *result = (char *) calloc(1, MAX_TEXT_LEN);
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

/** Destructor for the filter.
*/
static void filter_close(mlt_filter filter)
{
    private_data *pdata = (private_data *) filter->child;

    default_priv_data(pdata);
    free(pdata);

    filter->child = NULL;
    filter->close = NULL;
    filter->parent.close = NULL;
    mlt_service_close(&filter->parent);
}

/** Constructor for the filter.
*/
extern "C" {
mlt_filter filter_gpstext_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    (void) type; // unused
    (void) id;   // unused

    mlt_filter filter = mlt_filter_new();
    private_data *pdata = (private_data *) calloc(1, sizeof(private_data));
    default_priv_data(pdata);
    mlt_filter text_filter = mlt_factory_filter(profile, "qtext", NULL);

    if (!text_filter)
        text_filter = mlt_factory_filter(profile, "text", NULL);

    if (!text_filter)
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "Unable to create text filter.\n");

    if (filter && text_filter && pdata) {
        mlt_properties my_properties = MLT_FILTER_PROPERTIES(filter);

        // Register the text filter for reuse/destruction
        mlt_properties_set_data(my_properties,
                                "_text_filter",
                                text_filter,
                                0,
                                (mlt_destructor) mlt_filter_close,
                                NULL);

        // Assign default values
        mlt_properties_set_string(my_properties,
                                  "argument",
                                  arg ? arg
                                      : "Speed: #gps_speed#km/h\n"
                                        "Distance: #gps_dist#m\n"
                                        "Altitude: #gps_elev#m\n\n"
                                        "GPS time: #gps_datetime_now# UTC\n"
                                        "GPS location: #gps_lat#, #gps_lon#");
        mlt_properties_set_string(my_properties, "geometry", "10%/10%:80%x80%:100%");
        mlt_properties_set_string(my_properties, "family", "Sans");
        mlt_properties_set_string(my_properties, "size", "26");
        mlt_properties_set_string(my_properties, "weight", "400");
        mlt_properties_set_string(my_properties, "style", "normal");
        mlt_properties_set_string(my_properties, "fgcolour", "0xffffffff");
        mlt_properties_set_string(my_properties, "bgcolour", "0x00000000");
        mlt_properties_set_string(my_properties, "olcolour", "0x000000ff");
        mlt_properties_set_string(my_properties, "pad", "5");
        mlt_properties_set_string(my_properties, "halign", "left");
        mlt_properties_set_string(my_properties, "valign", "bottom");
        mlt_properties_set_string(my_properties, "outline", "0");
        mlt_properties_set_string(my_properties, "opacity", "1.0");
        mlt_properties_set_int(my_properties, "_filter_private", 1);

        mlt_properties_set_int(my_properties, "time_offset", 0);
        mlt_properties_set_int(my_properties, "smoothing_value", 5);
        mlt_properties_set_int(my_properties, "speed_multiplier", 1);
        mlt_properties_set_int(my_properties, "updates_per_second", 1);

        filter->close = filter_close;
        filter->process = filter_process;
        filter->child = pdata;
    } else {
        if (filter) {
            mlt_filter_close(filter);
        }

        if (text_filter) {
            mlt_filter_close(text_filter);
        }

        free(pdata);
        filter = NULL;
    }
    return filter;
}
}
