#include <MltMiracle.h>
using namespace Mlt;

int main( int argc, char **argv )
{
	Miracle server( "miracle++" );
	server.start( );
	server.wait_for_shutdown( );
	return 0;
}

