/*
 * filter_gpstext.c -- overlays GPS data as text on video
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

#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define MAX_TEXT_LEN 1024
#define GPS_UNINIT -9999
#define MAX_GPS_DIFF 10

typedef struct
{
	//always raw values from file (if present)
	double lat, lon, speed, total_dist;
	long long time; //time (converted) in seconds since epoch //TODO: milliseconds?
	//values from file + interpolated (.tcx has plenty of extra data to consider adding)
	double ele;
	short hr;
	//processed data, always changes depending on smoothing value
	char smooth_loc;
	char calc_bearing_c[3];
	double lat_smoothed, lon_smoothed;
	double calc_speed, calc_total_dist;
	int calc_bearing;
	double calc_d_elev, calc_elev_up, calc_elev_down, calc_dist_up, calc_dist_down, calc_dist_flat; //altitude differece, total vertical distance up/down, total horizontal distance up/down/flat
} gps_point;

//0 is a valid value for some, use GPS_UNINIT (-9999) to differentiate missing values
static const gps_point uninit_gps_point = {
	.lat=GPS_UNINIT, .lon=GPS_UNINIT, .speed=GPS_UNINIT, .total_dist = GPS_UNINIT,
	.time=GPS_UNINIT, .ele=GPS_UNINIT, .hr = GPS_UNINIT,
	.smooth_loc = 0, .calc_bearing_c = "-",
	.lat_smoothed=GPS_UNINIT, .lon_smoothed=GPS_UNINIT,
	.calc_speed = GPS_UNINIT, .calc_total_dist = GPS_UNINIT,
	.calc_bearing = GPS_UNINIT, .calc_d_elev = GPS_UNINIT, .calc_elev_up = GPS_UNINIT, .calc_elev_down = GPS_UNINIT,
	.calc_dist_up = GPS_UNINIT, .calc_dist_down = GPS_UNINIT, .calc_dist_flat = GPS_UNINIT
	};

#define has_valid_location(x) (((x).lat)!=GPS_UNINIT && ((x).lon)!=GPS_UNINIT)
#define has_valid_location_smoothed(x) (((x).lat_smoothed)!=GPS_UNINIT && ((x).lon_smoothed)!=GPS_UNINIT)
#define has_valid_location_ptr(x) ((x) && ((x)->lat)!=GPS_UNINIT && ((x)->lon)!=GPS_UNINIT)
#define has_valid_location_smoothed_ptr(x) ((x) && ((x)->lat_smoothed)!=GPS_UNINIT && ((x)->lon_smoothed)!=GPS_UNINIT)

#define MATH_PI 3.14159265358979323846
#define to_rad(x) ((x)*MATH_PI/180.0)
#define to_deg(x) ((x)*180.0/MATH_PI)

typedef struct
{
	gps_point* gps_points; //the array where gps data is stored
	int gps_points_size;
	int last_smooth_lvl; //last applied smoothing level
	int last_searched_index; //used to cache repeated searches
	time_t first_gps_time;
	time_t last_gps_time;
	char last_filename[256]; //gps file fullpath
	int video_file_timezone_s; //caches the video file's timezone offset (seconds, includes DST)
	time_t gps_proc_start_t; //process only points after this time (epoch seconds)
	double speed_multiplier;
} private_data;

/* Sets the private data to default values and frees gps points array
 */
static void cleanup_priv_data(private_data* pdata)
{
	if (pdata)
	{
		if (pdata->gps_points)
			free (pdata->gps_points);
		memset (pdata, 0 , sizeof(private_data));
		pdata->video_file_timezone_s = -1;
		pdata->speed_multiplier = 1;
	}
}

/** Get the next token and indicate whether it is enclosed in "# #".
*/
static int get_next_token(char* str, int* pos, char* token, int* is_keyword)
{
		int token_pos = 0;
		int str_len = strlen( str );

		if( (*pos) >= str_len || str[*pos] == '\0' )
		{
				return 0;
		}

		if( str[*pos] == '#' )
		{
				*is_keyword = 1;
				(*pos)++;
		}
		else
		{
				*is_keyword = 0;
		}

		while( *pos < str_len && token_pos < MAX_TEXT_LEN - 1)
		{
				if( str[*pos] == '\\' && str[(*pos) + 1] == '#' )
				{
						// Escape Sequence - "#" preceded by "\" - copy the # into the token.
						token[token_pos] = '#';
						token_pos++;
						(*pos)++; // skip "\"
						(*pos)++; // skip "#"
				}
				else if( str[*pos] == '#' )
				{
						if( *is_keyword )
						{
								// Found the end of the keyword
								(*pos)++;
						}
						break;
				}
				else
				{
						token[token_pos] = str[*pos];
						token_pos++;
						(*pos)++;
				}
		}

		token[token_pos] = '\0';

		return 1;
}


/* Converts the datetime string from gps file into seconds since epoch in local timezone
 * Note: assumes UTC
 */
static time_t datetimeXMLstring_to_seconds(const char* text, char* format)
{
	char def_format[] = "%Y-%m-%dT%H:%M:%S";
	//format: xsd:datetime [-]CCYY-MM-DDThh:mm:ss[Z|(+|-)hh:mm] , seconds can be float to store ms
	//samples: 2020-07-11T09:03:23.000Z or 2021-02-27T12:10:00+00:00 //Z=UTC timezone
	struct tm tm_time;
	tm_time.tm_isdst = -1; //force dst detection

	if (format == NULL)
		format = def_format;

	//TODO: add milisecond support
	if (strptime(text, format, &tm_time) == NULL) {
	//	mlt_log_warning(NULL, "filter_gpsText.c datetimeXMLstring_to_seconds strptime failed on string: %.25s", text);
		return 0;
	}

	/* NOTE: mktime assumes local time-zone for the tm_time but GPS time is UTC.
	 * time.h provides an extern: "timezone" which stores the local timezone in seconds
	 * this is used by mktime to "correct" the time so we must substract it to keep UTC
	 */
	return mktime(&tm_time) - (timezone-3600*tm_time.tm_isdst);
}

/* Converts seconds to a date-time with optional format,
 * Note: gmtime completely ignores timezones - this keeps time consistent
 *   as all times in this file are (hopefully) in UTC
 */
static void seconds_to_timestring (time_t seconds, char* format, char* result)
{
	struct tm * ptm = gmtime(&seconds);
	if (!format)
		strftime(result, 25, "%Y-%m-%d %H:%M:%S", ptm);
	else
		strftime(result, 50, format, ptm);
}

//returns the distance between 2 gps points, accurate enough (<0.1% error) for distances below 4km
static double distance_equirectangular_2p (double p1_lat, double p1_lon, double p2_lat, double p2_lon)
{
	const int R = 6371000;
	double x = (to_rad(p2_lon) - to_rad(p1_lon)) * cos((to_rad(p2_lat) + to_rad(p1_lat)) / 2.0);
	double y = to_rad(p1_lat) - to_rad(p2_lat);
	return sqrt(x*x + y*y) * R;
	//NOTE: adding altitude is very risky, GPS altitude is pretty bad and with large jumps //sqrt(2D_distance^2 + delta_alt^2)
	//TODO: consider adding distance_2d and distance_3d fields
}

/* Simple formula for fast realtime processing (<0.1% error on 5km scale)
 * -returns the arithmetic mean of latitudes, longitudes
 * Treats opposite points of 180*
 */
static void midpoint_simple_2p (double p1_lat, double p1_lon, double p2_lat, double p2_lon, double* p3_lat, double* p3_lon)
{
	//first check if we have valid locations, if one of them is invalid, return the other (GPS_UNINIT otherwise)
	if (p1_lat == GPS_UNINIT || p1_lon == GPS_UNINIT || p2_lat == GPS_UNINIT || p2_lon == GPS_UNINIT)
	{
		if (p1_lat != GPS_UNINIT && p1_lon != GPS_UNINIT) {
			*p3_lat = p1_lat;
			*p3_lon = p1_lon;
			return;
		}
		if (p2_lat != GPS_UNINIT && p2_lon != GPS_UNINIT) {
			*p3_lat = p2_lat;
			*p3_lon = p2_lon;
			return;
		}
		*p3_lat = *p3_lon = GPS_UNINIT;
		return;
	}

	//check if we are on opposite sides of 180*
	int opposite = 0;
	if (abs(p1_lon - p2_lon) > 180)
		opposite = 1;
	*p3_lat = (p1_lat + p2_lat)/2.0;
	*p3_lon = (p1_lon + p2_lon)/2.0;
	if (opposite)	//the midpoint will be in the exact opposite location so if result = 10->-170, -10->170
		*p3_lon = ((*p3_lon)<0 ? 1 : -1)*180 + (*p3_lon);
}

