/*
 * producer_avformat/vdpau.c -- VDPAU functions for the avformat producer
 * Copyright (C) 2009 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <libavcodec/vdpau.h>
#include <X11/Xlib.h>
#include <dlfcn.h>

extern pthread_mutex_t mlt_sdl_mutex;

static VdpGetProcAddress       *vdp_get_proc_address;
static VdpGetErrorString       *vdp_get_error_string;
static VdpGetApiVersion        *vdp_get_api_version;
static VdpGetInformationString *vdp_get_information_string;
static VdpVideoSurfaceCreate   *vdp_surface_create;
static VdpVideoSurfaceDestroy  *vdp_surface_destroy;
static VdpVideoSurfaceGetBitsYCbCr *vdp_surface_get_bits;
static VdpDecoderCreate        *vdp_decoder_create;
static VdpDecoderDestroy       *vdp_decoder_destroy;
static VdpDecoderRender        *vdp_decoder_render;

struct
{
	VdpDevice device;
	VdpDecoder decoder;
	void *producer;
} *g_vdpau = NULL;

/** VDPAUD functions
*/

static void vdpau_decoder_close();

static int vdpau_init( producer_avformat self )
{
	mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent), "vdpau_init\n" );
	int success = 0;
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( self->parent );
	Display *display = XOpenDisplay( NULL );
	
	if ( !display || mlt_properties_get_int( properties, "novdpau" )
	     || ( getenv( "MLT_NO_VDPAU" ) && strcmp( getenv( "MLT_NO_VDPAU" ), "1" ) == 0 ) )
		return success;

	if ( !g_vdpau )
	{
		int flags = RTLD_NOW;
		void *object = dlopen( "/usr/lib64/libvdpau.so", flags );

		if ( !object )
			object = dlopen( "/usr/lib/libvdpau.so", flags );
		if ( object )
		{
			VdpDeviceCreateX11 *create_device = dlsym( object, "vdp_device_create_x11" );
			if ( create_device )
			{
				int screen = mlt_properties_get_int( properties, "x11_screen" );
				VdpDevice device;
				
				mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent), "X11 Display = %p\n", display );
				if ( VDP_STATUS_OK == create_device( display, screen, &device, &vdp_get_proc_address ) )
				{
					// Allocate the global VDPAU context
					g_vdpau = calloc( 1, sizeof( *g_vdpau ) );
					if ( g_vdpau )
					{
						g_vdpau->device = device;
						g_vdpau->decoder = VDP_INVALID_HANDLE;
						g_vdpau->producer = self;
						vdp_get_proc_address( g_vdpau->device, VDP_FUNC_ID_GET_ERROR_STRING, (void**) &vdp_get_error_string );
						vdp_get_proc_address( g_vdpau->device, VDP_FUNC_ID_GET_API_VERSION, (void**) &vdp_get_api_version );
						vdp_get_proc_address( g_vdpau->device, VDP_FUNC_ID_GET_INFORMATION_STRING, (void**) &vdp_get_information_string );
						vdp_get_proc_address( g_vdpau->device, VDP_FUNC_ID_VIDEO_SURFACE_CREATE, (void**) &vdp_surface_create );
						vdp_get_proc_address( g_vdpau->device, VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, (void**) &vdp_surface_destroy );
						vdp_get_proc_address( g_vdpau->device, VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, (void**) &vdp_surface_get_bits );
						vdp_get_proc_address( g_vdpau->device, VDP_FUNC_ID_DECODER_CREATE, (void**) &vdp_decoder_create );
						vdp_get_proc_address( g_vdpau->device, VDP_FUNC_ID_DECODER_DESTROY, (void**) &vdp_decoder_destroy );
						vdp_get_proc_address( g_vdpau->device, VDP_FUNC_ID_DECODER_RENDER, (void**) &vdp_decoder_render );
						success = 1;
					}
				}
			}
			if ( !success )
			{
				mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent), "VDPAU failed to initialize device\n" );
				dlclose( object );
			}
		}
		else
		{
			mlt_log( MLT_PRODUCER_SERVICE(self->parent), MLT_LOG_WARNING, "%s: failed to dlopen libvdpau.so\n  (%s)\n", __FUNCTION__, dlerror() );
		}
	}
	else
	{
		success = 1;
		mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent), "VDPAU already initialized\n" );
	}
	
	if ( g_vdpau && g_vdpau->producer != self )
		vdpau_decoder_close();
	
	return success;
}

