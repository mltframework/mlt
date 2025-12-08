/*
 * gps_parser.h -- Contains gps parsing (.gpx and .tcx) and processing code
 * Copyright (C) 2011-2022 Meltytech, LLC
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

#define __USE_XOPEN
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "gps_parser.h"

#define _x (const xmlChar *)
#define _s (const char *)
#define _qxml_getthestring(s) qUtf8Printable((s).toString())

// Maps are usually in ESPG3857 format, not in GPS format EPSG4326 / WG84. We have to project
// the latitude, otherwise big maps have wrong aspect ratio
double project_latitude(double lat)
{
    return log(tan(((90 + (double)lat) * MATH_PI) / 360)) / (MATH_PI / 180);
}

double unproject_latitude(double lat_projected)
{
    return (atan(pow(exp(1), lat_projected * (MATH_PI / 180)))*360) / MATH_PI - 90;
}

//shifts all (longitude) values from near 180 to 0
double get_180_swapped(double lon)
{
    if (lon == GPS_UNINIT)
        return lon;
    return lon + (lon > 0 ? -180 : 180);
}

//testing max valid time between 2 points = 10 * avg_gps_time, better option would be a user entered value
int get_max_gps_diff_ms(gps_private_data gdata)
{
    return (10 * get_avg_gps_time_ms(gdata)) * 1000;
}

/** Computes the average time between recorded gps points
 * Note: needs first/last gps time already computed
*/
double get_avg_gps_time_ms(gps_private_data gdata)
{
    int64_t tdiff = *gdata.last_gps_time - *gdata.first_gps_time;
    int gp_size = *gdata.gps_points_size;
    if (gp_size == 0)
        return 0;
    return (double) tdiff / gp_size;
}

/* Converts the datetime string from gps file into seconds since epoch
 * Note: assumes UTC
 */
int64_t datetimeXMLstring_to_mseconds(const char *text, char *format /* = NULL*/)
{
    char def_format[] = "%Y-%m-%dT%H:%M:%S";
    int64_t ret = 0;
    int ms = 0;
    struct tm tm_time;
    //samples: 2020-07-11T09:03:23.000Z or 2021-02-27T12:10:00+00:00
    tm_time.tm_isdst = -1; //force dst detection

    if (format == NULL)
        format = def_format;

    if (strptime(text, format, &tm_time) == NULL) {
        mlt_log_warning(
            NULL,
            "filter_gpsText.c datetimeXMLstring_to_seconds strptime failed on string: %.25s",
            text);
        return 0;
    }

    ret = internal_timegm(&tm_time);

    //check if we have miliseconds, 3 digits only
    const char *ms_part = strchr(text, '.');
    if (ms_part != NULL) {
        ms = strtol(ms_part + 1, NULL, 10);
        while (abs(ms) > 999)
            ms /= 10;
    }
    ret = ret * 1000 + ms;

    // mlt_log_info(NULL, "datetimeXMLstring_to_mseconds: text:%s, ms:%d (/1000)", text, ret/1000);
    return ret;
}

//checks if provided char* is only made of whitespace chars
static int is_whitespace_string(char *str)
{
    unsigned i;
    for (i = 0; i < strlen(str); i++) {
        if (!isspace(str[i]))
            return 0;
    }
    return 1;
}

//Converts miliseconds to a date-time with optional format (no miliesconds in output)
void mseconds_to_timestring(int64_t seconds, char *format, char *result)
{
    time_t secs = llabs(seconds) / 1000;
    struct tm *ptm = gmtime(&secs);
    if (!format || is_whitespace_string(format))
        strftime(result, 25, "%Y-%m-%d %H:%M:%S", ptm);
    else
        strftime(result, 50, format, ptm);
}

//returns the distance between 2 gps points (accurate for very large distances)
double distance_haversine_2p(double p1_lat, double p1_lon, double p2_lat, double p2_lon)
{
    const int R = 6371000;
    double dlat = to_rad(p2_lat - p1_lat);
    double dlon = to_rad(p2_lon - p1_lon);
    double a = sin(dlat / 2.0) * sin(dlat / 2.0)
               + cos(to_rad(p1_lat)) * cos(to_rad(p2_lat)) * sin(dlon / 2.0) * sin(dlon / 2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1 - a));
    return R * c; //return is in meters because radius is in meters
}

// Returns the distance between 2 gps points (uses haversine formula if needed)
double distance_equirectangular_2p(double p1_lat, double p1_lon, double p2_lat, double p2_lon)
{
    //use the haversine formula for very far points
    if (fabs(p1_lat - p2_lat) > 0.05
        || fabs(p1_lat - p2_lat) > 0.05) { //~5.5km at equator to ~2km at arctic circle
        mlt_log_info(NULL,
                     "distance_equirectangular_2p: points are too far away, doing haversine (%f,%f "
                     "to %f,%f)\n",
                     p1_lat,
                     p1_lon,
                     p2_lat,
                     p2_lon);
        return distance_haversine_2p(p1_lat, p1_lon, p2_lat, p2_lon);
    }

    const int R = 6371000;
    double x = (to_rad(p2_lon) - to_rad(p1_lon)) * cos((to_rad(p2_lat) + to_rad(p1_lat)) / 2.0);
    double y = to_rad(p1_lat) - to_rad(p2_lat);
    return sqrt(x * x + y * y) * R;
    //NOTE: adding altitude is very risky, GPS altitude seems pretty bad and with sudden large jumps
    //maybe consider adding distance_2d and distance_3d fields //sqrt(2D_distance^2 + delta_alt^2)
}

// Computes bearing from 2 gps points
double bearing_2p(double p1_lat, double p1_lon, double p2_lat, double p2_lon)
{
    double lat1 = to_rad(p1_lat);
    double lat2 = to_rad(p2_lat);
    double dlon = to_rad(p2_lon - p1_lon);

    double y = sin(dlon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dlon);
    double bearing = to_deg(atan2(y, x));
    //normalize to 0-360
    return fmod(bearing + 360, 360);
}

/** Searches for and returns the first valid time with locaton in the gps_points_r array
 *  Returns in miliesconds, 0 on error
*/
void get_first_gps_time(gps_private_data gdata)
{
    gps_point_raw *gps_points = gdata.gps_points_r;
    if (!gps_points) {
        *gdata.first_gps_time = 0;
        return;
    }

    int i = 0;
    while (i < *gdata.gps_points_size) {
        if (gps_points[i].time && has_valid_location(gps_points[i])) {
            //mlt_log_info(gdata.filter, "get_first_gps_time found: %d", gps_points[i].time);
            *gdata.first_gps_time = gps_points[i].time;
            return;
        }
        i++;
    }
    *gdata.first_gps_time = 0;
}

/** Searches for and returns the last valid time with location in the gps_points_r array
 *	returns in miliesconds,	 returns 0 on error
*/
void get_last_gps_time(gps_private_data gdata)
{
    gps_point_raw *gps_points = gdata.gps_points_r;
    if (!gps_points) {
        *gdata.last_gps_time = 0;
        return;
    }

    int i = *gdata.gps_points_size - 1;
    while (i >= 0) {
        if (gps_points[i].time && has_valid_location(gps_points[i])) {
            //mlt_log_info(gdata.filter, "get_last_gps_time found: %d", gps_points[i].time);
            *gdata.last_gps_time = gps_points[i].time;
            return;
        }
        i--;
    }
    *gdata.last_gps_time = 0;
}