/* Computes bearing from 2 gps points
 */
static int bearing_2p (double p1_lat, double p1_lon, double p2_lat, double p2_lon)
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

/** Returns the nearest gps point in time, but not farther than MAX_GPS_DIFF (10 seconds)
 * or -1 if search fails
*/
static int binary_search_gps(mlt_filter filter, time_t video_time)
{
	private_data* pdata = (private_data*)filter->child;
	gps_point* gps_points = pdata->gps_points;
	const int gps_points_size = pdata->gps_points_size;
	int last_index = pdata->last_searched_index;

	int il = 0;
	int ir = gps_points_size-1;

	if (!gps_points || gps_points_size==0)
		return -1;

	//optimize repeated and normal playback match calls
	if (last_index < gps_points_size && gps_points[last_index].time == video_time)
		return last_index;

	if (++last_index < gps_points_size && gps_points[last_index].time == video_time) {
		pdata->last_searched_index = last_index;
		return last_index;
	}

	//optimize for outside values
	if (video_time < gps_points[0].time - MAX_GPS_DIFF || video_time > gps_points[gps_points_size-1].time + MAX_GPS_DIFF)
		return -1;

	//binary search
	while (il < ir)
	{
		int mid = (ir+il)/2;
		//mlt_log_info(filter, "binary_search_gps: mid=%d, l=%d, r=%d, mid-time=%d", mid, il, ir, gps_points[mid].time);
		if (gps_points[mid].time == video_time) {
			pdata->last_searched_index = mid;
			return mid;
		}
		if (gps_points[mid].time < video_time)
			il = mid+1;
		else
			ir = mid-1;
	}

	//don't return the closest gps point if time difference is too large:
	if (abs(video_time - gps_points[il].time) > MAX_GPS_DIFF) {
	//	mlt_log_info(filter, "binary_search_gps: TIME DIFF too large (>MAX_GPS_DIFFs) return -1: vid_time(synced)%d, gps_time:%d", video_time, gps_points[il].time);
		return -1;
	}
	pdata->last_searched_index = il;
	return il;
}

/* Writes the smoothed version of gpx lat/lon + time(local) to a .gpx file
 */
