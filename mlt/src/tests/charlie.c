#include <framework/mlt.h>
#include <stdio.h>
#include <string.h>

mlt_producer create_producer( char *file )
{
	mlt_producer result = NULL;

	// 1st Line preferences
	if ( strstr( file, ".mpg" ) )
		result = mlt_factory_producer( "mcmpeg", file );
	else if ( strstr( file, ".mpeg" ) )
		result = mlt_factory_producer( "mcmpeg", file );
	else if ( strstr( file, ".dv" ) )
		result = mlt_factory_producer( "mcdv", file );
	else if ( strstr( file, ".jpg" ) )
		result = mlt_factory_producer( "pixbuf", file );
	else if ( strstr( file, ".png" ) )
		result = mlt_factory_producer( "pixbuf", file );

	// 2nd Line fallbacks
	if ( result == NULL && strstr( file, ".dv" ) )
		result = mlt_factory_producer( "libdv", file );

	return result;
}

mlt_consumer create_consumer( char *id )
{
	return mlt_factory_consumer( id, NULL );
}

void set_properties( mlt_service service, char *namevalue )
{
	mlt_properties properties = mlt_service_properties( service );
	mlt_properties_parse( properties, namevalue );
}

void transport( mlt_producer producer )
{
	char temp[ 132 ];
	fprintf( stderr, "Press return to continue\n" );
	fgets( temp, 132, stdin );
}

int main( int argc, char **argv )
{
	int i;
	mlt_service  service = NULL;
	mlt_consumer consumer = NULL;
	mlt_producer producer = NULL;

	mlt_factory_init( "../modules" );

	// Parse the arguments
	for ( i = 1; i < argc; i ++ )
	{
		if ( !strcmp( argv[ i ], "-vo" ) )
		{
			consumer = create_consumer( argv[ ++ i ] );
			service = mlt_consumer_service( consumer );
		}
		else if ( strstr( argv[ i ], "=" ) )
		{
			set_properties( service, argv[ i ] );
		}
		else
		{
			producer = create_producer( argv[ i ] );
			service = mlt_producer_service( producer );
		}
	}

	// If we have no consumer, default to sdl
	if ( consumer == NULL )
		consumer= mlt_factory_consumer( "sdl", NULL );

	// Connect producer to consumer
	mlt_consumer_connect( consumer, mlt_producer_service( producer ) );

	// Transport functionality
	transport( producer );

/*
	// Create the producer(s)
	mlt_producer dv1 = mlt_factory_producer( "mcmpeg", file1 );
	mlt_producer dv2 = mlt_factory_producer( "mcmpeg", file2 );
	//mlt_producer dv1 = producer_ppm_init( NULL );
	//mlt_producer dv2 = producer_ppm_init( NULL );

	// Connect a producer to our sdl consumer
	mlt_consumer_connect( sdl_out, mlt_producer_service( dv1 ) );

	fprintf( stderr, "Press return to continue\n" );
	fgets( temp, 132, stdin );

	// Register producers(s) with a multitrack object
	mlt_multitrack multitrack = mlt_multitrack_init( );
	mlt_multitrack_connect( multitrack, dv1, 0 );
	mlt_multitrack_connect( multitrack, dv2, 1 );

	// Create a filter and associate it to track 0
	mlt_filter filter = mlt_factory_filter( "deinterlace", NULL );
	mlt_filter_connect( filter, mlt_multitrack_service( multitrack ), 0 );
	mlt_filter_set_in_and_out( filter, 0, 5 );

	// Create another
	mlt_filter greyscale = mlt_factory_filter( "greyscale", NULL );
	mlt_filter_connect( greyscale, mlt_filter_service( filter ), 0 );
	mlt_filter_set_in_and_out( greyscale, 0, 10 );

	// Buy a tractor and connect it to the filter
	mlt_tractor tractor = mlt_tractor_init( );
	mlt_tractor_connect( tractor, mlt_filter_service( greyscale ) );

	// Connect the tractor to the consumer
	mlt_consumer_connect( sdl_out, mlt_tractor_service( tractor ) );

	// Do stuff until we're told otherwise...

	// Close everything...
	//mlt_consumer_close( sdl_out );
	//mlt_tractor_close( tractor );
	//mlt_filter_close( filter );
	//mlt_filter_close( greyscale );
	//mlt_multitrack_close( multitrack );
	//mlt_producer_close( dv1 );
	//mlt_producer_close( dv2 );
*/

	mlt_factory_close( );

	return 0;
}
