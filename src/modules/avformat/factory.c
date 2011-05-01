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
#include <float.h>

#include <framework/mlt.h>

extern mlt_consumer consumer_avformat_init( mlt_profile profile, char *file );
extern mlt_filter filter_avcolour_space_init( void *arg );
extern mlt_filter filter_avdeinterlace_init( void *arg );
extern mlt_filter filter_avresample_init( char *arg );
extern mlt_filter filter_swscale_init( mlt_profile profile, char *arg );
extern mlt_producer producer_avformat_init( mlt_profile profile, const char *service, char *file );

// ffmpeg Header files
#include <libavformat/avformat.h>
#ifdef AVDEVICE
#include <libavdevice/avdevice.h>
#endif
#if LIBAVCODEC_VERSION_MAJOR > 52
#include <libavutil/opt.h>
#else
#include <libavcodec/opt.h>
#endif

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
#ifdef AVDEVICE
		avdevice_register_all();
#endif
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

static void add_parameters( mlt_properties params, void *object, int req_flags, const char *unit, const char *subclass )
{
	const AVOption *opt = NULL;

	// For each AVOption on the AVClass object
	while ( ( opt = av_next_option( object, opt ) ) )
	{
		// If matches flags and not a binary option (not supported by Mlt)
		if ( !( opt->flags & req_flags ) || ( opt->type == FF_OPT_TYPE_BINARY ) )
            continue;

		// Ignore constants (keyword values)
		if ( !unit && opt->type == FF_OPT_TYPE_CONST )
			continue;
		// When processing a groups of options (unit)...
		// ...ignore non-constants
		else if ( unit && opt->type != FF_OPT_TYPE_CONST )
			continue;
		// ...ignore constants not in this group
		else if ( unit && opt->type == FF_OPT_TYPE_CONST && strcmp( unit, opt->unit ) )
			continue;
		// ..add constants to the 'values' sequence
		else if ( unit && opt->type == FF_OPT_TYPE_CONST )
		{
			char key[20];
			snprintf( key, 20, "%d", mlt_properties_count( params ) );
			mlt_properties_set( params, key, opt->name );
			continue;
		}

		// Create a map for this option.
		mlt_properties p = mlt_properties_new();
		char key[20];
		snprintf( key, 20, "%d", mlt_properties_count( params ) );
		// Add the map to the 'parameters' sequence.
		mlt_properties_set_data( params, key, p, 0, (mlt_destructor) mlt_properties_close, NULL );

		// Add the parameter metadata for this AVOption.
		mlt_properties_set( p, "identifier", opt->name );
		if ( subclass )
		{
			char *s = malloc( strlen( opt->help ) + strlen( subclass ) + 4 );
			strcpy( s, opt->help );
			strcat( s, " (" );
			strcat( s, subclass );
			strcat( s, ")" );
			mlt_properties_set( p, "description", s );
			free( s );
		}
		else
			mlt_properties_set( p, "description", opt->help );


        switch ( opt->type )
		{
		case FF_OPT_TYPE_FLAGS:
			mlt_properties_set( p, "type", "string" );
			mlt_properties_set( p, "format", "flags" );
			break;
		case FF_OPT_TYPE_INT:
			if ( !opt->unit )
			{
				mlt_properties_set( p, "type", "integer" );
				if ( opt->min != INT_MIN )
					mlt_properties_set_int( p, "minimum", (int) opt->min );
				if ( opt->max != INT_MAX )
					mlt_properties_set_int( p, "maximum", (int) opt->max );
#if LIBAVUTIL_VERSION_MAJOR > 50
				mlt_properties_set_int( p, "default", (int) opt->default_val.dbl );
#endif
			}
			else
			{
				mlt_properties_set( p, "type", "string" );
				mlt_properties_set( p, "format", "integer or keyword" );
			}
			break;
		case FF_OPT_TYPE_INT64:
			mlt_properties_set( p, "type", "integer" );
			mlt_properties_set( p, "format", "64-bit" );
			if ( opt->min != INT64_MIN )
				mlt_properties_set_int64( p, "minimum", (int64_t) opt->min );
			if ( opt->max != INT64_MAX )
			mlt_properties_set_int64( p, "maximum", (int64_t) opt->max );
#if LIBAVUTIL_VERSION_MAJOR > 50
			mlt_properties_set_int64( p, "default", (int64_t) opt->default_val.dbl );
#endif
			break;
		case FF_OPT_TYPE_FLOAT:
			mlt_properties_set( p, "type", "float" );
			if ( opt->min != FLT_MIN && opt->min != -340282346638528859811704183484516925440.0 )
				mlt_properties_set_double( p, "minimum", opt->min );
			if ( opt->max != FLT_MAX )
				mlt_properties_set_double( p, "maximum", opt->max );
#if LIBAVUTIL_VERSION_MAJOR > 50
			mlt_properties_set_double( p, "default", opt->default_val.dbl );
#endif
			break;
		case FF_OPT_TYPE_DOUBLE:
			mlt_properties_set( p, "type", "float" );
			mlt_properties_set( p, "format", "double" );
			if ( opt->min != DBL_MIN )
				mlt_properties_set_double( p, "minimum", opt->min );
			if ( opt->max != DBL_MAX )
				mlt_properties_set_double( p, "maximum", opt->max );
#if LIBAVUTIL_VERSION_MAJOR > 50
			mlt_properties_set_double( p, "default", opt->default_val.dbl );
#endif
			break;
		case FF_OPT_TYPE_STRING:
			mlt_properties_set( p, "type", "string" );
#if LIBAVUTIL_VERSION_MAJOR > 50
			mlt_properties_set( p, "default", opt->default_val.str );
#endif
			break;
		case FF_OPT_TYPE_RATIONAL:
			mlt_properties_set( p, "type", "string" );
			mlt_properties_set( p, "format", "numerator:denominator" );
			break;
		case FF_OPT_TYPE_CONST:
		default:
			mlt_properties_set( p, "type", "integer" );
			mlt_properties_set( p, "format", "constant" );
			break;
        }
		// If the option belongs to a group (unit) and is not a constant (keyword value)
		if ( opt->unit && opt->type != FF_OPT_TYPE_CONST )
		{
			// Create a 'values' sequence.
			mlt_properties values = mlt_properties_new();

			// Recurse to add constants in this group to the 'values' sequence.
			add_parameters( values, object, req_flags, opt->unit, NULL );
			if ( mlt_properties_count( values ) )
				mlt_properties_set_data( p, "values", values, 0, (mlt_destructor) mlt_properties_close, NULL );
			else
				mlt_properties_close( values );
		}
	}
}