static void _DEBUG_write_smoothed_gps_to_file(mlt_filter filter, int smooth_lvl)
{
	private_data* pdata = (private_data*)filter->child;
	gps_point* gps_points = pdata->gps_points;
	const int gps_points_size = pdata->gps_points_size;

	int i;
	char intro_gpx[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><gpx version=\"1.0\" creator=\"shotcut:gpstextoverlay test export\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/0\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n"
						"<trk><trkseg>\n";
	char outro_gpx[] = "\n</trkseg></trk></gpx>";

	FILE* f = NULL;
	char fname[100];
	sprintf(fname, "smoothed_gpx_smooth_%d.gpx", smooth_lvl);
	f= fopen(fname, "w"); //created in home_dir by def
	if (!f) {
		mlt_log_warning(filter, "couldn't open file to write - %s", fname);
		return;
	}
	fputs(intro_gpx, f);
	for (i=0; i<gps_points_size; i++)
	{
		if (!has_valid_location_smoothed(gps_points[i]))
			continue;

		fprintf(f, "<trkpt lat=\"%f\" lon=\"%f\">", gps_points[i].lat_smoothed, gps_points[i].lon_smoothed);

		if (gps_points[i].ele !=GPS_UNINIT)
			fprintf(f, "<ele>%f</ele>", gps_points[i].ele);

		char time[50] = {0};
		seconds_to_timestring(gps_points[i].time, "%Y-%m-%dT%H:%M:%S.000Z", time);
		fprintf(f, "<time>%s</time>", time);

		if (gps_points[i].hr != GPS_UNINIT)
			fprintf(f, "\n\t<extensions> <gpxtpx:TrackPointExtension> <gpxtpx:hr>%d</gpxtpx:hr> </gpxtpx:TrackPointExtension> </extensions>", gps_points[i].hr);
		fprintf(f, "</trkpt>\n");
	}
	fputs(outro_gpx, f);
	fclose(f);
	mlt_log_info(filter, "done writing (%d points) smoothed file - %s", gps_points_size, fname);
}

/* Converts the bearing angle to a cardinal direction
 * 8 divisions
 */
static char* bearing_to_compass(int x)
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

/* Updates data in crt_point using lat/lon_smoothed;
 * Only updates fields with calc_ prefix
 */
static void recalculate_gps_data(mlt_filter filter, int req_smooth)
{
	//check to see if we must skip points at begining according to filter property
	private_data* pdata = (private_data*)filter->child;
	int offset = 0;
	if (pdata->gps_proc_start_t != 0) {
		if ((offset = binary_search_gps(filter, pdata->gps_proc_start_t)) == -1)
			offset = 0;
	}
	gps_point* gps_points = pdata->gps_points+offset;
	int gps_points_size = pdata->gps_points_size-offset;

	//mlt_log_info(filter, "recalculate_gps_data, offset=%d, points=%p, new:%p, size:%d, newSize:%d", offset,  pdata->gps_points, gps_points, pdata->gps_points_size, gps_points_size);

	int i = 0, ignore_points_before = 0;
	double total_dist = 0, total_d_elev = 0, total_elev_up = 0, total_elev_down = 0,
		   total_dist_up = 0, total_dist_down = 0, total_dist_flat = 0;
	gps_point *crt_point = NULL, *prev_point = NULL, *prev_nrsmooth_point = NULL;
	
	for (i=0; i<gps_points_size; i++)
	{
		crt_point = &gps_points[i];

		//this is practically the farthest valid point (behind current) that can be used for smoothing, limited by start of array or big gap in time
		int smooth_index = MAX(MAX(0, ignore_points_before), i-req_smooth);

		//ignore points with missing lat/lon, some devices use 0,0 for no fix so we ignore those too
		if (!has_valid_location_smoothed_ptr(crt_point) || (!crt_point->lat_smoothed && !crt_point->lon_smoothed))
			continue;

		//previous valid point must exist for math to happen
		if (prev_point == NULL) {
			prev_point = crt_point;

			//first local valid point, just set its distance to 0, other stuff can't be calculated
			crt_point->calc_total_dist = total_dist;
			continue;
		}

		//find a valid point from i-req_smooth to i (to use in smoothing calc)
		while (smooth_index<i && !has_valid_location(gps_points[smooth_index])) smooth_index++;
		if (smooth_index < i)
			prev_nrsmooth_point = &gps_points[smooth_index];

		//1) get distance difference between last 2 valid points
		double d_dist = distance_equirectangular_2p (prev_point->lat_smoothed, prev_point->lon_smoothed, crt_point->lat_smoothed, crt_point->lon_smoothed);

		//if time difference is way longer for one point than usual, don't do math on that gap (treat recording paused, move far, then resume)
		//5*average time or 10s
		if (crt_point->time - prev_point->time > MAX(MAX_GPS_DIFF, 5.0*(pdata->last_gps_time-pdata->first_gps_time)/pdata->gps_points_size))
		{
			prev_nrsmooth_point = NULL;
			ignore_points_before = i;
			crt_point->calc_total_dist = total_dist;
			prev_point = crt_point;
			continue;
		}

		//this is the total distance since beginning of gps processing time
		total_dist += d_dist;
		crt_point->calc_total_dist = total_dist;

		//2)+3) speed using delta_distance/time + bearing
		if (req_smooth < 2)
		{
			crt_point->calc_speed = d_dist / (crt_point->time - prev_point->time); //in m/s
			crt_point->calc_bearing = bearing_2p(prev_point->lat_smoothed, prev_point->lon_smoothed, crt_point->lat_smoothed, crt_point->lon_smoothed);
		}
		else //for "smoothing" we calculate distance between 2 points "nr_smoothing" distance behind
		{
			if (prev_nrsmooth_point) {
				double d_dist = crt_point->calc_total_dist - prev_nrsmooth_point->calc_total_dist;
				long long d_time = crt_point->time - prev_nrsmooth_point->time;
				crt_point->calc_speed = d_dist/d_time;

				crt_point->calc_bearing = bearing_2p(prev_nrsmooth_point->lat_smoothed, prev_nrsmooth_point->lon_smoothed, crt_point->lat_smoothed, crt_point->lon_smoothed);
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

			crt_point->calc_d_elev = total_d_elev;
			crt_point->calc_elev_up = total_elev_up;
			crt_point->calc_elev_down = total_elev_down;
			crt_point->calc_dist_up = total_dist_up;
			crt_point->calc_dist_down = total_dist_down;
			crt_point->calc_dist_flat = total_dist_flat;
		}

		prev_point = crt_point;
	}
	//_DEBUG_write_smoothed_gps_to_file(filter, req_smooth);
}


/** Process a .tcx gps location file, result is put in gps_points[],
 * ignores points without strictly increasing time (or 0)
*/
static void process_tcx(mlt_filter filter, char* file_content, int bytes_read)
{
	private_data* pdata = (private_data*)filter->child;

	// sample point from .TCX file, only lat and lon are mandatory but we also need time for syncing
	/*
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
	gps_point crt_point;
	char* ptext = file_content;
	const char* last_address = file_content+bytes_read;
	int i;
	
//	mlt_log_info(filter, "entered process_tcx, ptext=%p, last_address=%p", ptext, last_address);
//	mlt_log_info(filter, "entered process_tcx, ptext=%.100s", ptext);
	
	//ptext will point to the current location in the string where we're processing stuff
	//we will first look for the start of an entry (<Trackpoint>) then try to parse all tags <> </> until the end tag </Trackpoint>
		
	//before actual processing takes place, we need to know how many points there are in the file so we can allocate exact gps_points
	int nr_points = 0, crt_nr_point = 0, skipped_points = 0;
	while (ptext < last_address)
	{
		if (strncmp(ptext, "<Trackpoint>", 12)==0)
			nr_points++;
		ptext++;
	}
	//mlt_log_info(filter, "found %d gps points in file", nr_points);
	
	if (pdata->gps_points)
		free(pdata->gps_points);
	pdata->gps_points = malloc(sizeof(gps_point) * nr_points);
	if (!pdata->gps_points)
		return;
	pdata->gps_points_size = nr_points;
	
	//move pointer back to beginning
	ptext = file_content;
	
	//find the start of <Trackpoint> field and jump to it
	while (ptext<last_address && (ptext = strstr(ptext, "<Trackpoint>")) != NULL)
	{
		crt_point = uninit_gps_point;
		
		//iterate through the entire Trackpoint tag and find sub-tags
		for (ptext += strlen("<Trackpoint>"); strncmp(ptext, "</Trackpoint>", 13) && ptext < last_address; ptext++)
		{
			if (strncmp(ptext, "<Time>", 6) == 0)
			{
				//skip tag
				ptext+=6;
				//format: YYYY-MM-DDThh:mm:ss (+timezone optional?)
				if (ptext+19 >= last_address) {
					mlt_log_warning(filter, "<time> tag can't fit time, reached EOF");
					continue;
				}
				crt_point.time = datetimeXMLstring_to_seconds(ptext, NULL);
				ptext+=19;
				//skip closing </Time (intentional -1)
				ptext+=6;
			}
			else if (strncmp(ptext, "<Position>", 10) == 0)
			{
				ptext+=10;
				while (ptext<last_address && strncmp(ptext, "</Position>", 11))
				{
					if (strncmp(ptext, "<LatitudeDegrees>", 17) == 0)
					{
						ptext+=17;
						crt_point.lat = crt_point.lat_smoothed = strtod(ptext, &ptext);
		//				mlt_log_info(filter, "found gps.lat: %f", crt_point.lat);
						ptext+=17;
					}
					else if (strncmp(ptext, "<LongitudeDegrees>", 18) == 0)
					{
						ptext+=18;
						crt_point.lon = crt_point.lon_smoothed = strtod(ptext, &ptext);
		//				mlt_log_info(filter, "found gps.lon: %f", crt_point.lon);
						ptext+=18;
					}
					ptext++;
				}
				//skip closing </Position
				ptext+=10;
			}
			else if (strncmp(ptext, "<AltitudeMeters>", 16) == 0)
			{
				ptext+=16;
				crt_point.ele = strtod(ptext, &ptext);
		//		mlt_log_info(filter, "found gps.ele: %f", crt_point.ele);
				ptext+=16;
			}
			else if (strncmp(ptext, "<DistanceMeters>", 16) == 0)
			{
				ptext+=16;
				crt_point.total_dist = strtod(ptext, &ptext);
		//		mlt_log_info(filter, "found gps.total_dist: %f", crt_point.total_dist);
				ptext+=16;
			}
			else if (strncmp(ptext, "<HeartRateBpm", 13) == 0)
			{
				for (ptext += strlen("<HeartRateBpm"); strncmp(ptext, "</HeartRateBpm>", 15) && ptext < last_address; ptext++)
				{
					if (strncmp(ptext, "<Value>", 7) == 0)
					{
						ptext+=7;
						crt_point.hr = (short)strtod(ptext, &ptext);
				//		mlt_log_info(filter, "found gps.HR: %d", crt_point.hr);
						ptext+=7;
					}
				}
				//skip closing </HeartRateBpm
				ptext+=14;
			}
			//there are optional speed tags in schema but I don't have any example
			/*
			else if (strncmp(ptext, "<??speed??>", ??) == 0)
			{
				//skip tag
				ptext+=?;
				//convert string to float
				crt_point.speed = strtod(ptext, &ptext);
		//		mlt_log_info(filter, "found gps.speed: %f", crt_point.speed);
				//skip closing
				ptext+=?;
			}
			*/
		}
		
		//ignore points with missing or non increasing time
		if (crt_point.time == GPS_UNINIT || crt_point.time == 0 || (crt_nr_point-1>0 && crt_point.time<=pdata->gps_points[crt_nr_point-1].time)){
			//mlt_log_info(filter, "dropping GPS point[%d] with invalid time: crt.time: %d, prev.time: %d",crt_nr_point, crt_point.time, pdata->gps_points[MAX(crt_nr_point-1, 0)].time);
			skipped_points++;
			continue;
		}

		//add crt_point to array
		pdata->gps_points[crt_nr_point] = crt_point; //implicit copy ok
		crt_nr_point++;
		ptext++; //note, there is at least 1 char left in closing tags above to make sure we don't enter in the next tag here
	}
	
	//resize down array to only valid points:
	if (skipped_points) {
	//	mlt_log_info(filter, "realloc gps_points: (prev_size=%d, new_size=%d", nr_points, crt_nr_point);
		pdata->gps_points_size = crt_nr_point;
		pdata->gps_points = realloc(pdata->gps_points, crt_nr_point*sizeof(gps_point));
		if (pdata->gps_points == NULL) {
			mlt_log_error(filter, "memory alloc failed, BYE. realloc(size=%d)", crt_nr_point*sizeof(gps_point));
			pdata->gps_points_size = 0;
			return;
		}
	}

	//debug
	/*
	for (i=0; i<5 && i<pdata->gps_points_size; i++)
		mlt_log_info(filter, "process_tcx gps_points[%d]: lat:%f lon:%f time:%d ",
			i, pdata->gps_points[i].lat, pdata->gps_points[i].lon, pdata->gps_points[i].time);
	for (i=pdata->gps_points_size/2; i<pdata->gps_points_size/2+5 && i<pdata->gps_points_size; i++)
		mlt_log_info(filter, "process_tcx gps_points[%d]: lat:%f lon:%f time:%d ",
			i, pdata->gps_points[i].lat, pdata->gps_points[i].lon, pdata->gps_points[i].time);
	for (i=MAX(pdata->gps_points_size-5,0); i<pdata->gps_points_size; i++)
		mlt_log_info(filter, "process_tcx gps_points[%d]: lat:%f lon:%f time:%d ",
			i, pdata->gps_points[i].lat, pdata->gps_points[i].lon, pdata->gps_points[i].time);
	*/
}

