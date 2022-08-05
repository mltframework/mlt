/*
 * filter_gpsgraphic.cpp -- draws gps related graphics
 * Copyright (c) 2015-2022 Meltytech, LLC
 * Original author: Daniel F
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
#include "common.h"
#include "filter_gpsgraphic.h"

#include <QMutex>
static QMutex f_mutex;

// Sets the private data to default values and frees gps points array
static void default_priv_data(private_data* pdata)
{
	if (pdata)
	{
		if (pdata->gps_points_r)
			free (pdata->gps_points_r);
		if (pdata->gps_points_p)
			free (pdata->gps_points_p);
		memset (pdata, 0 , sizeof(private_data));
		pdata->speed_multiplier = 1;
		pdata->minmax.set_defaults();
	}
}

//like decimals_needed() but graph data source aware
int decimals_needed_bysrc(mlt_filter filter, double v)
{
	private_data* pdata = (private_data*)filter->child;

	if (pdata->graph_data_source == gspg_location_src)
		return 6;
	if (pdata->graph_data_source == gpsg_altitude_src || pdata->graph_data_source == gpsg_speed_src)
		return (decimals_needed(v));
	if (pdata->graph_data_source == gpsg_hr_src)
		return 0;
	return 0;
}

//convert_[distance|speed]_to_format but graph data source aware
double convert_bysrc_to_format(mlt_filter filter, double val)
{
	private_data* pdata = (private_data*)filter->child;
	char* legend_unit = mlt_properties_get(MLT_FILTER_PROPERTIES( filter ), "legend_unit");

	if (pdata->graph_data_source == gpsg_altitude_src)
		return convert_distance_to_format(val, legend_unit);
	if (pdata->graph_data_source == gpsg_speed_src)
		return convert_speed_to_format(val, legend_unit);
	return val;
}

//helps pass filter private data for gps_parser.cpp calls
gps_private_data filter_to_gps_data (mlt_filter filter) 
{
	private_data* pdata = (private_data*)filter->child;
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

	ret.gps_proc_start_t = 0;
	ret.last_smooth_lvl = pdata->last_smooth_lvl;
	ret.last_filename = pdata->last_filename;
	ret.filter = filter;
	return ret;
}

/* Gets the value from pdata->minmax (or custom|existing gps_point) depending on data_src value
   get_type represents: 0=current_value, 1=max_value, -1=min_value
   subtype is used for longitude (planned more but didn't make sense so far)
   gps_p is used to get (only for get_type=0) the proper value by source from points not in pdata->gps_points_p[] (mostly weighted point)
*/
double get_by_src (mlt_filter filter, int get_type, int i_gps /* = 0 */, int subtype /* = 0 */, gps_point_proc* gps_p /* = NULL */)
{
	private_data* pdata = (private_data*)filter->child;
	if (i_gps < 0 || i_gps >= pdata->gps_points_size) 
		return 0;

	if (pdata->graph_data_source == gspg_location_src)
	{
		if (get_type == -1)
		{
			if (subtype == gpsg_latitude_id) return pdata->minmax.min_lat;
			else if (subtype == gpsg_longitude_id) return pdata->minmax.min_lon;
		}
		else if (get_type == 1)
		{
			if (subtype == gpsg_latitude_id) return pdata->minmax.max_lat;
			else if (subtype == gpsg_longitude_id) return pdata->minmax.max_lon;
		}
		else if (get_type == 0)
		{
			if (subtype == gpsg_latitude_id) 
			{
				if (gps_p == NULL) return pdata->gps_points_p[i_gps].lat;
				else if (gps_p != NULL) return gps_p->lat;
			}
			else if (subtype == gpsg_longitude_id) 
			{
				if (gps_p == NULL) return pdata->gps_points_p[i_gps].lon;
				else if (gps_p != NULL) return gps_p->lon;
			}
			
		}
	}
	else if (pdata->graph_data_source == gpsg_altitude_src)
	{
		if (get_type == -1) return pdata->minmax.min_ele;
		else if (get_type == 1) return pdata->minmax.max_ele;
		else if (get_type == 0)
		{
			if (gps_p == NULL) return pdata->gps_points_p[i_gps].ele;
			else if (gps_p != NULL) return gps_p->ele;
		}
	}
	else if (pdata->graph_data_source == gpsg_hr_src) 
	{
		if (get_type == -1) return pdata->minmax.min_hr;
		else if (get_type == 1) return pdata->minmax.max_hr;
		else if (get_type == 0)
		{
			if (gps_p == NULL) return pdata->gps_points_p[i_gps].hr;
			else if (gps_p != NULL) return gps_p->hr;
		}
	}
	else if (pdata->graph_data_source == gpsg_speed_src) 
	{
		if (get_type == -1) return pdata->minmax.min_speed;
		else if (get_type == 1) return pdata->minmax.max_speed;
		else if (get_type == 0)
		{
			if (gps_p == NULL) return pdata->gps_points_p[i_gps].speed;
			else if (gps_p != NULL) return gps_p->speed;
		}
	}
	mlt_log_warning(filter, "Invalid combination of arguments to get_by_src: (get_type=%d, i_gps=%d, subtype=%d, gps_p=%p)\n", get_type, i_gps, subtype, gps_p);
	return 0;
}
//get_by_src helper shortcuts
double get_min_bysrc (mlt_filter filter, int subtype /* = 0 */)
{
	return get_by_src(filter, -1, 0, subtype, NULL);
}
double get_max_bysrc (mlt_filter filter, int subtype /* = 0 */)
{
	return get_by_src(filter, 1, 0, subtype, NULL);
}
double get_crtval_bysrc (mlt_filter filter, int i_gps, int subtype /* = 0 */, gps_point_proc* gps_p /* = NULL */)
{
	return get_by_src(filter, 0, i_gps, subtype, gps_p);
}

