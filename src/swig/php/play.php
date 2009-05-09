<?php
$filename = $argv[1];
dl("mlt.so");
mlt_factory_init(NULL);
$profile = new_profile("dv_ntsc");
$p = new_producer( $profile, $filename );
if ( $p ) {
	$c = new_consumer( $profile, "sdl" );
	consumer_connect( $c, $p );
	$e = properties_setup_wait_for( $c, "consumer-stopped" );
	consumer_start( $c );
	properties_wait_for( $c, $e );
	consumer_stop( $c );
	$e = NULL;
	$c = NULL;
}
$p = NULL;
$profile = NULL;
mlt_factory_close();
?>

