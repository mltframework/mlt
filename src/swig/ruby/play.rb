#!/usr/bin/env ruby

# Import required modules
require 'mlt'

# Create the mlt system
Mlt::Factory::init

# Establish the mlt profile
profile = Mlt::Profile.new

# Get and check the argument
file = ARGV.shift
raise "Usage: test.rb file" if file.nil?

# Create the producer
producer = Mlt::Factory::producer( profile, file )
raise "Unable to load #{file}" if !producer.is_valid

# Create the consumer
consumer = Mlt::Consumer.new( profile, "sdl" )
raise "Unable to open sdl consumer" if !consumer.is_valid

# Turn off the default rescaling
consumer.set( "rescale", "none" )

# Set up a 'wait for' event
event = consumer.setup_wait_for( "consumer-stopped" )

# Start the consumer
consumer.start

# Connect the producer to the consumer
consumer.connect( producer )

# Wait until the user stops the consumer
consumer.wait_for( event )

# Clean up consumer
consumer.stop