//checks if time value val is between gps_points[i] and gps_points[i+1] with size checks
//note: size must be inclusive (ie: gps_points[size] valid!)
int time_val_between_indices_raw(
    int64_t time_val, gps_point_raw *gp, int i, int size, int max_gps_diff_ms, bool force_result)
{
    if (i < 0 || i > size)
        return 0;
    else if (time_val == gp[i].time)
        return 1;
    else if (i + 1 <= size && gp[i].time <= time_val && time_val < gp[i + 1].time) {
        if (force_result)
            return 1;
        else if (llabs(gp[i + 1].time - gp[i].time) <= max_gps_diff_ms)
            return 1;
    }
    return 0;
}

//checks if time value val is between gps_points[i] and gps_points[i+1] with size checks
//note: size must be inclusive (ie: gps_points[size] valid)
int time_val_between_indices_proc(
    int64_t time_val, gps_point_proc *gp, int i, int size, int max_gps_diff_ms, bool force_result)
{
    if (i < 0 || i > size)
        return 0;
    else if (time_val == gp[i].time)
        return 1;
    else if (i + 1 <= size && gp[i].time <= time_val && time_val < gp[i + 1].time) {
        if (force_result)
            return 1;
        else if (llabs(gp[i + 1].time - gp[i].time) <= max_gps_diff_ms)
            return 1;
    }
    return 0;
}

/** Returns the [index of] nearest gps point in time, but not farther than MAX_GPS_DIFF (10 seconds)
 * or -1 if search fails
 * Searches in raw values directly
 * If force_result is nonzero, it will ignore MAX_GPS_DIFF restriction
*/
int binary_search_gps(gps_private_data gdata, int64_t video_time, bool force_result /* = false */)
{
    gps_point_raw *gps_points = gdata.gps_points_r;
    const int gps_points_size = *gdata.gps_points_size - 1; //size-1 !!
    int last_index = *gdata.last_searched_index;
    int max_gps_diff_ms = get_max_gps_diff_ms(gdata);

    int il = 0;
    int ir = gps_points_size;
    int mid = 0;

    if (!gps_points || gps_points_size <= 0)
        return -1;

    //optimize repeated calls (exact match or in between points)
    if (time_val_between_indices_raw(video_time,
                                     gps_points,
                                     last_index,
                                     gps_points_size,
                                     max_gps_diff_ms,
                                     force_result)) {
        //		printf("_binary_search_gps, last_index(%d) v1 video_time: %I64d match: %I64d\n", last_index, video_time, gps_points[last_index].time);
        return last_index;
    }

    //optimize consecutive playback calls
    last_index++;
    if (time_val_between_indices_raw(video_time,
                                     gps_points,
                                     last_index,
                                     gps_points_size,
                                     max_gps_diff_ms,
                                     force_result)) {
        *gdata.last_searched_index = last_index;
        return last_index;
    }

    //optimize for the previous index
    last_index -= 2; //cancel the +1 above
    if (last_index >= 0
        && time_val_between_indices_raw(video_time,
                                        gps_points,
                                        last_index,
                                        gps_points_size,
                                        max_gps_diff_ms,
                                        force_result)) {
        *gdata.last_searched_index = last_index;
        return last_index;
    }

    //optimize for outside values, if force result it will return 0 or (size-1)
    if (video_time < *gdata.first_gps_time - max_gps_diff_ms)
        return (force_result ? 0 : -1);
    if (video_time > *gdata.last_gps_time + max_gps_diff_ms)
        return (force_result ? gps_points_size : -1);

    //binary search
    while (il < ir) {
        mid = (ir + il) / 2;
        //mlt_log_info(gdata.filter, "binary_search_gps: mid=%d, l=%d, r=%d, mid-time=%d (s)", mid, il, ir, gps_points[mid].time/1000);
        if (time_val_between_indices_raw(video_time,
                                         gps_points,
                                         mid,
                                         gps_points_size,
                                         max_gps_diff_ms,
                                         force_result)) {
            *gdata.last_searched_index = mid;
            break;
        } else if (gps_points[mid].time > video_time)
            ir = mid;
        else
            il = mid + 1;
    }

    //don't return the closest gps point if time difference is too large (unless force_result is 1)
    if (llabs(video_time - gps_points[mid].time) > max_gps_diff_ms)
        return (force_result ? mid : -1);
    else
        return mid;
}

/** Returns a nicer number of decimal values for floats
 *  [ 1.23m | 12.3m | 123m ]
 *  - argument use_decimals defaults to -1 (in .h)
*/
int decimals_needed(double x, int use_decimals)
{
    if (use_decimals != -1)
        return use_decimals;

    if (fabs(x) < 10)
        return 2;
    if (fabs(x) < 100)
        return 1;
    return 0;
}

/** Converts the distance (stored in meters) to the unit requested in extended keyword
*/
double convert_distance_to_format(double x, const char *format)
{
    if (format == NULL || x == GPS_UNINIT)
        return x;

    if (strstr(format, "km") || strstr(format, "kilometer"))
        return x / 1000.0;
    else if (strstr(format, "mi") || strstr(format, "mile"))
        return x * 0.00062137;
    else if (strstr(format, "nm") || strstr(format, "nautical"))
        return x * 0.0005399568;
    else if (strstr(format, "ft") || strstr(format, "feet"))
        return x * 3.2808399;
    return x;
}

/** Converts the speed (stored in meters/second) to the unit requested in extended keyword
*/
double convert_speed_to_format(double x, const char *format)
{
    if (x == GPS_UNINIT)
        return x;
    //order is important as short keywords will match anywhere (ms in kms and mi in mi[nutes])
    if (format == NULL || strstr(format, "kms") || strstr(format, "km/s")
        || strstr(format, "kilometer"))
        return x * 3.6; //default km/h
    if (strstr(format, "ms") || strstr(format, "m/s") || strstr(format, "meter"))
        return x;
    if (strstr(format, "mmin") || strstr(format, "m/min"))
        return x * 60;
    if (strstr(format, "ftmin") || strstr(format, "ft/min"))
        return x * 196.850393;
    if (strstr(format, "mi") || strstr(format, "mi/h") || strstr(format, "mile"))
        return x * 2.23693629;
    if (strstr(format, "kn") || strstr(format, "nm/h") || strstr(format, "knots"))
        return x * 1.94384449;
    if (strstr(format, "ft") || strstr(format, "ft/s") || strstr(format, "feet"))
        return x * 3.2808399;

    return x * 3.6;
}

/* Converts the bearing angle (0-360) to a cardinal direction
 * 8 sub-divisions
 */
const char *bearing_to_compass(double x)
{
    if (x == GPS_UNINIT)
        return "--";

    if (x <= 22.5 || x >= 360 - 22.5)
        return "N";
    else if (x < 45 + 22.5)
        return "NE";
    else if (x <= 90 + 22.5)
        return "E";
    else if (x < 90 + 45 + 22.5)
        return "SE";
    else if (x <= 180 + 22.5)
        return "S";
    else if (x < 180 + 45 + 22.5)
        return "SW";
    else if (x <= 270 + 22.5)
        return "W";
    else if (x < 270 + 45 + 22.5)
        return "NW";

    return "-";
}