// Returns the unix time (miliseconds) of "Media Created" metadata, or fallbacks to "Modified Time" from OS
static int64_t get_original_video_file_time_mseconds (mlt_frame frame)
{
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	return mlt_producer_get_creation_time(producer);
}

/** Returns absolute* current frame time in miliseconds (original file creation + current timecode)
 *  *also applies speed_multiplier
 */
static int64_t get_current_frame_time_ms (mlt_filter filter, mlt_frame frame)
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	private_data* pdata = (private_data*)filter->child;
	int64_t file_time = 0, fr_time = 0;

	file_time = get_original_video_file_time_mseconds(frame);
	mlt_position frame_position = mlt_frame_original_position(frame);
	// mlt_log_info(filter, "frame_pos=%d, frame_orig_pos=%d; file_time=%d\n", mlt_frame_get_position(frame), mlt_frame_original_position(frame), file_time/1000);
	f_mutex.lock();
	char *s = mlt_properties_frames_to_time( properties, frame_position, mlt_time_clock );
	if (s) {
		int h = 0, m = 0, sec = 0, msec=0;
		sscanf(s, "%d:%d:%d.%d", &h, &m, &sec, &msec);
		fr_time = (h*3600 + m*60 + sec)*1000 + msec;
	}
	else 
		mlt_log_warning(filter, "get_current_frame_time_ms time string null, giving up [mlt_frame_original_position()=%d], retry result:%s\n", frame_position, mlt_properties_frames_to_time( properties, frame_position, mlt_time_clock ));
	f_mutex.unlock();

	return file_time + fr_time * pdata->speed_multiplier;
}

//gets the nearest gps point [index] according to video time + input offset
int get_now_gpspoint_index(mlt_filter filter, mlt_frame frame, bool force_result /* = true */)
{
	private_data* pdata = (private_data*)filter->child;
	int64_t video_time_synced = get_current_frame_time_ms(filter, frame) + pdata->gps_offset;
	return binary_search_gps(filter_to_gps_data(filter), video_time_synced, force_result);
}

