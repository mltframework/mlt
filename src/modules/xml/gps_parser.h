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

#include <framework/mlt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#define GPS_UNINIT -9999
#define MAX_GPS_DIFF_MS 10000

typedef struct
{
	double lat, lon, speed, total_dist, ele;
	int64_t time; //epoch milliseconds
	int bearing;
	short hr;
} gps_point_raw;

typedef struct gps_point_raw_list {
	gps_point_raw gp;
	struct gps_point_raw_list *next;
} gps_point_ll;

typedef struct
{
	double lat, lon, speed, total_dist, ele;
	int64_t time;
	double d_elev, elev_up, elev_down, dist_up, dist_down, dist_flat;
	int bearing;
	short hr;
} gps_point_proc;

//0 is a valid value for many fields, use GPS_UNINIT (-9999) to differentiate missing values
static const gps_point_raw uninit_gps_raw_point = {
	.lat=GPS_UNINIT, .lon=GPS_UNINIT, .speed=GPS_UNINIT, .total_dist=GPS_UNINIT,
	.ele=GPS_UNINIT, .time=GPS_UNINIT, .bearing=GPS_UNINIT, .hr=GPS_UNINIT
	};

static const gps_point_proc uninit_gps_proc_point = {
	.lat=GPS_UNINIT, .lon=GPS_UNINIT, .speed=GPS_UNINIT,
	.total_dist=GPS_UNINIT, .ele=GPS_UNINIT, .time=GPS_UNINIT,
	.d_elev=GPS_UNINIT, .elev_up=GPS_UNINIT, .elev_down=GPS_UNINIT,
	.dist_up=GPS_UNINIT, .dist_down=GPS_UNINIT, .dist_flat=GPS_UNINIT, 
	.bearing=GPS_UNINIT, .hr=GPS_UNINIT
	};

//structure used to ease argument passing between filter and GPS parser api
typedef struct
{
	gps_point_raw* gps_points_r; //raw gps data from file
	gps_point_proc* gps_points_p; //processed gps data
	gps_point_raw** ptr_to_gps_points_r; //if malloc is needed
	gps_point_proc** ptr_to_gps_points_p;
	int* gps_points_size;
	int* last_searched_index; //used to cache repeated searches
	int64_t* first_gps_time;
	int64_t* last_gps_time;
	char* interpolated;
	//read only:
	int64_t gps_proc_start_t; //process only points after this time (epoch miliseconds)
	int last_smooth_lvl;
	char* last_filename; //gps file fullpath
	mlt_filter filter;
} gps_private_data;

#define has_valid_location(x) (((x).lat)!=GPS_UNINIT && ((x).lon)!=GPS_UNINIT)
#define has_valid_location_ptr(x) ((x) && ((x)->lat)!=GPS_UNINIT && ((x)->lon)!=GPS_UNINIT)

#define MATH_PI 3.14159265358979323846
#define to_rad(x) ((x)*MATH_PI/180.0)
#define to_deg(x) ((x)*180.0/MATH_PI)

int64_t datetimeXMLstring_to_mseconds(const char* text, char* format);
void mseconds_to_timestring (int64_t seconds, char* format, char* result);
double distance_haversine_2p (double p1_lat, double p1_lon, double p2_lat, double p2_lon);
double distance_equirectangular_2p (double p1_lat, double p1_lon, double p2_lat, double p2_lon);
int bearing_2p (double p1_lat, double p1_lon, double p2_lat, double p2_lon);
void get_first_gps_time(gps_private_data gdata);
void get_last_gps_time(gps_private_data gdata);
int binary_search_gps(gps_private_data gdata, int64_t video_time, char force_result);
char* bearing_to_compass(int x);
void recalculate_gps_data(gps_private_data gdata);
void process_gps_smoothing(gps_private_data gdata, char do_processing);

double weighted_middle_double (double v1, int64_t t1, double v2, int64_t t2, int64_t new_t);
int64_t weighted_middle_int64 (int64_t v1, int64_t t1, int64_t v2, int64_t t2, int64_t new_t);
gps_point_proc weighted_middle_point_proc(gps_point_proc* p1, gps_point_proc* p2, int64_t new_t);

void xml_parse_gpx(xmlNodeSetPtr found_nodes, gps_point_ll **gps_list, int *count_pts);
void xml_parse_tcx(xmlNodeSetPtr found_nodes, gps_point_ll **gps_list, int *count_pts);
int xml_parse_file(gps_private_data gdata);

#endif /* GPS_PARSER_H */