#!/usr/bin/env ruby

# Required modules
require 'mltpp'

# Create the mlt system
Mltpp::Factory::init

# Get and check the argument
file = ARGV.shift
name = ARGV.shift
size = ARGV.shift
size = "192x144" if size.nil?
raise "Usage: thumbs.rb file name [ size ]" if file.nil? || name.nil?

# Create the producer
producer = Mltpp::Producer.new( file )
raise "Unable to load #{file}" if !producer.is_valid

# Construct the playlist
playlist = Mltpp::Playlist.new( )

# Get the out point
out = producer.get_int( "out" );

# Calculate position of frames
[ 0, 0.25, 0.5, 0.75, 1 ].each { |x| playlist.append( producer, x*out, x*out ) }

# Create the thumb nail generator
generator = Mltpp::Consumer.new( "avformat", "#{name}%d.jpg" )
generator.set( "real_time", "0" )
generator.set( "progressive", "1" )
generator.set( "size", size )

# Connect the consumer
generator.connect( playlist );
generator.run

