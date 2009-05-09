#!/usr/bin/env tclsh

load mlt.so
Factory_init
set profile [Profile]
set arg1 [lindex $argv 0]
set p [Factory_producer $profile loader $arg1]
set c [Factory_consumer $profile sdl ""]
Properties_set $c "rescale" "none"
Consumer_connect $c $p
Consumer_start $c
while { ![Consumer_is_stopped $c] } {
	after 1000
}
delete_Consumer $c
delete_Producer $p
Factory_close
