/*
 * factory.c for Linsys SDI consumer
*/

#include <framework/mlt.h>
#include <string.h>

extern mlt_consumer consumer_SDIstream_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

MLT_REPOSITORY
{
	MLT_REGISTER( consumer_type, "linsys_sdi", consumer_SDIstream_init );
}
