#include <Mlt.h>
using namespace Mlt;

void play(const char *filename)
{
	Profile profile; // defaults to dv_pal
	Producer producer(profile, filename);
	Consumer consumer(profile); // defaults to sdl

	// Prevent scaling to the profile size.
	// Let the sdl consumer do all scaling.
	consumer.set("rescale", "none");

	// Automatically exit at end of file.
	consumer.set("terminate_on_pause", 1);

	consumer.connect(producer);
	consumer.run();
	consumer.stop();
}

int main( int, char **argv )
{
	Factory::init();
	play(argv[1]);
	Factory::close();
	return 0;
}
