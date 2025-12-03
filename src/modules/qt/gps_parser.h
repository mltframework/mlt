/*
 * gps_parser.h -- Header for gps parsing (.gpx and .tcx) and processing
 * Copyright (C) 2011-2021 Meltytech, LLC
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
#ifndef GPS_PARSER_H
#define GPS_PARSER_H

#include <cmath>
#include <framework/mlt.h>
#include <QFile>
#include <QXmlStreamReader>

#define GPS_UNINIT -9999

typedef struct
{
    double lat, lon, lat_projected, speed, total_dist, ele, hr, bearing, cad, atemp, power;
    int64_t time; //epoch milliseconds
} gps_point_raw;

typedef struct gps_point_raw_list
{
    gps_point_raw gp;
    struct gps_point_raw_list *next;
} gps_point_ll;

typedef struct
{
    double lat, lon, lat_projected, lon3857, speed, speed_vertical, speed_3d, total_dist, ele, hr, bearing, cad, atemp,
        power;
    int64_t time;
    double d_elev, elev_up, elev_down, dist_up, dist_down, dist_flat, grade_p;
} gps_point_proc;

//0 is a valid value for many fields, use GPS_UNINIT (-9999) to differentiate missing values
static const gps_point_raw uninit_gps_raw_point = {.lat = GPS_UNINIT,
                                                   .lon = GPS_UNINIT,
                                                   .lat_projected = GPS_UNINIT,
                                                   .speed = GPS_UNINIT,
                                                   .total_dist = GPS_UNINIT,
                                                   .ele = GPS_UNINIT,
                                                   .hr = GPS_UNINIT,
                                                   .bearing = GPS_UNINIT,
                                                   .cad = GPS_UNINIT,
                                                   .atemp = GPS_UNINIT,
                                                   .power = GPS_UNINIT,
                                                   .time = GPS_UNINIT};

static const gps_point_proc uninit_gps_proc_point = {.lat = GPS_UNINIT,
                                                     .lon = GPS_UNINIT,
                                                     .lat_projected = GPS_UNINIT,
                                                     .speed = GPS_UNINIT,
                                                     .speed_vertical = GPS_UNINIT,
                                                     .speed_3d = GPS_UNINIT,
                                                     .total_dist = GPS_UNINIT,
                                                     .ele = GPS_UNINIT,
                                                     .hr = GPS_UNINIT,
                                                     .bearing = GPS_UNINIT,
                                                     .cad = GPS_UNINIT,
                                                     .atemp = GPS_UNINIT,
                                                     .power = GPS_UNINIT,
                                                     .time = GPS_UNINIT,
                                                     .d_elev = GPS_UNINIT,
                                                     .elev_up = GPS_UNINIT,
                                                     .elev_down = GPS_UNINIT,
                                                     .dist_up = GPS_UNINIT,
                                                     .dist_down = GPS_UNINIT,
                                                     .dist_flat = GPS_UNINIT,
                                                     .grade_p = GPS_UNINIT};

//structure used to ease argument passing between filter and GPS parser api
typedef struct
{
    gps_point_raw *gps_points_r;         //raw gps data from file
    gps_point_proc *gps_points_p;        //processed gps data
    gps_point_raw **ptr_to_gps_points_r; //if malloc is needed
    gps_point_proc **ptr_to_gps_points_p;
    int *gps_points_size;
    int *last_searched_index; //used to cache repeated searches
    int64_t *first_gps_time;
    int64_t *last_gps_time;
    char *interpolated;
    int *swap180;
    //read only:
    int64_t gps_proc_start_t; //process only points after this time (epoch miliseconds)
    int last_smooth_lvl;
    char *last_filename; //gps file fullpath
    mlt_filter filter;
} gps_private_data;

#define has_valid_location(x) (((x).lat) != GPS_UNINIT && ((x).lon) != GPS_UNINIT)
#define has_valid_location_ptr(x) ((x) && ((x)->lat) != GPS_UNINIT && ((x)->lon) != GPS_UNINIT)

#define MATH_PI 3.14159265358979323846
#define to_rad(x) ((x) *MATH_PI / 180.0)
#define to_deg(x) ((x) *180.0 / MATH_PI)

int64_t datetimeXMLstring_to_mseconds(const char *text, char *format = NULL);
void mseconds_to_timestring(int64_t seconds, char *format, char *result);
double distance_haversine_2p(double p1_lat, double p1_lon, double p2_lat, double p2_lon);
double distance_equirectangular_2p(double p1_lat, double p1_lon, double p2_lat, double p2_lon);
double bearing_2p(double p1_lat, double p1_lon, double p2_lat, double p2_lon);
void get_first_gps_time(gps_private_data gdata);
void get_last_gps_time(gps_private_data gdata);
double get_avg_gps_time_ms(gps_private_data gdata);
int get_max_gps_diff_ms(gps_private_data gdata);
int binary_search_gps(gps_private_data gdata, int64_t video_time, bool force_result = false);
int decimals_needed(double x, int use_decimals = -1);
double convert_distance_to_format(double x, const char *format);
double convert_speed_to_format(double x, const char *format);
const char *bearing_to_compass(double x);
void recalculate_gps_data(gps_private_data gdata);
void process_gps_smoothing(gps_private_data gdata, char do_processing);

double weighted_middle_double(
    double v1, int64_t t1, double v2, int64_t t2, int64_t new_t, int max_gps_diff_ms);
int64_t weighted_middle_int64(
    int64_t v1, int64_t t1, int64_t v2, int64_t t2, int64_t new_t, int max_gps_diff_ms);
gps_point_proc weighted_middle_point_proc(gps_point_proc *p1,
                                          gps_point_proc *p2,
                                          int64_t new_t,
                                          int max_gps_diff_ms);

void qxml_parse_gpx(QXmlStreamReader &reader, gps_point_ll **gps_list, int *count_pts);
void qxml_parse_tcx(QXmlStreamReader &reader, gps_point_ll **gps_list, int *count_pts);
int qxml_parse_file(gps_private_data gdata);

#define swap_180_if_needed(lon) (pdata->swap_180 ? get_180_swapped(lon) : lon)
double get_180_swapped(double lon);

int time_val_between_indices_proc(
    int64_t time_val, gps_point_proc *gp, int i, int size, int max_gps_diff_ms, bool force_result);
int time_val_between_indices_raw(
    int64_t time_val, gps_point_raw *gp, int i, int size, int max_gps_diff_ms, bool force_result);

//TODO: this is copied from src\framework\mlt_producer.c
/*
 * Boost implementation of timegm()
 * (C) Copyright Howard Hinnant
 * (C) Copyright 2010-2011 Vicente J. Botet Escriba
 */