//returns the next gps point with a valid value for crt_source (starting with crt_i+1)
int get_next_valid_gpspoint_index(mlt_filter filter, int crt_i)
{
	private_data* pdata = (private_data*)filter->child;
	while (++crt_i < pdata->gps_points_size && get_crtval_bysrc(filter, crt_i) == GPS_UNINIT);
	//maybe TODO: add restriction for MAX_GPS_TIME? and allow depending on force_result
	return CLAMP(crt_i, 0, pdata->gps_points_size-1);
}

//returns an interpolated gps point at the exact current video time + offset 
gps_point_proc get_now_weighted_gpspoint(mlt_filter filter, mlt_frame frame, bool force_result /* = true */)
{
	private_data* pdata = (private_data*)filter->child;
	gps_point_proc crt;
	int i_now = -1, non_forced_i = -1;
	int max_gps_diff_ms = get_max_gps_diff_ms(filter_to_gps_data(filter));

	int64_t video_time_synced = get_current_frame_time_ms(filter, frame) + pdata->gps_offset;
	i_now = binary_search_gps(filter_to_gps_data(filter), video_time_synced, force_result);
	//make sure we don't mistakenly interpolate between gps[0] and gps[0+1] because the "forced" search returned gps[0] due to outside time
	non_forced_i = binary_search_gps(filter_to_gps_data(filter), video_time_synced, false);

	if (i_now == -1) //if force_result is true this will never happen
		return uninit_gps_proc_point;

	//interpolate if everything ok
	int next_i = get_next_valid_gpspoint_index(filter, i_now);
	if (non_forced_i != -1)
		crt = weighted_middle_point_proc(&pdata->gps_points_p[i_now], &pdata->gps_points_p[next_i], video_time_synced, max_gps_diff_ms);
	else 
		crt = pdata->gps_points_p[i_now];
	return crt;
}

//initializes stuff and chooses which type of graph draw to call
static void draw_graphics( mlt_filter filter, mlt_frame frame, QImage* qimg, int width, int height, s_base_crops &used_crops )
{
	private_data* pdata = (private_data*)filter->child;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	QPainter p( qimg );

	prepare_canvas (filter, frame, qimg, p, width, height, used_crops);

	if (pdata->ui_crops.start_index < pdata->ui_crops.end_index)
	{
		//draw the main graph line
		if (pdata->graph_type == gpsg_2d_map_graph || pdata->graph_type == gpsg_crop_center_graph)
			draw_main_line_graph(filter, frame, p, used_crops);
		else if (pdata->graph_type == gpsg_speedometer_graph)
			draw_main_speedometer(filter, frame, p, used_crops);
	}
	else 
		mlt_log_info (filter, "min > max so nothing to print (index: start=%d,end=%d; start_p:%f,end_p:%f; vertical: UIbot=%f,UItop=%f; horizontal: UIleft:%f,UIright:%f)\n", 
			pdata->ui_crops.start_index, pdata->ui_crops.end_index, mlt_properties_get_double(properties, "trim_start_p"), mlt_properties_get_double(properties, "trim_end_p"), 
			pdata->ui_crops.bot, pdata->ui_crops.top, pdata->ui_crops.left, pdata->ui_crops.right);

	p.end();
}

template <typename T>
static double get_as_percentage (T val, T min, T max)
{
	double ret = 1;
	if (max - min != 0)
		ret = (double) (val - min) / (max - min);
	return ret;		
}

