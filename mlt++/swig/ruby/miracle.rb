require 'mltpp'
miracle = Mltpp::Miracle.new( "miracle-ruby" )
miracle.start
miracle.wait_for_shutdown

