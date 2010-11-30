#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Import required modules
import mlt

# Start the mlt system
mlt.Factory().init( )

# Create the consumer
c = mlt.Consumer( mlt.Profile(), "avformat" )

# Ask for video codecs supports
c.set( 'vcodec', 'list' )

# Start the consumer to generate the list
c.start()

# Get the vcodec property
codecs = mlt.Properties( c.get_data( 'vcodec' ) )

# Print the list of codecs
for i in range( 0, codecs.count()):
	print codecs.get( i )
