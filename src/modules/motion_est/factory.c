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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <limits.h>
#include <framework/mlt.h>

extern mlt_filter filter_motion_est_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_vismv_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_crop_detect_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_autotrack_rectangle_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_slowmotion_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/motion_est/%s", mlt_environment( "MLT_DATA" ), (char*) data );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	MLT_REGISTER( filter_type, "motion_est", filter_motion_est_init );
	MLT_REGISTER( filter_type, "vismv", filter_vismv_init );
	MLT_REGISTER( filter_type, "crop_detect", filter_crop_detect_init );
	MLT_REGISTER( filter_type, "autotrack_rectangle", filter_autotrack_rectangle_init );
	MLT_REGISTER( producer_type, "slowmotion", producer_slowmotion_init );

	MLT_REGISTER_METADATA( filter_type, "motion_est", metadata, "filter_motion_est.yml" );
	MLT_REGISTER_METADATA( filter_type, "vismv", metadata, "filter_vismv.yml" );
	MLT_REGISTER_METADATA( filter_type, "crop_detect", metadata, "filter_crop_detect.yml" );
	MLT_REGISTER_METADATA( filter_type, "autotrack_rectangle", metadata, "filter_autotrack_rectangle.yml" );
	MLT_REGISTER_METADATA( producer_type, "slowmotion", metadata, "producer_slowmotion.yml" );
}
