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
#include <framework/mlt.h>

extern mlt_filter filter_motion_est_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_vismv_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_crop_detect_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_autotrack_rectangle_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_slowmotion_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

void *mlt_create_filter( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	if ( !strcmp( id, "motion_est" ) )
		return filter_motion_est_init( profile, type, id, arg );
	if ( !strcmp( id, "vismv" ) )
		return filter_vismv_init( profile, type, id, arg );
	if ( !strcmp( id, "crop_detect" ) )
		return filter_crop_detect_init( profile, type, id, arg );
	if ( !strcmp( id, "autotrack_rectangle" ) )
		return filter_autotrack_rectangle_init( profile, type, id, arg );
	return NULL;
}

void *mlt_create_producer( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	if ( !strcmp( id, "slowmotion" ) )
		return producer_slowmotion_init( profile, type, id, arg );
	return NULL;
}