//computes used_crops from pdata->ui_crops (they change on each frame so can't store them in pdata)
static void process_frame_properties (mlt_filter filter, mlt_frame frame, s_base_crops &used_crops)
{
	private_data* pdata = (private_data*)filter->child;

	//if CROP_CENTER we'll compute used_crops from pdata->ui_crops every frame to keep the point in center
	if (pdata->graph_type == gpsg_crop_center_graph)
	{
		int i_now = get_now_gpspoint_index(filter, frame);
		gps_point_proc crt_weighted = get_now_weighted_gpspoint(filter, frame);

		if (get_crtval_bysrc(filter, i_now, 0, &crt_weighted) != GPS_UNINIT)
		{
			double now_perc_vertical = get_as_percentage(get_crtval_bysrc(filter, 0, 0, &crt_weighted), get_min_bysrc(filter), get_max_bysrc(filter));
			double now_perc_horizontal;
			if (pdata->graph_data_source == gspg_location_src)
				now_perc_horizontal = get_as_percentage(crt_weighted.lon, get_min_bysrc(filter, gpsg_longitude_id), get_max_bysrc(filter, gpsg_longitude_id));
			else
				now_perc_horizontal = get_as_percentage(crt_weighted.time, pdata->ui_crops.min_crop_time, pdata->ui_crops.max_crop_time);

			//crop_left = Pcrop*delta + left_min; left_min = perc_horiz - delta; right_min = perc_horiz
			//crop_right = (1-Pcrop)*delta + right_min, delta = perc_horiz if > 0.5 else (1-perc_horiz)
			double delta_h = now_perc_horizontal > 0.5 ? now_perc_horizontal : 1-now_perc_horizontal;
			used_crops.left = ( pdata->ui_crops.left / 100 * delta_h  + (now_perc_horizontal - delta_h) ) * 100;
			used_crops.right = ( (1 - pdata->ui_crops.left / 100) * delta_h  + now_perc_horizontal ) * 100;

			double delta_v = now_perc_vertical > 0.5 ? now_perc_vertical : 1-now_perc_vertical;
			used_crops.bot = ( pdata->ui_crops.bot / 100 * delta_v + (now_perc_vertical - delta_v) ) * 100;
			used_crops.top = ( (1 - pdata->ui_crops.bot / 100 ) * delta_v + now_perc_vertical ) * 100;
			
			//if not 2d map, center for vertical axis is not something we want, so just use standard crop logic
			if (pdata->graph_data_source != gspg_location_src)
			{
				used_crops.bot = pdata->ui_crops.bot;
				used_crops.top = pdata->ui_crops.top;
			}
		}
	}
	else /* --> not gpsg_crop_center_graph */
	{
		used_crops.bot = pdata->ui_crops.bot;
		used_crops.top = pdata->ui_crops.top;
		used_crops.left = pdata->ui_crops.left;
		used_crops.right = pdata->ui_crops.right;
	}
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = (mlt_filter)mlt_frame_pop_service( frame );
	s_base_crops used_crops = {0, 100, 0, 100};

	// Get the current image
	*format = mlt_image_rgba;
	error = mlt_frame_get_image( frame, image, format, width, height, writable );

	// Draw the graph
	if( !error ) {
		process_frame_properties(filter, frame, used_crops);
		QImage qimg( *width, *height, QImage::Format_ARGB32 );
		convert_mlt_to_qimage_rgba( *image, &qimg, *width, *height );
		draw_graphics( filter, frame, &qimg, *width, *height, used_crops );
		convert_qimage_to_mlt_rgba( &qimg, *image, *width, *height );
	}
	else 
		mlt_log_warning(MLT_FILTER_SERVICE(filter), "mlt_frame_get_image error=%d, can't draw at all\n", error);
	return error;
}

//if crop mode is absolute we'll need to convert those values to a percentage as the rest of the code expects
static void convert_crop_values_to_percentages(mlt_filter filter, mlt_frame frame)
{
	private_data* pdata = (private_data*)filter->child;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	int crop_mode_v = mlt_properties_get_int(properties, "crop_mode_v");
	int crop_mode_h = mlt_properties_get_int(properties, "crop_mode_h");

	if (crop_mode_h + crop_mode_v == 0) //0=already percentage, 1=need to convert
		return;

	double v_min = get_min_bysrc(filter);
	double v_max = get_max_bysrc(filter);
	v_min = convert_bysrc_to_format(filter, v_min);
	v_max = convert_bysrc_to_format(filter, v_max);

	//now calculate the percentages
	//resulted_crop_percentage = (input_absolute_value - crt_type_minimum_value) / (crt_type_maximum_value - crt_type_minimum_value) * 100
	if (crop_mode_v)
	{
		pdata->ui_crops.bot = (pdata->ui_crops.bot - v_min) / (v_max - v_min) * 100;
		pdata->ui_crops.top = (pdata->ui_crops.top - v_min) / (v_max - v_min) * 100;
	}
	//this is just to cover all cases, I doubt someone would manually input coordonates or unix time (in milliseconds) when they can just visually set the percentage
	if (crop_mode_h)
	{
		if (pdata->graph_data_source == gspg_location_src) //longitude
		{
			v_min = get_min_bysrc(filter, gpsg_longitude_id);
			v_max = get_max_bysrc(filter, gpsg_longitude_id);
			pdata->ui_crops.left = (pdata->ui_crops.left - v_min) / (v_max - v_min) * 100;
			pdata->ui_crops.right = (pdata->ui_crops.right - v_min) / (v_max - v_min) * 100;
		}
		else //time
		{
			v_min = pdata->ui_crops.min_crop_time;
			v_max = pdata->ui_crops.max_crop_time;
			pdata->ui_crops.left = (pdata->ui_crops.left - v_min) / (v_max - v_min) * 100;
			pdata->ui_crops.right = (pdata->ui_crops.right - v_min) / (v_max - v_min) * 100;
		}
	}
}