/** Process a .gpx gps location file, result is put in gps_points[],
 * ignores points without strictly increasing time (or 0)
*/
static void process_gpx(mlt_filter filter, const char* file_content, int bytes_read)
{
	private_data* pdata = (private_data*)filter->child;

	// sample point from .GPX file (version 1.0), only lat and lon are mandatory but we also need time for calculations
	/*
	<trkpt lat="45.227621666484104" lon="25.16533185322889">
		<ele>868.3005248873908</ele>
		<time>2020-07-11T09:03:23.000Z</time>
		<speed>0.0</speed>
		<geoidheight>36.1</geoidheight>
		<src>gps</src>
		<hdop>0.8</hdop>
		<vdop>1.3</vdop>
		<pdop>1.6</pdop>
	</trkpt>
	*/
	gps_point crt_point;
	char* ptext = file_content;
	const char* last_address = file_content+bytes_read;
	int i;
	
	//mlt_log_info(filter, "entered process_gpx, ptext=%p, last_address=%p", ptext, last_address);
	//mlt_log_info(filter, "entered process_gpx, ptext=%.100s", ptext);
	
	//ptext will point to the current location in the string where we're processing stuff
	//we will first look for the start of an entry (<trkpt ) then try to parse all tags <> </> until the end tag </trkpt>
		
	//before actual processing takes place, we need to know how many points there are in the file so we can allocate exact gps_points
	int nr_points = 0, crt_nr_point = 0, skipped_points = 0;
	while (ptext < last_address)
	{
		if (strncmp(ptext, "<trkpt", 6)==0)
			nr_points++;
		ptext++;
	}
	
	if (pdata->gps_points)
		free(pdata->gps_points);
	pdata->gps_points = (gps_point*) malloc(sizeof(gps_point) * nr_points);
	if (!pdata->gps_points) {
		mlt_log_error(filter, "can't malloc, BYE! (size=%d)", sizeof(gps_point)*nr_points);
		return;
	}
	pdata->gps_points_size = nr_points;
	
	//move pointer back to begining
	ptext = file_content;
	//find the start of <trkpt field and jump to it
	while (ptext<last_address && (ptext = strstr(ptext, "<trkpt ")) != NULL)
	{
		crt_point = uninit_gps_point;
		//iterate through the entire trkpt tag
		for (ptext += strlen("<trkpt "); strncmp(ptext, "</trkpt>", 8) && ptext < last_address; ptext++)
		{
			//find lat and lon (mandatory inside <trkpt ... >)
			if (strncmp(ptext, "lat=", 4) == 0)
			{
				//skip lat="
				ptext+=5;
				crt_point.lat = crt_point.lat_smoothed = strtod(ptext, &ptext);
				//mlt_log_info(filter, "found gps.lat: %f", crt_point.lat);
				//skip closing "
				ptext++;
			}
			else if (strncmp(ptext, "lon=", 4) == 0)
			{
				ptext+=5;
				crt_point.lon = crt_point.lon_smoothed = strtod(ptext, &ptext);
				//mlt_log_info(filter, "found gps.lon: %f", crt_point.lon);
				ptext++;
			}
			//find one of the optional tags
			else if (*ptext == '<')
			{
				if (strncmp(ptext, "<ele>", 5) == 0)
				{
					ptext+=5;
					crt_point.ele = strtod(ptext, &ptext);
				//	mlt_log_info(filter, "found gps.ele: %f", crt_point.ele);
					//skip closing </ele  //(intentionally left the '>' out)
					ptext+=5;
				}
				else if (strncmp(ptext, "<time>", 6) == 0)
				{
					ptext+=6;
					if (ptext+24 > last_address) {
						mlt_log_warning(filter, "<time> tag can't fit time, reached EOF");
						continue;
					}
					crt_point.time = datetimeXMLstring_to_seconds(ptext, NULL);
					//	mlt_log_info(filter, "found gps.time: %.24s -> %d", ptext, crt_point.time);
					ptext+=24+6;
				}
				else if (strncmp(ptext, "<speed>", 7) == 0)
				{
					ptext+=7;
					crt_point.speed = strtod(ptext, &ptext);
			//		mlt_log_info(filter, "found gps.speed: %f", crt_point.speed);
					ptext+=7;
				}
				else if (strncmp(ptext, "<extensions>", 12) == 0)
				{
					//support for garmin's gpx extension, heart rate
					/* sample:
					   <trkpt lat="44.4666450" lon="26.0680890">
						<ele>86.7</ele>
						<time>2021-02-27T14:38:49Z</time>
						<extensions>
						 <gpxtpx:TrackPointExtension>
						  <gpxtpx:hr>132</gpxtpx:hr>
						 </gpxtpx:TrackPointExtension>
						</extensions>
					   </trkpt>
					*/
					for (ptext += strlen("<extensions>"); strncmp(ptext, "</extensions>", 13) && ptext < last_address; ptext++)
					{
						if (strncmp(ptext, "<gpxtpx:hr>", 11) == 0)
						{
							ptext+=11;
							crt_point.hr = (short)strtod(ptext, &ptext);
				//			mlt_log_info(filter, "found gps.HR: %d", crt_point.hr);
							ptext+=11;
						}
					}
				}
				//ignore tags we don't use
			}
		}
		
		//ignore points with missing or non increasing time!
		if (crt_point.time == GPS_UNINIT || crt_point.time == 0 || (crt_nr_point-1>0 && crt_point.time<=pdata->gps_points[crt_nr_point-1].time)){
			mlt_log_warning(filter, "dropping GPS point with invalid time: crt.time: %d, prev.time: %d", crt_point.time, pdata->gps_points[crt_nr_point-1].time);
			skipped_points++;
			continue;
		}
		
		//add crt_point to our cached locations
		pdata->gps_points[crt_nr_point] = crt_point; //implicit copy ok
		crt_nr_point++;
		ptext++; //note, there is at least 1 char left in closing tags above to make sure we don't enter in the next tag (which would be skipped by strstr) but also treat empty trkpt tag
	}
	
	//resize down array to only valid points:
	if (skipped_points) {
		mlt_log_info(filter, "realloc gps_points: (prev_size=%d, new_size=%d", nr_points, crt_nr_point);
		pdata->gps_points_size = crt_nr_point;
		pdata->gps_points = realloc(pdata->gps_points, crt_nr_point*sizeof(gps_point));
		if (pdata->gps_points == NULL) {
			mlt_log_error(filter, "memory alloc failed, BYE - realloc(size=%d)", crt_nr_point*sizeof(gps_point));
			cleanup_priv_data(pdata);
			return;
		}
	}

	//debug
	/*
	for (i=0; i<MIN(5, pdata->gps_points_size); i++)
		mlt_log_info(filter, "process_gpx gps_points[%d]: lat:%f lon:%f time:%d ",
			i, pdata->gps_points[i].lat, pdata->gps_points[i].lon, pdata->gps_points[i].time);
	for (i=MAX(pdata->gps_points_size-5,0); i<pdata->gps_points_size; i++)
		mlt_log_info(filter, "process_gpx gps_points[%d]: lat:%f lon:%f time:%d ",
			i, pdata->gps_points[i].lat, pdata->gps_points[i].lon, pdata->gps_points[i].time);
	 */
}

/** Searches and returns the first valid time in the gps_points array
 *  returns 0 on error
*/
static time_t get_first_gps_time(mlt_filter filter)
{
	private_data* pdata = (private_data*)filter->child;
	gps_point* gps_points = pdata->gps_points;

	if (pdata->first_gps_time)
		return pdata->first_gps_time;

	if (!gps_points)
		return (time_t)0;

	int i = 0;
	while (i<pdata->gps_points_size)
	{
		if (gps_points[i].time && has_valid_location(gps_points[i]))
		{
			//mlt_log_info(filter, "get_first_gps_time found: %d", gps_points[i].time);
			pdata->first_gps_time = gps_points[i].time;
			return pdata->first_gps_time;
		}
		i++;
	}
	return 0;
}


/** Searches and returns the last valid time in the gps_points array
 *  returns 0 on error
*/
static time_t get_last_gps_time(mlt_filter filter)
{
	private_data* pdata = (private_data*)filter->child;
	gps_point* gps_points = pdata->gps_points;
	//mlt_log_info(filter, "get_last_gps_time: pdata->gps address: %p, gps_pts=%p", pdata->gps_points, gps_points);

	if (pdata->last_gps_time)
		return pdata->last_gps_time;

	if (!gps_points)
		return (time_t)0;

	int i = pdata->gps_points_size-1;
	while (i>=0)
	{
		if (gps_points[i].time && has_valid_location(gps_points[i]))
		{
			//mlt_log_info(filter, "get_last_gps_time found: %d", gps_points[i].time);
			pdata->last_gps_time = gps_points[i].time;
			return pdata->last_gps_time;
		}
		i--;
	}
	return 0;
}


/* Processes the entire gps_points[] array to fill the lat_smoothed, lon_smoothed values
 * Also does linear interpolation of HR, altitude (+lat/lon*) if necessary to fill missing values
 * After this, calls recalculate_gps_data to update distance, speed + other fields for all points
 */
