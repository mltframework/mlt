/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#include <string.h>

#include "producer_ppm.h"
#include "filter_brightness.h"
#include "filter_deinterlace.h"
#include "filter_gamma.h"
#include "filter_luma.h"
#include "filter_greyscale.h"
#include "filter_obscure.h"
#include "filter_resize.h"
#include "filter_region.h"
#include "filter_volume.h"
#include "filter_watermark.h"
#include "producer_colour.h"
#include "transition_composite.h"
#include "transition_luma.h"
#include "transition_mix.h"
#include "transition_region.h"

void *mlt_create_producer( char *id, void *arg )
{
	if ( !strcmp( id, "colour" ) )
		return producer_colour_init( arg );
	if ( !strcmp( id, "ppm" ) )
		return producer_ppm_init( arg );
	return NULL;
}

void *mlt_create_filter( char *id, void *arg )
{
	if ( !strcmp( id, "brightness" ) )
		return filter_brightness_init( arg );
	if ( !strcmp( id, "deinterlace" ) )
		return filter_deinterlace_init( arg );
	if ( !strcmp( id, "gamma" ) )
		return filter_gamma_init( arg );
	if ( !strcmp( id, "greyscale" ) )
		return filter_greyscale_init( arg );
	if ( !strcmp( id, "luma" ) )
		return filter_luma_init( arg );
	if ( !strcmp( id, "obscure" ) )
		return filter_obscure_init( arg );
	if ( !strcmp( id, "region" ) )
		return filter_region_init( arg );
	if ( !strcmp( id, "resize" ) )
		return filter_resize_init( arg );
	if ( !strcmp( id, "volume" ) )
		return filter_volume_init( arg );
	if ( !strcmp( id, "watermark" ) )
		return filter_watermark_init( arg );
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
	return NULL;
}

