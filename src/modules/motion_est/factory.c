/*
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

#include <string.h>

#include "filter_motion_est.h"

extern mlt_filter filter_motion_est_init(char *);
extern mlt_filter filter_vismv_init(char *);
extern mlt_filter filter_crop_detect_init(char *);
extern mlt_filter filter_autotrack_rectangle_init(char *);

void *mlt_create_filter( char *id, void *arg )
{
	if ( !strcmp( id, "motion_est" ) )
		return filter_motion_est_init( arg );
	if ( !strcmp( id, "vismv" ) )
		return filter_vismv_init( arg );
	if ( !strcmp( id, "crop_detect" ) )
		return filter_crop_detect_init( arg );
	if ( !strcmp( id, "autotrack_rectangle" ) )
		return filter_autotrack_rectangle_init( arg );
	return NULL;
}
