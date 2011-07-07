#include <stdio.h>
#include <unistd.h>
#include <framework/mlt.h>

mlt_producer create_playlist( int argc, char **argv )
{
	// We're creating a playlist here
	mlt_playlist playlist = mlt_playlist_init( );

	// We need the playlist properties to ensure clean up
	mlt_properties properties = mlt_playlist_properties( playlist );

	// Loop through each of the arguments
	int i = 0;
	for ( i = 1; i < argc; i ++ )
	{
		// Definie the unique key
		char key[ 256 ];

		// Create the producer
		mlt_producer producer = mlt_factory_producer( NULL, argv[ i ] );

		// Add it to the playlist
		mlt_playlist_append( playlist, producer );

		// Create a unique key for this producer
		sprintf( key, "producer%d", i );

		// Now we need to ensure the producers are destroyed
		mlt_properties_set_data( properties, key, producer, 0, ( mlt_destructor )mlt_producer_close, NULL );
	}

	// Return the playlist as a producer
	return mlt_playlist_producer( playlist );
}

mlt_producer create_tracks( int argc, char **argv )
{
	// Create the field
	mlt_field field = mlt_field_init( );

	// Obtain the multitrack
	mlt_multitrack multitrack = mlt_field_multitrack( field );

	// Obtain the tractor
	mlt_tractor tractor = mlt_field_tractor( field );

	// Obtain a composite transition
	mlt_transition transition = mlt_factory_transition( "composite", "10%/10%:15%x15%" );

	// Create track 0
	mlt_producer track0 = create_playlist( argc, argv );

	// Get the length of track0
	mlt_position length = mlt_producer_get_playtime( track0 );

	// Create the watermark track
	mlt_producer track1 = mlt_factory_producer( NULL, "pango:" );

	// Get the properties of track1
	mlt_properties properties = mlt_producer_properties( track1 );

	// Set the properties
	mlt_properties_set( properties, "text", "Hello\nWorld" );
	mlt_properties_set_position( properties, "in", 0 );
	mlt_properties_set_position( properties, "out", length - 1 );
	mlt_properties_set_position( properties, "length", length );

	// Now set the properties on the transition
	properties = mlt_transition_properties( transition );
	mlt_properties_set_position( properties, "in", 0 );
	mlt_properties_set_position( properties, "out", length - 1 );

	// Add our tracks to the multitrack
	mlt_multitrack_connect( multitrack, track0, 0 );
	mlt_multitrack_connect( multitrack, track1, 1 );

	// Now plant the transition
	mlt_field_plant_transition( field, transition, 0, 1 );

	// Now set the properties on the transition
	properties = mlt_tractor_properties( tractor );

	// Ensure clean up and set properties correctly
	mlt_properties_set_data( properties, "multitrack", multitrack, 0, ( mlt_destructor )mlt_multitrack_close, NULL );
	mlt_properties_set_data( properties, "field", field, 0, ( mlt_destructor )mlt_field_close, NULL );
	mlt_properties_set_data( properties, "track0", track0, 0, ( mlt_destructor )mlt_producer_close, NULL );
	mlt_properties_set_data( properties, "track1", track1, 0, ( mlt_destructor )mlt_producer_close, NULL );
	mlt_properties_set_data( properties, "transition", transition, 0, ( mlt_destructor )mlt_transition_close, NULL );
	mlt_properties_set_position( properties, "length", length );
	mlt_properties_set_position( properties, "out", length - 1 );

	// Return the tractor
	return mlt_tractor_producer( tractor );
}

int main( int argc, char **argv )
{
	// Initialise the factory
	if ( mlt_factory_init( NULL ) == 0 )
	{
		// Create the default consumer
		mlt_consumer hello = mlt_factory_consumer( NULL, NULL );

		// Create a producer using the default normalising selecter
		mlt_producer world = create_tracks( argc, argv );

		// Connect the producer to the consumer
		mlt_consumer_connect( hello, mlt_producer_service( world ) );

		// Start the consumer
		mlt_consumer_start( hello );

		// Wait for the consumer to terminate
		while( !mlt_consumer_is_stopped( hello ) )
			sleep( 1 );

		// Close the consumer
		mlt_consumer_close( hello );

		// Close the producer
		mlt_producer_close( world );

		// Close the factory
		mlt_factory_close( );
	}
	else
	{
		// Report an error during initialisation
		fprintf( stderr, "Unable to locate factory modules\n" );
	}

	// End of program
	return 0;
}

