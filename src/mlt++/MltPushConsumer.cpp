/**
 * MltPushConsumer.cpp - MLT Wrapper
 * Copyright (C) 2004-2015 Meltytech, LLC
 * Author: Charles Yates <charles.yates@gmail.com>
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

#include "MltPushConsumer.h"
#include "MltFilter.h"
using namespace Mlt;

namespace Mlt
{
	class PushPrivate
	{
		public:
			PushPrivate( )
			{
			}
	};
}

static void filter_destructor( void *arg )
{
	Filter *filter = ( Filter * )arg;
	delete filter;
}

PushConsumer::PushConsumer( Profile& profile, const char *id , const char *service ) :
	Consumer( profile, id, service ),
	m_private( new PushPrivate( ) )
{
	if ( is_valid( ) )
	{
		// Set up push mode (known as put mode in mlt)
		set( "real_time", 0 );
		set( "put_mode", 1 );
		set( "terminate_on_pause", 0 );
		set( "buffer", 0 );

		// We might need resize and rescale filters so we'll create them now
		// NB: Try to use the best rescaler available here
		Filter *resize = new Filter( profile, "resize" );
		Filter *rescale = new Filter( profile, "mcrescale" );
		if ( !rescale->is_valid( ) )
		{
			delete rescale;
			rescale = new Filter( profile, "gtkrescale" );
		}
		if ( !rescale->is_valid( ) )
		{
			delete rescale;
			rescale = new Filter( profile, "rescale" );
		}

		Filter *convert = new Filter( profile, "avcolour_space" );

		set( "filter_convert", convert, 0, filter_destructor );
		set( "filter_resize", resize, 0, filter_destructor );
		set( "filter_rescale", rescale, 0, filter_destructor );
	}
}

PushConsumer::~PushConsumer( )
{
}

void PushConsumer::set_render( int width, int height, double aspect_ratio )
{
	set( "render_width", width );
	set( "render_height", height );
	set( "render_aspect_ratio", aspect_ratio );
}

int PushConsumer::connect( Service &/*service*/ )
{
	return -1;
}

int PushConsumer::push( Frame *frame )
{
	frame->inc_ref( );

	// Here we have the option to process the frame at a render resolution (this will 
	// typically be PAL or NTSC) prior to scaling according to the consumers profile
	// This is done to optimise quality, esp. with regard to compositing positions 
	if ( get_int( "render_width" ) )
	{
		// Process the projects render resolution first
		mlt_image_format format = mlt_image_yuv422;
		int w = get_int( "render_width" );
		int h = get_int( "render_height" );
		frame->set( "consumer_aspect_ratio", get_double( "render_aspect_ratio" ) );
		frame->set( "consumer_deinterlace", get_int( "deinterlace" ) );
		frame->set( "deinterlace_method", get_int( "deinterlace_method" ) );
		frame->set( "rescale.interp", get( "rescale" ) );

		// Render the frame
		frame->get_image( format, w, h );

		// Now set up the post image scaling
		Filter *convert = ( Filter * )get_data( "filter_convert" );
		mlt_filter_process( convert->get_filter( ), frame->get_frame( ) );
		Filter *rescale = ( Filter * )get_data( "filter_rescale" );
		mlt_filter_process( rescale->get_filter( ), frame->get_frame( ) );
		Filter *resize = ( Filter * )get_data( "filter_resize" );
		mlt_filter_process( resize->get_filter( ), frame->get_frame( ) );
	}

	return mlt_consumer_put_frame( ( mlt_consumer )get_service( ), frame->get_frame( ) );
}

int PushConsumer::push( Frame &frame )
{
	return push( &frame );
}

int PushConsumer::drain( )
{
	return 0;
}

// Convenience function - generates a frame with an image of a given size
Frame *PushConsumer::construct( int size )
{
	mlt_frame f = mlt_frame_init( get_service() );
	Frame *frame = new Frame( f );
	uint8_t *buffer = ( uint8_t * )mlt_pool_alloc( size );
	frame->set( "image", buffer, size, mlt_pool_release );
	mlt_frame_close( f );
	return frame;
}

