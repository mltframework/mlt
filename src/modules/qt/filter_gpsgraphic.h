#ifndef GPS_GRAPHIC_H
#define GPS_GRAPHIC_H

//only using get_graph_colors() from graph.h
#include "gps_parser.h"
#include "graph.h"
#include <framework/mlt.h>
#include <QPainterPath>

struct s_crops
{
    double trim_start_p, trim_end_p;
    double bot, top, left, right;
    int64_t min_crop_time, max_crop_time;
    int start_index, end_index;
};

struct s_gps_data_bounds
{
    double min_lat, max_lat, min_lon, max_lon;
    double min_ele, max_ele, min_speed, max_speed;
    double min_hr, max_hr;

    void set_defaults()
    {
        min_lat = 90, min_lon = 180, max_lat = -90, max_lon = -180;
        min_ele = 99999, max_ele = -99999, min_speed = 99999, max_speed = -99999;
        min_hr = 99999, max_hr = 0;
    }
};

namespace {

typedef struct
{
    gps_point_raw *gps_points_r;  //raw gps data from file
    gps_point_proc *gps_points_p; //processed gps data
    int gps_points_size;
    int last_smooth_lvl;
    int last_searched_index; //used to optimize repeated searches
    int64_t first_gps_time, last_gps_time;
    int64_t gps_offset;
    double speed_multiplier;
    char last_filename[256]; //gps file fullpath
    char interpolated;
    s_gps_data_bounds minmax;
    s_crops ui_crops;
    int graph_data_source, graph_type;
    mlt_rect img_rect;
    char last_bg_img_path[256];
    double map_aspect_ratio_from_distance;
    QImage bg_img, bg_img_scaled;
    double bg_img_scaled_width, bg_img_scaled_height;
    QRectF bg_img_matched_rect;
    int swap_180;
} private_data;

} // namespace

typedef enum {
    gspg_location_src = 0,
    gpsg_altitude_src,
    gpsg_hr_src,
    gpsg_speed_src
} gpsg_data_sources;

typedef enum {
    gpsg_latitude_id = gspg_location_src,
    gpsg_longitude_id = gpsg_latitude_id + 100
} gpsg_data_subids;

typedef enum {
    gpsg_2d_map_graph = 0,
    gpsg_crop_center_graph,
    gpsg_speedometer_graph
    //gpsg_ladder_graph, 	//was thinking a vertical ruler/gauge thing for altitude, couldn't design something that actually looks good
    //gpsg_compass_graph  	//probably easy to adapt from speedometer but not really that important to be worth the extra code
} gpsg_graph_types;

typedef enum {
    gpsg_color_by_solid = 0,
    gpsg_color_by_solid_past_future,
    gpsg_color_by_solid_past,
    gpsg_color_by_solid_future,
    gpsg_color_by_vertical_gradient,
    gpsg_color_by_horizontal_gradient,
    gpsg_color_by_duration,
    gpsg_color_by_altitude,
    gpsg_color_by_hr,
    gpsg_color_by_speed
} gpsg_color_styles;

struct s_base_crops
{
    double bot, top, left, right;
};

struct point_2d
{
    double x, y;
};

/**** functions in filter_gpsgraphic.cpp needed in gps_drawing.cpp ****/

//apply crop percentages to [min, max] then compute value val as a percentage of [new_min, new_max]
//if inside_only is true, result is always inside interval [0..1]
template<typename T>
double crop_and_normalize(T val, T min, T max, double p_min, double p_max, bool inside_only = false)
{
    double ret = 0;
    T delta = max - min;
    T new_min = min + p_min * delta / 100.0;
    T new_max = min + delta * p_max / 100.0;
    if (new_max == new_min) //divide by zero -> draw a constant line
        ret = 0.5;
    else
        ret = (double) (val - new_min) / (new_max - new_min);
    // mlt_log_info(NULL, "crop_and_normalize (val=%f, min=%f, max=%f, p_min=%f, p_max=%f) result=%f [restricted to 0..1]\n", val, min, max, p_min, p_max, ret);
    if (inside_only)
        ret = CLAMP(ret, 0, 1);
    return ret;
}
//convert_[distance|speed]_to_format but graph data source aware
double convert_bysrc_to_format(mlt_filter filter, double val);
//like decimals_needed() but graph data source aware
int decimals_needed_bysrc(mlt_filter filter, double v);
/* Gets the value from pdata->minmax (or custom|existing gps_point) depending on data_src value
   get_type represents: 0=current_value, 1=max_value, -1=min_value
   subtype is used for longitude
   gps_p is used to get (only for get_type=0) the proper value by source from points not in pdata->gps_points_p[]
*/
double get_by_src(
    mlt_filter filter, int get_type, int i_gps = 0, int subtype = 0, gps_point_proc *gps_p = NULL);
//get_by_src shortcuts
double get_min_bysrc(mlt_filter filter, int subtype = 0);
double get_max_bysrc(mlt_filter filter, int subtype = 0);
double get_crtval_bysrc(mlt_filter filter, int i_gps, int subtype = 0, gps_point_proc *gps_p = NULL);
//returns the next gps point with a valid value for crt_source (starting with crt_i+1)
int get_next_valid_gpspoint_index(mlt_filter filter, int crt_i);
//gets the nearest gps point [index] according to video time + input offset
int get_now_gpspoint_index(mlt_filter filter, mlt_frame frame, bool force_result = true);
//returns an interpolated gps point at the exact current video time + offset
gps_point_proc get_now_weighted_gpspoint(mlt_filter filter,
                                         mlt_frame frame,
                                         bool force_result = true);
gps_private_data filter_to_gps_data(mlt_filter filter);

/**** functions in gps_drawing.cpp needed in filter_gpsgraphic.cpp ****/

//draws 5 horizontal (+5 vertical for 2D map) with small text for each line showing the graph value at that point
void draw_legend_grid(mlt_filter filter, mlt_frame frame, QPainter &p, s_base_crops &used_crops);
//inits drawing rect +clipping, antialiasing, and applies rotate
void prepare_canvas(mlt_filter filter,
                    mlt_frame frame,
                    QImage *qimg,
                    QPainter &p,
                    int width,
                    int height,
                    s_base_crops &used_crops);
//draws a small dot/disc on the map/graph according to the current video time + sync
void draw_now_dot(mlt_filter filter, mlt_frame frame, QPainter &p, s_base_crops &used_crops);
//draws a speedometer from the current data source (for map it displays % of total)
void draw_main_speedometer(mlt_filter filter,
                           mlt_frame frame,
                           QPainter &p,
                           s_base_crops &used_crops);
//draws a 2d graph of the chosen graph source (for non-location, second axis is time)
void draw_main_line_graph(mlt_filter filter, mlt_frame frame, QPainter &p, s_base_crops &used_crops);

#endif
