/*
 * mlt_profile.c -- video output definition
 * Copyright (C) 2007 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "mlt_profile.h"
#include "mlt_factory.h"
#include "mlt_properties.h"

#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#define PROFILES_DIR "/share/mlt/profiles/"

static mlt_profile profile = NULL;

/** Get the current profile
* Builds one for PAL DV if non-existing
*/

mlt_profile mlt_profile_get( )
{
	if ( !profile )
	{
		profile = calloc( 1, sizeof( struct mlt_profile_s ) );
		if ( profile )
		{
			mlt_environment_set( "MLT_PROFILE", "dv_pal" );
			profile->description = strdup( "PAL 4:3 DV or DVD" );
			profile->frame_rate_num = 25;
			profile->frame_rate_den = 1;
			profile->width = 720;
			profile->height = 576;
			profile->progressive = 0;
			profile->sample_aspect_num = 59;
			profile->sample_aspect_den = 54;
			profile->display_aspect_num = 4;
			profile->display_aspect_den = 3;
		}
	}
	return profile;
}


/** Load a profile from the system folder
*/

mlt_profile mlt_profile_select( const char *name )
{
	const char *prefix = PREFIX;
	char *filename = calloc( 1, strlen( prefix ) + strlen( PROFILES_DIR ) + strlen( name ) + 1 );
	strcpy( filename, prefix );
	if ( filename[ strlen( filename ) - 1 ] != '/' )
		filename[ strlen( filename ) ] = '/';
	strcat( filename, PROFILES_DIR );
	strcat( filename, name );
	return mlt_profile_load_file( filename );
}

/** Load a profile from specific file
*/

mlt_profile mlt_profile_load_file( const char *file )
{
	// Load the profile as properties
	mlt_properties properties = mlt_properties_load( file );
	if ( properties && mlt_properties_get_int( properties, "width" ) )
	{
		mlt_profile_load_properties( properties );
		mlt_properties_close( properties );

		// Set MLT_PROFILE to basename
		char *filename = strdup( file );
		mlt_environment_set( "MLT_PROFILE", basename( filename ) );
		free( filename );
	}
	else
	{
		// Cleanup
		mlt_properties_close( properties );
		mlt_profile_close();
		// Failover
		mlt_profile_get();
	}

	// Set MLT_NORMALISATION to appease legacy modules
	char *profile_name = mlt_environment( "MLT_PROFILE" );
	if ( strstr( profile_name, "_ntsc" ) ||
	     strstr( profile_name, "_atsc" ) ||
	     strstr( profile_name, "_60i" ) ||
	     strstr( profile_name, "_30p" ) )
	{
		mlt_environment_set( "MLT_NORMALISATION", "NTSC" );
	}
	else if ( strstr( profile_name, "_pal" ) ||
	          strstr( profile_name, "_50i" ) ||
	          strstr( profile_name, "_25p" ) )
	{
		mlt_environment_set( "MLT_NORMALISATION", "PAL" );
	}

	return profile;
}

/** Load a profile from a properties object
*/

mlt_profile mlt_profile_load_properties( mlt_properties properties )
{
	mlt_profile_close();
	profile = calloc( 1, sizeof( struct mlt_profile_s ) );
	if ( profile )
	{
		if ( mlt_properties_get( properties, "name" ) )
			mlt_environment_set( "MLT_PROFILE", mlt_properties_get( properties, "name" ) );
		if ( mlt_properties_get( properties, "description" ) )
			profile->description = strdup( mlt_properties_get( properties, "description" ) );
		profile->frame_rate_num = mlt_properties_get_int( properties, "frame_rate_num" );
		profile->frame_rate_den = mlt_properties_get_int( properties, "frame_rate_den" );
		profile->width = mlt_properties_get_int( properties, "width" );
		profile->height = mlt_properties_get_int( properties, "height" );
		profile->progressive = mlt_properties_get_int( properties, "progressive" );
		profile->sample_aspect_num = mlt_properties_get_int( properties, "sample_aspect_num" );
		profile->sample_aspect_den = mlt_properties_get_int( properties, "sample_aspect_den" );
		profile->display_aspect_num = mlt_properties_get_int( properties, "display_aspect_num" );
		profile->display_aspect_den = mlt_properties_get_int( properties, "display_aspect_den" );
	}
	return profile;
}

/** Load an anonymous profile from string
*/

mlt_profile mlt_profile_load_string( const char *string )
{
	mlt_properties properties = mlt_properties_new();
	if ( properties )
	{
		const char *p = string;
		while ( p )
		{
			if ( strcmp( p, "" ) && p[ 0 ] != '#' )
				mlt_properties_parse( properties, p );
			p = strchr( p, '\n' );
			if ( p ) p++;
		}
	}
	return mlt_profile_load_properties( properties );
}

/** Get the framerate as float
*/

double mlt_profile_fps( mlt_profile aprofile )
{
	if ( aprofile )
		return ( double ) aprofile->frame_rate_num / aprofile->frame_rate_den;
	else
		return ( double ) mlt_profile_get()->frame_rate_num / mlt_profile_get()->frame_rate_den;
}

/** Get the sample aspect ratio as float
*/

double mlt_profile_sar( mlt_profile aprofile )
{
	if ( aprofile )
		return ( double ) aprofile->sample_aspect_num / aprofile->sample_aspect_den;
	else
		return ( double ) mlt_profile_get()->sample_aspect_num / mlt_profile_get()->sample_aspect_den;
}

/** Get the display aspect ratio as float
*/

double mlt_profile_dar( mlt_profile aprofile )
{
	if ( aprofile )
		return ( double ) aprofile->display_aspect_num / aprofile->display_aspect_den;
	else
		return ( double ) mlt_profile_get()->display_aspect_num / mlt_profile_get()->display_aspect_den;
}

/** Free up the global profile resources
*/

void mlt_profile_close( )
{
	if ( profile )
	{
		if ( profile->description )
			free( profile->description );
		profile->description = NULL;
		free( profile );
		profile = NULL;
	}
}
