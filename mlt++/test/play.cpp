
#include <iostream>
#include <string>
using namespace std;

#include <Mlt.h>
using namespace Mlt;

#include <time.h>

int main( int argc, char **argv )
{
	Factory::init( NULL );
	Producer *producer = new Producer( argv[ 1 ] );
	if ( !producer->is_valid( ) )
	{
		cerr << "Can't construct producer for " << argv[ 1 ] << endl;
		return 0;
	}
	Consumer *consumer = new Consumer( "sdl" );
	consumer->set( "rescale", "none" );
	Filter *filter = new Filter( "greyscale" );
	filter->connect( *producer );
	consumer->connect( *filter );
	consumer->start( );
	struct timespec tm = { 1, 0 };
	while ( !consumer->is_stopped( ) )
		nanosleep( &tm, NULL );
	consumer->stop( );
	delete consumer;
	delete producer;
	delete filter;
	Factory::close( );
	return 0;
}
