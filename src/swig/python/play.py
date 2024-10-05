#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Import required modules
from __future__ import print_function
import mlt7 as mlt
import time
import sys

# Start the mlt system
mlt.Factory().init()

# Establish a default (usually "dv_pal") profile
profile = mlt.Profile()

# Create the producer
p = mlt.Producer(profile, sys.argv[1])

if p.is_valid():
	# Derive a profile based on the producer
	profile.from_producer(p)

	# Reload the producer using the derived profile
	p = mlt.Producer(profile, sys.argv[1])

	# Create the consumer
	c = mlt.Consumer(profile, "sdl2")

	if not c.is_valid():
		print("Failed to open the sdl2 consumer")
		sys.exit(1)

	# Connect the producer to the consumer
	c.connect(p)
	
	# Start the consumer
	c.start()
	
	# Wait until the user stops the consumer
	while not c.is_stopped():
		time.sleep(1)
else:
	# Diagnostics
	print("Unable to open ", sys.argv[1])
	sys.exit(1)
