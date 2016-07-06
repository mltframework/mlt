/*
 * Copyright (C) 2013 Dan Dennedy <dan@dennedy.org>
 * factory.c -- the factory method interfaces
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <limits.h>
#include <framework/mlt.h>



extern mlt_consumer consumer_xgl_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_glsl_manager_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_blur_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_convert_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_crop_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_deconvolution_sharpen_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_diffusion_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_glow_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_lift_gamma_gain_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_mirror_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_opacity_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_rect_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_resample_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_resize_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_saturation_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_movit_vignette_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_white_balance_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_transition transition_movit_luma_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_transition transition_movit_mix_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_transition transition_movit_overlay_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/opengl/%s", mlt_environment( "MLT_DATA" ), (char*) data );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
#if !defined(__APPLE__) && !defined(_WIN32)
	MLT_REGISTER( consumer_type, "xgl", consumer_xgl_init );
#endif
	MLT_REGISTER( filter_type, "glsl.manager", filter_glsl_manager_init );
	MLT_REGISTER( filter_type, "movit.blur", filter_movit_blur_init );
	MLT_REGISTER( filter_type, "movit.convert", filter_movit_convert_init );
	MLT_REGISTER( filter_type, "movit.crop", filter_movit_crop_init );
	MLT_REGISTER( filter_type, "movit.diffusion", filter_movit_diffusion_init );
	MLT_REGISTER( filter_type, "movit.glow", filter_movit_glow_init );
	MLT_REGISTER( filter_type, "movit.lift_gamma_gain", filter_lift_gamma_gain_init );
	MLT_REGISTER( filter_type, "movit.mirror", filter_movit_mirror_init );
	MLT_REGISTER( filter_type, "movit.opacity", filter_movit_opacity_init );
	MLT_REGISTER( filter_type, "movit.rect", filter_movit_rect_init );
	MLT_REGISTER( filter_type, "movit.resample", filter_movit_resample_init );
	MLT_REGISTER( filter_type, "movit.resize", filter_movit_resize_init );
	MLT_REGISTER( filter_type, "movit.saturation", filter_movit_saturation_init );
	MLT_REGISTER( filter_type, "movit.sharpen", filter_deconvolution_sharpen_init );
	MLT_REGISTER( filter_type, "movit.vignette", filter_movit_vignette_init );
	MLT_REGISTER( filter_type, "movit.white_balance", filter_white_balance_init );
	MLT_REGISTER( transition_type, "movit.luma_mix", transition_movit_luma_init );
	MLT_REGISTER( transition_type, "movit.mix", transition_movit_mix_init );
	MLT_REGISTER( transition_type, "movit.overlay", transition_movit_overlay_init );

	MLT_REGISTER_METADATA( filter_type, "movit.blur", metadata, "filter_movit_blur.yml" );
	MLT_REGISTER_METADATA( filter_type, "movit.diffusion", metadata, "filter_movit_diffusion.yml" );
	MLT_REGISTER_METADATA( filter_type, "movit.glow", metadata, "filter_movit_glow.yml" );
	MLT_REGISTER_METADATA( filter_type, "movit.lift_gamma_gain", metadata, "filter_movit_lift_gamma_gain.yml" );
	MLT_REGISTER_METADATA( filter_type, "movit.mirror", metadata, "filter_movit_mirror.yml" );
	MLT_REGISTER_METADATA( filter_type, "movit.opacity", metadata, "filter_movit_opacity.yml" );
	MLT_REGISTER_METADATA( filter_type, "movit.rect", metadata, "filter_movit_rect.yml" );
	MLT_REGISTER_METADATA( filter_type, "movit.saturation", metadata, "filter_movit_saturation.yml" );
	MLT_REGISTER_METADATA( filter_type, "movit.sharpen", metadata, "filter_movit_deconvolution_sharpen.yml" );
	MLT_REGISTER_METADATA( filter_type, "movit.vignette", metadata, "filter_movit_vignette.yml" );
	MLT_REGISTER_METADATA( filter_type, "movit.white_balance", metadata, "filter_movit_white_balance.yml" );
	MLT_REGISTER_METADATA( transition_type, "movit.luma_mix", metadata, "transition_movit_luma.yml" );
	MLT_REGISTER_METADATA( transition_type, "movit.mix", metadata, "transition_movit_mix.yml" );
	MLT_REGISTER_METADATA( transition_type, "movit.overlay", metadata, "transition_movit_overlay.yml" );
}
