#!/usr/bin/env tclsh

load mltpp.so
mltpp.Factory.init
set profile [Profile]
set arg1 [lindex $argv 0]
set p [factory_producer $profile fezzik $arg1]
set c [factory_consumer $profile sdl ""]
set r [mlt_consumer_properties $c]
mlt_properties_set $r "rescale" "none"
consumer_connect $c $p
mlt_consumer_start $c
while { ![mlt_consumer_is_stopped $c] } {
	after 1000
}
mlt_consumer_close $c
mlt_producer_close $p
factory_close
