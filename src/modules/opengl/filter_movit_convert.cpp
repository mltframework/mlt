/*
 * filter_movit_convert.cpp
 * Copyright (C) 2013 Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <framework/mlt.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "glsl_manager.h"
#include <movit/effect_chain.h>
#include <movit/util.h>
#include "mlt_movit_input.h"


static void yuv422_to_yuv422p( uint8_t *yuv422, uint8_t *yuv422p, int width, int height )
{
	uint8_t *Y = yuv422p;
	uint8_t *U = Y + width * height;
	uint8_t *V = U + width * height / 2;
	int n = width * height / 2 + 1;
	while ( --n ) {
		*Y++ = *yuv422++;
		*U++ = *yuv422++;
		*Y++ = *yuv422++;
		*V++ = *yuv422++;
	}
}

static int convert_on_cpu( mlt_frame frame, uint8_t **image, mlt_image_format *format, mlt_image_format output_format )
{
	int error = 0;
	mlt_filter cpu_csc = (mlt_filter) mlt_properties_get_data( MLT_FRAME_PROPERTIES( frame ), "cpu_csc", NULL );
	if ( cpu_csc ) {
		int (* save_fp )( mlt_frame self, uint8_t **image, mlt_image_format *input, mlt_image_format output )
			= frame->convert_image;
		frame->convert_image = NULL;
		mlt_filter_process( cpu_csc, frame );
		error = frame->convert_image( frame, image, format, output_format );
		frame->convert_image = save_fp;
	} else {
		error = 1;
	}
	return error;
}

static int convert_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, mlt_image_format output_format )
{
	// Nothing to do!
	if ( *format == output_format )
		return 0;

	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	mlt_log_debug( NULL, "filter_movit_convert: %s -> %s\n",
		mlt_image_format_name( *format ), mlt_image_format_name( output_format ) );

	// Use CPU if glsl not initialized or not supported.
	GlslManager* glsl = GlslManager::get_instance();
	if ( !glsl || !glsl->get_int("glsl_supported" ) )
		return convert_on_cpu( frame, image, format, output_format );

	// Do non-GL image conversions on a CPU-based image converter.
	if ( *format != mlt_image_glsl && output_format != mlt_image_glsl && output_format != mlt_image_glsl_texture )
		return convert_on_cpu( frame, image, format, output_format );

	int error = 0;
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );
	int img_size = mlt_image_format_size( *format, width, height, NULL );
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	mlt_service service = MLT_PRODUCER_SERVICE(producer);
	EffectChain* chain = GlslManager::get_chain( service );
	MltInput* input = GlslManager::get_input( service );

	// Use a temporary chain to convert image in RAM to OpenGL texture.
	if ( output_format == mlt_image_glsl_texture && *format != mlt_image_glsl ) {
		// We might already have a texture from a previous conversion from mlt_image_glsl.
		glsl_texture texture = (glsl_texture) mlt_properties_get_data( properties, "movit.convert", NULL );
		if ( texture ) {
			*image = (uint8_t*) &texture->texture;
			mlt_frame_set_image( frame, *image, 0, NULL );
			mlt_properties_set_int( properties, "format", output_format );
			*format = output_format;
			return error;
		} else {
			input = new MltInput( width, height );
			chain = new EffectChain( width, height );
			chain->add_input( input );
		}
	}

	if ( *format != mlt_image_glsl ) {
		if ( *format == mlt_image_rgb24a || *format == mlt_image_opengl ) { 
			input->useFlatInput( chain, FORMAT_RGBA_POSTMULTIPLIED_ALPHA, width, height );
			input->set_pixel_data( *image );
		}
		else if ( *format == mlt_image_rgb24 ) {
			input->useFlatInput( chain, FORMAT_RGB, width, height );
			input->set_pixel_data( *image );
		}
		else if ( *format == mlt_image_yuv420p ) {
			ImageFormat image_format;
			YCbCrFormat ycbcr_format;
			if ( 709 == mlt_properties_get_int( properties, "colorspace" ) ) {
				image_format.color_space = COLORSPACE_REC_709;
				image_format.gamma_curve = GAMMA_REC_709;
				ycbcr_format.luma_coefficients = YCBCR_REC_709;
			} else if ( 576 == mlt_properties_get_int( properties, "height" ) ) {
				image_format.color_space = COLORSPACE_REC_601_625;
				image_format.gamma_curve = GAMMA_REC_601;
				ycbcr_format.luma_coefficients = YCBCR_REC_601;
			} else {
				image_format.color_space = COLORSPACE_REC_601_525;
				image_format.gamma_curve = GAMMA_REC_601;
				ycbcr_format.luma_coefficients = YCBCR_REC_601;
			}
			ycbcr_format.full_range = mlt_properties_get_int( properties, "force_full_luma" );
			ycbcr_format.chroma_subsampling_x = ycbcr_format.chroma_subsampling_y = 2;
			// TODO: make new frame properties set by producers
			ycbcr_format.cb_x_position = ycbcr_format.cr_x_position = 0.0f;
			ycbcr_format.cb_y_position = ycbcr_format.cr_y_position = 0.5f;
			input->useYCbCrInput( chain, image_format, ycbcr_format, width, height );
			input->set_pixel_data( *image );
		}
		else if ( *format == mlt_image_yuv422 ) {
			ImageFormat image_format;
			YCbCrFormat ycbcr_format;
			if ( 709 == mlt_properties_get_int( properties, "colorspace" ) ) {
				image_format.color_space = COLORSPACE_REC_709;
				image_format.gamma_curve = GAMMA_REC_709;
				ycbcr_format.luma_coefficients = YCBCR_REC_709;
			} else if ( 576 == height ) {
				image_format.color_space = COLORSPACE_REC_601_625;
				image_format.gamma_curve = GAMMA_REC_601;
				ycbcr_format.luma_coefficients = YCBCR_REC_601;
			} else {
				image_format.color_space = COLORSPACE_REC_601_525;
				image_format.gamma_curve = GAMMA_REC_601;
				ycbcr_format.luma_coefficients = YCBCR_REC_601;
			}
			ycbcr_format.full_range = mlt_properties_get_int( properties, "force_full_luma" );
			ycbcr_format.chroma_subsampling_x = 2;
			ycbcr_format.chroma_subsampling_y = 1;
			// TODO: make new frame properties set by producers
			ycbcr_format.cb_x_position = ycbcr_format.cr_x_position = 0.0f;
			ycbcr_format.cb_y_position = ycbcr_format.cr_y_position = 0.5f;
			input->useYCbCrInput( chain, image_format, ycbcr_format, width, height );
			
			// convert chunky to planar
			uint8_t* planar = (uint8_t*) mlt_pool_alloc( img_size );
			yuv422_to_yuv422p( *image, planar, width, height );
			input->set_pixel_data( planar );
			mlt_frame_set_image( frame, planar, img_size, mlt_pool_release );
		}
	}

	if ( output_format != mlt_image_glsl ) {
		glsl_fbo fbo = glsl->get_fbo( width, height );

		if ( output_format == mlt_image_glsl_texture ) {
			glsl_texture texture = glsl->get_texture( width, height, GL_RGBA );

			glBindFramebuffer( GL_FRAMEBUFFER, fbo->fbo );
			check_error();
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0 );
			check_error();
			glBindFramebuffer( GL_FRAMEBUFFER, 0 );
			check_error();

			// Using a temporary chain to convert image in RAM to OpenGL texture.
			if ( *format != mlt_image_glsl )
				GlslManager::reset_finalized( service );
			GlslManager::render( service, chain, fbo->fbo, width, height );

			glFinish();
			check_error();
			glBindFramebuffer( GL_FRAMEBUFFER, 0 );
			check_error();
			// Using a temporary chain to convert image in RAM to OpenGL texture.
			if ( *format != mlt_image_glsl )
				delete chain;

			*image = (uint8_t*) &texture->texture;
			mlt_frame_set_image( frame, *image, 0, NULL );
			mlt_properties_set_data( properties, "movit.convert", texture, 0,
				(mlt_destructor) GlslManager::release_texture, NULL );
			mlt_properties_set_int( properties, "format", output_format );
			*format = output_format;
		}
		else {
			// Use a PBO to hold the data we read back with glReadPixels()
			// (Intel/DRI goes into a slow path if we don't read to PBO)
			GLenum gl_format = ( output_format == mlt_image_rgb24a || output_format == mlt_image_opengl )?
				GL_RGBA : GL_RGB;
			img_size = width * height * ( gl_format == GL_RGB? 3 : 4 );
			glsl_pbo pbo = glsl->get_pbo( img_size );
			glsl_texture texture = glsl->get_texture( width, height, gl_format );

			if ( fbo && pbo && texture ) {
				// Set the FBO
				glBindFramebuffer( GL_FRAMEBUFFER, fbo->fbo );
				check_error();
				glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture, 0 );
				check_error();
				glBindFramebuffer( GL_FRAMEBUFFER, 0 );
				check_error();

				GlslManager::render( service, chain, fbo->fbo, width, height );
	
				// Read FBO into PBO
				glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, pbo->pbo );
				check_error();
				glBufferData( GL_PIXEL_PACK_BUFFER_ARB, img_size, NULL, GL_STREAM_READ );
				check_error();
				glReadPixels( 0, 0, width, height, gl_format, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0) );
				check_error();
	
				// Copy from PBO
				uint8_t* buf = (uint8_t*) glMapBuffer( GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY );
				check_error();

				if ( output_format == mlt_image_yuv422 || output_format == mlt_image_yuv420p ) {
					*image = buf;
					*format = mlt_image_rgb24;
					error = convert_on_cpu( frame, image, format, output_format );
				}
				else {
					*image = (uint8_t*) mlt_pool_alloc( img_size );
					mlt_frame_set_image( frame, *image, img_size, mlt_pool_release );
					memcpy( *image, buf, img_size );
				}
	
				// Release PBO and FBO
				glUnmapBuffer( GL_PIXEL_PACK_BUFFER_ARB );
				check_error();
				glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, 0 );
				check_error();
				glBindFramebuffer( GL_FRAMEBUFFER, 0 );
				check_error();
				glBindTexture( GL_TEXTURE_2D, 0 );
				check_error();
				mlt_properties_set_data( properties, "movit.convert", texture, 0,
					(mlt_destructor) GlslManager::release_texture, NULL);
	
				mlt_properties_set_int( properties, "format", output_format );
				*format = output_format;
			}
			else {
				error = 1;
			}
		}
		if ( fbo ) GlslManager::release_fbo( fbo );
	}
	else {
		mlt_properties_set_int( properties, "format", output_format );
		*format = output_format;
	}

	return error;
}

static mlt_frame process( mlt_filter filter, mlt_frame frame )
{
	// Set a default colorspace on the frame if not yet set by the producer.
	// The producer may still change it during get_image.
	// This way we do not have to modify each producer to set a valid colorspace.
	mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
	if ( mlt_properties_get_int( properties, "colorspace" ) <= 0 )
		mlt_properties_set_int( properties, "colorspace", mlt_service_profile( MLT_FILTER_SERVICE(filter) )->colorspace );

	frame->convert_image = convert_image;

	mlt_filter cpu_csc = (mlt_filter) mlt_properties_get_data( MLT_FILTER_PROPERTIES( filter ), "cpu_csc", NULL );
	mlt_properties_inc_ref( MLT_FILTER_PROPERTIES(cpu_csc) );
	mlt_properties_set_data( properties, "cpu_csc", cpu_csc, 0,
		(mlt_destructor) mlt_filter_close, NULL );

	return frame;
}

static mlt_filter create_filter( mlt_profile profile, char *effect )
{
	mlt_filter filter = NULL;
	char *id = strdup( effect );
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';

	// The swscale and avcolor_space filters require resolution as arg to test compatibility
	if ( !strcmp( effect, "avcolor_space" ) )
		arg = (char*) profile->width;

	filter = mlt_factory_filter( profile, id, arg );
	if ( filter )
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "_loader", 1 );
	free( id );
	return filter;
}

extern "C" {

mlt_filter filter_movit_convert_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = NULL;
	GlslManager* glsl = GlslManager::get_instance();

	if ( glsl && ( filter = mlt_filter_new() ) )
	{
		mlt_filter cpu_csc = create_filter( profile, "avcolor_space" );
		if ( !cpu_csc )
			cpu_csc = create_filter( profile, "imageconvert" );
		if ( cpu_csc )
			mlt_properties_set_data( MLT_FILTER_PROPERTIES( filter ), "cpu_csc", cpu_csc, 0,
				(mlt_destructor) mlt_filter_close, NULL );
		filter->process = process;
	}
	return filter;
}

}
