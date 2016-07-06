/*
 * consumer_xgl.c
 * Copyright (C) 2012 Christophe Thommeret
 * Author: Christophe Thommeret <hftom@free.fr>
 * Based on Nehe's GLX port by Mihael.Vrbanec@stud.uni-karlsruhe.de
 * http://nehe.gamedev.net/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glext.h>

#include <GL/glx.h>

#include <X11/keysym.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include <framework/mlt.h>


#define STARTWIDTH 1280
#define STARTHEIGHT 720


extern int XInitThreads();



typedef struct consumer_xgl_s *consumer_xgl;

struct consumer_xgl_s
{
	struct mlt_consumer_s parent;
	mlt_properties properties;
	mlt_deque queue;
	pthread_t thread;
	int joined;
	int running;
	int playing;
	int xgl_started;
};


typedef struct
{
	pthread_t thread;
	int running;
} thread_video;


typedef struct
{
	int width;
	int height;
	double aspect_ratio;
	GLuint texture;
	pthread_mutex_t mutex;
	int new;
	mlt_frame mlt_frame_ref;
} frame_new;


typedef struct
{
	int width;
	int height;
	GLuint fbo;
	GLuint texture;
} fbo;


typedef struct
{
	Display *dpy;
    int screen;
    Window win;
    GLXContext ctx;
} HiddenContext;


typedef struct
{
    Display *dpy;
    int screen;
    Window win;
    GLXContext ctx;
    XSetWindowAttributes attr;
    int x, y;
    unsigned int width, height;
    unsigned int depth;
} GLWindow;


static GLWindow GLWin;
static HiddenContext hiddenctx;

static frame_new new_frame;
static fbo fb;
static thread_video vthread;
static consumer_xgl xgl;
static mlt_filter glsl_manager;



static void* video_thread( void *arg );

static void update()
{
	int _width = GLWin.width;
	int _height = GLWin.height;
	GLfloat left, right, top, bottom;
	GLfloat war = (GLfloat)_width/(GLfloat)_height;

	if ( war < new_frame.aspect_ratio ) {
		left = -1.0;
		right = 1.0;
		top = war / new_frame.aspect_ratio;
		bottom = -war / new_frame.aspect_ratio;
	}
	else {
		top = 1.0;
		bottom = -1.0;
		left = -new_frame.aspect_ratio / war;
		right = new_frame.aspect_ratio / war;
	}

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glLoadIdentity();

	glPushMatrix();

	glTranslatef( _width/2, _height/2, 0 );
	glScalef( _width/2, _height/2, 1.0 );

	glBindTexture( GL_TEXTURE_2D, fb.texture );

	glBegin( GL_QUADS );
		glTexCoord2f( 0.0f, 0.0f ); glVertex2f( left, top );
		glTexCoord2f( 0.0f, 1.0f ); glVertex2f( left, bottom );
		glTexCoord2f( 1.0f, 1.0f ); glVertex2f( right, bottom );
		glTexCoord2f( 1.0f, 0.0f ); glVertex2f( right, top );
	glEnd();

	glPopMatrix();
	
	glXSwapBuffers( GLWin.dpy, GLWin.win );
	
	if ( !vthread.running ) {
		pthread_create( &vthread.thread, NULL, video_thread, NULL );
		vthread.running = 1;
	}
}



static void show_frame()
{
	if ( (fb.width != new_frame.width) || (fb.height != new_frame.height) ) {
		glDeleteFramebuffers( 1, &fb.fbo );
		glDeleteTextures( 1, &fb.texture );
		fb.fbo = 0;
		fb.width = new_frame.width;
		fb.height = new_frame.height;
		glGenFramebuffers( 1, &fb.fbo );
		glGenTextures( 1, &fb.texture );
		glBindTexture( GL_TEXTURE_2D, fb.texture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, fb.width, fb.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glBindFramebuffer( GL_FRAMEBUFFER, fb.fbo );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.texture, 0 );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}

	glPushAttrib(GL_VIEWPORT_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
	
	glBindFramebuffer( GL_FRAMEBUFFER, fb.fbo );
	
	glViewport( 0, 0, new_frame.width, new_frame.height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0.0, new_frame.width, 0.0, new_frame.height, -1.0, 1.0 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, new_frame.texture );

	glBegin( GL_QUADS );
		glTexCoord2f( 0.0f, 0.0f ); glVertex2f( 0.0f, 0.0f );
		glTexCoord2f( 0.0f, 1.0f ); glVertex2f( 0.0f, new_frame.height );
		glTexCoord2f( 1.0f, 1.0f ); glVertex2f( new_frame.width, new_frame.height );
		glTexCoord2f( 1.0f, 0.0f ); glVertex2f( new_frame.width, 0.0f );
	glEnd();

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	mlt_events_fire( MLT_CONSUMER_PROPERTIES(&xgl->parent), "consumer-frame-show", new_frame.mlt_frame_ref, NULL );
	mlt_frame_close( new_frame.mlt_frame_ref );
	new_frame.mlt_frame_ref = NULL;

	glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
	
	update();
	
	new_frame.new = 0;
}



void* video_thread( void *arg )
{
	mlt_frame next = NULL;
	mlt_consumer consumer = &xgl->parent;
	mlt_properties consumer_props = MLT_CONSUMER_PROPERTIES( consumer );
	struct timeval start, end;
	double duration = 0;
	
	gettimeofday( &start, NULL );
	
	while ( vthread.running )
	{
		// Get a frame from the attached producer
		next = mlt_consumer_rt_frame( consumer );

		if ( !mlt_properties_get_int( MLT_FILTER_PROPERTIES( glsl_manager ), "glsl_supported" ) ) {
			mlt_log_error( MLT_CONSUMER_SERVICE(consumer), "OpenGL Shading Language is not supported on this machine.\n" );
			xgl->running = 0;
			break;
		}

		// Ensure that we have a frame
		if ( next )
		{
			mlt_properties properties =  MLT_FRAME_PROPERTIES( next );
			if ( mlt_properties_get_int( properties, "rendered" ) == 1 )
			{
				// Get the image, width and height
				mlt_image_format vfmt = mlt_image_glsl_texture;
				int width = 0, height = 0;
				GLuint *image = 0;
				int error = mlt_frame_get_image( next, (uint8_t**) &image, &vfmt, &width, &height, 0 );
				if ( !error && image && width && height && !new_frame.new ) {
					new_frame.width = width;
					new_frame.height = height;
					new_frame.texture = *image;
					new_frame.mlt_frame_ref = next;
					new_frame.aspect_ratio = ((double)width / (double)height) * mlt_properties_get_double( properties, "aspect_ratio" );
					new_frame.new = 1;
					
					int loop = 200;
					while ( new_frame.new && --loop )
						usleep( 500 );
				}
				else
				{
					mlt_frame_close( next );
				}
				new_frame.new = 0;
				
				gettimeofday( &end, NULL );
				duration = 1000000.0 / mlt_properties_get_double( consumer_props, "fps" );
				duration -= ( end.tv_sec * 1000000 + end.tv_usec ) - ( start.tv_sec * 1000000 + start.tv_usec );
				if ( duration > 0 )
					usleep( (int)duration );
				gettimeofday( &start, NULL );
			}
			else
			{
				mlt_frame_close( next );
				static int dropped = 0;
				mlt_log_info( MLT_CONSUMER_SERVICE(consumer), "dropped video frame %d\n", ++dropped );
			}
		}
		else
			usleep( 1000 );
	}
	mlt_consumer_stopped( consumer );
	
	return NULL;
}



static void resizeGLScene()
{
	glXMakeCurrent( GLWin.dpy, GLWin.win, GLWin.ctx );
	
	if ( GLWin.height == 0 )
		GLWin.height = 1;
	if ( GLWin.width == 0 )
		GLWin.width = 1;
	glViewport( 0, 0, GLWin.width, GLWin.height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0.0, GLWin.width, 0.0, GLWin.height, -1.0, 1.0 );
	glMatrixMode( GL_MODELVIEW );

	update();
}



static void initGL( void )
{
	glXMakeCurrent( GLWin.dpy, GLWin.win, GLWin.ctx );
	
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClearDepth( 1.0f );
	glDepthFunc( GL_LEQUAL );
	glEnable( GL_DEPTH_TEST );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );
	glShadeModel( GL_SMOOTH );
	glEnable( GL_TEXTURE_2D );
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );

	typedef int (*GLXSWAPINTERVALSGI) ( int );
	GLXSWAPINTERVALSGI mglXSwapInterval = (GLXSWAPINTERVALSGI)glXGetProcAddressARB( (const GLubyte*)"glXSwapIntervalSGI" );
	if ( mglXSwapInterval )
		mglXSwapInterval( 1 );

	fb.fbo = 0;
	fb.width = STARTWIDTH;
	fb.height = STARTHEIGHT;
	glGenFramebuffers( 1, &fb.fbo );
	glGenTextures( 1, &fb.texture );
	glBindTexture( GL_TEXTURE_2D, fb.texture );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, fb.width, fb.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glBindFramebuffer( GL_FRAMEBUFFER, fb.fbo );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.texture, 0 );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	
	resizeGLScene();
}



static void createGLWindow()
{
	const char* title = "OpenGL consumer";
	int width = STARTWIDTH;
	int height = STARTHEIGHT;
	
	int attrListSgl[] = { GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 16, None };

	int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 16, None };

	XVisualInfo *vi;
	Colormap cmap;
	Atom wmDelete;
	Window winDummy;
	unsigned int borderDummy;

	GLWin.dpy = XOpenDisplay( 0 );
	GLWin.screen = DefaultScreen( GLWin.dpy );

	vi = glXChooseVisual( GLWin.dpy, GLWin.screen, attrListDbl );
	if ( !vi )
		vi = glXChooseVisual( GLWin.dpy, GLWin.screen, attrListSgl );

	GLWin.ctx = glXCreateContext( GLWin.dpy, vi, 0, GL_TRUE );

	cmap = XCreateColormap( GLWin.dpy, RootWindow( GLWin.dpy, vi->screen ), vi->visual, AllocNone );
	GLWin.attr.colormap = cmap;
	GLWin.attr.border_pixel = 0;

	GLWin.attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;
	GLWin.win = XCreateWindow( GLWin.dpy, RootWindow(GLWin.dpy, vi->screen), 0, 0, width, height,
		0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &GLWin.attr );
	wmDelete = XInternAtom( GLWin.dpy, "WM_DELETE_WINDOW", True );
	XSetWMProtocols( GLWin.dpy, GLWin.win, &wmDelete, 1 );
	XSetStandardProperties( GLWin.dpy, GLWin.win, title, title, None, NULL, 0, NULL );
	XMapRaised( GLWin.dpy, GLWin.win );

	glXMakeCurrent( GLWin.dpy, GLWin.win, GLWin.ctx );
	XGetGeometry( GLWin.dpy, GLWin.win, &winDummy, &GLWin.x, &GLWin.y,
		&GLWin.width, &GLWin.height, &borderDummy, &GLWin.depth );

	// Verify GLSL works on this machine
	hiddenctx.ctx = glXCreateContext( GLWin.dpy, vi, GLWin.ctx, GL_TRUE );
	if ( hiddenctx.ctx ) {
		hiddenctx.dpy = GLWin.dpy;
		hiddenctx.screen = GLWin.screen;
		hiddenctx.win = RootWindow( hiddenctx.dpy, hiddenctx.screen );
	}

	initGL();
}



static void killGLWindow()
{		
	if ( GLWin.ctx ) {
		if ( !glXMakeCurrent( GLWin.dpy, None, NULL ) ) {
			printf("Error releasing drawing context : killGLWindow\n");
		}
		glXDestroyContext( GLWin.dpy, GLWin.ctx );
		GLWin.ctx = NULL;
	}
	
	if ( hiddenctx.ctx )
		glXDestroyContext( hiddenctx.dpy, hiddenctx.ctx );

	XCloseDisplay( GLWin.dpy );
}



static void run()
{
	XEvent event;

	while ( xgl->running ) {
		while ( XPending( GLWin.dpy ) > 0 ) {
			XNextEvent( GLWin.dpy, &event );
			switch ( event.type ) {
				case Expose:
					if ( event.xexpose.count != 0 )
						break;
					break;
				case ConfigureNotify:
					if ( (event.xconfigure.width != GLWin.width) || (event.xconfigure.height != GLWin.height) ) {
						GLWin.width = event.xconfigure.width;
						GLWin.height = event.xconfigure.height;
						resizeGLScene();
					}
					break;
				case KeyPress:
					switch ( XLookupKeysym( &event.xkey, 0 ) ) {
						case XK_Escape:									
							xgl->running = 0;
							break;
						default: {
							mlt_producer producer = mlt_properties_get_data( xgl->properties, "transport_producer", NULL );
							char keyboard[ 2 ] = " ";
							void (*callback)( mlt_producer, char * ) = mlt_properties_get_data( xgl->properties, "transport_callback", NULL );
							if ( callback != NULL && producer != NULL )
							{
								keyboard[ 0 ] = ( char )XLookupKeysym( &event.xkey, 0 );
								callback( producer, keyboard );
							}
							break;
						}
					}
					break;
				case ClientMessage:
					if ( *XGetAtomName( GLWin.dpy, event.xclient.message_type ) == *"WM_PROTOCOLS" )
						xgl->running = 0;
					break;
				default:
					break;
			}
		}
		
		if ( new_frame.new )
			show_frame();
		else
			usleep( 1000 );
	}
}



void start_xgl( consumer_xgl consumer )
{
	xgl = consumer;
	
	pthread_mutex_init( &new_frame.mutex, NULL );
	new_frame.aspect_ratio = 16.0 / 9.0;
	new_frame.new = 0;
	new_frame.width = STARTWIDTH;
	new_frame.height = STARTHEIGHT;
	new_frame.mlt_frame_ref = NULL;
	
	vthread.running = 0;
	xgl->xgl_started = 1;

	createGLWindow();
	run();
	if ( vthread.running ) {
		vthread.running = 0;
		pthread_join( vthread.thread, NULL );
	}
	xgl->running = 0;
}

static void on_consumer_thread_started( mlt_properties owner, HiddenContext* context )
{
	// Initialize this thread's OpenGL state
	glXMakeCurrent( context->dpy, context->win, context->ctx );
	mlt_events_fire( MLT_FILTER_PROPERTIES(glsl_manager), "init glsl", NULL );
}

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );
static int consumer_stop( mlt_consumer parent );
static int consumer_is_stopped( mlt_consumer parent );
static void consumer_close( mlt_consumer parent );
static void *consumer_thread( void * );



/** This is what will be called by the factory - anything can be passed in
	via the argument, but keep it simple.
*/

