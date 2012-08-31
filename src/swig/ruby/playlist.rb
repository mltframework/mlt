#!/usr/bin/env ruby

# Import required modules
require 'mlt'

# Create the mlt system
Mlt::Factory::init

# Establish the mlt profile
profile = Mlt::Profile.new

# Get and check the argument
file = ARGV.shift
raise "Usage: test.rb file1 file2" if file.nil?
file2 = ARGV.shift
raise "Usage: test.rb file1 file2" if file2.nil?

pls = Mlt::Playlist.new(profile)

# Create the producer
producer = Mlt::Factory::producer( profile, file )
raise "Unable to load #{file}" if !producer.is_valid

producer2 = Mlt::Factory::producer( profile, file2 )
raise "Unable to load #{file2}" if !producer2.is_valid

pls.append(producer)
#pls.repeat(0, 2)
pls.append(producer2)

# Create the consumer
consumer = Mlt::Consumer.new( profile, "sdl" )
raise "Unable to open sdl consumer" if !consumer.is_valid

# Turn off the default rescaling
consumer.set( "rescale", "none" )
consumer.set("volume", 0.1)
consumer.set("terminate_on_pause", 1)

Mlt::PlaylistNextListener.new(pls, Proc.new { |i|
    info = pls.clip_info(i)
    puts "finished playing #{info.resource}\n"
})


# Set up a 'wait for' event
event = consumer.setup_wait_for( "consumer-stopped" )

# Start the consumer
consumer.start

# Connect the producer to the consumer
consumer.connect( pls )

# Wait until the user stops the consumer
consumer.wait_for( event )

# Clean up consumer
consumer.stop