static inline int32_t is_leap(int32_t year)
{
    if (year % 400 == 0)
        return 1;
    if (year % 100 == 0)
        return 0;
    if (year % 4 == 0)
        return 1;
    return 0;
}

static inline int32_t days_from_0(int32_t year)
{
    year--;
    return 365 * year + (year / 400) - (year / 100) + (year / 4);
}

static inline int32_t days_from_1970(int32_t year)
{
    const int days_from_0_to_1970 = days_from_0(1970);
    return days_from_0(year) - days_from_0_to_1970;
}

static inline int32_t days_from_1jan(int32_t year, int32_t month, int32_t day)
{
    static const int32_t days[2][12] = {{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
                                        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}};
    return days[is_leap(year)][month - 1] + day - 1;
}

static inline time_t internal_timegm(struct tm const *t)
{
    int year = t->tm_year + 1900;
    int month = t->tm_mon;
    if (month > 11) {
        year += month / 12;
        month %= 12;
    } else if (month < 0) {
        int years_diff = (-month + 11) / 12;
        year -= years_diff;
        month += 12 * years_diff;
    }
    month++;
    int day = t->tm_mday;
    int day_of_year = days_from_1jan(year, month, day);
    int days_since_epoch = days_from_1970(year) + day_of_year;

    time_t seconds_in_day = 3600 * 24;
    time_t result = seconds_in_day * days_since_epoch + 3600 * t->tm_hour + 60 * t->tm_min
                    + t->tm_sec;

    return result;
}

/* End of Boost implementation of timegm(). */

#endif /* GPS_PARSER_H */
