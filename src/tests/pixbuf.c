
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
		fprintf( stderr, "usage: pixbuf file.mpeg file.{png,jpg,etc}\n" );
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
	mlt_playlist pl1 = mlt_playlist_init();
	mlt_producer dv1 = mlt_factory_producer( "mcmpeg", file1 );
	mlt_playlist_append( pl1, dv1 );

	mlt_playlist pl2 = mlt_playlist_init();
	mlt_producer overlay = mlt_factory_producer( "pixbuf", file2 );
	mlt_playlist_append( pl2, overlay );
	mlt_properties_set_int( mlt_producer_properties( overlay ), "video_standard", mlt_video_standard_pal );
	mlt_properties_set_int( mlt_producer_properties( overlay ), "x", 600 );
	mlt_properties_set_int( mlt_producer_properties( overlay ), "y", 460 );
	mlt_properties_set_double( mlt_producer_properties( overlay ), "mix", 0.8 );

	// Register producers(s) with a multitrack object
	mlt_multitrack multitrack = mlt_multitrack_init( );
	mlt_multitrack_connect( multitrack, mlt_playlist_producer( pl1 ), 0 );
	mlt_multitrack_connect( multitrack, mlt_playlist_producer( pl2 ), 1 );

	// Define a transition
	mlt_transition transition = mlt_factory_transition( "composite", NULL );
	mlt_transition_connect( transition, mlt_multitrack_service( multitrack ), 0, 1 );
	mlt_transition_set_in_and_out( transition, 0.0, 9999.0 );

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
	mlt_playlist_close( pl1 );
	mlt_playlist_close( pl2 );
	mlt_producer_close( dv1 );
	mlt_producer_close( overlay );

	return 0;
}
