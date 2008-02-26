
#include <framework/mlt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



int main( int argc, char **argv )
{
	mlt_properties p = mlt_properties_parse_yaml( argv[1] );
	mlt_properties q = mlt_properties_new();
	mlt_properties_set_data( q, "metadata", p, 0, ( mlt_destructor )mlt_properties_close, ( mlt_serialiser )mlt_properties_serialise_yaml ); 
	printf( "%s", mlt_properties_get( q, "metadata" ) );
	mlt_properties_close( q );
	
	mlt_repository repo = mlt_factory_init( NULL );
	mlt_properties metadata = mlt_repository_metadata( repo, producer_type, "avformat" );
	if ( metadata )
	{
		char *s = mlt_properties_serialise_yaml( metadata );
		printf( "%s", s );
		free( s );
	}
	mlt_factory_close();
	return 0;
}
