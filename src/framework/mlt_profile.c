/**
 * \file mlt_profile.c
 * \brief video output definition
 * \see mlt_profile_s
 *
 * Copyright (C) 2007-2009 Ushodaya Enterprises Limited
 * \author Dan Dennedy <dan@dennedy.org>
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

#include "mlt.h"

#include <stdlib.h>
#include <string.h>
#include <libgen.h>


/** the default subdirectory of the prefix for holding profiles */
#define PROFILES_DIR "/share/mlt/profiles/"

/** Load a profile from the system folder.
 *
 * The environment variable MLT_PROFILES_PATH overrides the default \p PROFILES_DIR.
 *
 * \private \memberof mlt_profile_s
 * \param name the name of a profile settings file located in the standard location or
 * the full path name to a profile settings file
 * \return a profile or NULL on error
 */

static mlt_profile mlt_profile_select( const char *name )
{
	char *filename = NULL;
	const char *prefix = getenv( "MLT_PROFILES_PATH" );
	mlt_properties properties = mlt_properties_load( name );
	mlt_profile profile = NULL;

	// Try to load from file specification
	if ( properties && mlt_properties_get_int( properties, "width" ) )
	{
		filename = calloc( 1, strlen( name ) + 1 );
	}
	// Load from $prefix/share/mlt/profiles
	else if ( prefix == NULL )
	{
		prefix = PREFIX;
		filename = calloc( 1, strlen( prefix ) + strlen( PROFILES_DIR ) + strlen( name ) + 2 );
		strcpy( filename, prefix );
		if ( filename[ strlen( filename ) - 1 ] != '/' )
			filename[ strlen( filename ) ] = '/';
		strcat( filename, PROFILES_DIR );
	}
	// Use environment variable instead
	else
	{
		filename = calloc( 1, strlen( prefix ) + strlen( name ) + 2 );
		strcpy( filename, prefix );
		if ( filename[ strlen( filename ) - 1 ] != '/' )
			filename[ strlen( filename ) ] = '/';
	}

	// Finish loading
	strcat( filename, name );
	profile = mlt_profile_load_file( filename );

	// Cleanup
	mlt_properties_close( properties );
	free( filename );

	return profile;
}

/** Construct a profile.
 *
 * This will never return NULL as it uses the dv_pal settings as hard-coded fallback default.
 *
 * \public \memberof mlt_profile_s
 * \param name the name of a profile settings file located in the standard location or
 * the full path name to a profile settings file
 * \return a profile
 */

mlt_profile mlt_profile_init( const char *name )
{
	mlt_profile profile = NULL;

	// Explicit profile by name gets priority over environment variables
	if ( name )
		profile = mlt_profile_select( name );

	// Try to load by environment variable
	if ( profile == NULL )
	{
		// MLT_PROFILE is preferred environment variable
		if ( getenv( "MLT_PROFILE" ) )
			profile = mlt_profile_select( getenv( "MLT_PROFILE" ) );
		// MLT_NORMALISATION backwards compatibility
		else if ( getenv( "MLT_NORMALISATION" ) && strcmp( getenv( "MLT_NORMALISATION" ), "PAL" ) )
			profile = mlt_profile_select( "dv_ntsc" );
		else
			profile = mlt_profile_select( "dv_pal" );

		// If still not loaded (no profile files), default to PAL
		if ( profile == NULL )
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
				profile->sample_aspect_num = 16;
				profile->sample_aspect_den = 15;
				profile->display_aspect_num = 4;
				profile->display_aspect_den = 3;
				profile->colorspace = 601;
			}
		}
	}
	return profile;
}

static void set_mlt_normalisation( const char *profile_name )
{
	if ( profile_name )
	{
		if ( strstr( profile_name, "_ntsc" ) ||
		     strstr( profile_name, "_60" ) ||
		     strstr( profile_name, "_5994" ) ||
		     strstr( profile_name, "_2997" ) ||
		     strstr( profile_name, "_30" ) )
		{
			mlt_environment_set( "MLT_NORMALISATION", "NTSC" );
		}
		else if ( strstr( profile_name, "_pal" ) ||
		          strstr( profile_name, "_50" ) ||
		          strstr( profile_name, "_25" ) )
		{
			mlt_environment_set( "MLT_NORMALISATION", "PAL" );
		}
	}
}