// Updates gps-coords-based data in gps_points_p using already smoothed lat/lon
void recalculate_gps_data(gps_private_data gdata)
{
    int i;
    int req_smooth = gdata.last_smooth_lvl;

    if (req_smooth == 0)
        return;
    if (gdata.gps_points_r == NULL) {
        mlt_log_warning(gdata.filter, "recalculate_gps_data - gps_points_r is null!");
        return;
    }
    if (gdata.gps_points_p == NULL) {
        if ((*gdata.ptr_to_gps_points_p = (gps_point_proc *) calloc(*gdata.gps_points_size,
                                                                    sizeof(gps_point_proc)))
            == NULL) {
            mlt_log_warning(gdata.filter,
                            "calloc error, size=%u\n",
                            (unsigned) (*gdata.gps_points_size * sizeof(gps_point_proc)));
            return;
        } else { //alloc ok
            gdata.gps_points_p = *gdata.ptr_to_gps_points_p;
            process_gps_smoothing(gdata, 0);
        }
    }

    gps_point_proc *gps_points = gdata.gps_points_p;
    const int gps_points_size = *gdata.gps_points_size;

    //compute gps_start_time actual offset
    int offset_start = 0;
    if (gdata.gps_proc_start_t != 0)
        offset_start = binary_search_gps(gdata, gdata.gps_proc_start_t, true) + 1;

    //mlt_log_info(gdata.filter, "recalculate_gps_data, offset=%d, points=%p, new:%p, size:%d, newSize:%d", offset,	 gdata.gps_points, gps_points, *gdata.gps_points_size, gps_points_size);

    int ignore_points_before = 0;
    double total_dist = 0, total_d_elev = 0, total_elev_up = 0, total_elev_down = 0,
           total_dist_up = 0, total_dist_down = 0, total_dist_flat = 0;
    double start_dist = 0, start_d_elev = 0, start_elev_up = 0, start_elev_down = 0,
           start_dist_up = 0, start_dist_down = 0, start_dist_flat = 0;
    gps_point_proc *crt_point = NULL, *prev_point = NULL, *prev_nrsmooth_point = NULL;

    int grade_bucket[100], speed_avg_count = 0;
    double speed_avg = 0, last_grade = 0;
    memset(&grade_bucket, 0, 100 * sizeof(int));

    for (i = 0; i < gps_points_size; i++) {
        //store values at processing_start_time to substract them at the end
        if (i - 1 == offset_start) {
            start_dist = total_dist;
            start_d_elev = total_d_elev;
            start_elev_up = total_elev_up;
            start_elev_down = total_elev_down;
            start_dist_up = total_dist_up;
            start_dist_down = total_dist_down;
            start_dist_flat = total_dist_flat;
        }

        crt_point = &gps_points[i];

        //this is the farthest valid point (behind current) that can be used for smoothing, limited by start of array or big gap in time
        int smooth_index = MAX(ignore_points_before, MAX(0, i - req_smooth));

        //ignore points with missing lat/lon, some devices use 0,0 for no fix so we ignore those too
        if (!has_valid_location_ptr(crt_point) || (!crt_point->lat && !crt_point->lon)) {
            //set the last valid values to these points so output won't be "--"
            crt_point->total_dist = total_dist;
            crt_point->d_elev = 0;
            crt_point->elev_up = total_elev_up;
            crt_point->elev_down = total_elev_down;
            crt_point->dist_up = total_dist_up;
            crt_point->dist_down = total_dist_down;
            crt_point->dist_flat = total_dist_flat;
            continue;
        }

        //previous valid point must exist for math to happen
        if (prev_point == NULL) {
            prev_point = crt_point;
            //first local valid point, just set its distance to 0, other stuff can't be calculated
            crt_point->total_dist = total_dist;
            continue;
        }

        //find a valid point from i-req_smooth to i (to use in smoothing calc)
        while (smooth_index < i && !has_valid_location(gps_points[smooth_index]))
            smooth_index++;
        if (smooth_index < i)
            prev_nrsmooth_point = &gps_points[smooth_index];
        else
            prev_nrsmooth_point = NULL;

        //1) get distance difference between last 2 valid points
        double d_dist = distance_equirectangular_2p(prev_point->lat,
                                                    prev_point->lon,
                                                    crt_point->lat,
                                                    crt_point->lon);
        double d_elev = 0, d_dist_smoothed = 0, d_elev_smoothed = 0;
        //the int64 diff will be small enough for double
        double d_time_sec = (crt_point->time - prev_point->time) / 1000.0;
        double d_time_smoothed_sec = d_time_sec;

        //if time difference is way longer for one point than usual, don't do math on that gap (treat recording paused, move far, then resume)
        if (d_time_sec
            > 10.0 * (*gdata.last_gps_time - *gdata.first_gps_time) / *gdata.gps_points_size) {
            prev_nrsmooth_point = NULL;
            ignore_points_before = i;
            crt_point->total_dist = total_dist;
            prev_point = crt_point;
            continue;
        }

        //this is the total distance since beginning of gps processing time
        total_dist += d_dist;
        crt_point->total_dist = total_dist;

        //distance and height difference if smoothed point can be used
        if (prev_nrsmooth_point) {
            d_time_smoothed_sec = (crt_point->time - prev_nrsmooth_point->time) / 1000.0;
            d_dist_smoothed = crt_point->total_dist - prev_nrsmooth_point->total_dist;
            if (crt_point->ele != GPS_UNINIT && prev_nrsmooth_point->ele != GPS_UNINIT)
                d_elev_smoothed = crt_point->ele - prev_nrsmooth_point->ele;
        }

        //2)+3) speed and bearing
        if (req_smooth < 2) {
            crt_point->speed = d_dist / d_time_sec; //in m/s
            crt_point->bearing
                = bearing_2p(prev_point->lat, prev_point->lon, crt_point->lat, crt_point->lon);
        } else if (prev_nrsmooth_point) {
            //for "smoothing" we calculate distance between 2 points "nr_smoothing" distance behind
            crt_point->speed = d_dist_smoothed / d_time_smoothed_sec;

            crt_point->bearing = bearing_2p(prev_nrsmooth_point->lat,
                                            prev_nrsmooth_point->lon,
                                            crt_point->lat,
                                            crt_point->lon);
        }

        //continuously compute speed average
        if (crt_point->speed > 0.27) {
            speed_avg = (speed_avg * speed_avg_count + crt_point->speed) / (speed_avg_count + 1);
            ++speed_avg_count;
        }

        //4) altitude stuff
        if (crt_point->ele != GPS_UNINIT && prev_point->ele != GPS_UNINIT) {
            d_elev = crt_point->ele - prev_point->ele;
            total_d_elev += d_elev;
            if (crt_point->ele > prev_point->ele) {
                total_elev_up += d_elev;
                total_dist_up += d_dist;
            } else if (crt_point->ele < prev_point->ele) {
                total_elev_down += d_elev;
                total_dist_down += d_dist;
            } else {
                total_dist_flat += d_dist;
            }
            crt_point->d_elev = total_d_elev;
            crt_point->elev_up = total_elev_up;
            crt_point->elev_down = total_elev_down;
            crt_point->dist_up = total_dist_up;
            crt_point->dist_down = total_dist_down;
            crt_point->dist_flat = total_dist_flat;
        }

        //5) grade, vertical and 3d speed (if altitude present)
        if (crt_point->ele != GPS_UNINIT) {
            bool ok = 1;
            double used_d_elev = d_elev, used_d_dist = d_dist, used_d_time_sec = d_time_sec;
            if (prev_nrsmooth_point && prev_nrsmooth_point->ele != GPS_UNINIT) {
                used_d_elev = d_elev_smoothed;
                used_d_dist = d_dist_smoothed;
                used_d_time_sec = d_time_smoothed_sec;
            } else if (prev_point->ele == GPS_UNINIT) {
                //this looks cleaner; no previous elevation available
                ok = 0;
            }

            if (ok) {
                //cut-off grade computation (->just duplicate it) under 2km/h and half the avg speed (but don't cut-off over 15km/h)
                if (used_d_dist > 0.28 * (req_smooth + 1) //this covers division by 0
                    && used_d_dist > MIN(speed_avg / 2, 4) * req_smooth) {
                    crt_point->grade_p = 100.0 * used_d_elev / used_d_dist;
                    int indx = CLAMP(abs(crt_point->grade_p) / 10, 0, 99);
                    grade_bucket[indx]++;
                    last_grade = crt_point->grade_p;
                } else {
                    //better option: set this to GPS_UNINIT and run an interpolation at the end to re-fill
                    crt_point->grade_p = last_grade / 2;
                }
                //vertical + 3d speed
                crt_point->speed_vertical = used_d_elev / used_d_time_sec;
                crt_point->speed_3d = sqrt(used_d_dist * used_d_dist + used_d_elev * used_d_elev)
                                      / used_d_time_sec;
            }
        }
        prev_point = crt_point;
    }

    //for gps grade only: trim down the highest 1st percentile of values
    if (req_smooth > 1) {
        int nr_1p = gps_points_size / 100;
        int max_grade_1p = -1;
        for (i = 99; i > 1; --i) {
            if (grade_bucket[i] > nr_1p) {
                max_grade_1p = (i + 1) * 10;
                break;
            }
        }
        if (max_grade_1p > 1) {
            for (i = 0; i < gps_points_size; i++) {
                if (gps_points[i].grade_p != GPS_UNINIT
                    && abs(gps_points[i].grade_p) > max_grade_1p) {
                    //if speed is over 15km/h assume grade is correct
                    if (gps_points[i].speed < MAX(speed_avg / 2, 4)) {
                        // mlt_log_info(NULL,
                        //              "clamping [%d].grade_p from %f to %d; speed = %f",
                        //              i,
                        //              gps_points[i].grade_p,
                        //              max_grade_1p,
                        //              gps_points[i].speed);
                        gps_points[i].grade_p = (gps_points[i].grade_p >= 0 ? max_grade_1p
                                                                            : -max_grade_1p);
                    }
                }
            }
        }
    }

    //clean up computed values for relative stuff (distances mostly) before gps_processing_start time
    if (gdata.gps_proc_start_t != 0 && offset_start > 0 && offset_start < gps_points_size) {
        //mlt_log_info(gdata.filter, "recalculate_gps_data: clearing gps data from 0 to %d due to set GPS start time:%d s", offset_start, gdata.gps_proc_start_t/1000);
        for (i = 0; i < offset_start; i++) {
            gps_point_proc *crt_point = &(gdata.gps_points_p[i]);
            if (crt_point->total_dist != 0)
                start_dist = crt_point->total_dist;
            crt_point->total_dist = 0;
            crt_point->d_elev = 0;
            crt_point->elev_up = 0;
            crt_point->elev_down = 0;
            crt_point->dist_up = 0;
            crt_point->dist_down = 0;
            crt_point->dist_flat = 0;
        }
        //remove the distances from before
        //mlt_log_info(gdata.filter, "recalculate_gps_data: substracting values at start time! (start_dist=%f @ start index=%d)", start_dist, offset_start);
        for (i = offset_start; i < gps_points_size; i++) {
            gps_point_proc *crt_point = &(gdata.gps_points_p[i]);
            crt_point->total_dist -= start_dist;
            crt_point->d_elev -= start_d_elev;
            crt_point->elev_up -= start_elev_up;
            crt_point->elev_down -= start_elev_down;
            crt_point->dist_up -= start_dist_up;
            crt_point->dist_down -= start_dist_down;
            crt_point->dist_flat -= start_dist_flat;
        }
    }
}