//goes through ALL gps data and finds min/max for gps, altitude, hr + computes map's aspect ratio and middle point
static void find_minmax_of_data(mlt_filter filter) 
{
	private_data* pdata = (private_data*)filter->child;
	pdata->minmax.set_defaults();
	
	#define assign_if_smaller(x, m) if (x != GPS_UNINIT && x < m) m = x;
	#define assign_if_bigger(x, m) if (x != GPS_UNINIT && x > m) m = x;
	for (int i=0; i<pdata->gps_points_size; i++) {
		gps_point_proc *crt = &pdata->gps_points_p[i];
		assign_if_smaller(crt->lat, pdata->minmax.min_lat);
		assign_if_bigger(crt->lat, pdata->minmax.max_lat);
		assign_if_smaller(crt->lon, pdata->minmax.min_lon);
		assign_if_bigger(crt->lon, pdata->minmax.max_lon);
		assign_if_smaller(crt->ele, pdata->minmax.min_ele);
		assign_if_bigger(crt->ele, pdata->minmax.max_ele);
		assign_if_smaller(crt->speed, pdata->minmax.min_speed);
		assign_if_bigger(crt->speed, pdata->minmax.max_speed);
		assign_if_smaller(crt->hr, pdata->minmax.min_hr);
		assign_if_bigger(crt->hr, pdata->minmax.max_hr);
	}
	#undef assign_if_smaller
	#undef assign_if_bigger

	//compute the map aspect ratio using real distances (used to correctly overlay background image over gps track)
	double map_aspect_ratio = 1;
	double map_width = distance_haversine_2p(pdata->minmax.min_lat, pdata->minmax.min_lon, pdata->minmax.min_lat, pdata->minmax.max_lon);
	double map_height = distance_haversine_2p(pdata->minmax.min_lat, pdata->minmax.min_lon, pdata->minmax.max_lat, pdata->minmax.min_lon);
	if (map_width && map_height)
		map_aspect_ratio = map_width / map_height;
	pdata->map_aspect_ratio_from_distance = map_aspect_ratio;
	mlt_properties_set_double(MLT_FILTER_PROPERTIES( filter ), "map_original_aspect_ratio", map_aspect_ratio);

	// //compute the gps track aspect ratio (from coords) // the other one seems better almost every time
	// map_aspect_ratio = 1;
	// map_width = pdata->minmax.max_lon - pdata->minmax.min_lon;
	// map_height = pdata->minmax.max_lat - pdata->minmax.min_lat;
	// if (map_width && map_height)
	// 	map_aspect_ratio = map_width / map_height;
	// mlt_properties_set_double(MLT_FILTER_PROPERTIES( filter ), "map_original_aspect_ratio", map_aspect_ratio);

	char middle_point[255];
	double middle_lat = (pdata->minmax.min_lat + pdata->minmax.max_lat)/2;
	double middle_lon = (pdata->minmax.min_lon + pdata->minmax.max_lon)/2;
	snprintf(middle_point, 255, "%.6f, %.6f", middle_lat, swap_180_if_needed(middle_lon)); 
	mlt_properties_set(MLT_FILTER_PROPERTIES( filter ), "map_coords_hint", middle_point);

	mlt_log_info(filter, "gps file minmax: min_lat,lon-max_lat,lon: %f,%f:%f,%f; ele: %f,%f; speed:%f,%f; hr:%f,%f; map_ar:%f, mid_point:%s \n", 
		pdata->minmax.min_lat, pdata->minmax.min_lon, pdata->minmax.max_lat, pdata->minmax.max_lon,
		pdata->minmax.min_ele, pdata->minmax.max_ele, pdata->minmax.min_speed, pdata->minmax.max_speed,	pdata->minmax.min_hr, pdata->minmax.max_hr,
		map_aspect_ratio, middle_point );
}