mlt_consumer consumer_xgl_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create the consumer object
	consumer_xgl this = calloc( sizeof( struct consumer_xgl_s ), 1 );

	// If no malloc'd and consumer init ok
	if ( this != NULL && mlt_consumer_init( &this->parent, this, profile ) == 0 )
	{
		// Create the queue
		this->queue = mlt_deque_init( );

		// Get the parent consumer object
		mlt_consumer parent = &this->parent;

		// We have stuff to clean up, so override the close method
		parent->close = consumer_close;

		// get a handle on properties
		mlt_service service = MLT_CONSUMER_SERVICE( parent );
		this->properties = MLT_SERVICE_PROPERTIES( service );

		// Default scaler
		mlt_properties_set( this->properties, "rescale", "bilinear" );
		mlt_properties_set( this->properties, "deinterlace_method", "onefield" );

		// default image format
		mlt_properties_set( this->properties, "mlt_image_format", "glsl" );

		// Default buffer for low latency
		mlt_properties_set_int( this->properties, "buffer", 1 );

		// Ensure we don't join on a non-running object
		this->joined = 1;
		this->xgl_started = 0;

		// Allow thread to be started/stopped
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;

		// "init glsl" is required to instantiate glsl filters.
		glsl_manager = mlt_factory_filter( profile, "glsl.manager", NULL );
		if ( glsl_manager ) {
			mlt_events_listen( this->properties, &hiddenctx, "consumer-thread-started", (mlt_listener) on_consumer_thread_started );
		} else {
			mlt_consumer_close( parent );
			parent = NULL;
		}

		// Return the consumer produced
		return parent;
	}

	// malloc or consumer init failed
	free( this );

	// Indicate failure
	return NULL;
}