/* Returns a weighted (type:double) value for an intermediary time
 * Notes: time limit 10s, if one value is GPS_UNINIT, returns the other
*/
double weighted_middle_double(
    double v1, int64_t t1, double v2, int64_t t2, int64_t new_t, int max_gps_diff_ms)
{
    int64_t d_time = t2 - t1;
    if (v1 == GPS_UNINIT)
        return v2;
    if (v2 == GPS_UNINIT)
        return v1;
    if (d_time > max_gps_diff_ms || d_time == 0)
        return v1;

    double prev_weight = 1 - (double) (new_t - t1) / d_time;
    double next_weight = 1 - (double) (t2 - new_t) / d_time;
    double rez = v1 * prev_weight + v2 * next_weight;
    //mlt_log_info(NULL, "weighted_middle_double in: v:%f-%f, %d-%d %d out: %f ", v1, v2, t1/1000, t2/1000, new_t/1000, rez);
    return rez;
}

/* Returns a weighted (type:int64_t) value for an intermediary time
 * Notes: time limit 10s, if one value is GPS_UNINIT, returns the other
*/
int64_t weighted_middle_int64(
    int64_t v1, int64_t t1, int64_t v2, int64_t t2, int64_t new_t, int max_gps_diff_ms)
{
    int64_t d_time = t2 - t1;
    if (v1 == GPS_UNINIT)
        return v2;
    if (v2 == GPS_UNINIT)
        return v1;
    if (d_time > max_gps_diff_ms || d_time == 0)
        return v1;

    double prev_weight = 1 - (double) (new_t - t1) / d_time;
    double next_weight = 1 - (double) (t2 - new_t) / d_time;
    int64_t rez = v1 * prev_weight + v2 * next_weight;
    //mlt_log_info(NULL, "weighted_middle_int64 in: v:%d-%d, %d-%d %d out: %d (weights: prev=%f, next=%f)", v1%1000000, v2%1000000, t1/1000, t2/1000, new_t/1000, rez%1000000, prev_weight, next_weight);
    return rez;
}

//compute a virtual point at a specific time between 2 real points
gps_point_proc weighted_middle_point_proc(gps_point_proc *p1,
                                          gps_point_proc *p2,
                                          int64_t new_t,
                                          int max_gps_diff_ms)
{
    if (p1 == p2)
        return *p1;
    if (llabs(p2->time - p1->time) > max_gps_diff_ms)
        return *p1;
    if (new_t < MIN(p1->time, p2->time))
        return *p1;
    if (new_t > MAX(p1->time, p2->time))
        return *p2;

    gps_point_proc crt_point = uninit_gps_proc_point;
    crt_point.lat
        = weighted_middle_double(p1->lat, p1->time, p2->lat, p2->time, new_t, max_gps_diff_ms);
    crt_point.lat_projected = project_latitude(crt_point.lat);
    crt_point.lon
        = weighted_middle_double(p1->lon, p1->time, p2->lon, p2->time, new_t, max_gps_diff_ms);
    crt_point.speed
        = weighted_middle_double(p1->speed, p1->time, p2->speed, p2->time, new_t, max_gps_diff_ms);
    crt_point.speed_vertical = weighted_middle_double(p1->speed_vertical,
                                                      p1->time,
                                                      p2->speed_vertical,
                                                      p2->time,
                                                      new_t,
                                                      max_gps_diff_ms);
    crt_point.speed_3d = weighted_middle_double(p1->speed_3d,
                                                p1->time,
                                                p2->speed_3d,
                                                p2->time,
                                                new_t,
                                                max_gps_diff_ms);
    crt_point.total_dist = weighted_middle_double(p1->total_dist,
                                                  p1->time,
                                                  p2->total_dist,
                                                  p2->time,
                                                  new_t,
                                                  max_gps_diff_ms);
    crt_point.ele
        = weighted_middle_double(p1->ele, p1->time, p2->ele, p2->time, new_t, max_gps_diff_ms);
    crt_point.time
        = weighted_middle_int64(p1->time, p1->time, p2->time, p2->time, new_t, max_gps_diff_ms);
    crt_point.d_elev
        = weighted_middle_double(p1->d_elev, p1->time, p2->d_elev, p2->time, new_t, max_gps_diff_ms);
    crt_point.elev_up = weighted_middle_double(p1->elev_up,
                                               p1->time,
                                               p2->elev_up,
                                               p2->time,
                                               new_t,
                                               max_gps_diff_ms);
    crt_point.elev_down = weighted_middle_double(p1->elev_down,
                                                 p1->time,
                                                 p2->elev_down,
                                                 p2->time,
                                                 new_t,
                                                 max_gps_diff_ms);
    crt_point.dist_up = weighted_middle_double(p1->dist_up,
                                               p1->time,
                                               p2->dist_up,
                                               p2->time,
                                               new_t,
                                               max_gps_diff_ms);
    crt_point.dist_down = weighted_middle_double(p1->dist_down,
                                                 p1->time,
                                                 p2->dist_down,
                                                 p2->time,
                                                 new_t,
                                                 max_gps_diff_ms);
    crt_point.dist_flat = weighted_middle_double(p1->dist_flat,
                                                 p1->time,
                                                 p2->dist_flat,
                                                 p2->time,
                                                 new_t,
                                                 max_gps_diff_ms);
    crt_point.bearing = weighted_middle_double(p1->bearing,
                                               p1->time,
                                               p2->bearing,
                                               p2->time,
                                               new_t,
                                               max_gps_diff_ms);
    crt_point.hr
        = weighted_middle_double(p1->hr, p1->time, p2->hr, p2->time, new_t, max_gps_diff_ms);
    crt_point.cad
        = weighted_middle_double(p1->cad, p1->time, p2->cad, p2->time, new_t, max_gps_diff_ms);
    crt_point.grade_p = weighted_middle_double(p1->grade_p,
                                               p1->time,
                                               p2->grade_p,
                                               p2->time,
                                               new_t,
                                               max_gps_diff_ms);
    crt_point.atemp
        = weighted_middle_double(p1->atemp, p1->time, p2->atemp, p2->time, new_t, max_gps_diff_ms);
    crt_point.power
        = weighted_middle_double(p1->power, p1->time, p2->power, p2->time, new_t, max_gps_diff_ms);
    return crt_point;
}