// Reads and updates all necessary filter properties, and smooths+processes the gps data if needed
static void process_filter_properties(mlt_filter filter, mlt_frame frame)
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	private_data* pdata = (private_data*)filter->child;
	char do_smoothing = 0;

//read properties
	int read_video_offset1 = mlt_properties_get_int(properties, "time_offset");
	int read_smooth_val = mlt_properties_get_int(properties, "smoothing_value");
	double read_speed_multiplier = mlt_properties_get_double(properties, "speed_multiplier");
	int64_t original_video_time = get_original_video_file_time_mseconds(frame);

//process properties
	pdata->gps_offset = (int64_t)read_video_offset1 * 1000;
	pdata->speed_multiplier = (read_speed_multiplier ? read_speed_multiplier : 1);
	if (pdata->last_smooth_lvl != read_smooth_val) {
		pdata->last_smooth_lvl = read_smooth_val;
		do_smoothing = 1;
	}

	char video_start_text[255], gps_start_text[255];
	mseconds_to_timestring(original_video_time, NULL, video_start_text);
	mseconds_to_timestring(pdata->first_gps_time, NULL, gps_start_text);

	if (do_smoothing) { //also always does processing + updates minmax
		process_gps_smoothing(filter_to_gps_data(filter), 1);
		find_minmax_of_data(filter);
	}

	pdata->graph_data_source = mlt_properties_get_int(properties, "graph_data_source");
	pdata->graph_type = mlt_properties_get_int(properties, "graph_type");

	pdata->ui_crops.trim_start_p = mlt_properties_get_double(properties, "trim_start_p");
	pdata->ui_crops.start_index = pdata->gps_points_size / 100.0 * pdata->ui_crops.trim_start_p;
	pdata->ui_crops.start_index = CLAMP(pdata->ui_crops.start_index, 0, pdata->gps_points_size-1);
	pdata->ui_crops.min_crop_time = pdata->gps_points_p[pdata->ui_crops.start_index].time;
	
	pdata->ui_crops.trim_end_p = mlt_properties_get_double(properties, "trim_end_p");
	pdata->ui_crops.end_index = pdata->gps_points_size / 100.0 * pdata->ui_crops.trim_end_p;
	pdata->ui_crops.end_index = CLAMP(pdata->ui_crops.end_index, 0, pdata->gps_points_size-1);
	pdata->ui_crops.max_crop_time = pdata->gps_points_p[pdata->ui_crops.end_index].time;

	pdata->ui_crops.bot = mlt_properties_get_double(properties, "crop_bot_p") ;
	pdata->ui_crops.top = mlt_properties_get_double(properties, "crop_top_p") ;
	pdata->ui_crops.left = mlt_properties_get_double(properties, "crop_left_p") ;
	pdata->ui_crops.right = mlt_properties_get_double(properties, "crop_right_p") ;
	
	convert_crop_values_to_percentages(filter, frame);

	//(re-)read background img file if path changed
	char *bg_path = mlt_properties_get(properties, "bg_img_path");
	bool new_img_loaded = false;
	if (bg_path && strlen(bg_path) > 0 && bg_path[0] != '!') 
	{
		if(strcmp(bg_path, pdata->last_bg_img_path))
		{
			if (pdata->bg_img.load(bg_path))
			{
				pdata->bg_img = pdata->bg_img.convertToFormat(QImage::Format_ARGB32);
				strncpy(pdata->last_bg_img_path, bg_path, 255);
				new_img_loaded = true;
			}
			else 
			{
				mlt_log_warning(filter, "Failed to load background image: %s\n", bg_path);
			}
		}
	}
	else //delete the previous image
	{
		if (strlen(pdata->last_bg_img_path) > 0)
		{
			pdata->last_bg_img_path[0] = 0;
			pdata->bg_img = QImage();
			pdata->bg_img_scaled = QImage();
		}
	}

	//process the image so it matches the map when overlaid
	//-> image center must equal gps track center
	//-> unless image size is already perfectly matching track min/max, we'll need user input to re-scale it
	double bg_scale_w = mlt_properties_get_double( properties, "bg_scale_w");
	if (!pdata->bg_img.isNull() && (new_img_loaded || bg_scale_w != pdata->bg_img_scaled_width)) //only update if something changed
	{
		double map_ar = pdata->map_aspect_ratio_from_distance; //mlt_properties_get_double(properties, "map_original_aspect_ratio");
		double bg_ar = (double)pdata->bg_img.width() / pdata->bg_img.height();
		double bg_scale_h = bg_scale_w * bg_ar / map_ar;
		pdata->bg_img_scaled_width = bg_scale_w;
		pdata->bg_img_scaled_height = bg_scale_h;
		// mlt_log_info(filter, "scale w=%f -> h=%f; bg_ar=%f, map_ar=%f", bg_scale_w, bg_scale_h, bg_ar, map_ar);

		//step 1) scale image width or height by map_aspect_ratio so now they both have the same aspect_ratio
		double new_bg_width = pdata->bg_img.width();
		double new_bg_height = pdata->bg_img.height();
		if (map_ar > 1)
			new_bg_width *= map_ar;
		else
			new_bg_height /= map_ar;
		pdata->bg_img_scaled = pdata->bg_img.scaled(new_bg_width, new_bg_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		// mlt_log_info(filter, "scaled image= x,y w,h= %d,%d %d,%d", pdata->bg_img_scaled.rect().x(), pdata->bg_img_scaled.rect().y(), pdata->bg_img_scaled.rect().width(), pdata->bg_img_scaled.rect().height());
		// pdata->bg_img_scaled.save("qimage_saved02_scaled_by_mapAR.jpg");

		//next steps are in prepare_canvas() as they depend on each frame position (used_crops)
	}
	
//write properties
	mlt_properties_set(properties, "gps_start_text", gps_start_text);
	mlt_properties_set(properties, "video_start_text", video_start_text);
	mlt_properties_set_int(properties, "auto_gps_offset_start", (pdata->first_gps_time - original_video_time)/1000);
	mlt_properties_set_int(properties, "auto_gps_offset_now", (pdata->first_gps_time - get_current_frame_time_ms(filter, frame))/1000);
}

