#include <framework/mlt.h>
#include <stdio.h>

int main( int argc, char **argv )
{
	char temp[ 132 ];
	char *file1 = NULL;
	char *file2 = NULL;

	mlt_factory_init( "../modules" );

	if ( argc >= 2 )
		file1 = argv[ 1 ];
	if ( argc >= 3 )
		file2 = argv[ 2 ];

	// Start the consumer...
	mlt_consumer sdl_out = mlt_factory_consumer( "sdl", NULL );

	fprintf( stderr, "Press return to continue\n" );
	fgets( temp, 132, stdin );

	// Create the producer(s)
	mlt_producer dv1 = mlt_factory_producer( "libdv", file1 );
	mlt_producer dv2 = mlt_factory_producer( "libdv", file2 );
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
	fprintf( stderr, "Press return to continue\n" );
	fgets( temp, 132, stdin );

	// Close everything...
	//mlt_consumer_close( sdl_out );
	//mlt_tractor_close( tractor );
	//mlt_filter_close( filter );
	//mlt_filter_close( greyscale );
	//mlt_multitrack_close( multitrack );
	//mlt_producer_close( dv1 );
	//mlt_producer_close( dv2 );

	mlt_factory_close( );

	return 0;
}