static void process_gps_smoothing(mlt_filter filter, int req_smooth)
{
	//check to see if we must skip points at begining according to filter property
	private_data* pdata = (private_data*)filter->child;
	int offset = 0;
	if (pdata->gps_proc_start_t != 0) {
		offset = binary_search_gps(filter, pdata->gps_proc_start_t);
		if (offset == -1)
			offset = 0;
		//allow a few extra points for smoothing so the algorithm works better
		if (offset- MAX(req_smooth/2, MAX_GPS_DIFF) > 0)
			offset -= MAX(req_smooth/2, MAX_GPS_DIFF);
		else
			offset = 0;
	}
	gps_point* gps_points = pdata->gps_points+offset;
	int gps_points_size = pdata->gps_points_size-offset;

	//mlt_log_info(filter, "process_gps_smoothing, offset=%d, points=%p, new:%p, size:%d, newSize:%d", offset,  pdata->gps_points, gps_points, pdata->gps_points_size, gps_points_size);

	char interpolated = 0;
	int i, j;
	int hr = GPS_UNINIT, nr_hr = 0;
	int ele = GPS_UNINIT, nr_ele = 0;
	double lat_sum = 0, lon_sum = 0;
	int nr_div = 0;

	for (i=0; i<gps_points_size; i++)
	{
		//linear interpolation for heart rate and elevation
		//NOTE: this is irreversible, there's no reason to want missing values back so it's not worth the extra memory and logic
		if (interpolated == 0 && req_smooth != 0)
		{
			//heart rate
			if (gps_points[i].hr != GPS_UNINIT) { //found valid hr
				if (hr != GPS_UNINIT && nr_hr>0) { //there were missing values and had a hr before
					for (j=i-1; j>=i-nr_hr; j--) { //go backwards and fill values
						gps_points[j].hr = hr + 1.0*(gps_points[i].hr - hr) * (1.0*(j-(i-nr_hr))/nr_hr);
					}
				}
				hr = gps_points[i].hr;
				nr_hr = 0;
			}
			else
				nr_hr++;

			//altitude
			if (gps_points[i].ele != GPS_UNINIT) { //found valid ele
				if (ele != GPS_UNINIT && nr_ele>0) { //there were missing values and had an ele before
					for (j=i-1; j>=i-nr_ele; j--) { //go backwards and fill values
						gps_points[j].ele = ele + 1.0*(gps_points[i].ele - ele) * (1.0*(j-(i-nr_ele))/nr_ele);
					}
				}
				ele = gps_points[i].ele;
				nr_ele = 0;
			}
			else
				nr_ele++;
		}


		if (req_smooth == 0) //clear all calculated data
		{
			gps_points[i].smooth_loc = 0;
			strcpy (gps_points[i].calc_bearing_c, "-");
			gps_points[i].lat_smoothed = GPS_UNINIT;
			gps_points[i].lon_smoothed = GPS_UNINIT;
			gps_points[i].calc_speed = GPS_UNINIT;
			gps_points[i].calc_total_dist = GPS_UNINIT;
			gps_points[i].calc_bearing = GPS_UNINIT;
			gps_points[i].calc_d_elev = GPS_UNINIT;
			gps_points[i].calc_elev_down = GPS_UNINIT;
			gps_points[i].calc_elev_up = GPS_UNINIT;
			gps_points[i].calc_dist_down = GPS_UNINIT;
			gps_points[i].calc_dist_up = GPS_UNINIT;
			gps_points[i].calc_dist_flat = GPS_UNINIT;
		}
		else if (req_smooth == 1) //copy lat/lon to lat/lon_smoothed
		{
			gps_points[i].smooth_loc = 1;
			gps_points[i].lat_smoothed = gps_points[i].lat;
			gps_points[i].lon_smoothed = gps_points[i].lon;

			//if current point has no lat/lon but nearby ones do AND time difference is lower than MAX_GPS_DIFF, interpolate 1 location
			//(this can happen if location is stored every 3s but altitude stored only once every 10s)
			if (!has_valid_location_smoothed(gps_points[i])
					&& i-1 >= 0 && i+1 < gps_points_size
					&& abs(gps_points[i+1].time - gps_points[i-1].time) < MAX_GPS_DIFF)
			{
				//TODO: this should be a weighted "middle" point depending on time difference, not exactly midpoint if time is not halfway
				midpoint_simple_2p(gps_points[i-1].lat, gps_points[i-1].lon, gps_points[i+1].lat, gps_points[i+1].lon, &gps_points[i].lat_smoothed, &gps_points[i].lon_smoothed);
				//mlt_log_info(filter, "interpolating position for smooth=1, new point[%d]:%f,%f @time=%d", i, gps_points[i].lat_smoothed, gps_points[i].lon_smoothed, gps_points[i].time);
			}
		}
		else if (req_smooth > 1) //average X nearby values into current index
		{
			if (i == 0) { //first point is special
				for (j=0; j<=req_smooth/2; j++) {
					if (has_valid_location(gps_points[j])) {
						lat_sum += gps_points[j].lat;
						lon_sum += gps_points[j].lon;
						nr_div++;
					}
				}
			}
			else { //add the next one, substract the one behind our range
				//TODO: somehow treat case of points on opposite sides of 180*
				int next = i+req_smooth/2;
				int prev = i-req_smooth/2-1;
				if (next < gps_points_size && has_valid_location(gps_points[next])) {
					nr_div++;
					lat_sum += gps_points[next].lat;
					lon_sum += gps_points[next].lon;
				}
				if (prev >= 0 && has_valid_location(gps_points[prev])) {
					nr_div--;
					lat_sum -= gps_points[prev].lat;
					lon_sum -= gps_points[prev].lon;
				}
			}
			if (nr_div!=0) {
				gps_points[i].lat_smoothed = lat_sum/nr_div;
				gps_points[i].lon_smoothed = lon_sum/nr_div;
			} else {
				gps_points[i].lat_smoothed = gps_points[i].lat;
				gps_points[i].lon_smoothed = gps_points[i].lon;
			}
			//mlt_log_info(filter, "i=%d, lat_sum=%f, lon_sum=%f, nr_div=%d, time=%d", i, lat_sum, lon_sum, nr_div, gps_points[i].time);
		}
	}
	if (req_smooth != 0)
	{
		interpolated = 1;
		//re-process the distance,speed and other fields:
		recalculate_gps_data(filter, req_smooth);
	}
}

/** Tries to read the provided file and calls the parser depending on extension;
*/
static void process_gps_file (mlt_filter filter, mlt_frame frame, const char* location_filename)
{
	FILE *f = NULL;
	int file_length = 0;
	
	//mlt_log_info(filter, "process_gps_file gps_file string: %s", location_filename==NULL?"null":location_filename);
	
	f = fopen( location_filename, "r" );
	if (!f)
		mlt_log_warning(filter, "couldn't open file to read - %s", location_filename);
	if (fseek(f, 0, SEEK_END)) {
		mlt_log_warning(filter, "couldn't seek to end of file - %s", location_filename);
		fclose(f);
		return;
	}
	//get filesize
	if ((file_length = ftell(f)) == -1) {
		mlt_log_warning(filter, "couldn't get size of file - %s", location_filename);
		fclose(f);
		return;
	}
	//move cursor back at the start of file
	if (fseek(f, 0, SEEK_SET)) {
		mlt_log_warning(filter, "couldn't seek to end of file - %s", location_filename);
		fclose(f);
		return;
	}
	//allocate memory and store the entire file in ram
	char* file_content = (char*) malloc(file_length+1);
	if (!file_content) {
		mlt_log_error(NULL, "couldn't allocate buffer for filesize (%d) - %s", file_length, location_filename);
		fclose(f);
		return;
	}
	
	int bytes_read = fread(file_content, 1, file_length, f);
	fclose(f);
	file_content[bytes_read] = '\0';
	
	//mlt_log_info(filter, "process_gps_file bytes_read %d; &file_content=%p, starts:%.100s", bytes_read, file_content, file_content);
	
	if (bytes_read != file_length) {
		mlt_log_warning(NULL, "read (%d) instead of expected bytes (%d) - %s", bytes_read, file_length, location_filename);
		free(file_content);
		return;
	}
	
	//parse the file depending on its extension
	char ext[5];
	strncpy (ext, &location_filename[strlen(location_filename)-4], 4);
	ext[4] = '\0';
	for (int i=0; i<4; i++) 	ext[i] = tolower(ext[i]);
	
	if (!strncmp(ext, ".gpx", 4)) {
		process_gpx(filter, file_content, bytes_read);
	}
	else if (!strncmp(ext, ".tcx", 4)) {
		process_tcx(filter, file_content, bytes_read);
		
	}
	else {
		mlt_log_warning(NULL, "unsupported file type: %s - %s", ext, location_filename);
		free(file_content);
		return;
	}
	
	//get first/last gps time and update first gps time property
	time_t first_datetime_sec = get_first_gps_time(filter);
	get_last_gps_time(filter);
	char first_datetime[25];
	if (first_datetime_sec)
		seconds_to_timestring(first_datetime_sec, NULL, first_datetime);
	else
		strcpy (first_datetime, "--");
	mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "gps_start_text", first_datetime);
	//mlt_log_info(filter, "setting gps_start_text to %s", first_datetime);

	free(file_content);
}

