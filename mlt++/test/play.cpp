
#include <Mlt.h>
using namespace Mlt;

int main( int, char **argv )
{
	Factory::init( NULL );
	Profile profile;
	Producer producer( profile, argv[ 1 ] );
	Consumer consumer( profile );
	consumer.set( "rescale", "none" );
	consumer.connect( producer );
	consumer.run( );
	return 0;
}