//checks whether 2 points (not necessarily consecutive) are not after a long pause in tracking
int in_gps_time_window(gps_private_data gdata, int crt, int next)
{
    gps_point_raw *gp = gdata.gps_points_r;
    int64_t d_time = llabs(gp[next].time - gp[crt].time);
    int d_indices = abs(next - crt);
    return d_time <= (get_avg_gps_time_ms(gdata) * d_indices + get_max_gps_diff_ms(gdata));
}

/* Processes the entire gps_points_p array to fill the lat, lon values
 * Also does linear interpolation of HR, altitude (+lat/lon*) if necessary to fill missing values
 * After this, if do_processing is 1, calls recalculate_gps_data to update distance, speed + other fields for all points
 * Returns without doing anything if smoothing level is 0
 */
void process_gps_smoothing(gps_private_data gdata, char do_processing)
{
    int req_smooth = gdata.last_smooth_lvl;
    if (gdata.last_smooth_lvl == 0)
        return;

    if (gdata.gps_points_r == NULL) {
        mlt_log_warning(gdata.filter, "process_gps_smoothing - gps_points_r is null!\n");
        return;
    }
    if (gdata.gps_points_p == NULL) {
        if ((*gdata.ptr_to_gps_points_p = (gps_point_proc *) calloc(*gdata.gps_points_size,
                                                                    sizeof(gps_point_proc)))
            == NULL) {
            mlt_log_warning(gdata.filter,
                            "calloc failed, size = %u\n",
                            (unsigned) (*gdata.gps_points_size * sizeof(gps_point_proc)));
            return;
        } else
            gdata.gps_points_p = *gdata.ptr_to_gps_points_p;
    }

    int max_gps_diff_ms = get_max_gps_diff_ms(gdata);
    int i, j, nr_hr = 0, nr_ele = 0, nr_cad = 0, nr_atemp = 0, nr_power = 0;
    double hr = GPS_UNINIT, ele = GPS_UNINIT, cad = GPS_UNINIT, atemp = GPS_UNINIT,
           power = GPS_UNINIT;

    //linear interpolation for heart rate, elevation, cadence and temperature, one time per file, ignores start offset
    if (*gdata.interpolated == 0) {
        //figure out how many seconds are between 2 average fixes so we can set a limit (in time) to interpolation
        double avg_time = (*gdata.last_gps_time - *gdata.first_gps_time) / 1000
                          / (double) *gdata.gps_points_size;
        double nr_one_minute = 60.0 / (avg_time ? avg_time : 1);

        gps_point_raw *gp_r = gdata.gps_points_r;
        gps_point_proc *gp_p = gdata.gps_points_p;
        for (i = 0; i < *gdata.gps_points_size; i++) {
            //calloc made everything 0, fill back with GPS_UNINIT if needed
            gp_p[i].hr = gp_r[i].hr;
            gp_p[i].ele = gp_r[i].ele;
            gp_p[i].cad = gp_r[i].cad;
            gp_p[i].atemp = gp_r[i].atemp;
            gp_p[i].power = gp_r[i].power;

            //heart rate
            if (gp_r[i].hr != GPS_UNINIT) { //found valid hr
                if (hr != GPS_UNINIT && nr_hr > 0
                    && nr_hr <= nr_one_minute) { //there were missing values and had a hr before
                    nr_hr++;
                    for (j = i; j > i - nr_hr; j--) { //go backwards and fill values
                        gp_p[j].hr = hr + (gp_r[i].hr - hr) * (1.0 * (j - (i - nr_hr)) / nr_hr);
                        //printf("_i=%d, j=%d, nr_hr=%d hr = %d; gp_r[i].hr=%f, gp_p[j].hr=%f  \n", i,j,nr_hr, hr, gp_r[i].hr, gp_p[j].hr);
                    }
                }
                hr = gp_r[i].hr;
                nr_hr = 0;
            } else
                nr_hr++;

            //altitude
            if (gp_r[i].ele != GPS_UNINIT) {
                if (ele != GPS_UNINIT && nr_ele > 0 && nr_ele <= nr_one_minute * 10) {
                    nr_ele++;
                    for (j = i; j > i - nr_ele; j--) {
                        gp_p[j].ele = ele
                                      + 1.0 * (gp_r[i].ele - ele)
                                            * (1.0 * (j - (i - nr_ele)) / nr_ele);
                        //printf("_i=%d, j=%d, nr_ele=%d ele = %f; gp_r[i].ele=%f, gp_p[j].ele=%f  \n", i,j,nr_ele, ele, gp_r[i].ele, gp_p[j].ele);
                    }
                }
                ele = gp_r[i].ele;
                nr_ele = 0;
            } else
                nr_ele++;

            //cadence
            if (gp_r[i].cad != GPS_UNINIT) {
                if (cad != GPS_UNINIT && nr_cad > 0 && nr_cad <= nr_one_minute) {
                    nr_cad++;
                    for (j = i; j > i - nr_cad; j--) {
                        gp_p[j].cad = cad
                                      + 1.0 * (gp_r[i].cad - cad)
                                            * (1.0 * (j - (i - nr_cad)) / nr_cad);
                    }
                }
                cad = gp_r[i].cad;
                nr_cad = 0;
            } else
                nr_cad++;

            //temperature
            if (gp_r[i].atemp != GPS_UNINIT) {
                if (atemp != GPS_UNINIT && nr_atemp > 0 && nr_atemp <= nr_one_minute * 60) {
                    nr_atemp++;
                    for (j = i; j > i - nr_atemp; j--) {
                        gp_p[j].atemp = atemp
                                        + 1.0 * (gp_r[i].atemp - atemp)
                                              * (1.0 * (j - (i - nr_atemp)) / nr_atemp);
                    }
                }
                atemp = gp_r[i].atemp;
                nr_atemp = 0;
            } else {
                nr_atemp++;
            }

            //power
            if (gp_r[i].power != GPS_UNINIT) {
                if (power != GPS_UNINIT && nr_power > 0 && nr_power <= nr_one_minute) {
                    nr_power++;
                    for (j = i; j > i - nr_power; j--) {
                        gp_p[j].power = power
                                        + 1.0 * (gp_r[i].power - power)
                                              * (1.0 * (j - (i - nr_power)) / nr_power);
                    }
                }
                power = gp_r[i].power;
                nr_power = 0;
            } else {
                nr_power++;
            }

            //these are not interpolated but as long as we're iterating we can copy them now
            gp_p[i].time = gp_r[i].time;
            gp_p[i].lat = gp_r[i].lat;
            gp_p[i].lat_projected = gp_r[i].lat_projected;
            gp_p[i].lon = gp_r[i].lon;
        }
    }

    gps_point_raw *gps_points_r = gdata.gps_points_r;
    gps_point_proc *gps_points_p = gdata.gps_points_p;
    const int gps_points_size = *gdata.gps_points_size;

    for (i = 0; i < gps_points_size; i++) {
        if (req_smooth == 1) {
            //copy raw lat/lon to calc lat/lon and interpolate 1 location if necessary
            gps_points_p[i].lat = gps_points_r[i].lat;
            gps_points_p[i].lat_projected = gps_points_r[i].lat_projected;
            gps_points_p[i].lon = gps_points_r[i].lon;

            //this can happen often if location and altitude are stored at different time intervals (every 3s vs every 10s)
            if (i - 1 >= 0 && i + 1 < gps_points_size //if we're not at start/end
                && !has_valid_location(gps_points_r[i]) && has_valid_location(gps_points_r[i - 1])
                && has_valid_location(
                    gps_points_r[i + 1]) //if current point has no lat/lon but nearby ones do
                && llabs(gps_points_r[i + 1].time - gps_points_r[i - 1].time)
                       < max_gps_diff_ms) //if time difference is lower than max_gps_diff_ms
            {
                //place a weighted "middle" point here depending on time difference
                gps_points_p[i].lat = weighted_middle_double(gps_points_r[i - 1].lat,
                                                             gps_points_r[i - 1].time,
                                                             gps_points_r[i + 1].lat,
                                                             gps_points_r[i + 1].time,
                                                             gps_points_r[i].time,
                                                             max_gps_diff_ms);
                gps_points_p[i].lat_projected = project_latitude(gps_points_p[i].lat);
                gps_points_p[i].lon = weighted_middle_double(gps_points_r[i - 1].lon,
                                                             gps_points_r[i - 1].time,
                                                             gps_points_r[i + 1].lon,
                                                             gps_points_r[i + 1].time,
                                                             gps_points_r[i].time,
                                                             max_gps_diff_ms);

                //mlt_log_info(gdata.filter, "interpolating position for smooth=1, new point[%d]:%f,%f @time=%d s", i, gps_points_p[i].lat, gps_points_p[i].lon, gps_points_r[i].time/1000);
            }
        } else if (req_smooth > 1) {
            //for each point average "req_smooth/2" values before and after
            double lat_sum = 0, lon_sum = 0;
            int nr_div = 0;

            //passing 180 meridian is ok, but passing both 180 and 0 (so more than one full circle around the earth) will smooth out badly; no fix planned for this
            for (j = MAX(0, i - req_smooth / 2); j < MIN(i + req_smooth / 2, gps_points_size); j++) {
                if (has_valid_location(gps_points_r[j]) && in_gps_time_window(gdata, i, j)) {
                    lat_sum += gps_points_r[j].lat;
                    lon_sum += gps_points_r[j].lon;
                    nr_div++;
                }
            }
            if (nr_div != 0) {
                gps_points_p[i].lat = lat_sum / nr_div;
                gps_points_p[i].lat_projected = project_latitude(gps_points_p[i].lat);
                gps_points_p[i].lon = lon_sum / nr_div;
            } else {
                gps_points_p[i].lat = gps_points_r[i].lat;
                gps_points_p[i].lat_projected = gps_points_r[i].lat;
                gps_points_p[i].lon = gps_points_r[i].lon;
            }
            //mlt_log_info(filter, "i=%d, lat_sum=%f, lon_sum=%f, nr_div=%d, time=%d", i, lat_sum, lon_sum, nr_div, gps_points[i].time);
        }
    }
    *gdata.interpolated = 1;
    if (req_smooth != 0 && do_processing == 1)
        recalculate_gps_data(gdata);
}

