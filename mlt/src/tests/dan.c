#include "mlt_producer.h"
#include "mlt_consumer.h"
#include "mlt_filter.h"
#include "mlt_tractor.h"
#include "mlt_transition.h"
#include "mlt_multitrack.h"

#include "producer_libdv.h"
#include "filter_deinterlace.h"
#include "consumer_sdl.h"
#include "producer_ppm.h"
#include "producer_pixbuf.h"
#include "transition_composite.h"

#include <stdio.h>

int main( int argc, char **argv )
{
	char temp[ 132 ];
	char *file1 = NULL;
	char *file2 = NULL;

	if ( argc >= 2 )
		file1 = argv[ 1 ];
	if ( argc >= 3 )
		file2 = argv[ 2 ];

	// Start the consumer...
	mlt_consumer sdl_out = consumer_sdl_init( NULL );

	// Create the producer(s)
	mlt_producer dv1 = producer_libdv_init( file1 );
	//mlt_producer dv1 = producer_pixbuf_init( file1 );
	//mlt_producer dv2 = producer_libdv_init( file2 );
	mlt_producer dv2 = producer_pixbuf_init( file2 );

	// Register producers(s) with a multitrack object
	mlt_multitrack multitrack = mlt_multitrack_init( );
	mlt_multitrack_connect( multitrack, dv1, 0 );
	mlt_multitrack_connect( multitrack, dv2, 1 );

	// Create a filter and associate it to track 0
	mlt_filter filter = filter_deinterlace_init( NULL );
	mlt_filter_connect( filter, mlt_multitrack_service( multitrack ), 0 );
	mlt_filter_set_in_and_out( filter, 0, 1000 );

	// Define a transition
	mlt_transition transition = transition_composite_init( NULL );
	mlt_transition_connect( transition, mlt_filter_service( filter ), 0, 1 );
	mlt_transition_set_in_and_out( transition, 0, 1000 );

	// Buy a tractor and connect it to the filter
	mlt_tractor tractor = mlt_tractor_init( );
	mlt_tractor_connect( tractor, mlt_transition_service( transition ) );

	// Connect the tractor to the consumer
	mlt_consumer_connect( sdl_out, mlt_tractor_service( tractor ) );

	// Do stuff until we're told otherwise...
	fprintf( stderr, "Press return to continue\n" );
	fgets( temp, 132, stdin );

	// Close everything...
	mlt_consumer_close( sdl_out );
	mlt_tractor_close( tractor );
	mlt_filter_close( filter );
	mlt_multitrack_close( multitrack );
	mlt_producer_close( dv1 );
	mlt_producer_close( dv2 );

	return 0;
}
