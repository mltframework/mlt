#!/usr/bin/env lua

require("mlt")

mlt.Factory_init()
profile = mlt.Profile()
producer = mlt.Producer( profile, arg[1] )
if producer:is_valid() then
  consumer = mlt.Consumer( profile, "sdl" )
  consumer:set( "rescale", "none" )
  consumer:set( "terminate_on_pause", 1 )
  consumer:connect( producer )
  event = consumer:setup_wait_for( "consumer-stopped" )
  consumer:start()
  consumer:wait_for( event )
else
  print( "Unable to open "..arg[1] )
end
mlt.Factory_close()