//File parsing with QT's XML stream reader

// Parses a .gpx file into a gps_point_raw linked list
void qxml_parse_gpx(QXmlStreamReader &reader, gps_point_ll **gps_list, int *count_pts)
{
    int64_t last_time = -1;
    //support no time in file because many services (Strava included) do this for exported routes
    gps_point_ll *no_time_head = NULL, *no_time_prev = NULL;
    int no_time_count = 0;
    /*
    // sample point from .GPX file (version 1.0) + TrackPointExtension
    <trkpt lat="45.227621666484104" lon="25.16533185322889">
        <ele>868.3005248873908</ele>
        <time>2020-07-11T09:03:23.000Z</time>
        <speed>0.0</speed>
        <course>337.1557</course>
        <extensions>
            <power>0</power>                      //strava ...
            <gpxtpx:TrackPointExtension>          //official garmin extension
                <gpxtpx:hr>132</gpxtpx:hr>            //heart rate in beats per minute
                <gpxtpx:cad>75</gpxtpx:cad>           //cadence in revolutions per minute
                <gpxtpx:atemp>27.5</gpxtpx:atemp>     //ambient temperature in degrees celsius
            </gpxtpx:TrackPointExtension>
        </extensions>
    </trkpt>
    */
    while (!reader.atEnd() && !reader.hasError()) {
        reader.readNext();
        // mlt_log_info(NULL, " readNext tag: %s type = %d\n", _qxml_getthestring(reader.name()), reader.tokenType());
        if (reader.isStartElement() && reader.name() == QStringLiteral("trkpt")) {
            gps_point_raw crt_point = uninit_gps_raw_point;

            QXmlStreamAttributes attributes = reader.attributes();
            if (attributes.hasAttribute("lat"))
            {
                crt_point.lat = attributes.value("lat").toDouble();
                crt_point.lat_projected = project_latitude( crt_point.lat);
            }
            if (attributes.hasAttribute("lon"))
                crt_point.lon = attributes.value("lon").toDouble();

            while (reader.readNext()
                   && !(reader.name() == QStringLiteral("trkpt")
                        && reader.tokenType() == QXmlStreamReader::EndElement)) //until closing trkpt
            {
                // mlt_log_info(NULL, "[trkpt->1] readNext tag: %s, type=%d \n", _qxml_getthestring(reader.name()), reader.tokenType());
                if (!reader.isStartElement())
                    continue;

                if (reader.name() == QStringLiteral("ele"))
                    crt_point.ele = reader.readElementText().toDouble();
                else if (reader.name() == QStringLiteral("time"))
                    crt_point.time = datetimeXMLstring_to_mseconds(
                        qUtf8Printable(reader.readElementText()));
                else if (reader.name() == QStringLiteral("speed"))
                    crt_point.speed = reader.readElementText().toDouble();
                else if (reader.name() == QStringLiteral("course"))
                    crt_point.bearing = reader.readElementText().toDouble();
                else if (reader.name() == QStringLiteral("extensions")) {
                    while (reader.readNext()
                           && !(reader.name() == QStringLiteral("extensions")
                                && reader.tokenType() == QXmlStreamReader::EndElement)) {
                        if (reader.name() == QStringLiteral("power"))
                            crt_point.power = reader.readElementText().toDouble();
                        else if (reader.name() == QStringLiteral("TrackPointExtension")) {
                            while (reader.readNext()
                                   && !(reader.name() == QStringLiteral("TrackPointExtension")
                                        && reader.tokenType() == QXmlStreamReader::EndElement)) {
                                if (reader.name() == QStringLiteral("hr"))
                                    crt_point.hr = reader.readElementText().toDouble();
                                else if (reader.name() == QStringLiteral("cad"))
                                    crt_point.cad = reader.readElementText().toDouble();
                                else if (reader.name() == QStringLiteral("atemp"))
                                    crt_point.atemp = reader.readElementText().toDouble();
                            }
                        }
                    }
                }
            }
            //now add the point to linked list (but only if increasing time)
            if (crt_point.time != GPS_UNINIT && crt_point.time > last_time) {
                *gps_list = (gps_point_ll *) calloc(1, sizeof(gps_point_ll));
                if (*gps_list == NULL)
                    return;

                *count_pts += 1;
                (*gps_list)->gp = crt_point;
                (*gps_list)->next = NULL;
                gps_list = &(*gps_list)->next;
                last_time = crt_point.time;
            } else if (crt_point.time == GPS_UNINIT) {
                //if no time is stored, we make it up starting from 1
                gps_point_ll *tmp = (gps_point_ll *) calloc(1, sizeof(gps_point_ll));
                if (tmp == NULL)
                    return;
                if (no_time_head == NULL) {
                    no_time_head = tmp;
                    no_time_prev = tmp;
                } else {
                    no_time_prev->next = tmp;
                    no_time_prev = tmp;
                }
                no_time_count += 1;
                crt_point.time = no_time_count * 1000;
                tmp->gp = crt_point;
                tmp->next = NULL;
            } else {
                mlt_log_info(NULL,
                             "qxml_parse_gpx: skipping point due to time [%d] %f,%f - crt:%u.%u, "
                             "last:%u.%u\n",
                             *count_pts,
                             crt_point.lat,
                             crt_point.lon,
                             (unsigned) (crt_point.time / 1000),
                             (unsigned) (crt_point.time % 1000),
                             (unsigned) (last_time / 1000),
                             (unsigned) (last_time % 1000));
            }
        }
    }
    //if no points had time, we replace the real list with the no_time one
    if (*count_pts == 0 && no_time_count > 0) {
        mlt_log_info(NULL,
                     "qxml_parse_gpx: all GPS points are missing time values, inserting each "
                     "point at 1 second interval!");
        *gps_list = no_time_head;
        *count_pts = no_time_count;
    } else if (*count_pts > 0 && no_time_count > 0) {
        mlt_log_info(NULL,
                     "qxml_parse_gpx: %d GPS points have no time in original file and have been "
                     "discarded (total valid points kept: %d)!",
                     no_time_count,
                     *count_pts);
        //cleanup the no_time list as we're not using it
        while (no_time_head) {
            gps_point_ll *tmp = no_time_head;
            no_time_head = no_time_head->next;
            free(tmp);
        }
    }
}

