/*
 * filter_movit_convert.cpp
 * Copyright (C) 2013-2015 Dan Dennedy <dan@dennedy.org>
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string>

#include "filter_glsl_manager.h"
#include <movit/effect_chain.h>
#include <movit/util.h>
#include "mlt_movit_input.h"
#include <mlt++/MltProducer.h>
#include "mlt_flip_effect.h"

using namespace movit;

static void set_movit_parameters( GlslChain *chain, mlt_service service, mlt_frame frame );

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

static void delete_chain( EffectChain* chain )
{
	delete chain;
}

// Copied from libavcodec, but we can not add that as a dependency to this module
// simply for this.

enum AVColorTransferCharacteristic {
	AVCOL_TRC_BT709        =  1, ///< also ITU-R BT1361
	AVCOL_TRC_UNSPECIFIED  =  2,
	AVCOL_TRC_GAMMA22      =  4, ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
	AVCOL_TRC_GAMMA28      =  5, ///< also ITU-R BT470BG
	AVCOL_TRC_SMPTE170M    =  6, ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
	AVCOL_TRC_SMPTE240M    =  7,
	AVCOL_TRC_LINEAR       =  8, ///< "Linear transfer characteristics"
	AVCOL_TRC_LOG          =  9, ///< "Logarithmic transfer characteristic (100:1 range)"
	AVCOL_TRC_LOG_SQRT     = 10, ///< "Logarithmic transfer characteristic (100 * Sqrt( 10 ) : 1 range)"
	AVCOL_TRC_IEC61966_2_4 = 11, ///< IEC 61966-2-4
	AVCOL_TRC_BT1361_ECG   = 12, ///< ITU-R BT1361 Extended Colour Gamut
	AVCOL_TRC_IEC61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
	AVCOL_TRC_BT2020_10    = 14, ///< ITU-R BT2020 for 10 bit system
	AVCOL_TRC_BT2020_12    = 15, ///< ITU-R BT2020 for 12 bit system
	AVCOL_TRC_NB               , ///< Not part of ABI
};

// Get the gamma from the frame "color_trc" property as set by producer or this filter.
static GammaCurve getGammaCurve( int color_trc )
{
	switch ( color_trc ) {
	case AVCOL_TRC_LINEAR:
		return GAMMA_LINEAR;
	case AVCOL_TRC_GAMMA22:
	case AVCOL_TRC_IEC61966_2_1:
		return GAMMA_sRGB;
	case AVCOL_TRC_BT2020_10:
		return GAMMA_REC_2020_10_BIT;
	case AVCOL_TRC_BT2020_12:
		return GAMMA_REC_2020_12_BIT;
	default:
		return GAMMA_REC_709;
	}
}

// Get the gamma from the consumer's "color_trc" property.
// Also, update the frame's color_trc property with the selection.
static GammaCurve getGammaCurve( mlt_properties properties )
{
	const char *color_trc = mlt_properties_get( properties, "consumer_color_trc" );
	if ( color_trc ) {
		// If specified with enum or int.
		int n = mlt_properties_get_int( properties, "consumer_color_trc" );
		switch ( n ) {
		case AVCOL_TRC_BT709:
		case AVCOL_TRC_SMPTE170M:
			mlt_properties_set_int( properties, "color_trc", n );
			return GAMMA_REC_709;
		case AVCOL_TRC_LINEAR:
			mlt_properties_set_int( properties, "color_trc", n );
			return GAMMA_LINEAR;
		case AVCOL_TRC_BT2020_10:
			mlt_properties_set_int( properties, "color_trc", n );
			return GAMMA_REC_2020_10_BIT;
		case AVCOL_TRC_BT2020_12:
			mlt_properties_set_int( properties, "color_trc", n );
			return GAMMA_REC_2020_12_BIT;
		default:
			// If specified by string.
			if ( !strcmp( color_trc, "bt709" ) ) {
				mlt_properties_set_int( properties, "color_trc", AVCOL_TRC_BT709 );
				return GAMMA_REC_709;
			} else if ( !strcmp( color_trc, "smpte170m" ) ) {
				mlt_properties_set_int( properties, "color_trc", AVCOL_TRC_SMPTE170M );
				return GAMMA_REC_709;
			} else if ( !strcmp( color_trc, "linear" ) ) {
				mlt_properties_set_int( properties, "color_trc", AVCOL_TRC_LINEAR );
				return GAMMA_LINEAR;
			} else if ( !strcmp( color_trc, "bt2020_10bit" ) ) {
				mlt_properties_set_int( properties, "color_trc", AVCOL_TRC_BT2020_10 );
				return GAMMA_REC_2020_10_BIT;
			} else if ( !strcmp( color_trc, "bt2020_12bit" ) ) {
				mlt_properties_set_int( properties, "color_trc", AVCOL_TRC_BT2020_12 );
				return GAMMA_REC_2020_12_BIT;
			}
			break;
		}
	}
	return GAMMA_sRGB;
}

static void get_format_from_properties( mlt_properties properties, ImageFormat* image_format, YCbCrFormat* ycbcr_format )
{
	switch ( mlt_properties_get_int( properties, "colorspace" ) ) {
	case 601:
		ycbcr_format->luma_coefficients = YCBCR_REC_601;
		break;
	case 709:
	default:
		ycbcr_format->luma_coefficients = YCBCR_REC_709;
		break;
	}

	switch ( mlt_properties_get_int( properties, "color_primaries" ) ) {
	case 601625:
		image_format->color_space = COLORSPACE_REC_601_625;
		break;
	case 601525:
		image_format->color_space = COLORSPACE_REC_601_525;
		break;
	case 709:
	default:
		image_format->color_space = COLORSPACE_REC_709;
		break;
	}

	image_format->gamma_curve = getGammaCurve( mlt_properties_get_int( properties, "color_trc" ) );

	if ( mlt_properties_get_int( properties, "force_full_luma" ) ) {
		ycbcr_format->full_range = true;
	} else {
		ycbcr_format->full_range = ( mlt_properties_get_int( properties, "full_luma" ) == 1 );
	}

	// TODO: make new frame properties set by producers
	ycbcr_format->cb_x_position = ycbcr_format->cr_x_position = 0.0f;
	ycbcr_format->cb_y_position = ycbcr_format->cr_y_position = 0.5f;
}

static void build_fingerprint( mlt_service service, mlt_frame frame, std::string *fingerprint )
{
	if ( service == (mlt_service) -1 ) {
		fingerprint->append( "input" );
		return;
	}

	Effect* effect = GlslManager::get_effect( service, frame );
	assert( effect );
	mlt_service input_a = GlslManager::get_effect_input( service, frame );
	fingerprint->push_back( '(' );
	build_fingerprint( input_a, frame, fingerprint );
	fingerprint->push_back( ')' );

	mlt_frame frame_b;
	mlt_service input_b;
	GlslManager::get_effect_secondary_input( service, frame, &input_b, &frame_b );
	if ( input_b ) {
		fingerprint->push_back( '(' );
		build_fingerprint( input_b, frame_b, fingerprint );
		fingerprint->push_back( ')' );
	}

	GlslManager::get_effect_third_input( service, frame, &input_b, &frame_b );
	if ( input_b ) {
		fingerprint->push_back( '(' );
		build_fingerprint( input_b, frame_b, fingerprint );
		fingerprint->push_back( ')' );
	}

	fingerprint->push_back( '(' );
	fingerprint->append( mlt_properties_get( MLT_SERVICE_PROPERTIES( service ), "_unique_id" ) );

	const char* effect_fingerprint = mlt_properties_get( MLT_SERVICE_PROPERTIES( service ), "_movit fingerprint" );
	if ( effect_fingerprint ) {
		fingerprint->push_back( '[' );
		fingerprint->append( effect_fingerprint );
		fingerprint->push_back( ']' );
	}

	bool disable = mlt_properties_get_int( MLT_SERVICE_PROPERTIES( service ), "_movit.parms.int.disable" );
	if ( disable ) {
		fingerprint->push_back( 'd' );
	}
	fingerprint->push_back( ')' );
}

static Effect* build_movit_chain( mlt_service service, mlt_frame frame, GlslChain *chain )
{
	if ( service == (mlt_service) -1 ) {
		mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
		MltInput* input = GlslManager::get_input( producer, frame );
		GlslManager::set_input( producer, frame, NULL );
		chain->effect_chain->add_input( input->get_input() );
		chain->inputs.insert(std::make_pair( producer, input ) );
		return input->get_input();
	}

	Effect* effect = GlslManager::get_effect( service, frame );
	assert( effect );
	GlslManager::set_effect( service, frame, NULL );

	mlt_service input_a = GlslManager::get_effect_input( service, frame );
	mlt_service input_b, input_c;
	mlt_frame frame_b, frame_c;
	GlslManager::get_effect_secondary_input( service, frame, &input_b, &frame_b );
	GlslManager::get_effect_third_input( service, frame, &input_c, &frame_c );
	Effect *effect_a = build_movit_chain( input_a, frame, chain );

	if ( input_c && input_b ) {
		Effect *effect_b = build_movit_chain( input_b, frame_b, chain );
		Effect *effect_c = build_movit_chain( input_c, frame_c, chain );
		chain->effect_chain->add_effect( effect, effect_a, effect_b, effect_c );
	} else 	if ( input_b ) {
		Effect *effect_b = build_movit_chain( input_b, frame_b, chain );
		chain->effect_chain->add_effect( effect, effect_a, effect_b );
	} else {
		chain->effect_chain->add_effect( effect, effect_a );
	}
		
	chain->effects.insert(std::make_pair( service, effect ) );
	return effect;
}

static void dispose_movit_effects( mlt_service service, mlt_frame frame )
{
	if ( service == (mlt_service) -1 ) {
		mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
		delete GlslManager::get_input( producer, frame );
		GlslManager::set_input( producer, frame, NULL );
		return;
	}

	delete GlslManager::get_effect( service, frame );
	GlslManager::set_effect( service, frame, NULL );

	mlt_service input_a = GlslManager::get_effect_input( service, frame );
	mlt_service input_b;
	mlt_frame frame_b;
	GlslManager::get_effect_secondary_input( service, frame, &input_b, &frame_b );
	dispose_movit_effects( input_a, frame );

	if ( input_b ) {
		dispose_movit_effects( input_b, frame_b );
	}
	GlslManager::get_effect_third_input( service, frame, &input_b, &frame_b );
	if ( input_b ) {
		dispose_movit_effects( input_b, frame_b );
	}
}

static void finalize_movit_chain( mlt_service leaf_service, mlt_frame frame )
{
	GlslChain* chain = GlslManager::get_chain( leaf_service );

	std::string new_fingerprint;
	build_fingerprint( leaf_service, frame, &new_fingerprint );

	// Build the chain if needed.
	if ( !chain || new_fingerprint != chain->fingerprint ) {
		mlt_log_debug( leaf_service, "=== CREATING NEW CHAIN (old chain=%p, leaf=%p, fingerprint=%s) ===\n", chain, leaf_service, new_fingerprint.c_str() );
		mlt_profile profile = mlt_service_profile( leaf_service );
		chain = new GlslChain;
		chain->effect_chain = new EffectChain(
			profile->display_aspect_num,
			profile->display_aspect_den,
			GlslManager::get_instance()->get_resource_pool()
		);
		chain->fingerprint = new_fingerprint;

		build_movit_chain( leaf_service, frame, chain );
		set_movit_parameters( chain, leaf_service, frame );
		chain->effect_chain->add_effect( new Mlt::VerticalFlip );

		ImageFormat output_format;
		output_format.color_space = COLORSPACE_sRGB;
		output_format.gamma_curve = getGammaCurve( MLT_FRAME_PROPERTIES(frame) );
		chain->effect_chain->add_output(output_format, OUTPUT_ALPHA_FORMAT_POSTMULTIPLIED);
		chain->effect_chain->set_dither_bits(8);
		chain->effect_chain->finalize();

		GlslManager::set_chain( leaf_service, chain );
	} else {
		// Delete all the created Effect instances to avoid memory leaks.
		dispose_movit_effects( leaf_service, frame );
	}
}

static void set_movit_parameters( GlslChain *chain, mlt_service service, mlt_frame frame )
{
	if ( service == (mlt_service) -1 ) {
		mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
		MltInput* input = chain->inputs[ producer ];
		input->set_pixel_data( GlslManager::get_input_pixel_pointer( producer, frame ) );
		return;
	}

	Effect* effect = chain->effects[ service ];
	mlt_service input_a = GlslManager::get_effect_input( service, frame );
	set_movit_parameters( chain, input_a, frame );

	mlt_service input_b;
	mlt_frame frame_b;
	GlslManager::get_effect_secondary_input( service, frame, &input_b, &frame_b );
	if ( input_b ) {
		set_movit_parameters( chain, input_b, frame_b );
	}
	GlslManager::get_effect_third_input( service, frame, &input_b, &frame_b );
	if ( input_b ) {
		set_movit_parameters( chain, input_b, frame_b );
	}

	mlt_properties properties = MLT_SERVICE_PROPERTIES( service );
	int count = mlt_properties_count( properties );
	for (int i = 0; i < count; ++i) {
		const char *name = mlt_properties_get_name( properties, i );
		if (strncmp(name, "_movit.parms.float.", strlen("_movit.parms.float.")) == 0 &&
			mlt_properties_get_value( properties, i )) {
			bool ok = effect->set_float(name + strlen("_movit.parms.float."),
				mlt_properties_get_double( properties, name ));
			assert(ok);
		}
		if (strncmp(name, "_movit.parms.int.", strlen("_movit.parms.int.")) == 0 &&
			mlt_properties_get_value( properties, i )) {
			bool ok = effect->set_int(name + strlen("_movit.parms.int."),
				mlt_properties_get_int( properties, name ));
			assert(ok);
		}
		if (strncmp(name, "_movit.parms.vec3.", strlen("_movit.parms.vec3.")) == 0 &&
		    strcmp(name + strlen(name) - 3, "[0]") == 0 &&
		    mlt_properties_get_value( properties, i )) {
			float val[3];
			char *name_copy = strdup(name);
			char *index_char = name_copy + strlen(name_copy) - 2;
			val[0] = mlt_properties_get_double( properties, name_copy );
			*index_char = '1';
			val[1] = mlt_properties_get_double( properties, name_copy );
			*index_char = '2';
			val[2] = mlt_properties_get_double( properties, name_copy );
			index_char[-1] = '\0';
			bool ok = effect->set_vec3(name_copy + strlen("_movit.parms.vec3."), val);
			assert(ok);
			free(name_copy);
		}
		if (strncmp(name, "_movit.parms.vec4.", strlen("_movit.parms.vec4.")) == 0 &&
		    strcmp(name + strlen(name) - 3, "[0]") == 0 &&
		    mlt_properties_get_value( properties, i )) {
			float val[4];
			char *name_copy = strdup(name);
			char *index_char = name_copy + strlen(name_copy) - 2;
			val[0] = mlt_properties_get_double( properties, name_copy );
			*index_char = '1';
			val[1] = mlt_properties_get_double( properties, name_copy );
			*index_char = '2';
			val[2] = mlt_properties_get_double( properties, name_copy );
			*index_char = '3';
			val[3] = mlt_properties_get_double( properties, name_copy );
			index_char[-1] = '\0';
			bool ok = effect->set_vec4(name_copy + strlen("_movit.parms.vec4."), val);
			assert(ok);
			free(name_copy);
		}
	}
}

static void dispose_pixel_pointers( GlslChain *chain, mlt_service service, mlt_frame frame )
{
	if ( service == (mlt_service) -1 ) {
		mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
		MltInput* input = chain->inputs[ producer ];
		input->invalidate_pixel_data();
		mlt_pool_release( GlslManager::get_input_pixel_pointer( producer, frame ) );
		return;
	}

	mlt_service input_a = GlslManager::get_effect_input( service, frame );
	dispose_pixel_pointers( chain, input_a, frame );

	mlt_service input_b;
	mlt_frame frame_b;
	GlslManager::get_effect_secondary_input( service, frame, &input_b, &frame_b );
	if ( input_b ) {
		dispose_pixel_pointers( chain, input_b, frame_b );
	}
	GlslManager::get_effect_third_input( service, frame, &input_b, &frame_b );
	if ( input_b ) {
		dispose_pixel_pointers( chain, input_b, frame_b );
	}
}

static int movit_render( EffectChain *chain, mlt_frame frame, mlt_image_format *format, mlt_image_format output_format, int width, int height, uint8_t **image )
{
	GlslManager* glsl = GlslManager::get_instance();
	int error;
	if ( output_format == mlt_image_glsl_texture ) {
		error = glsl->render_frame_texture( chain, frame, width, height, image );
	}
	else {
		error = glsl->render_frame_rgba( chain, frame, width, height, image );
		if ( !error && output_format != mlt_image_rgb24a ) {
			*format = mlt_image_rgb24a;
			error = convert_on_cpu( frame, image, format, output_format );
		}
	}
	return error;
}

// Create an MltInput for an image with the given format and dimensions.
static MltInput* create_input( mlt_properties properties, mlt_image_format format, int aspect_width, int aspect_height, int width, int height )
{
	MltInput* input = new MltInput( format );
	if ( format == mlt_image_rgb24a || format == mlt_image_opengl ) {
		// TODO: Get the color space if available.
		input->useFlatInput( FORMAT_RGBA_POSTMULTIPLIED_ALPHA, width, height );
	}
	else if ( format == mlt_image_rgb24 ) {
		// TODO: Get the color space if available.
		input->useFlatInput( FORMAT_RGB, width, height );
	}
	else if ( format == mlt_image_yuv420p ) {
		ImageFormat image_format = {};
		YCbCrFormat ycbcr_format = {};
		get_format_from_properties( properties, &image_format, &ycbcr_format );
		ycbcr_format.chroma_subsampling_x = ycbcr_format.chroma_subsampling_y = 2;
		input->useYCbCrInput( image_format, ycbcr_format, width, height );
	}
	else if ( format == mlt_image_yuv422 ) {
		ImageFormat image_format = {};
		YCbCrFormat ycbcr_format = {};
		get_format_from_properties( properties, &image_format, &ycbcr_format );
		ycbcr_format.chroma_subsampling_x = 2;
		ycbcr_format.chroma_subsampling_y = 1;
		input->useYCbCrInput( image_format, ycbcr_format, width, height );
	}
	return input;
}

// Make a copy of the given image (allocated using mlt_pool_alloc) suitable
// to pass as pixel pointer to an MltInput (created using create_input
// with the same parameters), and return that pointer.
static uint8_t* make_input_copy( mlt_image_format format, uint8_t *image, int width, int height )
{
	int img_size = mlt_image_format_size( format, width, height, NULL );
	uint8_t* img_copy = (uint8_t*) mlt_pool_alloc( img_size );
	if ( format == mlt_image_yuv422 ) {
		yuv422_to_yuv422p( image, img_copy, width, height );
	} else {
		memcpy( img_copy, image, img_size );
	}
	return img_copy;
}

static int convert_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, mlt_image_format output_format )
{
	// Nothing to do!
	if ( *format == output_format )
		return 0;

	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	mlt_log_debug( NULL, "filter_movit_convert: %s -> %s (%d)\n",
		mlt_image_format_name( *format ), mlt_image_format_name( output_format ),
		mlt_frame_get_position( frame ) );

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
	GlslManager::get_instance()->lock_service( frame );
	
	// If we're at the beginning of a series of Movit effects, store the input
	// sent into the chain.
	if ( output_format == mlt_image_glsl ) {
		mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
		mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );
		MltInput *input = create_input( properties, *format, profile->width, profile->height, width, height );
		GlslManager::set_input( producer, frame, input );
		uint8_t *img_copy = make_input_copy( *format, *image, width, height );
		GlslManager::set_input_pixel_pointer( producer, frame, img_copy );

		*image = (uint8_t *) -1;
		mlt_frame_set_image( frame, *image, 0, NULL );
	}

	// If we're at the _end_ of a series of Movit effects, render the chain.
	if ( *format == mlt_image_glsl ) {
		mlt_service leaf_service = (mlt_service) *image;

		if ( leaf_service == (mlt_service) -1 ) {
			// Something on the way requested conversion to mlt_glsl,
			// but never added an effect. Don't build a Movit chain;
			// just do the conversion and we're done.
			mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
			MltInput *input = GlslManager::get_input( producer, frame );
			*image = GlslManager::get_input_pixel_pointer( producer, frame );
			*format = input->get_format();
			delete input;
			GlslManager::get_instance()->unlock_service( frame );
			return convert_on_cpu( frame, image, format, output_format );
		}

		// Construct the chain unless we already have a good one.
		finalize_movit_chain( leaf_service, frame );

		// Set per-frame parameters now that we know which Effect instances to set them on.
		// (finalize_movit_chain may already have done this, though, but twice doesn't hurt.)
		GlslChain *chain = GlslManager::get_chain( leaf_service );
		set_movit_parameters( chain, leaf_service, frame );

		error = movit_render( chain->effect_chain, frame, format, output_format, width, height, image );

		dispose_pixel_pointers( chain, leaf_service, frame );
	}

	// If we've been asked to render some frame directly to a texture (without any
	// effects in-between), we create a new mini-chain to do so.
	if ( *format != mlt_image_glsl && output_format == mlt_image_glsl_texture ) {
		// We might already have a texture from a previous conversion from mlt_image_glsl.
		glsl_texture texture = (glsl_texture) mlt_properties_get_data( properties, "movit.convert.texture", NULL );
		// XXX: requires a special property set on the frame by the app for now
		// because we do not have reliable way to clear the texture property
		// when a downstream filter has changed image.
		if ( texture && mlt_properties_get_int( properties, "movit.convert.use_texture") ) {
			*image = (uint8_t*) &texture->texture;
			mlt_frame_set_image( frame, *image, 0, NULL );
		} else {
			// Use a separate chain to convert image in RAM to OpenGL texture.
			// Use cached chain if available and compatible.
			Mlt::Producer producer( mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) ) );
			EffectChain *chain = (EffectChain*) producer.get_data( "movit.convert.chain" );
			MltInput *input = (MltInput*) producer.get_data( "movit.convert.input" );
			int w = producer.get_int( "movit.convert.width" );
			int h = producer.get_int( "movit.convert.height" );
			mlt_image_format f = (mlt_image_format) producer.get_int( "movit.convert.format" );
			if ( !chain || !input || width != w || height != h || *format != f ) {
				chain = new EffectChain( width, height, GlslManager::get_instance()->get_resource_pool() );
				input = create_input( properties, *format, width, height, width, height );
				chain->add_input( input->get_input() );
				chain->add_effect( new Mlt::VerticalFlip() );
				ImageFormat movit_output_format;
				movit_output_format.color_space = COLORSPACE_sRGB;
				movit_output_format.gamma_curve = getGammaCurve( properties );
				chain->add_output(movit_output_format, OUTPUT_ALPHA_FORMAT_POSTMULTIPLIED);
				chain->set_dither_bits(8);
				chain->finalize();
				producer.set( "movit.convert.chain", chain, 0, (mlt_destructor) delete_chain );
				producer.set( "movit.convert.input", input, 0, NULL );
				producer.set( "movit.convert.width", width );
				producer.set( "movit.convert.height", height );
				producer.set( "movit.convert.format", *format );
			}

			if ( *format == mlt_image_yuv422 ) {
				// We need to convert to planar, which make_input_copy() will do for us.
				uint8_t *planar = make_input_copy( *format, *image, width, height );
				input->set_pixel_data( planar );
				error = movit_render( chain, frame, format, output_format, width, height, image );
				mlt_pool_release( planar );
			} else {
				input->set_pixel_data( *image );
				error = movit_render( chain, frame, format, output_format, width, height, image );
			}
		}
	}

	GlslManager::get_instance()->unlock_service( frame );

	mlt_properties_set_int( properties, "format", output_format );
	*format = output_format;

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

static mlt_filter create_filter( mlt_profile profile, const char *effect )
{
	mlt_filter filter;
	char *id = strdup( effect );
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';

	// The swscale and avcolor_space filters require resolution as arg to test compatibility
	if ( !strcmp( effect, "avcolor_space" ) )
		filter = mlt_factory_filter( profile, id, &profile->width );
	else
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
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		glsl->add_ref( properties );
#ifdef _WIN32
		// XXX avcolor_space is crashing on Windows in this context!
		mlt_filter cpu_csc = NULL;
#else
		mlt_filter cpu_csc = create_filter( profile, "avcolor_space" );
#endif
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