/** Converts the distance (stored in meters) to the unit requested in extended keyword
*/
static double convert_distance_to_format(double x, const char* format)
{
	if (format == NULL)
		return x;
	//mlt_log_info(NULL, "filter_gps.c convert_distance_to_format, format=%s len=%d", format, strlen(format));
	
	//x is in meters
	if (strstr(format, "km") || strstr(format, "kilometer"))
		return x/1000.0;
	else if (strstr(format, "mi") || strstr(format, "mile"))
		return x*0.00062137;
	else if (strstr(format, "ft") || strstr(format, "feet"))
		return x*3.2808399;
	return x;
}

/** Converts the speed (stored in meters/second) to the unit requested in extended keyword
*/
static double convert_speed_to_format(double x, const char* format)
{
	if (format == NULL)
		return x*3.6;	//default km/h
	//mlt_log_info(NULL, "filter_gps.c convert_speed_to_format, format=%s len=%d", format, strlen(format));
	
	//x is in meters/sec
	if (strstr(format, "ms") || strstr(format, "m/s") || strstr(format, "meter"))
		return x;
	else if (strstr(format, "mi") || strstr(format, "mi/h") || strstr(format, "mile"))
		return x*2.23693629;
	else if (strstr(format, "ft") || strstr(format, "ft/s") || strstr(format, "feet"))
		return x*3.2808399;
	
	return x*3.6;
}

/** Returns the number of decimal values for floats
*/
static inline int decimals_needed(double x) {
	if (x<10) return 2;
	if (x<100) return 1;
	return 0;
}

/** Does the actual replacement of keyword with valid data, also parses any keyword extra format
*/
static void gps_point_to_output(gps_point crt_point, char* keyword, time_t video_time_original, char* gps_text)
{
	if ( !strncmp( keyword, "gps_lat_r", strlen("gps_lat_r") ) && crt_point.lat != GPS_UNINIT )
	{
		snprintf(gps_text, 10, "%3.6f", crt_point.lat);
	}
	else if ( !strncmp( keyword, "gps_lon_r", strlen("gps_lon_r") ) && crt_point.lon != GPS_UNINIT )
	{
		snprintf(gps_text, 10, "%3.6f", crt_point.lon);
	}
	if ( !strncmp( keyword, "gps_lat_c", strlen("gps_lat_c") ) && crt_point.lat_smoothed != GPS_UNINIT )
	{
		snprintf(gps_text, 10, "%3.6f", crt_point.lat_smoothed);
	}
	else if ( !strncmp( keyword, "gps_lon_c", strlen("gps_lon_c") ) && crt_point.lon_smoothed != GPS_UNINIT )
	{
		snprintf(gps_text, 10, "%3.6f", crt_point.lon_smoothed);
	}
	else if ( !strncmp( keyword, "gps_elev", strlen("gps_elev") ) && crt_point.ele != GPS_UNINIT )
	{
		//support format conversion
		char* format = NULL;
		if (strlen(keyword) > strlen("gps_elev"))
			format = keyword + strlen("gps_elev");
		double val = convert_distance_to_format(crt_point.ele, format);
		snprintf(gps_text, 10, "%.*f", decimals_needed(val), val);
	}
	else if ( !strncmp( keyword, "gps_speed_r", strlen("gps_speed_r") ) && crt_point.speed != GPS_UNINIT )
	{
		char* format = NULL;
		if (strlen(keyword) > strlen("gps_speed_r"))
			format = keyword + strlen("gps_speed_r");
		double val = convert_speed_to_format(crt_point.speed, format);
		snprintf(gps_text, 10, "%.*f", decimals_needed(val), val);
	}
	else if ( !strncmp( keyword, "gps_speed_c", strlen("gps_speed_c") ) && crt_point.calc_speed != GPS_UNINIT )
	{
		char* format = NULL;
		if (strlen(keyword) > strlen("gps_speed_c"))
			format = keyword + strlen("gps_speed_c");
		double val = convert_speed_to_format(crt_point.calc_speed, format);
		snprintf(gps_text, 10, "%.*f", decimals_needed(val), val);
	}
	else if ( !strncmp( keyword, "gps_hr", strlen("gps_hr") ) && crt_point.hr != GPS_UNINIT )
	{
		snprintf(gps_text, 10, "%d", crt_point.hr);
	}
	else if ( !strncmp( keyword, "gps_bearing", strlen("gps_bearing") ) && crt_point.calc_bearing != GPS_UNINIT )
	{
		snprintf(gps_text, 10, "%d", crt_point.calc_bearing);
	}
	else if ( !strncmp( keyword, "gps_compass", strlen("gps_compass") ) && crt_point.calc_bearing != GPS_UNINIT )
	{
		snprintf(gps_text, 4, "%s", bearing_to_compass(crt_point.calc_bearing));
	}
	else if ( !strncmp( keyword, "gps_dist_r", strlen("gps_dist_r") ) && crt_point.total_dist != GPS_UNINIT )
	{
		char* format = NULL;
		if (strlen(keyword) > strlen("gps_dist_r"))
			format = keyword + strlen("gps_dist_r");
		double val = convert_distance_to_format(crt_point.total_dist, format);
		snprintf(gps_text, 10, "%.*f", decimals_needed(val), val);
	}
	else if ( !strncmp( keyword, "gps_dist_c", strlen("gps_dist_c") ) && crt_point.calc_total_dist != GPS_UNINIT )
	{
		char* format = NULL;
		if (strlen(keyword) > strlen("gps_dist_c"))
			format = keyword + strlen("gps_dist_c");
		double val = convert_distance_to_format(crt_point.calc_total_dist, format);
		snprintf(gps_text, 10, "%.*f", decimals_needed(val), val);
	}
//	else if ( !strncmp( keyword, "gps_vdist_total", 15 ) && crt_point.calc_d_elev != GPS_UNINIT )
//	{
//		//support format conversion
//		char* format = NULL;
//		if (strlen(keyword) > strlen("gps_vdist_total"))
//			format = keyword + strlen("gps_vdist_total");
//		snprintf(gps_text, 10, "%.2f", convert_distance_to_format(crt_point.calc_d_elev, format));
//	}
	else if ( !strncmp( keyword, "gps_vdist_up", strlen("gps_vdist_up") ) && crt_point.calc_elev_up != GPS_UNINIT )
	{
		char* format = NULL;
		if (strlen(keyword) > strlen("gps_vdist_up"))
			format = keyword + strlen("gps_vdist_up");
		double val = convert_distance_to_format(abs(crt_point.calc_elev_up), format);
		snprintf(gps_text, 10, "%.*f", decimals_needed(val), val);
	}
	else if ( !strncmp( keyword, "gps_vdist_down", strlen("gps_vdist_down") ) && crt_point.calc_elev_down != GPS_UNINIT )
	{
		char* format = NULL;
		if (strlen(keyword) > strlen("gps_vdist_down"))
			format = keyword + strlen("gps_vdist_down");
		double val = convert_distance_to_format(abs(crt_point.calc_elev_down), format);
		snprintf(gps_text, 10, "%.*f", decimals_needed(val), val);
	}
	else if ( !strncmp( keyword, "gps_dist_uphill", strlen("gps_dist_uphill") ) && crt_point.calc_dist_up != GPS_UNINIT )
	{
		char* format = NULL;
		if (strlen(keyword) > strlen("gps_dist_uphill"))
			format = keyword + strlen("gps_dist_uphill");
		double val = convert_distance_to_format(crt_point.calc_dist_up, format);
		snprintf(gps_text, 10, "%.*f", decimals_needed(val), val);
	}
	else if ( !strncmp( keyword, "gps_dist_downhill", strlen("gps_dist_downhill") ) && crt_point.calc_dist_down != GPS_UNINIT )
	{
		char* format = NULL;
		if (strlen(keyword) > strlen("gps_dist_downhill"))
			format = keyword + strlen("gps_dist_downhill");
		double val = convert_distance_to_format(crt_point.calc_dist_down, format);
		snprintf(gps_text, 10, "%.*f", decimals_needed(val), val);
	}
	else if ( !strncmp( keyword, "gps_dist_flat", strlen("gps_dist_flat") ) && crt_point.calc_dist_flat != GPS_UNINIT )
	{
		char* format = NULL;
		if (strlen(keyword) > strlen("gps_dist_flat"))
			format = keyword + strlen("gps_dist_flat");
		double val = convert_distance_to_format(crt_point.calc_dist_flat, format);
		snprintf(gps_text, 10, "%.*f", decimals_needed(val), val);
	}
	else if ( !strncmp( keyword, "gps_datetime_now", strlen("gps_datetime_now") ) && crt_point.time != GPS_UNINIT )
	{
		char* format = NULL;
		if (strlen(keyword) > strlen("gps_datetime_now"))
			format = keyword + strlen("gps_datetime_now");
		seconds_to_timestring(crt_point.time, format, gps_text);
	}
//	mlt_log_info(NULL, "filter_gps.c gps_point_to_output, keyword=%s, result=%s", keyword, gps_text);
}

