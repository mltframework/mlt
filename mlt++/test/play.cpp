
#include <iostream>
#include <string>
using namespace std;

#include <mlt++/MltFactory.h>
using namespace Mlt;

#include <time.h>

int main( int argc, char **argv )
{
	Factory::init( NULL );
	Consumer *consumer = Factory::consumer( "sdl" );
	consumer->set( "rescale", "none" );
	Producer *producer = Factory::producer( argv[ 1 ] );
	Filter *filter = Factory::filter( "greyscale" );
	filter->connect( *producer );
	consumer->connect( *filter );
	consumer->start( );
	while ( !consumer->is_stopped( ) )
	{
		struct timespec tm = { 1, 0 };
		nanosleep( &tm, NULL );
	}
	consumer->stop( );
	delete consumer;
	delete producer;
	delete filter;
	return 0;
}
