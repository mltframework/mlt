#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Import required modules
import mlt
import time
import sys

# Start the mlt system
mlt.Factory().init( )

# Establish a profile
profile = mlt.Profile( )

# Create the producer
p = mlt.Producer( profile, sys.argv[1] )

if p:
	# Create the consumer
	c = mlt.Consumer( profile, "sdl" )

	# Turn off the default rescaling
	c.set( "rescale", "none" )
	
	# Connect the producer to the consumer
	c.connect( p )
	
	# Start the consumer
	c.start( )
	
	# Wait until the user stops the consumer
	while c.is_stopped( ) == 0:
		time.sleep( 1 )
else:
	# Diagnostics
	print "Unable to open ", sys.argv[ 1 ]

