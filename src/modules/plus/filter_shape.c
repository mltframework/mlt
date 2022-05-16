/*
 * filter_shape.c -- Arbitrary alpha channel shaping
 * Copyright (C) 2008-2022 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt.h>
#include <string.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>

typedef struct
{
	uint8_t *alpha;
	uint8_t *mask;
	int width;
	int height;
	double softness;
	double mix;
	int invert;
	int invert_mask;
	double offset;
	double divisor;
} slice_desc;

static inline double smoothstep( const double e1, const double e2, const double a )
{
    if ( a < e1 ) return 0.0;
    if ( a >= e2 ) return 1.0;
    double v = ( a - e1 ) / ( e2 - e1 );
    return ( v * v * ( 3 - 2 * v ) );
}


static int slice_alpha_proc(int id, int index, int jobs, void* data)
{
	(void) id; // unused
	slice_desc* desc = ((slice_desc*) data);
	int slice_line_start, slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
	int size = desc->width * slice_height + 1;
	uint8_t *p = desc->alpha + slice_line_start * desc->width;
	uint8_t *q = desc->mask + slice_line_start * desc->width;
	double a = 0;
	double b = 0;

	while (--size) {
		a = (double)(*q++ ^ desc->invert_mask) / desc->divisor;
		b = 1.0 - smoothstep(a, a + desc->softness, desc->mix);
		*p = (uint8_t)(*p * b) ^ desc->invert;
		p++;
	}

	return 0;
}

static int slice_luma_proc(int id, int index, int jobs, void* data)
{
	(void) id; // unused
	slice_desc* desc = ((slice_desc*) data);
	int slice_line_start, slice_height = mlt_slices_size_slice(jobs, index, desc->height, &slice_line_start);
	int size = desc->width * slice_height + 1;
	uint8_t *p = desc->alpha + slice_line_start * desc->width;
	uint8_t *q = desc->mask + slice_line_start * desc->width * 2;
	double a = 0;
	double b = 0;

	while (--size) {
		a = ((double)(*q ^ desc->invert_mask) - desc->offset) / desc->divisor;
		b = smoothstep(a, a + desc->softness, desc->mix);
		*p = (uint8_t )(*p * b) ^ desc->invert;
		p++;
		q += 2;
	}

	return 0;
}



/** Get the images and apply the luminance of the mask to the alpha of the frame.
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Fetch the data from the stack (mix, mask, filter)
	double mix = mlt_deque_pop_back_double( MLT_FRAME_IMAGE_STACK( frame ) );
	mlt_frame mask = mlt_frame_pop_service( frame );
	mlt_filter filter = mlt_frame_pop_service( frame );

	// Obtain the constants
	double softness = mlt_properties_get_double( MLT_FILTER_PROPERTIES( filter ), "softness" );
	int use_luminance = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "use_luminance" );
	int use_mix = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "use_mix" );
	int invert = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "invert" ) * 255;
	int invert_mask = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "invert_mask" ) * 255;

	if (mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "reverse")) {
		mix = 1.0 - mix;
		invert = !mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "invert" ) * 255;
	}

	// Render the frame
	*format = mlt_image_yuv422;
	*width -= *width % 2;
	if ( mlt_frame_get_image( frame, image, format, width, height, 1 ) == 0 &&
		 (!use_luminance || !use_mix || (int) mix != 1 || invert == 255 || invert_mask == 255) )
	{
		// Obtain a scaled/distorted mask to match
		uint8_t *mask_img = NULL;
		mlt_image_format mask_fmt = mlt_image_yuv422;
		mlt_properties_set_int( MLT_FRAME_PROPERTIES( mask ), "distort", 1 );
		mlt_properties_copy(MLT_FRAME_PROPERTIES(mask), MLT_FRAME_PROPERTIES(frame), "consumer.");

		if ( mlt_frame_get_image( mask, &mask_img, &mask_fmt, width, height, 0 ) == 0 )
		{
			int size = *width * *height;
			uint8_t* p = mlt_frame_get_alpha( frame );
			if ( !p )
			{
				int alphasize = *width * *height;
				p = mlt_pool_alloc( alphasize );
				memset( p, 255, alphasize );
				mlt_frame_set_alpha( frame, p, alphasize, mlt_pool_release );
			}

			if ( !use_luminance )
			{
				uint8_t* q = mlt_frame_get_alpha( mask );
				if ( !q )
				{
					mlt_log_warning(MLT_FILTER_SERVICE(filter), "failed to get alpha channel from mask: %s\n",
						mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "resource"));
					int alphasize = *width * *height;
					q = mlt_pool_alloc( alphasize );
					memset( q, 255, alphasize );
					mlt_frame_set_alpha( mask, q, alphasize, mlt_pool_release );
				}
				if ( use_mix )
				{
					slice_desc desc = {
					    .alpha = p,
					    .mask = q,
					    .width = *width,
					    .height = *height,
					    .softness = softness,
					    .mix = mix,
					    .invert = invert,
					    .invert_mask = invert_mask,
					    .offset = 0.0,
					    .divisor = 255.0
					};
					mlt_slices_run_normal(0, slice_alpha_proc, &desc);
				}
				else
				{
					while( size -- )
						*p++ = *q++;
				}
			}
			else if ( !use_mix )
			{
				// Do not apply threshold filter.
				uint8_t *q = mask_img;
				while( size -- )
				{
					*p = *q ^ invert_mask;
					p++;
					q += 2;
				}
			}
			else if ((int) mix != 1 || invert == 255 || invert_mask == 255)
			{
				int full_range = mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "full_range" );
				slice_desc desc = {
				    .alpha = p,
				    .mask = mask_img,
				    .width = *width,
				    .height = *height,
				    .softness = softness * (1.0 - mix),
				    .mix = mix,
				    .invert = invert,
				    .invert_mask = invert_mask,
				    .offset = full_range ? 0.0 : 16.0,
				    .divisor = full_range ? 255.0 : 235.0
				};
				mlt_slices_run_normal(0, slice_luma_proc, &desc);
			}
		}
	}

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	// Obtain the shape instance
	char *resource = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "resource" );
	if ( !resource ) return frame;
	char *last_resource = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "_resource" );
	mlt_producer producer = mlt_properties_get_data( MLT_FILTER_PROPERTIES( filter ), "instance", NULL );

	// Calculate the position and length
	int position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );

	// If we haven't created the instance or it's changed
	if ( producer == NULL || !last_resource || strcmp( resource, last_resource ) )
	{
		mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
		char temp[ 512 ];

		// Store the last resource now
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "_resource", resource );

		// This is a hack - the idea is that we can indirectly reference the
		// luma modules pgm or png images by a short cut like %luma01.pgm - we then replace
		// the % with the full path to the image and use it if it exists, if not, check for
		// the file ending in a .png, and failing that, default to a fade in
		if ( strchr( resource, '%' ) )
		{
			FILE *test;
			sprintf( temp, "%s/lumas/%s/%s", mlt_environment( "MLT_DATA" ), mlt_profile_lumas_dir(profile), strchr( resource, '%' ) + 1 );
			test = mlt_fopen( temp, "r" );

			if ( test == NULL )
			{
				strcat( temp, ".png" );
				test = mlt_fopen( temp, "r" );
			}

			if ( test ) {
				fclose( test );
				resource = temp;
			}
		}

		producer = mlt_factory_producer( profile, NULL, resource );
		if ( producer != NULL )
			mlt_properties_set( MLT_PRODUCER_PROPERTIES( producer ), "eof", "loop" );
		mlt_properties_set_data( MLT_FILTER_PROPERTIES( filter ), "instance", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );
	}

	// We may still not have a producer in which case, we do nothing
	if ( producer != NULL )
	{
		mlt_frame mask = NULL;
		double alpha_mix = mlt_properties_anim_get_double( MLT_FILTER_PROPERTIES(filter), "mix", position, length );
		mlt_properties_pass( MLT_PRODUCER_PROPERTIES( producer ), MLT_FILTER_PROPERTIES( filter ), "producer." );
		mlt_producer_seek( producer, position );
		if ( mlt_service_get_frame( MLT_PRODUCER_SERVICE( producer ), &mask, 0 ) == 0 )
		{
			char name[64];
			snprintf( name, sizeof(name), "shape %s",
			mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "_unique_id" ) );
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), name, mask, 0, ( mlt_destructor )mlt_frame_close, NULL );
			mlt_frame_push_service( frame, filter );
			mlt_frame_push_service( frame, mask );
			mlt_deque_push_back_double( MLT_FRAME_IMAGE_STACK( frame ), alpha_mix / 100.0 );
			mlt_frame_push_get_image( frame, filter_get_image );
			if ( mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "audio_match" ) )
			{
				mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "meta.mixdown", 1 );
				mlt_properties_set_double( MLT_FRAME_PROPERTIES( frame ), "meta.volume", alpha_mix / 100.0 );
			}
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "always_scale", 1 );
		}
	}

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_shape_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "resource", arg );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "mix", "100" );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "use_mix", 1 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "audio_match", 1 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "invert", 0 );
		mlt_properties_set_double( MLT_FILTER_PROPERTIES( filter ), "softness", 0.1 );
		filter->process = filter_process;
	}
	return filter;
}