// Parses a .tcx file into a gps_point_raw linked list
void qxml_parse_tcx(QXmlStreamReader &reader, gps_point_ll **gps_list, int *count_pts)
{
    int64_t last_time = -1;
    //support no time in file because many services (Strava included) do this for exported routes
    gps_point_ll *no_time_head = NULL, *no_time_prev = NULL;
    int no_time_count = 0;
    /*
    //sample point from .TCX file
    <Trackpoint>
        <Time>2021-02-27T14:03:02+00:00</Time>
        <Position>
            <LatitudeDegrees>44.48205950925148</LatitudeDegrees>
            <LongitudeDegrees>26.1843781367422</LongitudeDegrees>
        </Position>
        <AltitudeMeters>65.0</AltitudeMeters>
        <DistanceMeters>25919.213486999884</DistanceMeters>
        <HeartRateBpm xsi:type="HeartRateInBeatsPerMinute_t">
            <Value>124</Value>
        </HeartRateBpm>
        <Cadence>78</Cadence>                   //unofficial, but some popular converters use it
        <Temperature>25</Temperature>           //unofficial/not widely used
        <Extensions>                            //garmin's extension for power
            <ns3:TPX>
                <ns3:Speed>0.3449999988079071</ns3:Speed>
                <ns3:Watts>0</ns3:Watts>
            </ns3:TPX>
        </Extensions>
    </Trackpoint>
    */
    while (!reader.atEnd() && !reader.hasError()) {
        reader.readNext();
        // mlt_log_info(NULL, " readNextStartElement tag: %s \n", _qxml_getthestring(reader.name()));
        if (reader.isStartElement() && reader.name() == QStringLiteral("Trackpoint")) {
            gps_point_raw crt_point = uninit_gps_raw_point;
            while (
                reader.readNext()
                && !(reader.name() == QStringLiteral("Trackpoint")
                     && reader.tokenType()
                            == QXmlStreamReader::EndElement)) //loop until we hit the closing </Trackpoint>
            {
                // mlt_log_info(NULL, "[Trackpoint->1] readNext tag: %s, type=%d \n", _qxml_getthestring(reader.name()), reader.tokenType());
                if (!reader.isStartElement())
                    continue;

                if (reader.name() == QStringLiteral("Time"))
                    crt_point.time = datetimeXMLstring_to_mseconds(
                        qUtf8Printable(reader.readElementText()));
                else if (reader.name() == QStringLiteral("Position")) {
                    reader.readNextStartElement();
                    if (reader.name() == QStringLiteral("LatitudeDegrees")) {
                        crt_point.lat = reader.readElementText().toDouble();
                        crt_point.lat_projected = project_latitude(crt_point.lat);
                    }
                    reader.readNextStartElement();
                    if (reader.name() == QStringLiteral("LongitudeDegrees"))
                        crt_point.lon = reader.readElementText().toDouble();
                } else if (reader.name() == QStringLiteral("AltitudeMeters")) {
                    crt_point.ele = reader.readElementText().toDouble();
                } else if (reader.name() == QStringLiteral("DistanceMeters")) {
                    crt_point.total_dist = reader.readElementText().toDouble();
                } else if (reader.name() == QStringLiteral("HeartRateBpm")) {
                    reader.readNextStartElement();
                    if (reader.name() == QStringLiteral("Value"))
                        crt_point.hr = reader.readElementText().toDouble();
                } else if (reader.name() == QStringLiteral("Cadence")) {
                    crt_point.cad = reader.readElementText().toDouble();
                } else if (reader.name() == QStringLiteral("Temperature")) {
                    crt_point.atemp = reader.readElementText().toDouble();
                } else if (reader.isStartElement()
                           && reader.name() == QStringLiteral("Extensions")) {
                    while (reader.readNext()
                           && !(reader.name() == QStringLiteral("TPX")
                                && reader.tokenType() == QXmlStreamReader::EndElement)) {
                        if (reader.name() == QStringLiteral("Speed"))
                            crt_point.speed = reader.readElementText().toDouble();
                        else if (reader.name() == QStringLiteral("Watts"))
                            crt_point.power = reader.readElementText().toDouble();
                    }
                }
            }
            //now add the point to linked list (but only if increasing time)
            if (crt_point.time != GPS_UNINIT && crt_point.time > last_time) {
                *gps_list = (gps_point_ll *) calloc(1, sizeof(gps_point_ll));
                if (*gps_list == NULL)
                    return;

                *count_pts += 1;
                (*gps_list)->gp = crt_point;
                (*gps_list)->next = NULL;
                gps_list = &(*gps_list)->next;
                last_time = crt_point.time;
            } else if (crt_point.time == GPS_UNINIT) {
                //if no time is stored, we make it up starting from 1
                gps_point_ll *tmp = (gps_point_ll *) calloc(1, sizeof(gps_point_ll));
                if (tmp == NULL)
                    return;
                if (no_time_head == NULL) {
                    no_time_head = tmp;
                    no_time_prev = tmp;
                } else {
                    no_time_prev->next = tmp;
                    no_time_prev = tmp;
                }
                no_time_count += 1;
                crt_point.time = no_time_count * 1000;
                tmp->gp = crt_point;
                tmp->next = NULL;
            } else
                mlt_log_info(NULL,
                             "qxml_parse_tcx: skipping point due to time [%d] %f,%f - crt:%u.%u, "
                             "last:%u.%u\n",
                             *count_pts,
                             crt_point.lat,
                             crt_point.lon,
                             (unsigned) (crt_point.time / 1000),
                             (unsigned) (crt_point.time % 1000),
                             (unsigned) (last_time / 1000),
                             (unsigned) (last_time % 1000));
        }
    }
    //if no points had time, we replace the real list with the no_time one
    if (*count_pts == 0 && no_time_count > 0) {
        mlt_log_info(NULL,
                     "qxml_parse_gpx: all GPS points are missing time values, inserting each "
                     "point at 1 second interval!");
        *gps_list = no_time_head;
        *count_pts = no_time_count;
    } else if (*count_pts > 0 && no_time_count > 0) {
        //cleanup the no_time list as we're not using it
        mlt_log_info(NULL,
                     "qxml_parse_gpx: %d GPS points have no time in original file and have been "
                     "discarded (total valid points kept: %d)!",
                     no_time_count,
                     *count_pts);
        while (no_time_head) {
            gps_point_ll *tmp = no_time_head;
            no_time_head = no_time_head->next;
            free(tmp);
        }
    }
}

