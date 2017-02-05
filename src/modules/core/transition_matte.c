/*
 * transition_matte.c -- replace alpha channel of track
 *
 * Copyright (C) 2003-2014 Meltytech, LLC
 * Author: Maksym Veremeyenko <verem@m1stereo.tv>
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

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#if defined(USE_SSE) && defined(ARCH_X86_64)
static void __attribute__((noinline)) copy_Y_to_A_scaled_luma_sse(uint8_t* alpha_a, uint8_t* image_b, int cnt)
{
	const static unsigned char const1[] =
	{
		43, 0, 43, 0, 43, 0, 43, 0, 43, 0, 43, 0, 43, 0, 43, 0
	};
	const static unsigned char const2[] =
	{
		16, 0, 16, 0, 16, 0, 16, 0, 16, 0, 16, 0, 16, 0, 16, 0
	};
	const static unsigned char const3[] =
	{
		235, 0, 235, 0, 235, 0, 235, 0, 235, 0, 235, 0, 235, 0, 235, 0
	};
	const static unsigned char const4[] =
	{
		255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0
	};

	__asm__ volatile
	(
		"movdqu         (%[equ43]), %%xmm7      \n\t"   /* load multiplier 43 */
		"movdqu         (%[equ16]), %%xmm6      \n\t"   /* load bottom value 16 */
		"movdqu         (%[equ235]), %%xmm5     \n\t"   /* load bottom value 235 */
		"movdqu         (%[equ255]), %%xmm4     \n\t"   /* load bottom value 0xff */

		"loop_start:                            \n\t"

		/* load pixels block 1 */
		"movdqu         0(%[image_b]), %%xmm0   \n\t"
		"add            $0x10, %[image_b]       \n\t"

		/* load pixels block 2 */
		"movdqu         0(%[image_b]), %%xmm1   \n\t"
		"add            $0x10, %[image_b]       \n\t"

		/* leave only Y */
		"pand           %%xmm4, %%xmm0          \n\t"
		"pand           %%xmm4, %%xmm1          \n\t"

		/* upper range clip */
		"pminsw         %%xmm5, %%xmm0          \n\t"
		"pminsw         %%xmm5, %%xmm1          \n\t"

		/* upper range clip */
		"pmaxsw         %%xmm6, %%xmm0          \n\t"
		"pmaxsw         %%xmm6, %%xmm1          \n\t"

		/* upper range clip */
		"psubw          %%xmm6, %%xmm0          \n\t"
		"psubw          %%xmm6, %%xmm1          \n\t"

		/* duplicate values */
		"movdqa         %%xmm0,%%xmm2           \n\t"
		"movdqa         %%xmm1,%%xmm3           \n\t"

		/* regA = regA << 8 */
		"psllw          $8, %%xmm0              \n\t"
		"psllw          $8, %%xmm1              \n\t"

		/* regB = regB * 47 */
		"pmullw         %%xmm7, %%xmm2          \n\t"
		"pmullw         %%xmm7, %%xmm3          \n\t"

		/* regA = regA + regB */
		"paddw          %%xmm2, %%xmm0          \n\t"
		"paddw          %%xmm3, %%xmm1          \n\t"

		/* regA = regA >> 8 */
		"psrlw          $8, %%xmm0              \n\t"
		"psrlw          $8, %%xmm1              \n\t"

		/* pack to 8 bit value */
		"packuswb       %%xmm1, %%xmm0          \n\t"

		/* store */
		"movdqu         %%xmm0, (%[alpha_a])    \n\t"
		"add            $0x10, %[alpha_a]       \n\t"

		/* loop if we done */
		"dec            %[cnt]                  \n\t"
		"jnz            loop_start              \n\t"
		:
		: [cnt]"r" (cnt), [alpha_a]"r"(alpha_a), [image_b]"r"(image_b), [equ43]"r"(const1), [equ16]"r"(const2), [equ235]"r"(const3), [equ255]"r"(const4)
	);
};
#endif

static void copy_Y_to_A_scaled_luma(uint8_t* alpha_a, int stride_a, uint8_t* image_b, int stride_b, int width, int height)
{
	int i, j;

	for(j = 0; j < height; j++)
	{
		i = 0;
#if defined(USE_SSE) && defined(ARCH_X86_64)
		if(width >= 16)
		{
			copy_Y_to_A_scaled_luma_sse(alpha_a, image_b, width >> 4);
			i = (width >> 4) << 4;
		}
#endif
		for(; i < width; i++)
		{
			unsigned int p = image_b[2*i];

			if(p < 16)
				p = 16;
			if(p > 235)
				p = 235;
			/* p = (p - 16) * 255 / 219; */
			p -= 16;
			p = ((p << 8) + (p * 43)) >> 8;

			alpha_a[i] = p;
		};

		alpha_a += stride_a;
		image_b += stride_b;
	};
};

/** Get the image.
*/

static int transition_get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );

	mlt_frame_get_image( a_frame, image, format, width, height, 1 );

	// Get the properties of the a frame
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );

	// Get the properties of the b frame
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	int
		width_a = mlt_properties_get_int( a_props, "width" ),
		width_b = mlt_properties_get_int( b_props, "width" ),
		height_a = mlt_properties_get_int( a_props, "height" ),
		height_b = mlt_properties_get_int( b_props, "height" );

	uint8_t *alpha_a, *image_b;

	// This transition is yuv422 only
	*format = mlt_image_yuv422;

	// Get the image from the a frame
	mlt_frame_get_image( b_frame, &image_b, format, &width_b, &height_b, 1 );
	alpha_a = mlt_frame_get_alpha_mask( a_frame );

	// copy data
	copy_Y_to_A_scaled_luma
	(
		alpha_a, width_a, image_b, width_b * 2,
		(width_a > width_b)?width_b:width_a,
		(height_a > height_b)?height_b:height_a
	);

	// Extract the a_frame image info
	*width = mlt_properties_get_int( a_props, "width" );
	*height = mlt_properties_get_int( a_props, "height" );
	*image = mlt_properties_get_data( a_props, "image", NULL );

	return 0;
}


/** Matte transition processing.
*/

static mlt_frame transition_process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	// Push the b_frame on to the stack
	mlt_frame_push_frame( a_frame, b_frame );

	// Push the transition method
	mlt_frame_push_get_image( a_frame, transition_get_image );
	
	return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_matte_init( mlt_profile profile, mlt_service_type type, const char *id, char *lumafile )
{
	mlt_transition transition = mlt_transition_new( );
	if ( transition != NULL )
	{
		// Set the methods
		transition->process = transition_process;
		
		// Inform apps and framework that this is a video only transition
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "_transition_type", 1 );

		return transition;
	}
	return NULL;
}
