#!/bin/env perl

# Import required modules
use mltpp;

# Not sure why the mltpp::Factory.init method fails...
mltpp::mlt_factory_init( undef );

# Create the producer
$p = new mltpp::Producer( $ARGV[0] );

if ( $p->is_valid( ) )
{
	# Loop the video
	$p->set( "eof", "loop" );

	# Create the consumer
	$c = new mltpp::Consumer( "sdl" );

	# Turn of the default rescaling
	$c->set( "rescale", "none" );

	# Connect the producer to the consumer
	$c->connect( $p );
	
	# Start the consumer
	$c->start;

	# Wait until the user stops the consumer
	while ( !$c->is_stopped ) {
		sleep( 1 );
	}
}
else
{
	print "Unable to open $ARGV[0]\n";
}

