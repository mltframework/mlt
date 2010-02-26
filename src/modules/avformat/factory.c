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
#include <pthread.h>
#include <limits.h>

#include <framework/mlt.h>

extern mlt_consumer consumer_avformat_init( mlt_profile profile, char *file );
extern mlt_filter filter_avcolour_space_init( void *arg );
extern mlt_filter filter_avdeinterlace_init( void *arg );
extern mlt_filter filter_avresample_init( char *arg );
extern mlt_filter filter_swscale_init( mlt_profile profile, char *arg );
extern mlt_producer producer_avformat_init( mlt_profile profile, const char *service, char *file );

// ffmpeg Header files
#include <avformat.h>
#include <avdevice.h>

// A static flag used to determine if avformat has been initialised
static int avformat_initialised = 0;

// A locking mutex
static pthread_mutex_t avformat_mutex;

#if 0
// These 3 functions should override the alloc functions in libavformat
// but some formats or codecs seem to crash when used (wmv in particular)

void *av_malloc( unsigned int size )
{
	return mlt_pool_alloc( size );
}

void *av_realloc( void *ptr, unsigned int size )
{
	return mlt_pool_realloc( ptr, size );
}

void av_free( void *ptr )
{
	return mlt_pool_release( ptr );
}
#endif

void avformat_destroy( void *ignore )
{
	// Clean up
	// av_free_static( ); -XXX this is deprecated

	// Destroy the mutex
	pthread_mutex_destroy( &avformat_mutex );
}

void avformat_lock( )
{
	// Lock the mutex now
	pthread_mutex_lock( &avformat_mutex );
}

void avformat_unlock( )
{
	// Unlock the mutex now
	pthread_mutex_unlock( &avformat_mutex );
}

static void avformat_init( )
{
	// Initialise avformat if necessary
	if ( avformat_initialised == 0 )
	{
		avformat_initialised = 1;
		pthread_mutex_init( &avformat_mutex, NULL );
		av_register_all( );
		avdevice_register_all();
		mlt_factory_register_for_clean_up( NULL, avformat_destroy );
		av_log_set_level( mlt_log_get_level() );
	}
}

static void *create_service( mlt_profile profile, mlt_service_type type, const char *id, void *arg )
{
	avformat_init( );
#ifdef CODECS
	if ( !strncmp( id, "avformat", 8 ) )
	{
		if ( type == producer_type )
			return producer_avformat_init( profile, id, arg );
		else if ( type == consumer_type )
			return consumer_avformat_init( profile, arg );
	}
#endif
#ifdef FILTERS
	if ( !strcmp( id, "avcolor_space" ) )
		return filter_avcolour_space_init( arg );
	if ( !strcmp( id, "avcolour_space" ) )
		return filter_avcolour_space_init( arg );
	if ( !strcmp( id, "avdeinterlace" ) )
		return filter_avdeinterlace_init( arg );
	if ( !strcmp( id, "avresample" ) )
		return filter_avresample_init( arg );
#ifdef SWSCALE
	if ( !strcmp( id, "swscale" ) )
		return filter_swscale_init( profile, arg );
#endif
#endif
	return NULL;
}

static mlt_properties avformat_metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	const char *service_type = NULL;
	switch ( type )
	{
		case consumer_type:
			service_type = "consumer";
			break;
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
	snprintf( file, PATH_MAX, "%s/avformat/%s_%s.yml", mlt_environment( "MLT_DATA" ), service_type, id );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
#ifdef CODECS
	MLT_REGISTER( consumer_type, "avformat", create_service );
	MLT_REGISTER( producer_type, "avformat", create_service );
	MLT_REGISTER( producer_type, "avformat-novalidate", create_service );
	MLT_REGISTER_METADATA( producer_type, "avformat", avformat_metadata, NULL );
#endif
#ifdef FILTERS
	MLT_REGISTER( filter_type, "avcolour_space", create_service );
	MLT_REGISTER( filter_type, "avcolor_space", create_service );
	MLT_REGISTER( filter_type, "avdeinterlace", create_service );
	MLT_REGISTER( filter_type, "avresample", create_service );
#ifdef SWSCALE
	MLT_REGISTER( filter_type, "swscale", create_service );
#endif
#endif
}
