/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt.h>
#include <string.h>
#include <limits.h>

extern mlt_consumer consumer_null_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_audiochannels_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_audioconvert_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_audiowave_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_brightness_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_channelcopy_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_crop_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_data_feed_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_data_show_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_gamma_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_greyscale_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_imageconvert_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_luma_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_mirror_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_mono_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_obscure_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_panner_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_region_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_rescale_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_resize_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_transition_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_watermark_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_colour_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_consumer_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_loader_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_hold_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_noise_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_ppm_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
#include "transition_composite.h"
extern mlt_transition transition_luma_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_transition transition_mix_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
#include "transition_region.h"

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	const char *service_type = NULL;
	switch ( type )
	{
		case filter_type:
			service_type = "filter";
			break;
		case producer_type:
			service_type = "producer";
			break;
		case transition_type:
			service_type = "transition";
			break;
		default:
			return NULL;
	}
	if ( !strcmp( id, "grayscale" ) )
		id = "greyscale";
	else if ( !strcmp( id, "color" ) )
		id = "colour";
	else if ( !strcmp( id, "channelswap" ) )
		id = "channelcopy";
	snprintf( file, PATH_MAX, "%s/core/%s_%s.yml", mlt_environment( "MLT_DATA" ), service_type, id );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	MLT_REGISTER( consumer_type, "null", consumer_null_init );
	MLT_REGISTER( filter_type, "audiochannels", filter_audiochannels_init );
	MLT_REGISTER( filter_type, "audioconvert", filter_audioconvert_init );
	MLT_REGISTER( filter_type, "audiowave", filter_audiowave_init );
	MLT_REGISTER( filter_type, "brightness", filter_brightness_init );
	MLT_REGISTER( filter_type, "channelcopy", filter_channelcopy_init );
    MLT_REGISTER( filter_type, "channelswap", filter_channelcopy_init );
    MLT_REGISTER( filter_type, "crop", filter_crop_init );
	MLT_REGISTER( filter_type, "data_feed", filter_data_feed_init );
	MLT_REGISTER( filter_type, "data_show", filter_data_show_init );
	MLT_REGISTER( filter_type, "gamma", filter_gamma_init );
	MLT_REGISTER( filter_type, "greyscale", filter_greyscale_init );
	MLT_REGISTER( filter_type, "grayscale", filter_greyscale_init );
	MLT_REGISTER( filter_type, "imageconvert", filter_imageconvert_init );
	MLT_REGISTER( filter_type, "luma", filter_luma_init );
	MLT_REGISTER( filter_type, "mirror", filter_mirror_init );
	MLT_REGISTER( filter_type, "mono", filter_mono_init );
	MLT_REGISTER( filter_type, "obscure", filter_obscure_init );
	MLT_REGISTER( filter_type, "panner", filter_panner_init );
	MLT_REGISTER( filter_type, "region", filter_region_init );
	MLT_REGISTER( filter_type, "rescale", filter_rescale_init );
	MLT_REGISTER( filter_type, "resize", filter_resize_init );
	MLT_REGISTER( filter_type, "transition", filter_transition_init );
	MLT_REGISTER( filter_type, "watermark", filter_watermark_init );
	MLT_REGISTER( producer_type, "abnormal", producer_loader_init );
	MLT_REGISTER( producer_type, "color", producer_colour_init );
	MLT_REGISTER( producer_type, "colour", producer_colour_init );
	MLT_REGISTER( producer_type, "consumer", producer_consumer_init );
	MLT_REGISTER( producer_type, "loader", producer_loader_init );
	MLT_REGISTER( producer_type, "hold", producer_hold_init );
	MLT_REGISTER( producer_type, "noise", producer_noise_init );
	MLT_REGISTER( producer_type, "ppm", producer_ppm_init );
	MLT_REGISTER( transition_type, "composite", transition_composite_init );
	MLT_REGISTER( transition_type, "luma", transition_luma_init );
	MLT_REGISTER( transition_type, "mix", transition_mix_init );
	MLT_REGISTER( transition_type, "region", transition_region_init );

	MLT_REGISTER_METADATA( filter_type, "audiowave", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "brightness", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "channelcopy", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "channelswap", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "crop", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "data_show", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "gamma", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "greyscale", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "grayscale", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "luma", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "mirror", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "mono", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "obscure", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "region", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "transition", metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "watermark", metadata, NULL );
	MLT_REGISTER_METADATA( producer_type, "colour", metadata, NULL );
	MLT_REGISTER_METADATA( producer_type, "color", metadata, NULL );
	MLT_REGISTER_METADATA( producer_type, "consumer", metadata, NULL );
	MLT_REGISTER_METADATA( producer_type, "hold", metadata, NULL );
	MLT_REGISTER_METADATA( producer_type, "noise", metadata, NULL );
	MLT_REGISTER_METADATA( transition_type, "composite", metadata, NULL );
	MLT_REGISTER_METADATA( transition_type, "luma", metadata, NULL );
	MLT_REGISTER_METADATA( transition_type, "mix", metadata, NULL );
	MLT_REGISTER_METADATA( transition_type, "region", metadata, NULL );
}
