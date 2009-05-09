#!/usr/bin/env perl

# Import required modules
use mlt;

# Not sure why the mlt::Factory.init method fails...
mlt::mlt_factory_init( undef );

# Establish the MLT profile
$profile = new mlt::Profile( undef );

# Create the producer
$p = new mlt::Producer( $profile, $ARGV[0] );

if ( $p->is_valid( ) )
{
	# Loop the video
	$p->set( "eof", "loop" );

	# Create the consumer
	$c = new mlt::FilteredConsumer( $profile, "sdl" );

	# Turn of the default rescaling
	$c->set( "rescale", "none" );

	# Connect the producer to the consumer
	$c->connect( $p );
	
	$e = $c->setup_wait_for( "consumer-stopped" );

	# Start the consumer
	$c->start;

	# Wait until the user stops the consumer
	$c->wait_for( $e );

	$e = undef;
	$c = undef;
	$p = undef;
}
else
{
	print "Unable to open $ARGV[0]\n";
}

mlt::mlt_factory_close( );