/** Checks if file is processed or needs to be read from disk,
 *  finds the closest (time-wise) gps_point and uses it to call gps_point_to_output
*/
static void get_next_gps_frame_data (mlt_filter filter, mlt_frame frame, const time_t video_time_synced, const time_t video_time_original, const char* keyword, char gps_text[MAX_TEXT_LEN])
{
	private_data* pdata = (private_data*)filter->child;
	gps_point* gps_points = pdata->gps_points;

	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	char* location_filename = mlt_properties_get( properties, "gps.file");
	int crt_smooth = mlt_properties_get_int(properties, "smoothing_value");
	
	//if there's no file selected just return
	if ( !location_filename || !strcmp(location_filename, "") ) {
		//mlt_log_warning(filter, "get_next_gps_frame_data gps_file string null, %s", location_filename==0?"null":location_filename);
		return;
	}
	
	//check to see if we need to trim the gps start time:
	char* gps_processing_start_time = mlt_properties_get(properties, "gps_processing_start_time");
	if (gps_processing_start_time != NULL) {
		time_t gps_proc_t = 0;
		if (strlen(gps_processing_start_time)!=0 && strcmp(gps_processing_start_time, "yyyy-MM-dd hh:mm:ss"))
			gps_proc_t = datetimeXMLstring_to_seconds(gps_processing_start_time, "%Y-%m-%d %H:%M:%S");
		//if different start time is selected we must clear extra data, resmooth with level and recalculate
		if (gps_proc_t != pdata->gps_proc_start_t) {
			//mlt_log_info(filter, "get_next_gps_frame_data gps_processing_start_time=%s -> %d", gps_processing_start_time, gps_proc_t);
			pdata->gps_proc_start_t = gps_proc_t;
			process_gps_smoothing(filter, 0);
			process_gps_smoothing(filter, crt_smooth);
			recalculate_gps_data(filter, crt_smooth);
		}
	}

	//check if the gps file has been changed, if not, just use current data
	if (strcmp(pdata->last_filename, location_filename))
	{
		//mlt_log_info(filter, "get_next_gps_frame_data: last_filename (%s) != entered_filename (%s), reading data from file", pdata->last_filename, location_filename);
		cleanup_priv_data(pdata);
		strcpy(pdata->last_filename, location_filename);
		process_gps_file(filter, frame, location_filename);
		gps_points = pdata->gps_points;
	}
	
	//process smoothing
	if (pdata->last_smooth_lvl != crt_smooth) {
		pdata->last_smooth_lvl = crt_smooth;
		process_gps_smoothing(filter, crt_smooth);
	}

	int i_gps = -1;
	if ((i_gps = binary_search_gps(filter, video_time_synced)) == -1)
		return;

	//write the requested info in gps_text
	gps_point_to_output(gps_points[i_gps], keyword, video_time_original, gps_text);
}

/** Returns the unix time (seconds) of "Media Created" metadata, or file's "Modified Time" from OS
 */
static time_t get_original_file_time_seconds (mlt_filter filter, mlt_frame frame)
{
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	return mlt_producer_get_creation_time(producer)/1000;
}

/** Returns absolute current frame time in seconds (original media creation + current timecode)
 */
static time_t get_current_frame_time (mlt_filter filter, mlt_frame frame)
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	private_data* pdata = (private_data*)filter->child;
	time_t ret_time = 0, fr_time = 0;
	
	//get original file time:
	ret_time = get_original_file_time_seconds(filter, frame);

	//assign the time to a property to show it in ui
	char video_start_text[255];
	seconds_to_timestring(ret_time, NULL, video_start_text);
	mlt_properties_set(properties, "video_start_text", video_start_text); //this is set every frame, does it matter?
	
	//find out file date-time's timezone (including DST), one time process
	if (pdata->video_file_timezone_s == -1 || mlt_properties_get_int(properties, "videofile_timezone_seconds") != pdata->video_file_timezone_s)
	{
		struct tm* ptm = localtime(&ret_time);
		ptm->tm_isdst = -1; //force dst detection
		time_t time_local = mktime(ptm);
		pdata->video_file_timezone_s = timezone-(ptm->tm_isdst*3600);
		//mlt_log_info(filter, "get_current_frame_time, setting videofile_timezone_seconds=%d", pdata->video_file_timezone_s);
		mlt_properties_set_int(properties, "videofile_timezone_seconds", pdata->video_file_timezone_s);
	}

	//get current frame time (timecode, relative to beginning of video file even if video is trimmed):
	//TODO: find a way to get seconds directly instead of string and convert
	mlt_position frames = mlt_frame_get_position( frame );
	char *s = mlt_properties_frames_to_time( properties, frames, mlt_time_clock );
	if (s) {
		int h = 0, m = 0;
		float sec = 0;
		sscanf(s, "%d:%d:%f", &h, &m, &sec);
		fr_time = h*3600 + m*60 + (int)sec;
	//	mlt_log_info(filter, "get_current_frame_time, timecode: %s, timecode->seconds: %d", s, fr_time);
	}
	else 
		mlt_log_warning(filter, "get_current_frame_time, couldn't get timecode!");

	//check to see if we need to speed up/down the time
	double speed_multiplier = mlt_properties_get_double(properties, "speed_multiplier");
	if (pdata->speed_multiplier && pdata->speed_multiplier != speed_multiplier) {
		pdata->speed_multiplier = speed_multiplier;
	}

//	mlt_log_info(filter, "get_current_frame_time: timelapse read:%f timecode s:%d, with timelapse ratio:%d", speed_multiplier, fr_time, (int)(fr_time*(pdata->speed_multiplier)));
	fr_time *= pdata->speed_multiplier;

	return ret_time + (int)fr_time;
}

/** Reads and stores the offset_major (seconds) and offset_minor (seconds) from filter UI
 *  + provides for the UI the automatic gps offset to match file to gps begining
 */
static void update_gpsoffsets_ui(mlt_filter filter, mlt_frame frame, int* video_offset1, int* video_offset2)
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

	//calculate seconds diff: video create time vs first gps time and assign the time to a property to acces it from ui
	int auto_gps_sync = get_first_gps_time(filter) - get_original_file_time_seconds(filter, frame);

	mlt_properties_set_int(properties, "auto_gps_offset_start", auto_gps_sync);

	//read data from UI:
	*video_offset1 = mlt_properties_get_int(properties, "major_offset");
	*video_offset2 = mlt_properties_get_int(properties, "minor_offset");
//	mlt_log_info(filter, "video_offset2 = %d", *video_offset2);
}

