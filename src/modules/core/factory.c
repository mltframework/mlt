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

#include <string.h>

#include "producer_colour.h"
#include "producer_framebuffer.h"
#include "producer_noise.h"
#include "producer_ppm.h"
#include "filter_boxblur.h"
#include "filter_brightness.h"
#include "filter_channelcopy.h"
#include "filter_data.h"
#include "filter_gamma.h"
#include "filter_greyscale.h"
#include "filter_luma.h"
#include "filter_mirror.h"
#include "filter_mono.h"
#include "filter_obscure.h"
#include "filter_rescale.h"
#include "filter_resize.h"
#include "filter_region.h"
#include "filter_transition.h"
#include "filter_watermark.h"
#include "filter_wave.h"
#include "transition_composite.h"
#include "transition_luma.h"
#include "transition_mix.h"
#include "transition_region.h"
#include "consumer_null.h"

void *mlt_create_producer( char *id, void *arg )
{
	if ( !strcmp( id, "color" ) )
		return producer_colour_init( arg );
	if ( !strcmp( id, "colour" ) )
		return producer_colour_init( arg );
	if ( !strcmp( id, "framebuffer" ) )
		return producer_framebuffer_init( arg );
	if ( !strcmp( id, "noise" ) )
		return producer_noise_init( arg );
	if ( !strcmp( id, "ppm" ) )
		return producer_ppm_init( arg );
	return NULL;
}

void *mlt_create_filter( char *id, void *arg )
{
	if ( !strcmp( id, "boxblur" ) )
		return filter_boxblur_init( arg );
	if ( !strcmp( id, "brightness" ) )
		return filter_brightness_init( arg );
	if ( !strcmp( id, "channelcopy" ) )
		return filter_channelcopy_init( arg );
	if ( !strcmp( id, "data_feed" ) )
		return filter_data_feed_init( arg );
	if ( !strcmp( id, "data_show" ) )
		return filter_data_show_init( arg );
	if ( !strcmp( id, "gamma" ) )
		return filter_gamma_init( arg );
	if ( !strcmp( id, "greyscale" ) )
		return filter_greyscale_init( arg );
	if ( !strcmp( id, "luma" ) )
		return filter_luma_init( arg );
	if ( !strcmp( id, "mirror" ) )
		return filter_mirror_init( arg );
	if ( !strcmp( id, "mono" ) )
		return filter_mono_init( arg );
	if ( !strcmp( id, "obscure" ) )
		return filter_obscure_init( arg );
	if ( !strcmp( id, "region" ) )
		return filter_region_init( arg );
	if ( !strcmp( id, "rescale" ) )
		return filter_rescale_init( arg );
	if ( !strcmp( id, "resize" ) )
		return filter_resize_init( arg );
	else if ( !strcmp( id, "transition" ) )
		return filter_transition_init( arg );
	if ( !strcmp( id, "watermark" ) )
		return filter_watermark_init( arg );
	if ( !strcmp( id, "wave" ) )
		return filter_wave_init( arg );
	return NULL;
}

void *mlt_create_transition( char *id, void *arg )
{
	if ( !strcmp( id, "composite" ) )
		return transition_composite_init( arg );
	if ( !strcmp( id, "luma" ) )
		return transition_luma_init( arg );
	if ( !strcmp( id, "mix" ) )
		return transition_mix_init( arg );
	if ( !strcmp( id, "region" ) )
		return transition_region_init( arg );
	return NULL;
}

void *mlt_create_consumer( char *id, void *arg )
{
	if ( !strcmp( id, "null" ) )
		return consumer_null_init( arg );
	return NULL;
}