int consumer_start( mlt_consumer parent )
{
	consumer_xgl this = parent->child;

	if ( !this->running )
	{
		consumer_stop( parent );

		this->running = 1;
		this->joined = 0;

		pthread_create( &this->thread, NULL, consumer_thread, this );
	}

	return 0;
}



int consumer_stop( mlt_consumer parent )
{
	// Get the actual object
	consumer_xgl this = parent->child;
	
	if ( this->running && this->joined == 0 )
	{
		// Kill the thread and clean up
		this->joined = 1;
		this->running = 0;

		if ( this->thread )
			pthread_join( this->thread, NULL );
	}

	return 0;
}



int consumer_is_stopped( mlt_consumer parent )
{
	consumer_xgl this = parent->child;
	return !this->running;
}



static void *consumer_thread( void *arg )
{
	// Identify the arg
	consumer_xgl this = arg;

	XInitThreads();
	start_xgl( this );

	return NULL;
}



/** Callback to allow override of the close method.
*/

static void consumer_close( mlt_consumer parent )
{
	// Get the actual object
	consumer_xgl this = parent->child;

	// Stop the consumer
	///mlt_consumer_stop( parent );
	mlt_filter_close( glsl_manager );

	// Now clean up the rest
	mlt_consumer_close( parent );

	// Close the queue
	mlt_deque_close( this->queue );

	if ( this->xgl_started )
		killGLWindow();

	// Finally clean up this
	free( this );
}
