#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
using namespace std;

#include <Mlt.h>
using namespace Mlt;

class Custom : public Miracle
{
	private:
		Event *event;
		Profile profile;

	public:
		Custom( char *name = "Custom", int port = 5290, char *config = NULL ) :
			Miracle( name, port, config ),
			event( NULL )
		{
			// Ensure that we receive the westley document before it's deserialised
			set( "push-parser-off", 1 );
		}

		virtual ~Custom( )
		{
			delete event;
		}

		// Optional step - receive the westley document and do something with it
		Response *received( char *command, char *document )
		{
			cerr << document << endl;
			Producer producer( profile, "westley-xml", document );
			return push( command, &producer );
		}

		// Push handling - clear the playlist, append, seek to beginning and play
		Response *push( char *command, Service *service )
		{
			Playlist playlist( ( mlt_playlist )( unit( 0 )->get_data( "playlist" ) ) );
			Producer producer( *service );
			if ( producer.is_valid( ) && playlist.is_valid( ) )
			{
				playlist.lock( );
				playlist.clear( );
				playlist.append( producer );
				playlist.seek( 0 );
				playlist.set_speed( 1 );
				playlist.unlock( );
				return new Response( 200, "OK" );
			}
			return new Response( 400, "Invalid" );
		}

		// Custom command execution
		Response *execute( char *command )
		{
			Response *response = NULL;

			if ( !strcmp( command, "debug" ) )
			{
				// Example of a custom command 
				response = new Response( 200, "Diagnostics output" );
				for( int i = 0; unit( i ) != NULL; i ++ )
				{
					Properties *properties = unit( i );
					stringstream output;
					output << string( "Unit " ) << i << endl;
					for ( int j = 0; j < properties->count( ); j ++ )
						output << properties->get_name( j ) << " = " << properties->get( j ) << endl;
					response->write( output.str( ).c_str( ) );
				}
			}
			else
			{
				// Use the default command processing
				response = Miracle::execute( command );
			}

			// If no event exists and the first unit has been added...
			if ( event == NULL && unit( 0 ) != NULL )
			{
				// Set up the event handling
				Consumer consumer( ( mlt_consumer )( unit( 0 )->get_data( "consumer" ) ) );
				event = consumer.listen( "consumer-frame-render", this, ( mlt_listener )frame_render );

				// In this custom case, we'll loop everything on the unit
				Playlist playlist( ( mlt_playlist )( unit( 0 )->get_data( "playlist" ) ) );
				playlist.set( "eof", "loop" );
			}

			return response;
		}

		// Callback for frame render notification
		static void frame_render( mlt_consumer consumer, Custom *self, mlt_frame frame_ptr )
		{
			Frame frame( frame_ptr );
			self->frame_render_event( frame );
		}

		// Remove all supers and attributes
		void frame_render_event( Frame &frame )
		{
			// Fetch the c double ended queue structure
			mlt_deque deque = ( mlt_deque )frame.get_data( "data_queue" );

			// While the deque isn't empty
			while( deque != NULL && mlt_deque_peek_back( deque ) != NULL )
			{
				// Fetch the c properties structure
				mlt_properties cprops = ( mlt_properties )mlt_deque_pop_back( deque );

				// For fun, convert it to c++ and output it :-)
				Properties properties( cprops );
				properties.debug( );

				// Wipe it
				mlt_properties_close( cprops );
			}
		}
};
	
int main( int argc, char **argv )
{
	Custom server( "Server" );
	server.start( );
	server.execute( "uadd sdl" );
	server.execute( "play u0" );
	server.wait_for_shutdown( );
	return 0;
}