static mlt_properties avformat_metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	const char *service_type = NULL;
	mlt_properties result = NULL;

	// Convert the service type to a string.
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
	// Load the yaml file
	snprintf( file, PATH_MAX, "%s/avformat/%s_%s.yml", mlt_environment( "MLT_DATA" ), service_type, id );
	result = mlt_properties_parse_yaml( file );
	if ( result && ( type == consumer_type || type == producer_type ) )
	{
		// Annotate the yaml properties with AVOptions.
		mlt_properties params = (mlt_properties) mlt_properties_get_data( result, "parameters", NULL );
		AVFormatContext *avformat = avformat_alloc_context();
		AVCodecContext *avcodec = avcodec_alloc_context();
		int flags = ( type == consumer_type )? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;

		add_parameters( params, avformat, flags, NULL, NULL );
#if LIBAVFORMAT_VERSION_MAJOR > 52
		avformat_init();
		AVOutputFormat *f = NULL;
		while ( ( f = av_oformat_next( f ) ) )
			if ( f->priv_class )
				add_parameters( params, &f->priv_class, flags, NULL, f->name );
#endif

		add_parameters( params, avcodec, flags, NULL, NULL );
#if LIBAVCODEC_VERSION_MAJOR > 52
		AVCodec *c = NULL;
		while ( ( c = av_codec_next( c ) ) )
			if ( c->priv_class )
				add_parameters( params, &c->priv_class, flags, NULL, c->name );
#endif

		av_free( avformat );
		av_free( avcodec );
	}
	return result;
}

MLT_REPOSITORY
{
#ifdef CODECS
	MLT_REGISTER( consumer_type, "avformat", create_service );
	MLT_REGISTER( producer_type, "avformat", create_service );
	MLT_REGISTER( producer_type, "avformat-novalidate", create_service );
	MLT_REGISTER_METADATA( consumer_type, "avformat", avformat_metadata, NULL );
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
