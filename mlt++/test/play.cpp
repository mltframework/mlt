
#include <iostream>
#include <string>
using namespace std;

#include <Mlt.h>
using namespace Mlt;

#include <time.h>

int main( int argc, char **argv )
{
	Factory::init( NULL );
	Producer producer( argv[ 1 ] );
	Consumer consumer( "sdl" );
	consumer.set( "rescale", "none" );
	consumer.connect( producer );
	Event *event = consumer.setup_wait_for( "consumer-stopped" );
	consumer.start( );
	consumer.wait_for( event, false );
	return 0;
}