/** Load a profile from specific file.
 *
 * \public \memberof mlt_profile_s
 * \param file the full path name to a properties file
 * \return a profile or NULL on error
 */

mlt_profile mlt_profile_load_file( const char *file )
{
	mlt_profile profile = NULL;

	// Load the profile as properties
	mlt_properties properties = mlt_properties_load( file );
	if ( properties )
	{
		// Simple check if the profile is valid
		if ( mlt_properties_get_int( properties, "width" ) )
		{
			profile = mlt_profile_load_properties( properties );

			// Set MLT_PROFILE to basename
			char *filename = strdup( file );
			mlt_environment_set( "MLT_PROFILE", basename( filename ) );
			set_mlt_normalisation( basename( filename ) );
			free( filename );
		}
		mlt_properties_close( properties );
	}

	// Set MLT_NORMALISATION to appease legacy modules
	char *profile_name = mlt_environment( "MLT_PROFILE" );
	set_mlt_normalisation( profile_name );
	return profile;
}

/** Load a profile from a properties object.
 *
 * \public \memberof mlt_profile_s
 * \param properties a properties list
 * \return a profile or NULL if out of memory
 */

mlt_profile mlt_profile_load_properties( mlt_properties properties )
{
	mlt_profile profile = calloc( 1, sizeof( struct mlt_profile_s ) );
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
		profile->colorspace = mlt_properties_get_int( properties, "colorspace" );
	}
	return profile;
}

/** Load an anonymous profile from string.
 *
 * \public \memberof mlt_profile_s
 * \param string a newline-delimited list of properties as name=value pairs
 * \return a profile or NULL if out of memory
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

/** Get the video frame rate as a floating point value.
 *
 * \public \memberof mlt_profile_s
 * @param profile a profile
 * @return the frame rate
 */

double mlt_profile_fps( mlt_profile profile )
{
	if ( profile )
		return ( double ) profile->frame_rate_num / profile->frame_rate_den;
	else
		return 0;
}

/** Get the sample aspect ratio as a floating point value.
 *
 * \public \memberof mlt_profile_s
 * \param profile a profile
 * \return the pixel aspect ratio
 */

double mlt_profile_sar( mlt_profile profile )
{
	if ( profile )
		return ( double ) profile->sample_aspect_num / profile->sample_aspect_den;
	else
		return 0;
}

/** Get the display aspect ratio as floating point value.
 *
 * \public \memberof mlt_profile_s
 * \param profile a profile
 * \return the image aspect ratio
 */

double mlt_profile_dar( mlt_profile profile )
{
	if ( profile )
		return ( double ) profile->display_aspect_num / profile->display_aspect_den;
	else
		return 0;
}

/** Free up the global profile resources.
 *
 * \public \memberof mlt_profile_s
 * \param profile a profile
 */

void mlt_profile_close( mlt_profile profile )
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

/** Make a copy of a profile.
  *
  * \public \memberof mlt_profile_s
  * \param profile the profile to clone
  * \return a copy of the profile
  */

mlt_profile mlt_profile_clone( mlt_profile profile )
{
	mlt_profile clone = NULL;

	if ( profile )
	{
		clone = calloc( 1, sizeof( *profile ) );
		if ( clone )
		{
			memcpy( clone, profile, sizeof( *profile ) );
			clone->description = strdup( profile->description );
		}
	}
	return clone;
}


/** Get the list of profiles.
 *
 * The caller MUST close the returned properties object!
 * Each entry in the list is keyed on its name, and its value is another
 * properties object that contains the attributes of the profile.
 * \public \memberof mlt_profile_s
 * \return a list of profiles
 */

