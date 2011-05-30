/*
 * factory.c for Linsys SDI consumer
*/

#include <framework/mlt.h>
#include <limits.h>
#include <string.h>

extern mlt_consumer consumer_SDIstream_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/linsys/%s", mlt_environment( "MLT_DATA" ), (char*) data );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	MLT_REGISTER( consumer_type, "sdi", consumer_SDIstream_init );
	MLT_REGISTER_METADATA( consumer_type, "sdi", metadata, "consumer_sdi.yml" );
}
