
#include <framework/mlt.h>

#include <stdio.h>

int main( int argc, char **argv )
{
	char temp[ 132 ];
	char *file1 = NULL;
	char *text = NULL;

	mlt_factory_init( "../modules" );

	if ( argc < 3 )
	{
		fprintf( stderr, "usage: pango file.mpeg  text_to_display\n" );
		return 1;
	}
	else
	{
		file1 = argv[ 1 ];
		text = argv[ 2 ];
	}

	// Start the consumer...
	mlt_consumer consumer = mlt_factory_consumer( "sdl", "PAL" );

	// Create the producer(s)
	mlt_playlist pl1 = mlt_playlist_init();
	mlt_producer dv1 = mlt_factory_producer( "mcmpeg", file1 );
	mlt_playlist_append( pl1, dv1 );

	mlt_playlist pl2 = mlt_playlist_init();
	mlt_producer title = mlt_factory_producer( "pango", NULL ); //"<span font_desc=\"Sans Bold 36\">Mutton <span font_desc=\"Luxi Serif Bold Oblique 36\">Lettuce</span> Tomato</span>" );
	mlt_playlist_append( pl2, title );
	mlt_properties_set_int( mlt_producer_properties( title ), "video_standard", mlt_video_standard_pal );
	mlt_properties_set( mlt_producer_properties( title ), "font", "Sans Bold 36" );
	mlt_properties_set( mlt_producer_properties( title ), "text", text );
	mlt_properties_set_int( mlt_producer_properties( title ), "bgcolor", 0x0000007f );
	mlt_properties_set_int( mlt_producer_properties( title ), "pad", 8 );
	mlt_properties_set_int( mlt_producer_properties( title ), "align", 1 );
	mlt_properties_set_int( mlt_producer_properties( title ), "x", 20 );
	mlt_properties_set_int( mlt_producer_properties( title ), "y", 40 );
	mlt_properties_set_double( mlt_producer_properties( title ), "mix", 0.8 );

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
	mlt_producer_close( title );

	return 0;
}