mlt_properties mlt_profile_list( )
{
	char *filename = NULL;
	const char *prefix = getenv( "MLT_PROFILES_PATH" );
	mlt_properties properties = mlt_properties_new();
	mlt_properties dir = mlt_properties_new();
	int sort = 1;
	const char *wildcard = NULL;
	int i;

	// Load from $prefix/share/mlt/profiles if no env var
	if ( prefix == NULL )
	{
		prefix = PREFIX;
		filename = calloc( 1, strlen( prefix ) + strlen( PROFILES_DIR ) + 2 );
		strcpy( filename, prefix );
		if ( filename[ strlen( filename ) - 1 ] != '/' )
			filename[ strlen( filename ) ] = '/';
		strcat( filename, PROFILES_DIR );
		prefix = filename;
	}

	mlt_properties_dir_list( dir, prefix, wildcard, sort );

	for ( i = 0; i < mlt_properties_count( dir ); i++ )
	{
		char *filename = mlt_properties_get_value( dir, i );
		char *profile_name = basename( filename );
		if ( profile_name[0] != '.' && strcmp( profile_name, "Makefile" ) &&
		     profile_name[ strlen( profile_name ) - 1 ] != '~' )
		{
			mlt_properties profile = mlt_properties_load( filename );
			if ( profile )
			{
				mlt_properties_set_data( properties, profile_name, profile, 0,
					(mlt_destructor) mlt_properties_close, NULL );
			}
		}
	}
	mlt_properties_close( dir );
	if ( filename )
		free( filename );

	return properties;
}

/** Update the profile using the attributes of a producer.
 *
 * Use this to make an "auto-profile." Typically, you need to re-open the producer
 * after you use this because some producers (e.g. avformat) adjust their framerate
 * to that of the profile used when you created it.
 * \public \memberof mlt_profile_s
 * \param profile the profile to update
 * \param producer the producer to inspect
 */

void mlt_profile_from_producer( mlt_profile profile, mlt_producer producer )
{
	mlt_frame fr = NULL;
	uint8_t *buffer;
	mlt_image_format fmt = mlt_image_yuv422;
	mlt_properties p;
	int w = profile->width;
	int h = profile->height;

	if ( ! mlt_service_get_frame( MLT_PRODUCER_SERVICE(producer), &fr, 0 ) && fr )
	{
		mlt_properties_set_double( MLT_FRAME_PROPERTIES( fr ), "consumer_aspect_ratio", mlt_profile_sar( profile ) );
		if ( ! mlt_frame_get_image( fr, &buffer, &fmt, &w, &h, 0 ) )
		{
			// Some source properties are not exposed until after the first get_image call.
			mlt_frame_close( fr );
			mlt_service_get_frame( MLT_PRODUCER_SERVICE(producer), &fr, 0 );
			p = MLT_FRAME_PROPERTIES( fr );
//			mlt_properties_dump(p, stderr);
			if ( mlt_properties_get_int( p, "meta.media.frame_rate_den" ) && mlt_properties_get_int( p, "meta.media.sample_aspect_den" ) )
			{
				profile->width = mlt_properties_get_int( p, "meta.media.width" );
				profile->height = mlt_properties_get_int( p, "meta.media.height" );
				profile->progressive = mlt_properties_get_int( p, "meta.media.progressive" );
				profile->frame_rate_num = mlt_properties_get_int( p, "meta.media.frame_rate_num" );
				profile->frame_rate_den = mlt_properties_get_int( p, "meta.media.frame_rate_den" );
				// AVCHD is mis-reported as double frame rate.
				if ( profile->progressive == 0 && (
				     profile->frame_rate_num / profile->frame_rate_den == 50 ||
				     profile->frame_rate_num / profile->frame_rate_den == 59 ) )
					profile->frame_rate_num /= 2;
				profile->sample_aspect_num = mlt_properties_get_int( p, "meta.media.sample_aspect_num" );
				profile->sample_aspect_den = mlt_properties_get_int( p, "meta.media.sample_aspect_den" );
				profile->colorspace = mlt_properties_get_int( p, "meta.media.colorspace" );
				profile->display_aspect_num = (int) ( (double) profile->sample_aspect_num * profile->width / profile->sample_aspect_den + 0.5 );
				profile->display_aspect_den = profile->height;
				free( profile->description );
				profile->description = strdup( "automatic" );
				profile->is_explicit = 0;
			}
		}
	}
	mlt_frame_close( fr );
	mlt_producer_seek( producer, 0 );
}
