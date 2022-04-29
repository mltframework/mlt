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
#define _GNU_SOURCE

#include "gps_parser.h"
#include <inttypes.h>

#define _x (const xmlChar*)
#define _s (const char*)

/* Converts the datetime string from gps file into seconds since epoch in local timezone
 * Note: assumes UTC
 */
int64_t datetimeXMLstring_to_mseconds(const char* text, char* format)
{
	char def_format[] = "%Y-%m-%dT%H:%M:%S";
	int64_t ret = 0;
	int ms = 0;
	char * ms_part = NULL;
	struct tm tm_time;
	//samples: 2020-07-11T09:03:23.000Z or 2021-02-27T12:10:00+00:00
	tm_time.tm_isdst = -1; //force dst detection

	if (format == NULL)
		format = def_format;

	if (strptime(text, format, &tm_time) == NULL) {
		mlt_log_warning(NULL, "filter_gpsText.c datetimeXMLstring_to_seconds strptime failed on string: %.25s", text);
		return 0;
	}

	ret = mktime(&tm_time);
#if defined(__GLIBC__) || defined(__APPLE__)
	/* NOTE: mktime assumes local time-zone for the tm_time but GPS time is UTC.
	 * time.h provides an extern: "timezone" which stores the local timezone in seconds
	 * this is used by mktime to "correct" the time so we must substract it to keep UTC
	 */
	ret -= timezone - 3600 * tm_time.tm_isdst;
#endif

	//check if we have miliesconds //NOTE: 3 digits only
	if ( (ms_part = strchr(text, '.')) != NULL ) {
		ms = strtol(ms_part+1, NULL, 10);
		while (abs(ms) > 999)
			ms /= 10;
	}
	ret = ret*1000 + ms;

	//mlt_log_info(NULL, "datetimeXMLstring_to_mseconds: text:%s, ms:%d (/1000)", text, ret/1000);
	return ret;
}

//checks if provided char* is only made of whitespace chars
static int is_whitespace_string(char* str) {
	unsigned i;
	for (i=0; i<strlen(str); i++) {
		if (!isspace(str[i]))
			return 0;
	}
	return 1;
}

/* Converts miliseconds to a date-time with optional format (no miliesconds in output)
 * Note: gmtime completely ignores timezones - this keeps time consistent
 *       as all times in this file are (hopefully) in UTC
 */
void mseconds_to_timestring (int64_t seconds, char* format, char* result)
{
	time_t secs = seconds/1000;
	struct tm * ptm = gmtime(&secs);
	if (!format || is_whitespace_string(format))
		strftime(result, 25, "%Y-%m-%d %H:%M:%S", ptm);
	else
		strftime(result, 50, format, ptm);
}

//returns the distance between 2 gps points (accurate for very large distances)
double distance_haversine_2p (double p1_lat, double p1_lon, double p2_lat, double p2_lon)
{
	const int R = 6371000;
	double dlat = to_rad(p2_lat - p1_lat);
	double dlon = to_rad(p2_lon - p1_lon);
	double a = sin(dlat/2.0)*sin(dlat/2.0) + cos(to_rad(p1_lat))*cos(to_rad(p2_lat))*sin(dlon/2.0)*sin(dlon/2.0);
	double c = 2.0*atan2(sqrt(a), sqrt(1-a));
	return R * c; //return is in meters because radius is in meters
}

// Returns the distance between 2 gps points, accurate enough (<0.1% error) for distances below 4km
double distance_equirectangular_2p (double p1_lat, double p1_lon, double p2_lat, double p2_lon)
{
	//return 0 for very far points (implement haversine formula if needed)
	if (fabs(p1_lat-p2_lat) > 0.05 || fabs(p1_lat-p2_lat) > 0.05) { //~5.5km at equator to ~2km at arctic circle
		mlt_log_info(NULL, "distance_equirectangular_2p: points are too far away, doing haversine (%f,%f to %f,%f)", p1_lat, p1_lon, p2_lat, p2_lon);
		return distance_haversine_2p(p1_lat, p1_lon, p2_lat, p2_lon);
	}

	const int R = 6371000;
	double x = (to_rad(p2_lon) - to_rad(p1_lon)) * cos((to_rad(p2_lat) + to_rad(p1_lat)) / 2.0);
	double y = to_rad(p1_lat) - to_rad(p2_lat);
	return sqrt(x*x + y*y) * R;
	//NOTE: adding altitude is very risky, GPS altitude seems pretty bad and with sudden large jumps
	//maybe consider adding distance_2d and distance_3d fields //sqrt(2D_distance^2 + delta_alt^2)
}

