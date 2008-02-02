/*
 * Perform motion estimation
 * Zachary K Drew, Copyright 2004
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _FILTER_MOTION_EST_H_
#define _FILTER_MOTION_EST_H_

#include <framework/mlt_filter.h>

extern mlt_filter filter_motion_est_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

#define MAX_MSAD 0xffff

struct motion_vector_s
{
	int msad;	//<! Sum of Absolute Differences for this vector
	int dx;		//<! integer X displacement
	int dy;		//<! integer Y displacement
	int vert_dev;	//<! a measure of vertical color deviation
	int horiz_dev;	//<! a measure of horizontal color deviation
	int valid;
	int quality;
	uint8_t color;	//<! The color
};
	
typedef struct motion_vector_s motion_vector;
#endif
