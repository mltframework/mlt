/*
 * producer_melt.c -- load from melt command line syntax
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <framework/mlt.h>

mlt_producer producer_melt_init( mlt_profile profile, mlt_service_type type, const char *id, char **argv );

mlt_producer producer_melt_file_init( mlt_profile profile, mlt_service_type type, const char *id, char *file )
{
	FILE *input = fopen( file, "r" );
	char **args = calloc( sizeof( char * ), 1000 );
	int count = 0;
	char temp[ 2048 ];

	if ( input != NULL )
	{
		while( fgets( temp, 2048, input ) )
		{
			temp[ strlen( temp ) - 1 ] = '\0';
			if ( strcmp( temp, "" ) )
				args[ count ++ ] = strdup( temp );
		}
	}

	mlt_producer result = producer_melt_init( profile, type, id, args );

	if ( result != NULL )
	{
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( result );
		mlt_properties_set( properties, "resource", file );
	}

	while( count -- )
		free( args[ count ] );
	free( args );

	return result;
}

static void track_service( mlt_field field, void *service, mlt_destructor destructor )
{
	mlt_properties properties = mlt_field_properties( field );
	int registered = mlt_properties_get_int( properties, "registered" );
	char *key = mlt_properties_get( properties, "registered" );
	mlt_properties_set_data( properties, key, service, 0, destructor, NULL );
	mlt_properties_set_int( properties, "registered", ++ registered );
}

static mlt_producer create_producer( mlt_profile profile, mlt_field field, char *file )
{
	mlt_producer result = mlt_factory_producer( profile, NULL, file );

	if ( result != NULL )
		track_service( field, result, ( mlt_destructor )mlt_producer_close );

	return result;
}

static mlt_filter create_attach( mlt_profile profile, mlt_field field, char *id, int track )
{
	char *temp = strdup( id );
	char *arg = strchr( temp, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';
	mlt_filter filter = mlt_factory_filter( profile, temp, arg );
	if ( filter != NULL )
		track_service( field, filter, ( mlt_destructor )mlt_filter_close );
	free( temp );
	return filter;
}

static mlt_filter create_filter( mlt_profile profile, mlt_field field, char *id, int track )
{
	char *temp = strdup( id );
	char *arg = strchr( temp, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';
	mlt_filter filter = mlt_factory_filter( profile, temp, arg );
	if ( filter != NULL )
	{
		mlt_field_plant_filter( field, filter, track );
		track_service( field, filter, ( mlt_destructor )mlt_filter_close );
	}
	free( temp );
	return filter;
}

static mlt_transition create_transition( mlt_profile profile, mlt_field field, char *id, int track )
{
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';
	mlt_transition transition = mlt_factory_transition( profile, id, arg );
	if ( transition != NULL )
	{
		mlt_field_plant_transition( field, transition, track, track + 1 );
		track_service( field, transition, ( mlt_destructor )mlt_transition_close );
	}
	return transition;
}

mlt_producer producer_melt_init( mlt_profile profile, mlt_service_type type, const char *id, char **argv )
{
	int i;
	int track = 0;
	mlt_producer producer = NULL;
	mlt_tractor mix = NULL;
	mlt_playlist playlist = mlt_playlist_init( );
	mlt_properties group = mlt_properties_new( );
	mlt_tractor tractor = mlt_tractor_new( );
	mlt_properties properties = MLT_TRACTOR_PROPERTIES( tractor );
	mlt_field field = mlt_tractor_field( tractor );
	mlt_properties field_properties = mlt_field_properties( field );
	mlt_multitrack multitrack = mlt_tractor_multitrack( tractor );
	char *title = NULL;

	// Assistance for template construction (allows -track usage to specify the first track)
	mlt_properties_set_int( MLT_PLAYLIST_PROPERTIES( playlist ), "_melt_first", 1 );

	// We need to track the number of registered filters
	mlt_properties_set_int( field_properties, "registered", 0 );

	// Parse the arguments
	if ( argv )
	for ( i = 0; argv[ i ] != NULL; i ++ )
	{
		if ( argv[ i + 1 ] == NULL && (
			!strcmp( argv[ i ], "-attach" ) ||
			!strcmp( argv[ i ], "-attach-cut" ) ||
			!strcmp( argv[ i ], "-attach-track" ) ||
			!strcmp( argv[ i ], "-attach-clip" ) ||
			!strcmp( argv[ i ], "-repeat" ) ||
			!strcmp( argv[ i ], "-split" ) ||
			!strcmp( argv[ i ], "-join" ) ||
			!strcmp( argv[ i ], "-mixer" ) ||
			!strcmp( argv[ i ], "-mix" ) ||
			!strcmp( argv[ i ], "-filter" ) ||
			!strcmp( argv[ i ], "-transition" ) ||
			!strcmp( argv[ i ], "-blank" ) ) )
		{
			fprintf( stderr, "Argument missing for %s.\n", argv[ i ] );
			break;
		}
		if ( !strcmp( argv[ i ], "-group" ) )
		{
			if ( mlt_properties_count( group ) != 0 )
			{
				mlt_properties_close( group );
				group = mlt_properties_new( );
			}
			if ( group != NULL )
				properties = group;
		}
		else if ( !strcmp( argv[ i ], "-attach" ) || 
				  !strcmp( argv[ i ], "-attach-cut" ) ||
				  !strcmp( argv[ i ], "-attach-track" ) ||
				  !strcmp( argv[ i ], "-attach-clip" ) )
		{
			int type = !strcmp( argv[ i ], "-attach" ) ? 0 : 
					   !strcmp( argv[ i ], "-attach-cut" ) ? 1 : 
					   !strcmp( argv[ i ], "-attach-track" ) ? 2 : 3;
			mlt_filter filter = create_attach( profile, field, argv[ ++ i ], track );
			if ( producer != NULL && !mlt_producer_is_cut( producer ) )
			{
				mlt_playlist_clip_info info;
				mlt_playlist_append( playlist, producer );
				mlt_playlist_get_clip_info( playlist, &info, mlt_playlist_count( playlist ) - 1 );
				producer = info.cut;
			}

			if ( type == 1 || type == 2 )
			{
				mlt_playlist_clip_info info;
				mlt_playlist_get_clip_info( playlist, &info, mlt_playlist_count( playlist ) - 1 );
				producer = info.cut;
			}

			if ( filter != NULL && mlt_playlist_count( playlist ) > 0 )
			{
				if ( type == 0 )
					mlt_service_attach( ( mlt_service )properties, filter );
				else if ( type == 1 )
					mlt_service_attach( ( mlt_service )producer, filter );
				else if ( type == 2 )
					mlt_service_attach( ( mlt_service )playlist, filter );
				else if ( type == 3 )
					mlt_service_attach( ( mlt_service )mlt_producer_cut_parent( producer ), filter );

				properties = MLT_FILTER_PROPERTIES( filter );
				mlt_properties_inherit( properties, group );
			}
			else if ( filter != NULL )
			{
				mlt_service_attach( ( mlt_service )playlist, filter );
				properties = MLT_FILTER_PROPERTIES( filter );
				mlt_properties_inherit( properties, group );
			}
		}
		else if ( !strcmp( argv[ i ], "-repeat" ) )
		{
			int repeat = atoi( argv[ ++ i ] );
			if ( producer != NULL && !mlt_producer_is_cut( producer ) )
				mlt_playlist_append( playlist, producer );
			producer = NULL;
			if ( mlt_playlist_count( playlist ) > 0 )
			{
				mlt_playlist_clip_info info;
				mlt_playlist_repeat_clip( playlist, mlt_playlist_count( playlist ) - 1, repeat );
				mlt_playlist_get_clip_info( playlist, &info, mlt_playlist_count( playlist ) - 1 );
				producer = info.cut;
				properties = MLT_PRODUCER_PROPERTIES( producer );
			}
		}
		else if ( !strcmp( argv[ i ], "-split" ) )
		{
			int split = atoi( argv[ ++ i ] );
			if ( producer != NULL && !mlt_producer_is_cut( producer ) )
				mlt_playlist_append( playlist, producer );
			producer = NULL;
			if ( mlt_playlist_count( playlist ) > 0 )
			{
				mlt_playlist_clip_info info;
				mlt_playlist_get_clip_info( playlist, &info, mlt_playlist_count( playlist ) - 1 );
				split = split < 0 ? info.frame_out + split : split;
				mlt_playlist_split( playlist, mlt_playlist_count( playlist ) - 1, split );
				mlt_playlist_get_clip_info( playlist, &info, mlt_playlist_count( playlist ) - 1 );
				producer = info.cut;
				properties = MLT_PRODUCER_PROPERTIES( producer );
			}
		}
		else if ( !strcmp( argv[ i ], "-swap" ) )
		{
			if ( producer != NULL && !mlt_producer_is_cut( producer ) )
				mlt_playlist_append( playlist, producer );
			producer = NULL;
			if ( mlt_playlist_count( playlist ) >= 2 )
			{
				mlt_playlist_clip_info info;
				mlt_playlist_move( playlist, mlt_playlist_count( playlist ) - 2, mlt_playlist_count( playlist ) - 1 );
				mlt_playlist_get_clip_info( playlist, &info, mlt_playlist_count( playlist ) - 1 );
				producer = info.cut;
				properties = MLT_PRODUCER_PROPERTIES( producer );
			}
		}
		else if ( !strcmp( argv[ i ], "-join" ) )
		{
			int clips = atoi( argv[ ++ i ] );
			if ( producer != NULL && !mlt_producer_is_cut( producer ) )
				mlt_playlist_append( playlist, producer );
			producer = NULL;
			if ( mlt_playlist_count( playlist ) > 0 )
			{
				mlt_playlist_clip_info info;
				int clip = clips <= 0 ? 0 : mlt_playlist_count( playlist ) - clips - 1;
				if ( clip < 0 ) clip = 0;
				if ( clip >= mlt_playlist_count( playlist ) ) clip = mlt_playlist_count( playlist ) - 2;
				if ( clips < 0 ) clips =  mlt_playlist_count( playlist ) - 1;
				mlt_playlist_join( playlist, clip, clips, 0 );
				mlt_playlist_get_clip_info( playlist, &info, mlt_playlist_count( playlist ) - 1 );
				producer = info.cut;
				properties = MLT_PRODUCER_PROPERTIES( producer );
			}
		}
		else if ( !strcmp( argv[ i ], "-remove" ) )
		{
			if ( producer != NULL && !mlt_producer_is_cut( producer ) )
				mlt_playlist_append( playlist, producer );
			producer = NULL;
			if ( mlt_playlist_count( playlist ) > 0 )
			{
				mlt_playlist_clip_info info;
				mlt_playlist_remove( playlist, mlt_playlist_count( playlist ) - 1 );
				mlt_playlist_get_clip_info( playlist, &info, mlt_playlist_count( playlist ) - 1 );
				producer = info.cut;
				properties = MLT_PRODUCER_PROPERTIES( producer );
			}
		}
		else if ( !strcmp( argv[ i ], "-mix" ) )
		{
			int length = atoi( argv[ ++ i ] );
			if ( producer != NULL && !mlt_producer_is_cut( producer ) )
				mlt_playlist_append( playlist, producer );
			producer = NULL;
			if ( mlt_playlist_count( playlist ) >= 2 )
			{
				if ( mlt_playlist_mix( playlist, mlt_playlist_count( playlist ) - 2, length, NULL ) == 0 )
				{
					mlt_playlist_clip_info info;
					mlt_playlist_get_clip_info( playlist, &info, mlt_playlist_count( playlist ) - 1 );
					if ( mlt_properties_get_data( ( mlt_properties )info.producer, "mlt_mix", NULL ) == NULL )
						mlt_playlist_get_clip_info( playlist, &info, mlt_playlist_count( playlist ) - 2 );
					mix = ( mlt_tractor )mlt_properties_get_data( ( mlt_properties )info.producer, "mlt_mix", NULL );
					properties = NULL;
				}
				else
				{
					fprintf( stderr, "Mix failed?\n" );
				}
			}
			else
			{
				fprintf( stderr, "Invalid position for a mix...\n" );
			}
		}
		else if ( !strcmp( argv[ i ], "-mixer" ) )
		{
			if ( mix != NULL )
			{
				char *id = strdup( argv[ ++ i ] );
				char *arg = strchr( id, ':' );
				mlt_field field = mlt_tractor_field( mix );
				mlt_transition transition = NULL;
				if ( arg != NULL )
					*arg ++ = '\0';
				transition = mlt_factory_transition( profile, id, arg );
				if ( transition != NULL )
				{
					properties = MLT_TRANSITION_PROPERTIES( transition );
					mlt_properties_inherit( properties, group );
					mlt_field_plant_transition( field, transition, 0, 1 );
					mlt_properties_set_position( properties, "in", 0 );
					mlt_properties_set_position( properties, "out", mlt_producer_get_out( ( mlt_producer )mix ) );
					mlt_transition_close( transition );
				}
				free( id );
			}
			else
			{
				fprintf( stderr, "Invalid mixer...\n" );
			}
		}
		else if ( !strcmp( argv[ i ], "-filter" ) )
		{
			mlt_filter filter = create_filter( profile, field, argv[ ++ i ], track );
			if ( filter != NULL )
			{
				properties = MLT_FILTER_PROPERTIES( filter );
				mlt_properties_inherit( properties, group );
			}
		}
		else if ( !strcmp( argv[ i ], "-transition" ) )
		{
			mlt_transition transition = create_transition( profile, field, argv[ ++ i ], track - 1 );
			if ( transition != NULL )
			{
				properties = MLT_TRANSITION_PROPERTIES( transition );
				mlt_properties_inherit( properties, group );
			}
		}
		else if ( !strcmp( argv[ i ], "-blank" ) )
		{
			if ( producer != NULL && !mlt_producer_is_cut( producer ) )
				mlt_playlist_append( playlist, producer );
			producer = NULL;
			mlt_playlist_blank( playlist, atof( argv[ ++ i ] ) );
		}
		else if ( !strcmp( argv[ i ], "-track" ) ||
				  !strcmp( argv[ i ], "-null-track" ) ||
				  !strcmp( argv[ i ], "-video-track" ) ||
				  !strcmp( argv[ i ], "-audio-track" ) ||
				  !strcmp( argv[ i ], "-hide-track" ) ||
				  !strcmp( argv[ i ], "-hide-video" ) ||
				  !strcmp( argv[ i ], "-hide-audio" ) )
		{
			if ( producer != NULL && !mlt_producer_is_cut( producer ) )
				mlt_playlist_append( playlist, producer );
			producer = NULL;
			if ( !mlt_properties_get_int( MLT_PLAYLIST_PROPERTIES( playlist ), "_melt_first" ) || 
				  mlt_producer_get_playtime( MLT_PLAYLIST_PRODUCER( playlist ) ) > 0 )
			{
				mlt_multitrack_connect( multitrack, MLT_PLAYLIST_PRODUCER( playlist ), track ++ );
				track_service( field, playlist, ( mlt_destructor )mlt_playlist_close );
				playlist = mlt_playlist_init( );
			}
			if ( playlist != NULL )
			{
				properties = MLT_PLAYLIST_PROPERTIES( playlist );
				if ( !strcmp( argv[ i ], "-null-track" ) || !strcmp( argv[ i ], "-hide-track" ) )
					mlt_properties_set_int( properties, "hide", 3 );
				else if ( !strcmp( argv[ i ], "-audio-track" ) || !strcmp( argv[ i ], "-hide-video" ) )
					mlt_properties_set_int( properties, "hide", 1 );
				else if ( !strcmp( argv[ i ], "-video-track" ) || !strcmp( argv[ i ], "-hide-audio" ) )
					mlt_properties_set_int( properties, "hide", 2 );
			}
		}
		else if ( strchr( argv[ i ], '=' ) && strstr( argv[ i ], "<?xml" ) != argv[ i ] )
		{
			mlt_properties_parse( properties, argv[ i ] );
		}
		else if ( argv[ i ][ 0 ] != '-' )
		{
			if ( producer != NULL && !mlt_producer_is_cut( producer ) )
				mlt_playlist_append( playlist, producer );
			if ( title == NULL && strstr( argv[ i ], "<?xml" ) != argv[ i ] )
				title = argv[ i ];
			producer = create_producer( profile, field, argv[ i ] );
			if ( producer != NULL )
			{
				properties = MLT_PRODUCER_PROPERTIES( producer );
				mlt_properties_inherit( properties, group );
			}
			else
			{
				fprintf( stderr, "Failed to load \"%s\"\n", argv[ i ] );
			}
		}
		else
		{
			int backtrack = 0;
			if ( !strcmp( argv[ i ], "-serialise" ) ||
			     !strcmp( argv[ i ], "-consumer" ) ||
			     !strcmp( argv[ i ], "-profile" ) )
			{
				i += 2;
				backtrack = 1;
			}

			while ( argv[ i ] != NULL && strchr( argv[ i ], '=' ) )
			{
				i ++;
				backtrack = 1;
			}
			if ( backtrack )
				i --;
		}
	}

	// Connect last producer to playlist
	if ( producer != NULL && !mlt_producer_is_cut( producer ) )
		mlt_playlist_append( playlist, producer );

	// Track the last playlist too
	track_service( field, playlist, ( mlt_destructor )mlt_playlist_close );

	// We must have a playlist to connect
	if ( !mlt_properties_get_int( MLT_PLAYLIST_PROPERTIES( playlist ), "_melt_first" ) || 
		  mlt_producer_get_playtime( MLT_PLAYLIST_PRODUCER( playlist ) ) > 0 )
		mlt_multitrack_connect( multitrack, MLT_PLAYLIST_PRODUCER( playlist ), track );

	mlt_producer prod = MLT_TRACTOR_PRODUCER( tractor );
	mlt_producer_optimise( prod );
	mlt_properties props = MLT_TRACTOR_PROPERTIES( tractor );
	mlt_properties_set_data( props, "group", group, 0, ( mlt_destructor )mlt_properties_close, NULL );
	mlt_properties_set_position( props, "length", mlt_producer_get_out( MLT_MULTITRACK_PRODUCER( multitrack ) ) + 1 );
	mlt_producer_set_in_and_out( prod, 0, mlt_producer_get_out( MLT_MULTITRACK_PRODUCER( multitrack ) ) );
	if ( title != NULL )
		mlt_properties_set( props, "title", strchr( title, '/' ) ? strrchr( title, '/' ) + 1 : title );

	return prod;
}
