
#include <Mlt.h>
using namespace Mlt;

int main( int argc, char **argv )
{
	Factory::init( NULL );
	Producer producer( argv[ 1 ] );
	Consumer consumer;
	consumer.set( "rescale", "none" );
	consumer.connect( producer );
	consumer.run( );
	return 0;
}
