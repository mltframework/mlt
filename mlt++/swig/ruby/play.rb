#!/usr/bin/env ruby

# Import required modules
require 'mltpp'

# Create the mlt system
Mltpp::Factory::init

# Get and check the argument
file = ARGV.shift
raise "Usage: test.rb file" if file.nil?

# Create the producer
producer = Mltpp::Factory::producer( file )
raise "Unable to load #{file}" if !producer.is_valid

# Create the consumer
consumer = Mltpp::Consumer.new( "sdl" )
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