//checks if a new file is selected and if so, does private_data cleanup and calls qxml_parse_file 
//+ computes some one time pdata values
static void process_file(mlt_filter filter, mlt_frame frame)
{
	private_data* pdata = (private_data*)filter->child;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	char* filename = mlt_properties_get( properties, "resource");
	bool guess_offset = (mlt_properties_get_int(properties, "time_offset") == 0) && (strlen(pdata->last_filename) == 0);

	//if there's no file selected just return
	if ( !filename || !strcmp(filename, "") )
		return;

	//check if the file has been changed, if not, current data is ok, do nothing
	if (strcmp(pdata->last_filename, filename))
	{
		// mlt_log_info(filter, "Reading new file: last_filename (%s) != entered_filename (%s), swap_180 = %d \n", pdata->last_filename, filename, swap);
		default_priv_data(pdata);
		strcpy(pdata->last_filename, filename);

		if (qxml_parse_file(filter_to_gps_data(filter)) == 1) 
		{
			get_first_gps_time(filter_to_gps_data(filter));
			get_last_gps_time(filter_to_gps_data(filter));

			//when loading the first file, sync gps start with video start
			int64_t original_video_time = get_original_video_file_time_mseconds(frame);
			if (guess_offset)
			{
				pdata->gps_offset = pdata->first_gps_time - original_video_time;
				mlt_properties_set_int(properties, "time_offset", pdata->gps_offset/1000);
			}

			//assume smooth is 5 (default) so we can guarantee *gps_points_p and min/max allocation
			pdata->last_smooth_lvl = 5;
			process_gps_smoothing(filter_to_gps_data(filter), 1);
			find_minmax_of_data(filter);
		}
		else {
			default_priv_data(pdata);
			//store name in pdata or it'll retry reading every frame if bad file
			strcpy(pdata->last_filename, filename);
		}
	}
}

