#!/usr/bin/ruby

# Import required modules
require 'mltpp'

# Create the mlt system
Mltpp::Factory::init

# Get and check the argument
file = ARGV.shift
raise "Usage: test.rb file" if file.nil?

# Create the producer
producer = Mltpp::Factory::producer( file )
raise "Unable to load #{file}" if producer.is_valid == 0

# Create the consumer
consumer = Mltpp::Factory::consumer( "sdl" )
raise "Unable to open sdl consumer" if consumer.is_valid == 0

# Turn off the default rescaling
consumer.set( "rescale", "none" )

# Start the consumer
consumer.start

# Connect the producer to the consumer
consumer.connect( producer )

# Wait until the user stops the consumer
while consumer.is_stopped == 0 
	sleep( 1 )
end

# Clean up consumer
consumer.stop