static enum PixelFormat vdpau_get_format( struct AVCodecContext *s, const enum PixelFormat *fmt )
{
	return PIX_FMT_VDPAU_H264;
}

static int vdpau_get_buffer( AVCodecContext *codec_context, AVFrame *frame )
{
	int error = 0;
	producer_avformat self = codec_context->opaque;
	mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent), "vdpau_get_buffer\n" );
	
	if ( g_vdpau->producer == self && self->vdpau && mlt_deque_count( self->vdpau->deque ) )
	{
		struct vdpau_render_state *render = mlt_deque_pop_front( self->vdpau->deque );
		
		if ( render )
		{
			frame->data[0] = (uint8_t*) render;
			frame->data[1] = (uint8_t*) render;
			frame->data[2] = (uint8_t*) render;
			frame->linesize[0] = 0;
			frame->linesize[1] = 0;
			frame->linesize[2] = 0;
			frame->type = FF_BUFFER_TYPE_USER;
			render->state = FF_VDPAU_STATE_USED_FOR_REFERENCE;
			frame->reordered_opaque = codec_context->reordered_opaque;
			if ( frame->reference )
			{
				frame->age = self->vdpau->ip_age[0];
				self->vdpau->ip_age[0] = self->vdpau->ip_age[1] + 1;
				self->vdpau->ip_age[1] = 1;
				self->vdpau->b_age++;
			}
			else
			{
				frame->age = self->vdpau->b_age;
				self->vdpau->ip_age[0] ++;
				self->vdpau->ip_age[1] ++;
				self->vdpau->b_age = 1;
			}
		}
		else
		{
			mlt_log_warning( MLT_PRODUCER_SERVICE(self->parent), "VDPAU surface underrun\n" );
			error = -1;
		}
	}
	else
	{
		mlt_log_warning( MLT_PRODUCER_SERVICE(self->parent), "VDPAU surface underrun\n" );
		error = -1;
	}
	
	return error;
}

static void vdpau_release_buffer( AVCodecContext *codec_context, AVFrame *frame )
{
	producer_avformat self = codec_context->opaque;
	if ( self->vdpau )
	{
		struct vdpau_render_state *render = (struct vdpau_render_state*) frame->data[0];
		mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent), "vdpau_release_buffer (%x)\n", render->surface );
		int i;

		render->state &= ~FF_VDPAU_STATE_USED_FOR_REFERENCE;
		for ( i = 0; i < 4; i++ )
			frame->data[i] = NULL;
		mlt_deque_push_back( self->vdpau->deque, render );
	}
}

static void vdpau_draw_horiz( AVCodecContext *codec_context, const AVFrame *frame, int offset[4], int y, int type, int height )
{
	producer_avformat self = codec_context->opaque;
	if ( self->vdpau )
	{
		struct vdpau_render_state *render = (struct vdpau_render_state*) frame->data[0];
		VdpVideoSurface surface = render->surface;
		VdpStatus status = vdp_decoder_render( g_vdpau->decoder, surface, (void*) &render->info,
			render->bitstream_buffers_used, render->bitstream_buffers );

		if ( status != VDP_STATUS_OK )
		{
			self->vdpau->is_decoded = 0;
			mlt_log_warning( MLT_PRODUCER_SERVICE(self->parent), "VDPAU failed to decode (%s)\n",
				vdp_get_error_string( status ) );
		}
		else
		{
			self->vdpau->is_decoded = 1;
		}
	}
}

