
#include <framework/mlt.h>

#include <stdio.h>

int main( int argc, char **argv )
{
	char temp[ 132 ];
	char *file1 = NULL;
	char *file2 = NULL;

	mlt_factory_init( "../modules" );

	if ( argc < 3 )
	{
		fprintf( stderr, "usage: dissolve file1.mpeg file2.mpeg\n" );
		return 1;
	}
	else
	{
		file1 = argv[ 1 ];
		file2 = argv[ 2 ];
	}

	// Start the consumer...
	mlt_consumer consumer = mlt_factory_consumer( "sdl", "PAL" );

	// Create the producer(s)
	mlt_producer dv1 = mlt_factory_producer( "mcmpeg", file1 );
	mlt_producer dv2 = mlt_factory_producer( "mcmpeg", file2 );

	mlt_playlist playlist1 = mlt_playlist_init();
	mlt_playlist_append_io( playlist1, dv1, 0.0, 5.0 );
	mlt_playlist_blank( playlist1, 1.0 );

	mlt_playlist playlist2 = mlt_playlist_init();
	mlt_playlist_blank( playlist2, 2.9 );
	mlt_playlist_append( playlist2, dv2 );
	
	// Register producers(s) with a multitrack object
	mlt_multitrack multitrack = mlt_multitrack_init( );
	mlt_multitrack_connect( multitrack, mlt_playlist_producer( playlist1 ), 0 );
	mlt_multitrack_connect( multitrack, mlt_playlist_producer( playlist2 ), 1 );

	// Define a transition
	mlt_transition transition = mlt_factory_transition( "luma", NULL );
	mlt_transition_connect( transition, mlt_multitrack_service( multitrack ), 0, 1 );
	mlt_transition_set_in_and_out( transition, 3.0, 5.0 );

	// Buy a tractor and connect it to the filter
	mlt_tractor tractor = mlt_tractor_init( );
	mlt_tractor_connect( tractor, mlt_transition_service( transition ) );

	// Connect the tractor to the consumer
	mlt_consumer_connect( consumer, mlt_tractor_service( tractor ) );

	// Do stuff until we're told otherwise...
	fprintf( stderr, "Press return to continue\n" );
	fgets( temp, 132, stdin );

	// Close everything...
	mlt_consumer_close( consumer );
	mlt_tractor_close( tractor );
	mlt_transition_close( transition );
	mlt_multitrack_close( multitrack );
	mlt_playlist_close( playlist1 );
	mlt_playlist_close( playlist2 );
	mlt_producer_close( dv1 );
	mlt_producer_close( dv2 );

	return 0;
}