// Computes bearing from 2 gps points
int bearing_2p (double p1_lat, double p1_lon, double p2_lat, double p2_lon)
{
	double lat1 = to_rad(p1_lat);
	double lat2 = to_rad(p2_lat);
	double dlon = to_rad(p2_lon-p1_lon);

	double y = sin(dlon) * cos(lat2);
	double x = cos(lat1)*sin(lat2) - sin(lat1)*cos(lat2)*cos(dlon);
	double bearing = to_deg(atan2(y,x));
	//normalize to 0-360
	return (int)(bearing + 360) % 360;
}

/** Searches for and returns the first valid time with locaton in the gps_points_r array
 *  returns in miliesconds, 0 on error
*/
void get_first_gps_time(gps_private_data gdata)
{
	gps_point_raw* gps_points = gdata.gps_points_r;
	if (!gps_points) {
		*gdata.first_gps_time = 0;
		return;
	}

	int i = 0;
	while (i < *gdata.gps_points_size)
	{
		if (gps_points[i].time && has_valid_location(gps_points[i]))
		{
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
	gps_point_raw* gps_points = gdata.gps_points_r;
	if (!gps_points) {
		*gdata.last_gps_time = 0;
		return;
	}

	int i = *gdata.gps_points_size-1;
	while (i >= 0)
	{
		if (gps_points[i].time && has_valid_location(gps_points[i]))
		{
			//mlt_log_info(filter, "get_last_gps_time found: %d", gps_points[i].time);
			*gdata.last_gps_time = gps_points[i].time;
			return;
		}
		i--;
	}
	*gdata.last_gps_time = 0;
}

//checks if time value val is between gps_points[i] and gps_points[i+1] with size checks
static int time_val_between_indices(int64_t val, gps_point_raw* gp, int i, int size, char force_result) {
	if (i<0 || i>size)
		return 0;
	else if (val == gp[i].time)
		return 1;
	else if (i+1 < size && gp[i].time < val && val < gp[i+1].time) 
	{
		if (force_result)
			return 1;
		else if (llabs(gp[i].time - gp[i+1].time) <= MAX_GPS_DIFF_MS)
			return 1;
	}
	return 0;
}

/** Returns the [index of] nearest gps point in time, but not farther than MAX_GPS_DIFF (10 seconds)
 * or -1 if search fails
 * Seaches in raw values directly
 * If force_result is nonzero, it will ignore MAX_GPS_DIFF restriction
*/
int binary_search_gps(gps_private_data gdata, int64_t video_time, char force_result)
{
	gps_point_raw* gps_points = gdata.gps_points_r;
	const int gps_points_size = *gdata.gps_points_size;
	int last_index = *gdata.last_searched_index;

	int il = 0;
	int ir = gps_points_size-1;
	int mid = 0;

	if (!gps_points || gps_points_size==0)
		return -1;

	//optimize repeated calls (exact match or in between points)
	if (time_val_between_indices(video_time, gps_points, last_index, gps_points_size, force_result)) {
//		printf("_binary_search_gps, last_index(%d) v1 video_time: %I64d match: %I64d\n", last_index, video_time, gps_points[last_index].time);
		return last_index;
	}

	//optimize consecutive playback calls
	last_index++;
	if (time_val_between_indices(video_time, gps_points, last_index, gps_points_size, force_result)) {
//		printf("_binary_search_gps, last_index(%d) v2 video_time: %I64d match: %I64d\n", last_index, video_time, gps_points[last_index].time);
		*gdata.last_searched_index = last_index;
		return last_index;
	}

	//optimize for outside values
	if (video_time < *gdata.first_gps_time - MAX_GPS_DIFF_MS || video_time > *gdata.last_gps_time + MAX_GPS_DIFF_MS)
		return -1;

	//binary search
	while (il < ir)
	{
		mid = (ir+il)/2;
		//mlt_log_info(gdata.filter, "binary_search_gps: mid=%d, l=%d, r=%d, mid-time=%d (s)", mid, il, ir, gps_points[mid].time/1000);
		if (time_val_between_indices(video_time, gps_points, mid, gps_points_size, force_result)) {
			*gdata.last_searched_index = mid;
			break;
		}
		else if (gps_points[mid].time > video_time)
			ir = mid-1;
		else //if (gps_points[mid].time < video_time)
			il = mid+1;
	}

	//don't return the closest gps point if time difference is too large (unless force_result is 1)
	if (llabs(video_time - gps_points[mid].time) > MAX_GPS_DIFF_MS)
		return (force_result ? mid : -1);
	else 
		return mid;
}

/* Converts the bearing angle (0-360) to a cardinal direction
 * 8 sub-divisions
 */
char* bearing_to_compass(int x)
{
	if (x<=22.5 || x>=360-22.5)
		return "N";
	else if (x<45+22.5)
		return "NE";
	else if (x<=90+22.5)
		return "E";
	else if (x<90+45+22.5)
		return "SE";
	else if (x<=180+22.5)
		return "S";
	else if (x<180+45+22.5)
		return "SW";
	else if (x<=270+22.5)
		return "W";
	else if (x<270+45+22.5)
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
		if ((*gdata.ptr_to_gps_points_p = calloc(*gdata.gps_points_size, sizeof(gps_point_proc))) == NULL) {
			mlt_log_warning(gdata.filter, "calloc error, size=%"PRIu64"\n", *gdata.gps_points_size*sizeof(gps_point_proc));
			return;
		}
		else { //alloc ok
			gdata.gps_points_p = *gdata.ptr_to_gps_points_p;
			process_gps_smoothing(gdata, 0);
		}
	}

	gps_point_proc* gps_points = gdata.gps_points_p;
	const int gps_points_size = *gdata.gps_points_size;

	//compute gps_start_time actual offset
	int offset_start = 0;
	if (gdata.gps_proc_start_t != 0) {
		offset_start = binary_search_gps(gdata, gdata.gps_proc_start_t, 1);
		if (offset_start == -1) {//=either before or after entire gps track time
			if (gdata.gps_proc_start_t > *gdata.last_gps_time)
				offset_start = *gdata.gps_points_size;
			else
				offset_start = 0;
		}
		offset_start++;
	}

	//mlt_log_info(gdata.filter, "recalculate_gps_data, offset=%d, points=%p, new:%p, size:%d, newSize:%d", offset,	 gdata.gps_points, gps_points, *gdata.gps_points_size, gps_points_size);

	int ignore_points_before = 0;
	double total_dist = 0, total_d_elev = 0, total_elev_up = 0, total_elev_down = 0,
		   total_dist_up = 0, total_dist_down = 0, total_dist_flat = 0;
	double start_dist = 0, start_d_elev = 0, start_elev_up = 0, start_elev_down = 0, 
			start_dist_up = 0, start_dist_down = 0, start_dist_flat = 0;
	gps_point_proc *crt_point = NULL, *prev_point = NULL, *prev_nrsmooth_point = NULL;
	
	for (i=0; i<gps_points_size; i++)
	{
		//store values at processing_start_time to substract them at the end
		if (i-1 == offset_start) {
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
		int smooth_index = MAX(ignore_points_before, MAX(0, i-req_smooth));

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
		while (smooth_index<i && !has_valid_location(gps_points[smooth_index])) smooth_index++;
		if (smooth_index < i)
			prev_nrsmooth_point = &gps_points[smooth_index];

		//1) get distance difference between last 2 valid points
		double d_dist = distance_equirectangular_2p (prev_point->lat, prev_point->lon, crt_point->lat, crt_point->lon);

		//if time difference is way longer for one point than usual, don't do math on that gap (treat recording paused, move far, then resume)
		//5*average time
		if (crt_point->time - prev_point->time > 5.0 * (*gdata.last_gps_time - *gdata.first_gps_time) / *gdata.gps_points_size)
		{
			prev_nrsmooth_point = NULL;
			ignore_points_before = i;
			crt_point->total_dist = total_dist;
			prev_point = crt_point;
			continue;
		}

		//this is the total distance since beginning of gps processing time
		total_dist += d_dist;
		crt_point->total_dist = total_dist;

		//2)+3) speed and bearing
		if (req_smooth < 2)
		{
			crt_point->speed = d_dist / ((crt_point->time - prev_point->time)/1000.0); //in m/s
			crt_point->bearing = bearing_2p(prev_point->lat, prev_point->lon, crt_point->lat, crt_point->lon);
		}
		else //for "smoothing" we calculate distance between 2 points "nr_smoothing" distance behind
		{
			if (prev_nrsmooth_point) {
				double d_dist = crt_point->total_dist - prev_nrsmooth_point->total_dist;
				int64_t d_time = crt_point->time - prev_nrsmooth_point->time;
				crt_point->speed = d_dist/(d_time/1000.0);

				crt_point->bearing = bearing_2p(prev_nrsmooth_point->lat, prev_nrsmooth_point->lon, crt_point->lat, crt_point->lon);
			}

		}

		//4) altitude stuff
		if (crt_point->ele != GPS_UNINIT && prev_point->ele != GPS_UNINIT)
		{
			double d_elev = crt_point->ele - prev_point->ele;
			total_d_elev += d_elev;
			if (crt_point->ele > prev_point->ele) {
				total_elev_up += d_elev;
				total_dist_up += d_dist;
			}
			else if (crt_point->ele < prev_point->ele) {
				total_elev_down += d_elev;
				total_dist_down += d_dist;
			}
			else
				total_dist_flat += d_dist;

			crt_point->d_elev = total_d_elev;
			crt_point->elev_up = total_elev_up;
			crt_point->elev_down = total_elev_down;
			crt_point->dist_up = total_dist_up;
			crt_point->dist_down = total_dist_down;
			crt_point->dist_flat = total_dist_flat;
		}

		prev_point = crt_point;
	}
	
	//clean up computed values for relative stuff (distances mostly) before gps_processing_start time
	if (gdata.gps_proc_start_t != 0 && offset_start > 0 && offset_start < gps_points_size) {
		//mlt_log_info(gdata.filter, "recalculate_gps_data: clearing gps data from 0 to %d due to set GPS start time:%d s", offset_start, gdata.gps_proc_start_t/1000);
		for (i=0; i<offset_start; i++) {
			gps_point_proc* crt_point = &(gdata.gps_points_p[i]);
			if (crt_point->total_dist != 0) start_dist = crt_point->total_dist;
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
		for (i=offset_start; i<gps_points_size; i++) {
			gps_point_proc* crt_point = &(gdata.gps_points_p[i]);
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
double weighted_middle_double (double v1, int64_t t1, double v2, int64_t t2, int64_t new_t) {
	int64_t d_time = t2 - t1;
	if (v1 == GPS_UNINIT) return v2;
	if (v2 == GPS_UNINIT) return v1;
	if (d_time > MAX_GPS_DIFF_MS || d_time == 0) return v1;

	double prev_weight = 1 - (double)(new_t - t1)/d_time;
	double next_weight = 1 - (double)(t2 - new_t)/d_time;
	double rez = v1 * prev_weight + v2 * next_weight;
	//mlt_log_info(NULL, "weighted_middle_double in: v:%f-%f, %d-%d %d out: %f ", v1, v2, t1/1000, t2/1000, new_t/1000, rez);
	return rez;
}

/* Returns a weighted (type:int64_t) value for an intermediary time
 * Notes: time limit 10s, if one value is GPS_UNINIT, returns the other
*/
int64_t weighted_middle_int64 (int64_t v1, int64_t t1, int64_t v2, int64_t t2, int64_t new_t) {
	int64_t d_time = t2 - t1;
	if (v1 == GPS_UNINIT) return v2;
	if (v2 == GPS_UNINIT) return v1;
	if (d_time > MAX_GPS_DIFF_MS || d_time == 0) return v1;

	double prev_weight = 1 - (double)(new_t - t1)/d_time;
	double next_weight = 1 - (double)(t2 - new_t)/d_time;
	int64_t rez = v1 * prev_weight + v2 * next_weight;
	//mlt_log_info(NULL, "weighted_middle_int64 in: v:%d-%d, %d-%d %d out: %d ", v1%1000000, v2%1000000, t1/1000, t2/1000, new_t/1000, rez%1000000);
	return rez;
}

//compute a virtual point at a specific time between 2 real points
gps_point_proc weighted_middle_point_proc(gps_point_proc* p1, gps_point_proc* p2, int64_t new_t) {
	//strict time check, max 10s difference
	if (llabs(p2->time - p1->time) > MAX_GPS_DIFF_MS)
		return *p1;
	gps_point_proc crt_point = uninit_gps_proc_point;
	crt_point.lat = weighted_middle_double(p1->lat, p1->time, p2->lat, p2->time, new_t);
	crt_point.lon = weighted_middle_double(p1->lon, p1->time, p2->lon, p2->time, new_t);
	crt_point.speed = weighted_middle_double(p1->speed, p1->time, p2->speed, p2->time, new_t);
	crt_point.total_dist = weighted_middle_double(p1->total_dist, p1->time, p2->total_dist, p2->time, new_t);
	crt_point.ele = weighted_middle_double(p1->ele, p1->time, p2->ele, p2->time, new_t);
	crt_point.time = weighted_middle_int64(p1->time, p1->time, p2->time, p2->time, new_t);
	crt_point.d_elev = weighted_middle_double(p1->d_elev, p1->time, p2->d_elev, p2->time, new_t);
	crt_point.elev_up = weighted_middle_double(p1->elev_up, p1->time, p2->elev_up, p2->time, new_t);
	crt_point.elev_down = weighted_middle_double(p1->elev_down, p1->time, p2->elev_down, p2->time, new_t);
	crt_point.dist_up = weighted_middle_double(p1->dist_up, p1->time, p2->dist_up, p2->time, new_t);
	crt_point.dist_down = weighted_middle_double(p1->dist_down, p1->time, p2->dist_down, p2->time, new_t);
	crt_point.dist_flat = weighted_middle_double(p1->dist_flat, p1->time, p2->dist_flat, p2->time, new_t);
	crt_point.bearing = weighted_middle_int64(p1->bearing, p1->time, p2->bearing, p2->time, new_t);
	crt_point.hr = weighted_middle_int64(p1->hr, p1->time, p2->hr, p2->time, new_t);
	return crt_point;
}

//checks whether 2 points (not necessarily consecutive) are not after a long pause in tracking
int in_gps_time_window(gps_private_data gdata, int crt, int next, double max_time) {
	gps_point_raw* gp = gdata.gps_points_r;
	int64_t d_time = llabs(gp[next].time - gp[crt].time);
	int d_indices = abs(next - crt);
	return d_time <= (max_time * d_indices + MAX_GPS_DIFF_MS);
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
		mlt_log_warning(gdata.filter, "process_gps_smoothing - gps_points_r is null!");
		return;
	}
	if (gdata.gps_points_p == NULL) {
		if ((*gdata.ptr_to_gps_points_p = calloc(*gdata.gps_points_size, sizeof(gps_point_proc))) == NULL) {
			mlt_log_warning(gdata.filter, "calloc failed, size =%"PRIu64"\n", *gdata.gps_points_size * sizeof(gps_point_proc));
			return;
		}
		else 
			gdata.gps_points_p = *gdata.ptr_to_gps_points_p;
	}

	int i, j, nr_hr = 0, nr_ele = 0;
	short hr = GPS_UNINIT;
	double ele = GPS_UNINIT;

	//linear interpolation for heart rate and elevation, one time per file, ignores start offset
	if (*gdata.interpolated == 0)
	{
		//NOTE: interpolation limit is now 60 values = arbitrarily chosen @ 1min (if 1sec interval), find a better value?
		gps_point_raw* gp_r = gdata.gps_points_r;
		gps_point_proc* gp_p = gdata.gps_points_p;
		for (i=0; i<*gdata.gps_points_size; i++)
		{
			gp_p[i].hr = gp_r[i].hr; //calloc made everything 0, fill back with GPS_UNINIT if needed
			gp_p[i].ele = gp_r[i].ele;

			//heart rate
			if (gp_r[i].hr != GPS_UNINIT) { //found valid hr
				if (hr != GPS_UNINIT && nr_hr>0 && nr_hr<=60) { //there were missing values and had a hr before
					nr_hr++;
					for (j=i; j>i-nr_hr; j--) { //go backwards and fill values
						gp_p[j].hr = hr + 1.0*(gp_r[i].hr - hr) * (1.0*(j-(i-nr_hr))/nr_hr);
						//printf("_i=%d, j=%d, nr_hr=%d hr = %d; gp_r[i].hr=%d, gp_p[j].hr=%d  \n", i,j,nr_hr, hr, gp_r[i].hr, gp_p[j].hr);
					}
				}
				hr = gp_r[i].hr;
				nr_hr = 0;
			}
			else
				nr_hr++;

			//altitude
			if (gp_r[i].ele != GPS_UNINIT) {
				if (ele != GPS_UNINIT && nr_ele>0 && nr_ele<=60) {
					nr_ele++;
					for (j=i; j>i-nr_ele; j--) {
						gp_p[j].ele = ele + 1.0*(gp_r[i].ele - ele) * (1.0*(j-(i-nr_ele))/nr_ele);
						//printf("_i=%d, j=%d, nr_ele=%d ele = %f; gp_r[i].ele=%f, gp_p[j].ele=%f  \n", i,j,nr_ele, ele, gp_r[i].ele, gp_p[j].ele);
					}
				}
				ele = gp_r[i].ele;
				nr_ele = 0;
			}
			else
				nr_ele++;

			//these are not interpolated but as long as we're iterating we can copy them now
			gp_p[i].time = gp_r[i].time;
			gp_p[i].lat = gp_r[i].lat;
			gp_p[i].lon = gp_r[i].lon;
		}
	}

	gps_point_raw* gps_points_r = gdata.gps_points_r;
	gps_point_proc* gps_points_p = gdata.gps_points_p;
	const int gps_points_size = *gdata.gps_points_size;
	const double avg_gps_time = (*gdata.last_gps_time - *gdata.first_gps_time) / *gdata.gps_points_size;

	for (i=0; i<gps_points_size; i++)
	{
		if (req_smooth == 1) //copy raw lat/lon to calc lat/lon and interpolate 1 location if necessary
		{
			gps_points_p[i].lat = gps_points_r[i].lat;
			gps_points_p[i].lon = gps_points_r[i].lon;

			//this can happen often if location and altitude are stored at different time intervals (every 3s vs every 10s)
			if (	i-1 >= 0 && i+1 < gps_points_size //if we're not at start/end
					&& !has_valid_location(gps_points_r[i]) && has_valid_location(gps_points_r[i-1]) && has_valid_location(gps_points_r[i+1]) //if current point has no lat/lon but nearby ones do
					&& llabs(gps_points_r[i+1].time - gps_points_r[i-1].time) < MAX_GPS_DIFF_MS) //if time difference is lower than MAX_GPS_DIFF_MS
			{
				//place a weighted "middle" point here depending on time difference
				gps_points_p[i].lat = weighted_middle_double(gps_points_r[i-1].lat, gps_points_r[i-1].time,
										gps_points_r[i+1].lat, gps_points_r[i+1].time,
										gps_points_r[i].time);
				gps_points_p[i].lon = weighted_middle_double(gps_points_r[i-1].lon, gps_points_r[i-1].time,
										gps_points_r[i+1].lon, gps_points_r[i+1].time,
										gps_points_r[i].time);

				//mlt_log_info(gdata.filter, "interpolating position for smooth=1, new point[%d]:%f,%f @time=%d s", i, gps_points_p[i].lat, gps_points_p[i].lon, gps_points_r[i].time/1000);
			}
		}
		else if (req_smooth > 1) //for each point average "req_smooth/2" values before and after
		{
			double lat_sum = 0, lon_sum = 0;
			int nr_div = 0;

			//TODO: treat case of points on opposite sides of 180*
			for (j = MAX(0, i-req_smooth/2) ; j < MIN(i+req_smooth/2, gps_points_size); j++) 
			{
				if (has_valid_location(gps_points_r[j]) && in_gps_time_window(gdata, i, j, avg_gps_time)) {
					lat_sum += gps_points_r[j].lat;
					lon_sum += gps_points_r[j].lon;
					nr_div++;
				}
			}
			if (nr_div!=0) {
				gps_points_p[i].lat = lat_sum/nr_div;
				gps_points_p[i].lon = lon_sum/nr_div;
			} else {
				gps_points_p[i].lat = gps_points_r[i].lat;
				gps_points_p[i].lon = gps_points_r[i].lon;
			}
			//mlt_log_info(filter, "i=%d, lat_sum=%f, lon_sum=%f, nr_div=%d, time=%d", i, lat_sum, lon_sum, nr_div, gps_points[i].time);
		}
	}
	*gdata.interpolated = 1;
	if (req_smooth != 0 && do_processing == 1)
		recalculate_gps_data(gdata);
}

/* xml parsing */

// Parses a .gcx file into a gps_point_raw linked list
void xml_parse_gpx(xmlNodeSetPtr found_nodes, gps_point_ll **gps_list, int *count_pts)
{
	int i;
	xmlNode *crt_node = NULL, *gps_val = NULL, *gps_subval = NULL, *gps_subsubval = NULL;
	int64_t last_time = 0;

	/*
	// sample point from .GPX file (version 1.0) + HR extension
	<trkpt lat="45.227621666484104" lon="25.16533185322889">
		<ele>868.3005248873908</ele>
		<time>2020-07-11T09:03:23.000Z</time>
		<speed>0.0</speed>
		<course>337.1557</course>
		<extensions>
		 <gpxtpx:TrackPointExtension>
		  <gpxtpx:hr>132</gpxtpx:hr>
		 </gpxtpx:TrackPointExtension>
		</extensions>
	</trkpt>
	*/

	//mlt_log_info(NULL, "xml_parse_gpx - parsing %d elements\n", found_nodes->nodeNr);

	for (i=0; i<found_nodes->nodeNr; i++)
	{
		gps_point_raw crt_point = uninit_gps_raw_point;
		crt_node = found_nodes->nodeTab[i];

		if (xmlHasProp(crt_node, _x("lat"))) {
			xmlChar* str = xmlGetProp(crt_node, _x("lat"));
			crt_point.lat = strtod(_s(str), NULL);
			xmlFree(str);
		}
		if (xmlHasProp(crt_node, _x("lon"))) {
			xmlChar* str = xmlGetProp(crt_node, _x("lon"));
			crt_point.lon = strtod(_s(str), NULL);
			xmlFree(str);
		}
		for (gps_val = crt_node->children; gps_val; gps_val = gps_val->next)
		{
			if (strncmp(_s(gps_val->name), "ele", strlen("ele")) == 0)
				crt_point.ele = strtod(_s(gps_val->children->content), NULL);
			else if (strncmp(_s(gps_val->name), "time", strlen("time")) == 0)
				crt_point.time = datetimeXMLstring_to_mseconds(_s(gps_val->children->content), NULL);
			else if (strncmp(_s(gps_val->name), "bearing", strlen("bearing")) == 0)
				crt_point.bearing = (int)strtod(_s(gps_val->children->content), NULL);
			else if (strncmp(_s(gps_val->name), "speed", strlen("speed")) == 0)
				crt_point.speed = strtod(_s(gps_val->children->content), NULL);
			else if (strncmp(_s(gps_val->name), "extensions", strlen("extensions")) == 0)
			{
				for (gps_subval = gps_val->children; gps_subval; gps_subval=gps_subval->next) {
					if (strncmp(_s(gps_subval->name), "gpxtpx:TrackPointExtension", strlen("gpxtpx:TrackPointExtension")) == 0) {
						for (gps_subsubval = gps_subval->children; gps_subsubval; gps_subsubval=gps_subsubval->next) {
							if (strncmp(_s(gps_subsubval->name), "gpxtpx:hr", strlen("gpxtpx:hr")) == 0) {
								crt_point.hr = (short)strtod(_s(gps_subsubval->children->content), NULL);
							}
						}
					}
				}
			}
		}
//		printf("_xml_parse_gpx read point[%d]: time:%I64d, lat:%f, lon:%f, ele:%f, bearing:%d, speed:%f, hr:%d\n",
//				i, crt_point.time, crt_point.lat, crt_point.lon, crt_point.ele, crt_point.bearing, crt_point.speed, crt_point.hr);

		//add point to linked list (but only if increasing time)
		if (crt_point.time != GPS_UNINIT && crt_point.time > last_time) {
			*gps_list = calloc (1, sizeof(gps_point_ll));
			if (*gps_list == NULL) return;

			*count_pts+=1;
			(*gps_list)->gp = crt_point;
			(*gps_list)->next = NULL;
			gps_list = &(*gps_list)->next;
			last_time = crt_point.time;
		}
		else mlt_log_info(NULL, "xml_parse_gpx: skipping point due to time [%d] %f,%f - crt:%"PRId64", last:%"PRId64"\n", i, crt_point.lat, crt_point.lon, crt_point.time, last_time);
	}
}

// Parses a .tcx file into a gps_point_raw linked list
void xml_parse_tcx(xmlNodeSetPtr found_nodes, gps_point_ll **gps_list, int *count_pts)
{
	int i;
	xmlNode *crt_node = NULL, *gps_val = NULL, *gps_subval = NULL;
	int64_t last_time = 0;

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
	  </Trackpoint>
	*/
	//mlt_log_info(NULL, "xml_parse_tcx - parsing %d elements\n", found_nodes->nodeNr);

	for (i=0; i<found_nodes->nodeNr; i++)
	{
		gps_point_raw crt_point = uninit_gps_raw_point;
		crt_node = found_nodes->nodeTab[i];

		for (gps_val = crt_node->children; gps_val; gps_val = gps_val->next)
		{
			if (strncmp(_s(gps_val->name), "Time", strlen("Time")) == 0)
				crt_point.time = datetimeXMLstring_to_mseconds(_s(gps_val->children->content), NULL);
			else if (strncmp(_s(gps_val->name), "Position", strlen("Position")) == 0)
			{
				for (gps_subval = gps_val->children; gps_subval; gps_subval=gps_subval->next) {
					if (strncmp(_s(gps_subval->name), "LatitudeDegrees", strlen("LatitudeDegrees")) == 0)
						crt_point.lat = strtod(_s(gps_subval->children->content), NULL);
					else if (strncmp(_s(gps_subval->name), "LongitudeDegrees", strlen("LongitudeDegrees")) == 0)
						crt_point.lon = strtod(_s(gps_subval->children->content), NULL);
				}
			}
			else if (strncmp(_s(gps_val->name), "AltitudeMeters", strlen("AltitudeMeters")) == 0)
				crt_point.ele = strtod(_s(gps_val->children->content), NULL);
			else if (strncmp(_s(gps_val->name), "DistanceMeters", strlen("DistanceMeters")) == 0)
				crt_point.total_dist = strtod(_s(gps_val->children->content), NULL);
			else if (strncmp(_s(gps_val->name), "HeartRateBpm", strlen("HeartRateBpm")) == 0)
			{
				for (gps_subval = gps_val->children; gps_subval; gps_subval=gps_subval->next)
					if (strncmp(_s(gps_subval->name), "Value", strlen("Value")) == 0)
						crt_point.hr = (short)strtod(_s(gps_subval->children->content), NULL);
			}
		}
//		mlt_log_info(NULL, "_xml_parse_tcx read point [%d]: time:%d, lat:%.12f, lon:%.12f, ele:%f, distance:%f, hr:%d\n",
//				i, crt_point.time, crt_point.lat, crt_point.lon, crt_point.ele, crt_point.total_dist, crt_point.hr);

		//add point to linked list (but only if increasing time)
		if (crt_point.time != GPS_UNINIT && crt_point.time > last_time) {
			*gps_list = calloc (1, sizeof(gps_point_ll));
			if (*gps_list == NULL) return;

			*count_pts+=1;
			(*gps_list)->gp = crt_point;
			(*gps_list)->next = NULL;
			gps_list = &(*gps_list)->next;
			last_time = crt_point.time;
		}
		else mlt_log_info(NULL, "xml_parse_tcx: skipping point due to time [%d] %f,%f - crt:%"PRId64", last:%"PRId64"\n", i, crt_point.lat, crt_point.lon, crt_point.time, last_time);
	}
}

/* Reads file, parses it and stores the gps values in a gps_point_raw array (allocated inside) 
 * and its size in gps_points_size.
 * Returns 0 on failure, 1 on success.
 */
int xml_parse_file(gps_private_data gdata)
{
	int count_pts = 0, rv = 1;
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	xmlXPathContextPtr xpathCtx = NULL;
	xmlXPathObjectPtr xpathObj = NULL;
	gps_point_ll *gps_list_head = NULL;
	char* filename = gdata.last_filename;

	LIBXML_TEST_VERSION

	//mlt_log_info(gdata.filter, "in xml_parse_file, filename: %s", filename);

	doc = xmlParseFile(filename);
	if (doc == NULL) {
		mlt_log_warning(gdata.filter, "xmlParseFile couldn't read or parse file: %s", filename);
		rv = 0;
		goto cleanup;
	}

	root_element = xmlDocGetRootElement(doc);
	if (root_element == NULL) {
		mlt_log_info(gdata.filter, "xmlParseFile no root element found");
		rv = 0;
		goto cleanup;
	}

	//mlt_log_info(gdata.filter, "xml_parse_file root_elem= %s \n", root_element->name);

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		mlt_log_warning(gdata.filter, "xml_parse_file xmlXPathNewContext: unable to create new XPath context");
		rv = 0;
		goto cleanup;
	}

	//xpath query for each doc type
	if (strncmp(_s(root_element->name), "TrainingCenterDatabase", strlen("TrainingCenterDatabase"))==0) {
		const char* xpathExpr = "//*[local-name()='Trackpoint']";
		xpathObj = xmlXPathEvalExpression(_x(xpathExpr), xpathCtx);
		xmlNodeSetPtr nodeset = xpathObj->nodesetval;
		if (xmlXPathNodeSetIsEmpty(nodeset)) {
			mlt_log_warning(gdata.filter, "xml_parse_file xmlXPathEvalExpression: no result, expr='%s'\n", xpathExpr);
			rv = 0;
			goto cleanup;
		}
		xml_parse_tcx(nodeset, &gps_list_head, &count_pts);
	}
	else if (strncmp(_s(root_element->name), "gpx", strlen("gpx"))==0) {
		const char* xpathExpr = "//*[local-name()='trkpt']";
		xpathObj = xmlXPathEvalExpression(_x(xpathExpr), xpathCtx);
		xmlNodeSetPtr nodeset = xpathObj->nodesetval;
		if (xmlXPathNodeSetIsEmpty(nodeset)) {
			mlt_log_warning(gdata.filter, "xml_parse_file xmlXPathEvalExpression: no result, expr='%s'\n", xpathExpr);
			rv = 0;
			goto cleanup;
		}
		xml_parse_gpx(nodeset, &gps_list_head, &count_pts);
	}
	else {
		mlt_log_warning(gdata.filter, "Unsupported file type: root == %s, file=%s", root_element->name, filename);
		rv = 0;
		goto cleanup;
	}

	*gdata.ptr_to_gps_points_r = (gps_point_raw*) calloc(count_pts, sizeof(gps_point_raw));
	gps_point_raw* gps_array = *gdata.ptr_to_gps_points_r; //just an alias
	if (gps_array == NULL) {
		mlt_log_error(gdata.filter, "malloc error (size=%"PRIu64")\n", count_pts * sizeof(gps_point_raw));
		rv = 0;
		goto cleanup;
	}
	*gdata.gps_points_size = count_pts;

	//copy points to private array and free list
	int nr=0;
	gps_point_ll *tmp = NULL;
	while (gps_list_head) {
		gps_array[nr++] = gps_list_head->gp;
		tmp = gps_list_head;
		gps_list_head =	 gps_list_head->next;
		free (tmp);
	}

///debug result:
	// for (i = 0; i<*gdata.gps_points_size; i+=100) {
	//	gps_point_raw* crt_point = &gps_array[i];
	//	mlt_log_info(NULL, "_xml_parse_file read point[%d]: time:%d, lat:%f, lon:%f, ele:%f, distance:%f, hr:%d\n",
	//			i, crt_point->time/1000, crt_point->lat, crt_point->lon, crt_point->ele, crt_point->total_dist, crt_point->hr);
	// }

cleanup:
	//note: this order is important or there's a crash
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	return rv;
}