static int vdpau_decoder_init( producer_avformat self )
{
	mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent), "vdpau_decoder_init\n" );
	int success = 1;
	
	self->video_codec->opaque = self;
	self->video_codec->get_format = vdpau_get_format;
	self->video_codec->get_buffer = vdpau_get_buffer;
	self->video_codec->release_buffer = vdpau_release_buffer;
	self->video_codec->draw_horiz_band = vdpau_draw_horiz;
	self->video_codec->slice_flags = SLICE_FLAG_CODED_ORDER | SLICE_FLAG_ALLOW_FIELD;
	self->video_codec->pix_fmt = PIX_FMT_VDPAU_H264;
	
	VdpDecoderProfile profile = VDP_DECODER_PROFILE_H264_HIGH;
	uint32_t max_references = self->video_codec->refs;
	pthread_mutex_lock( &mlt_sdl_mutex );
	VdpStatus status = vdp_decoder_create( g_vdpau->device,
		profile, self->video_codec->width, self->video_codec->height, max_references, &g_vdpau->decoder );
	pthread_mutex_unlock( &mlt_sdl_mutex );
	
	if ( status == VDP_STATUS_OK )
	{
		if ( !self->vdpau )
		{
			int i, n = FFMIN( self->video_codec->refs + 2, MAX_VDPAU_SURFACES );
	
			self->vdpau = calloc( 1, sizeof( *self->vdpau ) );
			self->vdpau->deque = mlt_deque_init();
			for ( i = 0; i < n; i++ )
			{
				if ( VDP_STATUS_OK == vdp_surface_create( g_vdpau->device, VDP_CHROMA_TYPE_420,
					self->video_codec->width, self->video_codec->height, &self->vdpau->render_states[i].surface ) )
				{
					mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent), "successfully created VDPAU surface %x\n",
						self->vdpau->render_states[i].surface );
					mlt_deque_push_back( self->vdpau->deque, &self->vdpau->render_states[i] );
				}
				else
				{
					mlt_log_info( MLT_PRODUCER_SERVICE(self->parent), "failed to create VDPAU surface %dx%d\n",
						self->video_codec->width, self->video_codec->height );
					while ( mlt_deque_count( self->vdpau->deque ) )
					{
						struct vdpau_render_state *render = mlt_deque_pop_front( self->vdpau->deque );
						vdp_surface_destroy( render->surface );
					}
					mlt_deque_close( self->vdpau->deque );
					free( self->vdpau );
					self->vdpau = NULL;
					vdp_decoder_destroy( g_vdpau->decoder );
					g_vdpau->decoder = VDP_INVALID_HANDLE;
					success = 0;
					break;
				}
			}
			if ( self->vdpau )
				self->vdpau->b_age = self->vdpau->ip_age[0] = self->vdpau->ip_age[1] = 256*256*256*64; // magic from Avidemux
		}
		g_vdpau->producer = self;
	}
	else
	{
		success = 0;
		g_vdpau->decoder = VDP_INVALID_HANDLE;
		mlt_log_error( MLT_PRODUCER_SERVICE(self->parent), "VDPAU failed to initialize decoder (%s)\n",
			vdp_get_error_string( status ) );
	}
	
	return success;
}

static void vdpau_producer_close( producer_avformat self )
{
	if ( self->vdpau )
	{
		mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent), "vdpau_producer_close\n" );
		int i;
		for ( i = 0; i < MAX_VDPAU_SURFACES; i++ )
		{
			if ( self->vdpau->render_states[i].surface != VDP_INVALID_HANDLE )
				vdp_surface_destroy( self->vdpau->render_states[i].surface );
			self->vdpau->render_states[i].surface = VDP_INVALID_HANDLE;
		}
		mlt_deque_close( self->vdpau->deque );
		if ( self->vdpau->buffer )
			mlt_pool_release( self->vdpau->buffer );
		self->vdpau->buffer = NULL;
		free( self->vdpau );
		self->vdpau = NULL;
	}
}

static void vdpau_decoder_close( )
{
	mlt_log_debug( NULL, "vdpau_decoder_close (%x)\n", g_vdpau->decoder );
	if ( g_vdpau && g_vdpau->decoder != VDP_INVALID_HANDLE )
	{
		vdp_decoder_destroy( g_vdpau->decoder );
		g_vdpau->decoder = VDP_INVALID_HANDLE;
		g_vdpau->producer = NULL;
	}
	
}

#if 0
static void vdpau_close( void *ignored )
{
	mlt_log_debug( NULL, "vdpau_close\n" );
	if ( g_vdpau )
	{
		vdpau_decoder_close( );
		free( g_vdpau );
		g_vdpau = NULL;
	}
}
#endif
