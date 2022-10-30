/*
 * filter_drawing.cpp -- draws gps related graphics 
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
#include "filter_gpsgraphic.h"

//apply crop percentages to [min, max] then compute value at specific percentage
template <typename T> 
static double get_value_from_percent_after_crop (double perc, T min, T max, double p_min, double p_max) 
{
	double ret = 0;
	T delta = max-min;
	T new_min = min + p_min*delta/100.0;
	T new_max = min + delta*p_max/100.0;
	ret = (double)(new_min + perc*(new_max-new_min));
	// mlt_log_info(NULL, "get_value_from_percent_after_crop (perc=%f, min=%f, max=%f, p_min=%f, p_max=%f) rezult=%f [restricted to 0..1]\n", perc, min, max, p_min, p_max, ret);
	return ret;
}

//returns a 2d point of current source-type from gp, scaled to the rect area
static point_2d get_gpspoint_to_rect(mlt_filter filter, mlt_frame frame, gps_point_proc * gp, mlt_rect rect, s_base_crops used_crops)
{
	private_data* pdata = (private_data*)filter->child;
	point_2d tmp = {-1, -1};

	tmp.y = crop_and_normalize(get_crtval_bysrc(filter, 0, 0, gp), get_min_bysrc(filter), get_max_bysrc(filter), used_crops.bot, used_crops.top);
	if (pdata->graph_data_source == gspg_location_src) //if location, x-axis is longitude, else time
		tmp.x = crop_and_normalize(get_crtval_bysrc(filter, 0, gpsg_longitude_id, gp), get_min_bysrc(filter, gpsg_longitude_id), get_max_bysrc(filter, gpsg_longitude_id), used_crops.left, used_crops.right);
	else
		tmp.x = crop_and_normalize(gp->time, pdata->ui_crops.min_crop_time, pdata->ui_crops.max_crop_time, used_crops.left, used_crops.right);

	//scale point to rect area
	tmp.x = rect.x + tmp.x * rect.w;
	tmp.y = rect.y + rect.h - tmp.y * rect.h;
	return tmp;
}

//there isn't a Qt function for rect - line intersection check :(
static bool rect_intersects_line( QRectF rect, point_2d pt1, point_2d pt2)
{
	//if one point is inside the rect -> true
	if ( rect.contains(pt1.x, pt1.y) || rect.contains(pt2.x, pt2.y) )
		return true;

#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
	//check if each side of the rect intersects (bounded) with the line
	QLineF line = QLineF(pt1.x, pt1.y, pt2.x, pt2.y);
	if (line.intersects(QLineF(rect.topLeft(), rect.topRight()), NULL) == QLineF::BoundedIntersection)
		return true;
	if (line.intersects(QLineF(rect.bottomLeft(), rect.bottomRight()), NULL) == QLineF::BoundedIntersection)
		return true;
	if (line.intersects(QLineF(rect.topLeft(), rect.bottomLeft()), NULL) == QLineF::BoundedIntersection)
		return true;
	if (line.intersects(QLineF(rect.topRight(), rect.bottomRight()), NULL) == QLineF::BoundedIntersection)
		return true;
#else
	return true; //this is ok because rect clipping is enabled, it just skips the optimisation
#endif

	return false;
}

//get a smooth change in color when drawing individual lines for maps
//percentage is of the entire colors array, interpolation is done for the 2 nearby colors
static QColor interpolate_color_from_gradient(double p, QVector<QColor> &colors)
{
	QColor ret = Qt::black;
	if (colors.size() == 0)
		return ret;
	if (p == 1 || colors.size() == 1 )
		return colors[ colors.size() - 1]; 

	//the 2 colors we need to interpolate from
	int c1 = p * (colors.size()-1);
	c1 = CLAMP(c1, 0, colors.size()-1);
	int c2 = c1 + 1;
	c2 = CLAMP(c2, 0, colors.size()-1);
	double ratio = (p * (colors.size()-1)) - c1; //=the fractional part
	//Result = (color2 - color1) * ratio + color1
	#define interp(v1, v2, r) (v2-v1) * r + v1
	ret.setRed( interp(colors[c1].red(), colors[c2].red(), ratio) );
	ret.setGreen( interp(colors[c1].green(), colors[c2].green(), ratio) );
	ret.setBlue( interp(colors[c1].blue(), colors[c2].blue(), ratio) );
	ret.setAlpha( interp(colors[c1].alpha(), colors[c2].alpha(), ratio) );
	#undef interp
	return ret;
}

//draws 5 horizontal (+5 vertical for 2D map) with small text for each line showing the graph value at that point
void draw_legend_grid(mlt_filter filter, mlt_frame frame, QPainter &p, s_base_crops &used_crops)
{
	private_data* pdata = (private_data*)filter->child;
	mlt_rect rect = pdata->img_rect;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

	char* legend_unit = mlt_properties_get(properties, "legend_unit");
	int nr_legend_lines = 5;

	QPainterPath path_grid_lines;
	QPen pen_5lines;
	pen_5lines.setWidth(1);
	pen_5lines.setColor(Qt::white);
	QFont font = p.font();
	int text_size = MIN(rect.w, rect.h)/25;
	font.setPixelSize(text_size);
	p.setFont(font);
	p.setPen(pen_5lines);

	p.setClipping(false); //legend text goes a bit outside the rect
	for (int i=0; i<nr_legend_lines; i++) 
	{
		path_grid_lines.moveTo(rect.x, rect.y + rect.h - (rect.h / (nr_legend_lines-1) * i));
		//for each of the nr_legend_lines compute it's absolute value = percentage of min_max (after applying crop bot/top)
		double crt_val = get_value_from_percent_after_crop((double)i/(nr_legend_lines-1), get_min_bysrc(filter), get_max_bysrc(filter), used_crops.bot, used_crops.top);
		crt_val = convert_bysrc_to_format(filter, crt_val);
		p.drawText(path_grid_lines.currentPosition().x() + 3, path_grid_lines.currentPosition().y() - 3, QString::number(crt_val, 'f', decimals_needed_bysrc(filter, crt_val)) + legend_unit);
		path_grid_lines.lineTo(rect.x+rect.w, rect.y+rect.h - (rect.h/4*i));
	}

	//+vertical lines only for the map (longitude)
	if (pdata->graph_data_source == gspg_location_src)
	{
		for (int i=0; i<nr_legend_lines; i++) 
		{
			path_grid_lines.moveTo(rect.x + rect.w / (nr_legend_lines-1) * i, rect.y); 
			double crt_val = get_value_from_percent_after_crop((double)i/(nr_legend_lines-1), get_min_bysrc(filter, gpsg_longitude_id), get_max_bysrc(filter, gpsg_longitude_id), used_crops.left, used_crops.right);
			crt_val = swap_180_if_needed(crt_val);
			p.drawText(path_grid_lines.currentPosition().x() + 3, path_grid_lines.currentPosition().y() + text_size + 3, QString::number(crt_val, 'f', 6));
			path_grid_lines.lineTo(rect.x + rect.w / (nr_legend_lines-1) * i, rect.y+rect.h);		
		}
	}
	p.drawPath(path_grid_lines);
	p.setClipping(true);
}

//draws a small dot/disc on the map/graph according to the current video time + sync
void draw_now_dot(mlt_filter filter, mlt_frame frame, QPainter &p, s_base_crops &used_crops)
{
	private_data* pdata = (private_data*)filter->child;
	mlt_rect rect = pdata->img_rect;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	int thickness =  mlt_properties_get_int( properties, "thickness" );
	mlt_color dot_color = mlt_properties_get_color(properties, "now_dot_color");

	//disc with internal color = white and outer color=now dot color or last used for graph line
	QPen dot_pen = p.pen();
	dot_pen.setWidth( thickness / 1.5 ); //or fixed to 3px ?
	int disc_size = thickness * 1.5;
	if (dot_color.a != 0)
		dot_pen.setColor( QColor( dot_color.r, dot_color.g, dot_color.b, dot_color.a ) ); 
	p.setBrush( Qt::white );
	p.setPen( dot_pen );
	
	double px, py;
	bool inside_only = true; //the now_dot will never leave the rect area to help in visual sync
	gps_point_proc crt = get_now_weighted_gpspoint(filter, frame);
	if (get_crtval_bysrc(filter, 0, 0, &crt) != GPS_UNINIT)
	{
		py = crop_and_normalize(get_crtval_bysrc(filter, 0, 0, &crt), get_min_bysrc(filter), get_max_bysrc(filter), used_crops.bot, used_crops.top, inside_only);
		if (pdata->graph_data_source == gspg_location_src)
			px = crop_and_normalize(get_crtval_bysrc(filter, 0, gpsg_longitude_id, &crt), get_min_bysrc(filter, gpsg_longitude_id), get_max_bysrc(filter, gpsg_longitude_id), used_crops.left, used_crops.right, inside_only) ;
		else //time
			px = crop_and_normalize(crt.time, pdata->ui_crops.min_crop_time, pdata->ui_crops.max_crop_time, used_crops.left, used_crops.right, inside_only);
		
		QPointF disc;
		disc.setX( rect.x + px * rect.w );
		disc.setY( rect.y + rect.h - py * rect.h );
		p.setClipping(false);
		p.drawEllipse(disc, disc_size, disc_size);
		p.setClipping(true);
	}
}

//draws a 2d graph of the chosen graph source (for non-location, second axis is time)
void draw_main_line_graph(mlt_filter filter, mlt_frame frame, QPainter &p, s_base_crops &used_crops)
{
	private_data* pdata = (private_data*)filter->child;
	mlt_rect rect = pdata->img_rect;
	QRectF qrect = QRectF( rect.x, rect.y, rect.w, rect.h );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	int color_style = mlt_properties_get_int( properties, "color_style" ) ;
	int thickness = qAbs( mlt_properties_get_int( properties, "thickness" ) );
	int dots_only = mlt_properties_get_int( properties, "draw_individual_dots" );
	char* legend_unit = mlt_properties_get(properties, "legend_unit");
	QVector<QColor> colors = get_graph_colors( properties, position, length );
	QPen last_graph_pen;
	
	if (colors.size() < 2)
		colors.append(colors[0]);

	//original state
	p.save();

	//draw legend grid lines + text if enabled
	if ((pdata->graph_type == gpsg_2d_map_graph || pdata->graph_type == gpsg_crop_center_graph) && mlt_properties_get_int(properties, "show_grid"))
		draw_legend_grid(filter, frame, p, used_crops);

	int i_now = get_now_gpspoint_index(filter, frame);
	gps_point_proc gps_now = get_now_weighted_gpspoint(filter, frame);
	point_2d crt_pt = {-1, -1}, next_pt = {-1, -1};

	QPen pen_solid_color0;
	pen_solid_color0.setWidth( thickness );
	pen_solid_color0.setColor( colors[0]);
	pen_solid_color0.setCapStyle( Qt::RoundCap );
	last_graph_pen = pen_solid_color0;

	QPen pen_solid_color1;
	pen_solid_color1.setWidth( thickness );
	pen_solid_color1.setColor( colors[1]);
	pen_solid_color1.setCapStyle( Qt::RoundCap );

	QPen pen_thin_color1;
	//1px dots are too small, 1px line feels just too small and sometimes aliaseses weirdly at an angle
	//meanwhile, I've added the "Two colors" style if the future points/lines are too small, might reconsider this
	if (thickness > 2)
		pen_thin_color1.setWidth( 2 );
	else 
		pen_thin_color1.setWidth( 1 );
	pen_thin_color1.setColor( colors[1]);
	pen_thin_color1.setCapStyle( Qt::RoundCap );//( Qt::SquareCap ); // I don't remember what issue I had that made me use this, keeping it for now

	QPen pen_gradients;
	pen_gradients.setWidth( thickness );
	pen_gradients.setCapStyle( Qt::RoundCap );

	//enhancement TODO: if detected gap in GPS data draw a dashed line (but needs a bunch of extra changes)
	//pen_dash.setStyle( Qt::DashLine );

	//go through the [start_index..end_index] interval of gps_points_p and draw each pair of valid points
	for (int i=pdata->ui_crops.start_index; i<pdata->ui_crops.end_index; i++) 
	{
		int next_i = get_next_valid_gpspoint_index(filter, i);
		if (i == next_i || get_crtval_bysrc(filter, i) == GPS_UNINIT || get_crtval_bysrc(filter, next_i) == GPS_UNINIT)
		{
			// mlt_log_info(filter, "incomplete pair (i=%d, %d) GPS_UNINIT, skipping drawing", i, next_i);
			continue;
		}

		crt_pt = get_gpspoint_to_rect(filter, frame, &pdata->gps_points_p[i], rect, used_crops);
		next_pt = get_gpspoint_to_rect(filter, frame, &pdata->gps_points_p[next_i], rect, used_crops);
		
		//don't draw line at all if it is completely outside the rect
		if ( !rect_intersects_line(qrect, crt_pt, next_pt) )
			continue;

		//apply color style
		if (color_style == gpsg_color_by_solid)
		{
			p.setPen( pen_solid_color0 );
		}
		else if (color_style == gpsg_color_by_solid_past_future)
		{
			if (i <= i_now) 
			{
				p.setPen( pen_solid_color0 );
			}
			else if (i > i_now) 
				p.setPen( pen_solid_color1 );
		}
		else if ((color_style == gpsg_color_by_solid_past && i <= i_now) || (color_style == gpsg_color_by_solid_future && i > i_now))
		{
			p.setPen( pen_solid_color0 );
		}
		else if ((color_style == gpsg_color_by_solid_past && i > i_now) || (color_style == gpsg_color_by_solid_future && i <= i_now))
		{
			p.setPen( pen_thin_color1 );
		}
		else if (color_style == gpsg_color_by_vertical_gradient || color_style == gpsg_color_by_horizontal_gradient)
		{
			QLinearGradient gradient;
			gradient.setStart ( rect.x, rect.y );
			if (color_style == gpsg_color_by_vertical_gradient)
				gradient.setFinalStop ( rect.x, rect.y + rect.h );
			else
				gradient.setFinalStop ( rect.x + rect.w, rect.y );
			qreal step = 1.0 / ( colors.size() - 1 );
			for( int i = 0; i < colors.size(); i++ )
				gradient.setColorAt( (qreal)i * step, colors[i] );
			pen_gradients.setBrush( gradient );
			p.setPen( pen_gradients );
		}
		else if (color_style >= gpsg_color_by_duration && color_style <= gpsg_color_by_speed)
		{
			//compute current value as a percentage of min..max
			#define calc_perc(v, min, max) (double)(v-min) / ((max-min)!=0?(max-min):(v-min))
			double perc = 0;
			if (color_style == gpsg_color_by_duration) //this one is relative to trim, not entire gps track
				perc = calc_perc(pdata->gps_points_p[i].time, pdata->ui_crops.min_crop_time, pdata->ui_crops.max_crop_time);
			if (color_style == gpsg_color_by_altitude)
				perc = calc_perc(pdata->gps_points_p[i].ele, pdata->minmax.min_ele, pdata->minmax.max_ele);
			if (color_style == gpsg_color_by_hr)
				perc = calc_perc(pdata->gps_points_p[i].hr, pdata->minmax.min_hr, pdata->minmax.max_hr);
			if (color_style == gpsg_color_by_speed)
				perc = calc_perc(pdata->gps_points_p[i].speed, pdata->minmax.min_speed, pdata->minmax.max_speed);

			//assign the interpolated color at p% in the colors array
			pen_gradients.setColor( interpolate_color_from_gradient(perc, colors) );
			p.setPen( pen_gradients );
		}
		else {
			p.setPen ( pen_solid_color0 );
		}
		
		if (i == i_now)
			last_graph_pen = p.pen();

		//for the past/future segment we need to split it exactly at the now point into 2 different colors or it will look horrible if zoomed in enough
		if ( (i == i_now) && (color_style == gpsg_color_by_solid_past || color_style == gpsg_color_by_solid_future || color_style == gpsg_color_by_solid_past_future) )
		{
			point_2d now_pt = get_gpspoint_to_rect(filter, frame, &gps_now, rect, used_crops);

			//if we got a valid intermediary point for the current location, we'll draw the past/future with different styles
			if (get_crtval_bysrc(filter, 0, 0, &gps_now) != GPS_UNINIT)
			{
				//"past" sub-segment
				if (color_style == gpsg_color_by_solid_past || color_style == gpsg_color_by_solid_past_future)
					p.setPen( pen_solid_color0 );
				else if (color_style == gpsg_color_by_solid_future)
					p.setPen( pen_thin_color1 );

				if (dots_only)
					p.drawPoint(QPointF(crt_pt.x, crt_pt.y));
				else
					p.drawLine(QPointF(crt_pt.x, crt_pt.y), QPointF(now_pt.x, now_pt.y));

				//"future" sub-segment
				if (color_style == gpsg_color_by_solid_past)
					p.setPen( pen_thin_color1 );
				else if (color_style == gpsg_color_by_solid_future)
					p.setPen( pen_solid_color0 );
				else if (color_style == gpsg_color_by_solid_past_future)
					p.setPen( pen_solid_color1 );

				if (!dots_only)
					p.drawLine(QPointF(now_pt.x, now_pt.y), QPointF(next_pt.x, next_pt.y));
			}
			else
			{
				//if invalid point, consider the entire line "future"
				if (color_style == gpsg_color_by_solid_past)
					p.setPen( pen_thin_color1 );
				else if (color_style == gpsg_color_by_solid_future)
					p.setPen(pen_solid_color0 );
				else if (color_style == gpsg_color_by_solid_past_future)
					p.setPen(pen_solid_color1);

				if (dots_only)
					p.drawPoint(QPointF(crt_pt.x, crt_pt.y));
				else
					p.drawLine(QPointF(crt_pt.x, crt_pt.y), QPointF(next_pt.x, next_pt.y));
			}
		}
		else //= full segment lines not intersecting now_dot
		{
			//IMPORTANT: the function call without QPointF() cast loses precision due to int!! fun times debugging the small random wiggles from this one
			if (dots_only)
				p.drawPoint(QPointF(crt_pt.x, crt_pt.y));
			else
				p.drawLine(QPointF(crt_pt.x, crt_pt.y), QPointF(next_pt.x, next_pt.y));
		}
	}


	//draw the current value in the bot-right corner, big bold white text
	if (mlt_properties_get_int(properties, "show_now_text"))
	{
		double now_val = get_crtval_bysrc(filter, 0, 0, &gps_now);
		if (now_val != GPS_UNINIT)
		{
			QRectF now_text_rect = QRectF( rect.x, rect.y + rect.h/2, rect.w, rect.h/2 );
			QFont font = p.font();
			font.setPixelSize( rect.w/15 );
			font.setWeight( QFont::Bold );
			p.setFont(font);
			p.setPen( Qt::white );

			now_val = convert_bysrc_to_format(filter, now_val);
			QString now_text = QString::number(now_val, 'f', decimals_needed_bysrc(filter, now_val));
			if (pdata->graph_data_source == gspg_location_src) 
			{
				now_val = get_crtval_bysrc(filter, 0, gpsg_longitude_id, &gps_now);
				now_val = swap_180_if_needed(now_val);
				now_text += ", " + QString::number(now_val, 'f', decimals_needed_bysrc(filter, now_val));
			}
			now_text += QString(legend_unit);
			
			p.drawText(now_text_rect, Qt::AlignBottom | Qt::AlignRight, now_text);
		}
	}

	//restore to before legend + main_graph
	p.restore();
	//set color so by default the now_dot will be the same color as nearby graph
	p.setPen(last_graph_pen);

	//draw a circle for current point
	if (mlt_properties_get_int(properties, "show_now_dot") && pdata->graph_type <= 1)
		draw_now_dot(filter, frame, p, used_crops);
}

//draws a speedometer from the current data source (for map it displays % of total)
void draw_main_speedometer(mlt_filter filter, mlt_frame frame, QPainter &p, s_base_crops &used_crops)
{
	private_data* pdata = (private_data*)filter->child;
	mlt_rect rect = pdata->img_rect;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	// int color_style = mlt_properties_get_int( properties, "color_style" ) ;
	// int thickness = qAbs( mlt_properties_get_int( properties, "thickness" ) );
	int show_grid = mlt_properties_get_int(properties, "show_grid");
	QVector<QColor> colors = get_graph_colors( properties, position, length );

	if (colors.size() == 1)
		colors.append(colors[0]);

	//draw the divisions
	double canvas_angle = mlt_properties_get_double( properties, "angle" );
	const double max_angle = 240.0;
	const double start_angle = 90+30;
	const int nr_big_lines = 6;
	const int nr_small_lines = 3;
	int small_side = MIN(rect.w, rect.h);
	double big_line_width = small_side/10.0;
	double big_line_height = big_line_width/5.0;
	double big_angle = max_angle/nr_big_lines;
	double small_angle = big_angle/(nr_small_lines+1);
	QRectF big_line(0, -big_line_height/2, -big_line_width, big_line_height);
	QRectF small_line(0, big_line.y()/2, big_line.width()/2, big_line.height()/2);

	//legend = big line numbers
	QPen pen_speedo_nr;
	pen_speedo_nr.setWidth(1);
	pen_speedo_nr.setColor( Qt::white );
	QFont font = p.font();
	int text_size = big_line_height*3;
	font.setPixelSize(text_size);
	font.setWeight(QFont::Bold);
	p.setFont(font);
	p.setPen(pen_speedo_nr);

	p.translate(rect.x + rect.w/2, rect.y + rect.h/2);
	p.rotate(start_angle);
	for (int i=0; i<=nr_big_lines; i++)
	{
		//big lines
		p.rotate(i*big_angle);
		p.translate(small_side/2, 0);
		p.fillRect(big_line, colors[0]);
		//legend values
		if (show_grid)
		{
			p.save();

			double crt_val = 0;
			if (pdata->graph_data_source != gspg_location_src)
			{
				crt_val = get_value_from_percent_after_crop( (double)i/nr_big_lines, get_min_bysrc(filter), get_max_bysrc(filter), used_crops.bot, used_crops.top);
				crt_val = convert_bysrc_to_format(filter, crt_val);
			}
			else //just show a percentage of total for map
			{
				crt_val = 100.0*i/nr_big_lines;
			}
			//center & rotate the text: compute the text bounding box, translate to it's center, rotate, translate back and drawtext within the rect bounding box
			QString l_text = QString::number(crt_val, 'f', 0);
			int text_width = p.fontMetrics().horizontalAdvance(l_text) + p.fontMetrics().horizontalAdvance(" ");
			int text_height = p.fontMetrics().height();
			QRectF text_rect = QRectF(-text_width, -text_height/2, text_width, text_height);
			p.translate(-big_line_width, 0);
			p.translate(text_rect.center());
			p.rotate(-(canvas_angle + start_angle + i*big_angle));
			p.translate(-text_rect.center());
			p.drawText(text_rect, Qt::AlignCenter, l_text);

			p.restore();
		}

		// small lines
		p.translate(-small_side/2, 0);
		for (int j=1; j<=nr_small_lines && i!=nr_big_lines; j++) 
		{
			p.rotate(j*small_angle);
			p.translate(small_side/2, 0);
			p.fillRect(small_line, colors[1]);
			p.translate(-small_side/2, 0);
			p.rotate(-j*small_angle);
		}
		p.rotate(-i*big_angle);
	}

	//find out the needle's angle
	gps_point_proc gps_now = get_now_weighted_gpspoint(filter, frame);
	double now_value = 0, n_angle = 0;
	if (pdata->graph_data_source != gspg_location_src)
	{	
		if (get_crtval_bysrc(filter, 0, 0, &gps_now) != GPS_UNINIT)
		{
			n_angle = crop_and_normalize(get_crtval_bysrc(filter, 0, 0, &gps_now), get_min_bysrc(filter), get_max_bysrc(filter), used_crops.bot, used_crops.top, true);
			now_value = convert_bysrc_to_format(filter, get_crtval_bysrc(filter, 0, 0, &gps_now));
		}
	}
	else //just show a percentage of total
	{
		n_angle = crop_and_normalize(gps_now.time, pdata->ui_crops.min_crop_time, pdata->ui_crops.max_crop_time, used_crops.left, used_crops.right, true);
		now_value = n_angle*100;
	}

	p.rotate(n_angle*max_angle);

	//the needle considered as a "Now dot in UI"; just a long triangle
	if (mlt_properties_get_int(properties, "show_now_dot"))
	{
		mlt_color dot_color = mlt_properties_get_color(properties, "now_dot_color");
		if (dot_color.a == 0) // if transparent -> use main color
		{
			p.setBrush( colors[0] );
			p.setPen( colors[0] );
		}
		else 
		{
			p.setBrush( QColor( dot_color.r, dot_color.g, dot_color.b, dot_color.a ) );
			p.setPen( QColor( dot_color.r, dot_color.g, dot_color.b, dot_color.a ) );
		}
		double needle_base_len = big_line_width/8;
		QPointF needle[3] = {
			QPointF(-needle_base_len, -needle_base_len),
			QPointF(-needle_base_len, needle_base_len),
			QPointF(small_side/2.0, 0)
		};
		p.drawConvexPolygon(needle, 3);

		//small dot in center
		double needle_circle_len = big_line_width/2;
		p.drawEllipse(-needle_circle_len/2, -needle_circle_len/2, needle_circle_len, needle_circle_len);
	}

	p.rotate(-n_angle*max_angle);
	p.rotate(-start_angle);

	//draw big bold text in white here
	if (mlt_properties_get_int(properties, "show_now_text"))
	{	
		QRectF now_text_rect = QRectF(-small_side*0.25, 0, small_side*0.75, small_side/2-text_size*2);
		QString now_text = QString::number(now_value, 'f', decimals_needed(now_value));
		int now_text_size = text_size*3;
		font.setPixelSize(now_text_size);
		p.setFont(font);
		p.setPen( Qt::white );
		p.translate(now_text_rect.center());
		p.rotate(-canvas_angle);
		p.translate(-now_text_rect.center());
		p.drawText (now_text_rect, Qt::AlignBottom | Qt::AlignHCenter, now_text);

		//print the legend_unit with 1/3 smaller text size below
		QRectF now_text_unit_rect = QRectF(-small_side*0.25, small_side/2-2*text_size, small_side*0.75, text_size*2);
		QString now_text_unit = mlt_properties_get(properties, "legend_unit");
		font.setPixelSize(text_size);
		p.setFont(font);
		p.setPen( Qt::white );
		p.drawText (now_text_unit_rect, Qt::AlignTop | Qt::AlignHCenter, now_text_unit);
	}
}

//inits drawing rect +clipping, antialiasing, and applies rotate
void prepare_canvas(mlt_filter filter, mlt_frame frame, QImage* qimg, QPainter &p, int width, int height, s_base_crops &used_crops) 
{
	private_data* pdata = (private_data*)filter->child;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
	mlt_rect rect = mlt_properties_anim_get_rect( properties, "rect", position, length );

	//rect area and scaling stuff
	if ( strchr( mlt_properties_get( properties, "rect" ), '%' ) ) {
		rect.x *= qimg->width();
		rect.w *= qimg->width();
		rect.y *= qimg->height();
		rect.h *= qimg->height();
	}
	double scale = mlt_profile_scale_width(profile, width);
	rect.x *= scale;
	rect.w *= scale;
	scale = mlt_profile_scale_height(profile, height);
	rect.y *= scale;
	rect.h *= scale;

	pdata->img_rect = rect;
	QRectF r( rect.x, rect.y, rect.w, rect.h );

	// Apply rotation
	double angle = mlt_properties_get_double( properties, "angle" );
	if ( angle ) {
		p.translate( r.x() + r.width() / 2, r.y() + r.height() / 2 );
		p.rotate( angle );
		p.translate( -(r.x() + r.width() / 2), -(r.y() + r.height() / 2) );
	}

	p.setClipRect(r);

	//this almost doubles processing time if using large backgrounds (from 6-7ms to 10-11ms/frame) but massively improves drawImage smoothness (visible neighbor pixel jumps from frame to frame if disabled)
	p.setRenderHint(QPainter::SmoothPixmapTransform, true); 

	//draw background image (cropped and matched)
	if (pdata->bg_img_scaled_width!=0 && strlen(pdata->last_bg_img_path) > 0 && !pdata->bg_img.isNull())
	{
		//step 1 scales image to map aspect ratio (in process_filter_properties())
		//step 2) apply bg_scale_w/h to correctly match img to GPS track -> create rescaled_bg_rect instead of modifying image here
		double bg_width = pdata->bg_img_scaled.width();
		double bg_height = pdata->bg_img_scaled.height();

		double new_bg_width = bg_width * pdata->bg_img_scaled_width;
		double new_bg_height = bg_height * pdata->bg_img_scaled_height;
		double new_bg_x = (bg_width - new_bg_width) / 2;
		double new_bg_y = (bg_height - new_bg_height) / 2;
		QRectF rescaled_bg_rect = QRectF(new_bg_x, new_bg_y, new_bg_width, new_bg_height);

		//step 3) apply crop percentages to the rescaled_bg_rect to follow now_dot or user input
		QPointF top_left = QPointF(rescaled_bg_rect.x() + rescaled_bg_rect.width()*used_crops.left/100.0, rescaled_bg_rect.y() + rescaled_bg_rect.height()*(1-used_crops.top/100.0));
		QPointF bot_right = QPointF(rescaled_bg_rect.bottomRight().x() - rescaled_bg_rect.width()*(1-used_crops.right/100.0), rescaled_bg_rect.bottomRight().y()-rescaled_bg_rect.height()*used_crops.bot/100.0);
		QRectF crop_rect = QRectF(top_left, bot_right);
		// mlt_log_info(filter, "crop rect: x,y w,h= %f,%f %f,%f", crop_rect.x(), crop_rect.y(), crop_rect.width(), crop_rect.height());

		//step 4) crop the computed rect from the step 1) scaled image
		p.setOpacity( mlt_properties_get_double( properties, "bg_opacity") );
		p.drawImage(r, pdata->bg_img_scaled, crop_rect);
		p.setOpacity(1);
	}
	p.setRenderHint(QPainter::Antialiasing, true);
}