/** Filter processing.
*/
static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	private_data* pdata = (private_data*)filter->child;

	process_file(filter, frame);
	if (pdata->gps_points_r == NULL || pdata->gps_points_size == 0) {
		return frame;
	}
	process_filter_properties(filter, frame);

	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

static void filter_close( mlt_filter filter )
{
	private_data* pdata = (private_data*)filter->child;

	default_priv_data(pdata);
	free( pdata );

	filter->child = NULL;
	filter->close = NULL;
	filter->parent.close = NULL;
	mlt_service_close( &filter->parent );
}

/** Constructor for the filter.
*/
extern "C" {
mlt_filter filter_gpsgraphic_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if ( filter && pdata && createQApplicationIfNeeded( MLT_FILTER_SERVICE(filter) ) )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set_string( properties, "resource", arg );
		//sync stuff
		mlt_properties_set_int( properties, "time_offset", 0);
		mlt_properties_set_int( properties, "smoothing_value", 5);
		mlt_properties_set_double( properties, "speed_multiplier", 1);
		//graph data stuff
		mlt_properties_set_int( properties, "graph_data_source", 2);
		mlt_properties_set_int( properties, "graph_type", 0);
		mlt_properties_set_double ( properties, "trim_start_p", 0);
		mlt_properties_set_double ( properties, "trim_end_p", 100);
		mlt_properties_set_int( properties, "crop_mode_h", 0);
		mlt_properties_set_double ( properties, "crop_left_p", 0);
		mlt_properties_set_double ( properties, "crop_right_p", 100);
		mlt_properties_set_int( properties, "crop_mode_v", 0);
		mlt_properties_set_double ( properties, "crop_bot_p", 0);
		mlt_properties_set_double ( properties, "crop_top_p", 100);
		//graph design stuff
		mlt_properties_set_int( properties, "color_style", 1 );
		mlt_properties_set( properties, "color.1", "#ff00aaff" );
		mlt_properties_set( properties, "color.2", "#ff00e000" );
		mlt_properties_set( properties, "color.3", "#ffffff00" );
		mlt_properties_set( properties, "color.4", "#ffff8c00" );
		mlt_properties_set( properties, "color.5", "#ffff0000" );
		mlt_properties_set_int(properties, "show_now_dot", 1);
		mlt_properties_set( properties, "now_dot_color", "#00ffffff");
		mlt_properties_set_int ( properties, "show_now_text", 1);
		mlt_properties_set_double ( properties, "angle", 0);
		mlt_properties_set_int( properties, "thickness", 5 );
		mlt_properties_set( properties, "rect", "10% 10% 30% 30%" );
		mlt_properties_set_int(properties, "show_grid", 0);
		mlt_properties_set( properties, "legend_unit", "" );
		mlt_properties_set_int( properties, "draw_individual_dots", 0 );
		//background image
		mlt_properties_set(properties, "map_coords_hint", "<no location file processed>");
		mlt_properties_set(properties, "bg_img_path", "");
		mlt_properties_set_double(properties, "bg_scale_w", 1);
		mlt_properties_set_double(properties, "bg_opacity", 1);

		filter->close = filter_close;
		filter->process = filter_process;
		filter->child = pdata;
	}
	else
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Filter gps graphic failed\n" );

		if( filter )
		{
			mlt_filter_close( filter );
		}

		if( pdata )
		{
			free( pdata );
		}

		filter = NULL;
	}
	return filter;
}
}
