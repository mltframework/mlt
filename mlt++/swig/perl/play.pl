#!/usr/bin/env perl

# Import required modules
use mltpp;

# Not sure why the mltpp::Factory.init method fails...
mltpp::mlt_factory_init( undef );

# Establish the MLT profile
$profile = new mltpp::Profile( undef );

# Create the producer
$p = new mltpp::Producer( $profile, $ARGV[0] );

if ( $p->is_valid( ) )
{
	# Loop the video
	$p->set( "eof", "loop" );

	# Create the consumer
	$c = new mltpp::FilteredConsumer( $profile, "sdl" );

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

mltpp::mlt_factory_close( );
