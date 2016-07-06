/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2015 Meltytech, LLC
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

#include <string.h>
#include <limits.h>
#include <framework/mlt.h>

extern mlt_consumer consumer_blipflash_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_affine_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_charcoal_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_dynamictext_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_dynamic_loudness_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_lift_gamma_gain_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_loudness_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_loudness_meter_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_lumakey_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_invert_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_rgblut_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_sepia_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_blipflash_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_count_init( const char *arg );
extern mlt_transition transition_affine_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

#ifdef USE_FFTW
extern mlt_filter filter_dance_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_fft_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
#endif

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/plus/%s", mlt_environment( "MLT_DATA" ), (char*) data );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	MLT_REGISTER( consumer_type, "blipflash", consumer_blipflash_init );
	MLT_REGISTER( filter_type, "affine", filter_affine_init );
	MLT_REGISTER( filter_type, "charcoal", filter_charcoal_init );
	MLT_REGISTER( filter_type, "dynamictext", filter_dynamictext_init );
	MLT_REGISTER( filter_type, "dynamic_loudness", filter_dynamic_loudness_init );
	MLT_REGISTER( filter_type, "invert", filter_invert_init );
	MLT_REGISTER( filter_type, "lift_gamma_gain", filter_lift_gamma_gain_init );
	MLT_REGISTER( filter_type, "loudness", filter_loudness_init );
	MLT_REGISTER( filter_type, "loudness_meter", filter_loudness_meter_init );
	MLT_REGISTER( filter_type, "lumakey", filter_lumakey_init );
	MLT_REGISTER( filter_type, "rgblut", filter_rgblut_init );
	MLT_REGISTER( filter_type, "sepia", filter_sepia_init );
	MLT_REGISTER( producer_type, "blipflash", producer_blipflash_init );
	MLT_REGISTER( producer_type, "count", producer_count_init );
	MLT_REGISTER( transition_type, "affine", transition_affine_init );
#ifdef USE_FFTW
	MLT_REGISTER( filter_type, "dance", filter_dance_init );
	MLT_REGISTER( filter_type, "fft", filter_fft_init );
#endif

	MLT_REGISTER_METADATA( consumer_type, "blipflash", metadata, "consumer_blipflash.yml" );
	MLT_REGISTER_METADATA( filter_type, "affine", metadata, "filter_affine.yml" );
	MLT_REGISTER_METADATA( filter_type, "charcoal", metadata, "filter_charcoal.yml" );
	MLT_REGISTER_METADATA( filter_type, "dynamictext", metadata, "filter_dynamictext.yml" );
	MLT_REGISTER_METADATA( filter_type, "dynamic_loudness", metadata, "filter_dynamic_loudness.yml" );
	MLT_REGISTER_METADATA( filter_type, "invert", metadata, "filter_invert.yml" );
	MLT_REGISTER_METADATA( filter_type, "lift_gamma_gain", metadata, "filter_lift_gamma_gain.yml" );
	MLT_REGISTER_METADATA( filter_type, "loudness", metadata, "filter_loudness.yml" );
	MLT_REGISTER_METADATA( filter_type, "loudness_meter", metadata, "filter_loudness_meter.yml" );
	MLT_REGISTER_METADATA( filter_type, "lumakey", metadata, "filter_lumakey.yml" );
	MLT_REGISTER_METADATA( filter_type, "rgblut", metadata, "filter_rgblut.yml" );
	MLT_REGISTER_METADATA( filter_type, "sepia", metadata, "filter_sepia.yml" );
	MLT_REGISTER_METADATA( producer_type, "blipflash", metadata, "producer_blipflash.yml" );
	MLT_REGISTER_METADATA( producer_type, "count", metadata, "producer_count.yml" );
	MLT_REGISTER_METADATA( transition_type, "affine", metadata, "transition_affine.yml" );
#ifdef USE_FFTW
	MLT_REGISTER_METADATA( filter_type, "dance", metadata, "filter_dance.yml" );
	MLT_REGISTER_METADATA( filter_type, "fft", metadata, "filter_fft.yml" );
#endif
}