/* Reads file, parses it and stores the gps values in a gps_point_raw array (allocated inside) 
 * and its size in gps_points_size.
 * swap_to_180 -> shifts all longitudes around with 180 becoming 0, only use it if user requests it
 * Returns 0 on failure, 1 on success.
 */
int qxml_parse_file(gps_private_data gdata)
{
    gps_point_ll *gps_list_head = NULL, *tmp = NULL;
    int count_pts = 0, nr = 0;
    char *filename = gdata.last_filename;
    gps_point_raw *gps_array = NULL;
    int &swap_to_180 = *gdata.swap180;

    // mlt_log_info(gdata.filter, "in qxml_parse_file, filename: %s, swap_to_180=%d\n", filename, swap_to_180);

    QFile gps_file(filename);

    if (!gps_file.open(QFile::ReadOnly | QFile::Text)) {
        mlt_log_warning(gdata.filter, "qxml_parse_file couldn't read file: %s", filename);
        return 0;
    }

    QXmlStreamReader qxml_reader(&gps_file);
    qxml_reader.setNamespaceProcessing(false);
    while (!qxml_reader.atEnd() && !qxml_reader.hasError()) {
        qxml_reader.readNextStartElement();
        if (qxml_reader.isStartDocument()) {
            // mlt_log_info(NULL, "qxml_parse_file: StartDocument found. Encoding=%s\n", qUtf8Printable(qxml_reader.documentEncoding().toString()));
            continue;
        }
        // mlt_log_info(NULL, " readNext element tag: %s, type=%d \n", qUtf8Printable(qxml_reader.name().toString()), qxml_reader.tokenType());
        if (qxml_reader.name() == QStringLiteral("TrainingCenterDatabase")) {
            qxml_parse_tcx(qxml_reader, &gps_list_head, &count_pts);
        } else if (qxml_reader.name() == QStringLiteral("gpx")) {
            qxml_parse_gpx(qxml_reader, &gps_list_head, &count_pts);
        } else {
            mlt_log_warning(
                gdata.filter,
                "qxml_parse_file: fail to parse file: %s, unknown root element: %s. Aborting. \n",
                filename,
                qUtf8Printable(qxml_reader.name().toString()));
            return 0;
        }
    }
    if (qxml_reader.hasError()) {
        mlt_log_info(NULL,
                     "qxml_reader.hasError! line:%u, errString:%s\n",
                     (unsigned) qxml_reader.lineNumber(),
                     qUtf8Printable(qxml_reader.errorString()));
        return 0;
    }

    qxml_reader.clear(); //not sure if really needed if not reused

    if (count_pts < 2) {
        mlt_log_warning(gdata.filter,
                        "qxml_parse_file: less than 2 gps points read (%d). Aborting. \n",
                        count_pts);
        return 0;
    }

    *gdata.ptr_to_gps_points_r = (gps_point_raw *) calloc(count_pts, sizeof(gps_point_raw));
    gps_array = *gdata.ptr_to_gps_points_r; //just an alias
    if (gps_array == NULL) {
        mlt_log_error(gdata.filter,
                      "malloc error (size=%u)\n",
                      (unsigned) (count_pts * sizeof(gps_point_raw)));
    }
    *gdata.gps_points_size = count_pts;

    //copy points to private array and free list
    while (gps_list_head) {
        gps_array[nr++] = gps_list_head->gp;
        tmp = gps_list_head;
        gps_list_head = gps_list_head->next;
        free(tmp);
    }

    //search to find if we cross the 180 and/or the 0 meridian
    bool crosses0 = false, crosses180 = false;
    for (int i = 0; i < *gdata.gps_points_size - 1; i++) {
        double crt = gps_array[i].lon;
        double next = gps_array[i + 1].lon;
        if (crt == GPS_UNINIT || next == GPS_UNINIT)
            continue;
        if ((crt < 0 && next > 0) || (crt > 0 && next < 0)) {
            if (crt - next > 180 || next - crt > 180)
                crosses180 = true;
            else
                crosses0 = true;
        }
    }
    mlt_log_info(
        NULL,
        "_automatic 180 meridian detection: crosses180=%d, crosses0=%d --> swapping180=%d\n",
        crosses180,
        crosses0,
        (crosses180 && !crosses0));
    //if only 180 is crossed, we swap, otherswise we don't do anything
    if (crosses180 && !crosses0) {
        swap_to_180 = 1;
        for (int i = 0; i < *gdata.gps_points_size; i++)
            gps_array[i].lon = get_180_swapped(gps_array[i].lon);
    } else
        swap_to_180 = 0;

    // //debug result:
    // for (int i = 0; i < *gdata.gps_points_size; i += 100) {
    //     gps_point_raw *crt_point = &gps_array[i];
    //     mlt_log_info(NULL,
    //                  "_xml_parse_file read point[%d]: time:%d, lat:%f, lon:%f, speed:%f, "
    //                  "distance:%f, ele:%f, hr:%f, bearing:%f, cadence:%f, temp:%f, power:%f\n",
    //                  i,
    //                  crt_point->time / 1000,
    //                  crt_point->lat,
    //                  crt_point->lon,
    //                  crt_point->speed,
    //                  crt_point->total_dist,
    //                  crt_point->ele,
    //                  crt_point->hr,
    //                  crt_point->bearing,
    //                  crt_point->cad,
    //                  crt_point->atemp,
    //                  crt_point->power);
    //}

    return 1;
}
