#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <framework/mlt.h>

#include "io.h"

static void transport_action( mlt_producer producer, char *value )
{
	mlt_properties properties = mlt_producer_properties( producer );
	mlt_multitrack multitrack = mlt_properties_get_data( properties, "multitrack", NULL );

	if ( strlen( value ) == 1 )
	{
		switch( value[ 0 ] )
		{
			case 'q':
				mlt_properties_set_int( properties, "done", 1 );
				break;
			case '0':
				mlt_producer_set_speed( producer, 1 );
				mlt_producer_seek( producer, 0 );
				break;
			case '1':
				mlt_producer_set_speed( producer, -10 );
				break;
			case '2':
				mlt_producer_set_speed( producer, -5 );
				break;
			case '3':
				mlt_producer_set_speed( producer, -2 );
				break;
			case '4':
				mlt_producer_set_speed( producer, -1 );
				break;
			case '5':
				mlt_producer_set_speed( producer, 0 );
				break;
			case '6':
			case ' ':
				mlt_producer_set_speed( producer, 1 );
				break;
			case '7':
				mlt_producer_set_speed( producer, 2 );
				break;
			case '8':
				mlt_producer_set_speed( producer, 5 );
				break;
			case '9':
				mlt_producer_set_speed( producer, 10 );
				break;
			case 'g':
				if ( multitrack != NULL )
				{
					mlt_timecode time = mlt_multitrack_clip( multitrack, mlt_whence_relative_current, 0 );
					mlt_producer_seek( producer, time );
				}
				break;
			case 'h':
				if ( multitrack != NULL )
				{
					mlt_producer producer = mlt_multitrack_producer( multitrack );
					int64_t position = mlt_producer_frame_position( producer, mlt_producer_position( producer ) );
					mlt_producer_set_speed( producer, 0 );
					mlt_producer_seek_frame( producer, position - 1 >= 0 ? position - 1 : 0 );
				}
				break;
			case 'j':
				if ( multitrack != NULL )
				{
					mlt_timecode time = mlt_multitrack_clip( multitrack, mlt_whence_relative_current, 1 );
					mlt_producer_seek( producer, time );
				}
				break;
			case 'k':
				if ( multitrack != NULL )
				{
					mlt_timecode time = mlt_multitrack_clip( multitrack, mlt_whence_relative_current, -1 );
					mlt_producer_seek( producer, time );
				}
				break;
			case 'l':
				if ( multitrack != NULL )
				{
					mlt_producer producer = mlt_multitrack_producer( multitrack );
					int64_t position = mlt_producer_frame_position( producer, mlt_producer_position( producer ) );
					mlt_producer_set_speed( producer, 0 );
					mlt_producer_seek_frame( producer, position + 1 );
				}
				break;
		}
	}
}

static mlt_consumer create_consumer( char *id, mlt_producer producer )
{
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';
	mlt_consumer consumer = mlt_factory_consumer( id, arg );
	if ( consumer != NULL )
	{
		mlt_properties properties = mlt_consumer_properties( consumer );
		mlt_properties_set_data( properties, "transport_callback", transport_action, 0, NULL, NULL );
		mlt_properties_set_data( properties, "transport_producer", producer, 0, NULL, NULL );
	}
	return consumer;
}

static void transport( mlt_producer producer )
{
	mlt_properties properties = mlt_producer_properties( producer );

	term_init( );

	fprintf( stderr, "+-----+ +-----+ +-----+ +-----+ +-----+ +-----+ +-----+ +-----+ +-----+\n" );
	fprintf( stderr, "|1=-10| |2= -5| |3= -2| |4= -1| |5=  0| |6=  1| |7=  2| |8=  5| |9= 10|\n" );
	fprintf( stderr, "+-----+ +-----+ +-----+ +-----+ +-----+ +-----+ +-----+ +-----+ +-----+\n" );

	fprintf( stderr, "+---------------------------------------------------------------------+\n" );
	fprintf( stderr, "|                      h = previous,  l = next                        |\n" );
	fprintf( stderr, "|           g = start of clip, j = next clip, k = previous clip       |\n" );
	fprintf( stderr, "|                0 = restart, q = quit, space = play                  |\n" );
	fprintf( stderr, "+---------------------------------------------------------------------+\n" );

	while( mlt_properties_get_int( properties, "done" ) == 0 )
	{
		int value = term_read( );
		if ( value != -1 )
			transport_action( producer, ( char * )&value );
	}
}

int main( int argc, char **argv )
{
	int i;
	mlt_consumer consumer = NULL;
	mlt_producer inigo = NULL;
	FILE *store = NULL;
	char *name = NULL;

	// Construct the factory
	mlt_factory_init( getenv( "MLT_REPOSITORY" ) );

	for ( i = 1; i < argc; i ++ )
	{
		if ( !strcmp( argv[ i ], "-serialise" ) )
		{
			name = argv[ ++ i ];
			if ( strstr( name, ".inigo" ) )
				store = fopen( name, "w" );
		}
	}

	// Get inigo producer
	inigo = mlt_factory_producer( "inigo", &argv[ 1 ] );

	if ( inigo != NULL && mlt_producer_get_length( inigo ) > 0 )
	{
		// Get inigo's properties
		mlt_properties inigo_props = mlt_producer_properties( inigo );

		// Get the field service from inigo
		mlt_field field = mlt_properties_get_data( inigo_props, "field", 0 );

		// Get the last group
		mlt_properties group = mlt_properties_get_data( inigo_props, "group", 0 );

		// Parse the arguments
		for ( i = 1; i < argc; i ++ )
		{
			if ( !strcmp( argv[ i ], "-consumer" ) )
			{
				consumer = create_consumer( argv[ ++ i ], inigo );
				while ( argv[ i + 1 ] != NULL && strstr( argv[ i + 1 ], "=" ) )
					mlt_properties_parse( group, argv[ ++ i ] );
			}
			else if ( !strcmp( argv[ i ], "-serialise" ) )
			{
				i ++;
			}
			else
			{
				if ( store != NULL )
					fprintf( store, "%s\n", argv[ i ] );

				i ++;

				while ( argv[ i ] != NULL && argv[ i ][ 0 ] != '-' )
				{
					if ( store != NULL )
						fprintf( store, "%s\n", argv[ i ] );
					i += 1;
				}

				i --;
			}
		}

		// If we have no consumer, default to sdl
		if ( store == NULL && consumer == NULL )
			consumer = create_consumer( "sdl", inigo );

		if ( consumer != NULL && store == NULL )
		{
			// Apply group settings
			mlt_properties properties = mlt_consumer_properties( consumer );
			mlt_properties_inherit( properties, group );

			// Connect consumer to tractor
			mlt_consumer_connect( consumer, mlt_field_service( field ) );

			// Transport functionality
			transport( inigo );
			
		}
		else if ( store != NULL )
		{
			fprintf( stderr, "Project saved as %s.\n", name );
			fclose( store );
		}

	}
	else
	{
		fprintf( stderr, "Usage: inigo [ -group [ name=value ]* ]\n"
						 "             [ -consumer id[:arg] [ name=value ]* ]\n"
        				 "             [ -filter id[:arg] [ name=value ] * ]\n"
        				 "             [ -transition id[:arg] [ name=value ] * ]\n"
						 "             [ -blank time ]\n"
        				 "             [ producer [ name=value ] * ]+\n" );
	}

	// Close the consumer
	if ( consumer != NULL )
		mlt_consumer_close( consumer );

	// Close the producer
	if ( inigo != NULL )
		mlt_producer_close( inigo );

	// Close the factory
	mlt_factory_close( );

	return 0;
}
