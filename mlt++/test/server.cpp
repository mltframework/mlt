#include <MltMiracle.h>
using namespace Mlt;

int main( int argc, char **argv )
{
	Miracle server( "miracle++" );
	server.start( );
	server.execute( "uadd sdl" );
	server.execute( "play u0" );
	server.wait_for_shutdown( );
	return 0;
}