/** Processes data and tries to replace the GPS keywords with actual values.
 *  Puts "--" in place of keywords with no valid return.
*/
static void get_gps_str(const char* keyword, mlt_filter filter, mlt_frame frame, char* text )
{
	int video_offset1 = 0;
	int video_offset2 = 0;
	time_t video_time = 0, video_time_synced = 0, first_gps_time = 0;
	char gps_text[256];
	strcpy(gps_text, "--");

	video_time = get_current_frame_time(filter, frame);
	first_gps_time = get_first_gps_time(filter);

	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties_set_int(properties, "auto_gps_offset_now", first_gps_time - video_time);

	//read&update offset property
	update_gpsoffsets_ui(filter, frame, &video_offset1, &video_offset2);

	//add offset to video time:
	video_time_synced = video_time + video_offset1 + video_offset2;

//	mlt_log_info(filter, "get_gps_str, video_offset1: %d (1st gps: %d - 1st file: %d - crt vid_file: %d - sync_crt_file:%d)",
//			video_offset1, get_first_gps_time(filter), get_original_file_time_seconds(filter, frame),
//			video_time, video_time_synced);
	
	//find gps entry closest to our time
	get_next_gps_frame_data (filter, frame, video_time_synced, video_time, keyword, gps_text);
	
	strncat( text, gps_text, MAX_TEXT_LEN - strlen( text ) - 1);
}

/** Replaces file_datetime_now with absolute time-date string (video created + current timecode)
 */
static void get_current_frame_time_str(char* keyword, mlt_filter filter, mlt_frame frame, char* result )
{
	//check if format is requested for datetime: keyword would be: "#gps_datetime %H:%m:%s#"
	char* format = NULL;
	char text[MAX_TEXT_LEN];
	if (strlen(keyword) > strlen("file_datetime_now"))
		format = keyword + strlen("file_datetime_now");
	seconds_to_timestring(get_current_frame_time(filter, frame), format, text);

	strncat( result, text, MAX_TEXT_LEN - strlen( result ) - 1);
}

/** Perform substitution for keywords that are enclosed in "# #".
*/
static void substitute_keywords(mlt_filter filter, char* result, char* value, mlt_frame frame)
{
		char keyword[MAX_TEXT_LEN] = "";
		int pos = 0;
		int is_keyword = 0;

		while ( get_next_token(value, &pos, keyword, &is_keyword) )
		{
			if(!is_keyword)
			{
				strncat( result, keyword, MAX_TEXT_LEN - strlen( result ) - 1 );
			}
			/* gps related keywords, r=raw, c=computed */
			else if ( !strncmp( keyword, "gps_lat_r", strlen("gps_lat_r") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_lon_r", strlen("gps_lon_r") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_lat_c", strlen("gps_lat_c") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_lon_c", strlen("gps_lon_c") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_elev", strlen("gps_elev") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_vdist_up", strlen("gps_vdist_up") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_vdist_down", strlen("gps_vdist_down") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_dist_uphill", strlen("gps_dist_uphill") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_dist_downhill", strlen("gps_dist_downhill") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_dist_flat", strlen("gps_dist_flat") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_speed_r", strlen("gps_speed_r") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_speed_c", strlen("gps_speed_c") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_dist_r", strlen("gps_dist_r") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_dist_c", strlen("gps_dist_c") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_hr", strlen("gps_hr") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_bearing", strlen("gps_bearing") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_compass", strlen("gps_compass") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "gps_datetime_now", strlen("gps_datetime_now") ) )
			{
				get_gps_str( keyword, filter, frame, result );
			}
			else if ( !strncmp( keyword, "file_datetime_now", strlen("file_datetime_now") ) )
			{
				get_current_frame_time_str( keyword, filter, frame, result );
			}
			else
			{
				// replace keyword with property value from this frame
				mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
				char *frame_value = mlt_properties_get( frame_properties, keyword );
				if( frame_value )
				{
					strncat( result, frame_value, MAX_TEXT_LEN - strlen(result) - 1 );
				}
			}
		}
}

/** Filter processing.
*/
static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	char* dynamic_text = mlt_properties_get( properties, "argument" );

	if ( !dynamic_text || !strcmp( "", dynamic_text ) )
		return frame;

	mlt_filter text_filter = mlt_properties_get_data( properties, "_text_filter", NULL );
	mlt_properties text_filter_properties = mlt_frame_unique_properties( frame, MLT_FILTER_SERVICE(text_filter) );

	// Apply keyword substitution before passing the text to the filter.
	char* result = calloc( 1, MAX_TEXT_LEN );
	substitute_keywords( filter, result, dynamic_text, frame );
	mlt_properties_set_string( text_filter_properties, "argument", result );
	free( result );
	mlt_properties_pass_list( text_filter_properties, properties,
			"geometry family size weight style fgcolour bgcolour olcolour pad halign valign outline" );
	mlt_filter_set_in_and_out( text_filter, mlt_filter_get_in( filter ), mlt_filter_get_out( filter ) );
	return mlt_filter_process( text_filter, frame );
}

/** Destructor for the filter.
*/
static void filter_close (mlt_filter filter)
{
	private_data* pdata = (private_data*)filter->child;

	cleanup_priv_data(pdata);
	free(pdata);

	filter->child = NULL;
	filter->close = NULL;
	filter->parent.close = NULL;
	mlt_service_close( &filter->parent );
}

/** Constructor for the filter.
*/
mlt_filter filter_gpstext_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
		mlt_filter filter = mlt_filter_new();
		private_data* pdata = (private_data*) calloc(1, sizeof(private_data));
		mlt_filter text_filter = mlt_factory_filter( profile, "qtext", NULL );

		if( !text_filter )
				text_filter = mlt_factory_filter( profile, "text", NULL );

		if( !text_filter )
				mlt_log_warning( MLT_FILTER_SERVICE(filter), "Unable to create text filter.\n" );

		if ( filter && text_filter && pdata)
		{
				mlt_properties my_properties = MLT_FILTER_PROPERTIES( filter );

				// Register the text filter for reuse/destruction
				mlt_properties_set_data( my_properties, "_text_filter", text_filter, 0, ( mlt_destructor )mlt_filter_close, NULL );

				/*
				//all fields for debug
				mlt_properties_set_string( my_properties, "argument", arg ? arg:
						"GPS: #gps_lat_r#, #gps_lon_r#, alt:#gps_elev#m"
						"\nSmooth: #gps_lat_c#, #gps_lon_c#"
						"\nRaw: speed:#gps_speed_r#km/h, dist:#gps_dist_r#m, HR: #gps_hr#bpm"
						"\nCalc: speed:#gps_speed_c#km/h, dist:#gps_dist_c#m, bearing: #gps_bearing# (#gps_compass#)"
						"\nAlti: gain: #gps_vdist_up#, loss: #gps_vdist_down#, uphill: #gps_dist_uphill#, dhill: #gps_dist_downhill#, flat: #gps_dist_flat#"
						"\nTime: file:#file_datetime_now#, gps:#gps_datetime_now#" );
				*/

				// Assign default values
				mlt_properties_set_string( my_properties, "argument", arg ? arg:
						"Speed: #gps_speed_c#km/h\n"
						"Distance: #gps_dist_c#m\n"
						"Altitude: #gps_elev#m\n\n"
						"GPS time: #gps_datetime_now# UTC\n"
						"GPS location: #gps_lat_c#, #gps_lon_c#"
						);
				mlt_properties_set_string( my_properties, "geometry", "10%/10%:80%x80%:100%" );
				mlt_properties_set_string( my_properties, "family", "Sans" );
				mlt_properties_set_string( my_properties, "size", "26" );
				mlt_properties_set_string( my_properties, "weight", "400" );
				mlt_properties_set_string( my_properties, "style", "normal" );
				mlt_properties_set_string( my_properties, "fgcolour", "0xffffffff" );
				mlt_properties_set_string( my_properties, "bgcolour", "0x00000000" );
				mlt_properties_set_string( my_properties, "olcolour", "0x000000ff" );
				mlt_properties_set_string( my_properties, "pad", "5" );
				mlt_properties_set_string( my_properties, "halign", "left" );
				mlt_properties_set_string( my_properties, "valign", "bottom" );
				mlt_properties_set_string( my_properties, "outline", "0" );
				mlt_properties_set_int( my_properties, "_filter_private", 1 );

				mlt_properties_set_int( my_properties, "major_offset", 0);
				mlt_properties_set_int( my_properties, "minor_offset", 0);
				mlt_properties_set_int( my_properties, "smoothing_value", 5);
				mlt_properties_set_int( my_properties, "videofile_timezone_seconds", 0);
				mlt_properties_set_int( my_properties, "speed_multiplier", 1);

				filter->close = filter_close;
				filter->process = filter_process;
				filter->child = pdata;
		}
		else
		{
				if( filter )
				{
						mlt_filter_close( filter );
				}

				if( text_filter )
				{
						mlt_filter_close( text_filter );
				}

				free(pdata);
				filter = NULL;
		}
		return filter;
}
