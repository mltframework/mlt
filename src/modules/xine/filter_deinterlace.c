/*
 * filter_deinterlace.c -- deinterlace filter
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include <framework/mlt_filter.h>
#include <framework/mlt_log.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_events.h>
#include "deinterlace.h"
#include "yadif.h"

#include <framework/mlt_frame.h>

#include <string.h>
#include <stdlib.h>

#define YADIF_MODE_TEMPORAL_SPATIAL (0)
#define YADIF_MODE_TEMPORAL (2)

static yadif_filter *init_yadif( int width, int height )
{
	yadif_filter *yadif = mlt_pool_alloc( sizeof( *yadif ) );

	yadif->cpu = 0; // Pure C
#ifdef USE_SSE
	yadif->cpu |= AVS_CPU_INTEGER_SSE;
#endif
#ifdef USE_SSE2
	yadif->cpu |= AVS_CPU_SSE2;
#endif
	// Create intermediate planar planes
	yadif->yheight = height;
	yadif->ywidth  = width;
	yadif->uvwidth = yadif->ywidth / 2;
	yadif->ypitch  = ( yadif->ywidth +  15 ) / 16 * 16;
	yadif->uvpitch = ( yadif->uvwidth + 15 ) / 16 * 16;
	yadif->ysrc  = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->ypitch );
	yadif->usrc  = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->uvpitch);
	yadif->vsrc  = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->uvpitch );
	yadif->yprev = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->ypitch );
	yadif->uprev = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->uvpitch );
	yadif->vprev = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->uvpitch );
	yadif->ynext = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->ypitch );
	yadif->unext = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->uvpitch );
	yadif->vnext = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->uvpitch );
	yadif->ydest = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->ypitch );
	yadif->udest = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->uvpitch );
	yadif->vdest = (unsigned char *) mlt_pool_alloc( yadif->yheight * yadif->uvpitch );

	return yadif;
}

static void close_yadif(yadif_filter *yadif)
{
	mlt_pool_release( yadif->ysrc );
	mlt_pool_release( yadif->usrc );
	mlt_pool_release( yadif->vsrc );
	mlt_pool_release( yadif->yprev );
	mlt_pool_release( yadif->uprev );
	mlt_pool_release( yadif->vprev );
	mlt_pool_release( yadif->ynext );
	mlt_pool_release( yadif->unext );
	mlt_pool_release( yadif->vnext );
	mlt_pool_release( yadif->ydest );
	mlt_pool_release( yadif->udest );
	mlt_pool_release( yadif->vdest );
	mlt_pool_release( yadif );

#if defined(__GNUC__) && !defined(PIC)
	// Set SSSE3 bit to cpu
	asm (\
	"mov $1, %%eax \n\t"\
	"push %%ebx \n\t"\
	"cpuid \n\t"\
	"pop %%ebx \n\t"\
	"mov %%ecx, %%edx \n\t"\
	"shr $9, %%edx \n\t"\
	"and $1, %%edx \n\t"\
	"shl $9, %%edx \n\t"\
	"and $511, %%ebx \n\t"\
	"or %%edx, %%ebx \n\t"\
	: "=b"(yadif->cpu) : "p"(yadif->cpu) : "%eax", "%ecx", "%edx");
#endif
}

static int deinterlace_yadif( mlt_frame frame, mlt_filter filter, uint8_t **image, mlt_image_format *format, int *width, int *height, int mode )
{
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	mlt_frame previous_frame = mlt_properties_get_data( properties, "previous frame", NULL );
	uint8_t* previous_image = NULL;
	int previous_width = *width;
	int previous_height = *height;
	mlt_frame next_frame = mlt_properties_get_data( properties, "next frame", NULL );
	uint8_t* next_image = NULL;
	int next_width = *width;
	int next_height = *height;
	
	mlt_log_debug( MLT_FILTER_SERVICE(filter), "previous " MLT_POSITION_FMT " current " MLT_POSITION_FMT " next " MLT_POSITION_FMT "\n",
		previous_frame? mlt_frame_original_position(previous_frame) : -1,
		mlt_frame_original_position(frame),
		next_frame?  mlt_frame_original_position(next_frame) : -1);

	if ( !previous_frame || !next_frame )
		return 1;

	mlt_service_lock( MLT_FILTER_SERVICE(filter) );

	// Get the preceding frame's image
	int error = mlt_frame_get_image( previous_frame, &previous_image, format, &previous_width, &previous_height, 0 );
	int progressive = mlt_properties_get_int( MLT_FRAME_PROPERTIES( previous_frame ), "progressive" );

	// Check that we aren't already progressive
	if ( !error && previous_image && !progressive )
	{
		// OK, now we know we have work to do and can request the image in our format
		frame->convert_image( previous_frame, &previous_image, format, mlt_image_yuv422 );

		mlt_service_unlock( MLT_FILTER_SERVICE(filter) );

		// Get the current frame's image
		*format = mlt_image_yuv422;
		error = mlt_frame_get_image( frame, image, format, width, height, 1 );

		if ( !error && *image && *format == mlt_image_yuv422 )
		{
			// Get the following frame's image
			error = mlt_frame_get_image( next_frame, &next_image, format, &next_width, &next_height, 0 );

			if ( !error && next_image && *format == mlt_image_yuv422 )
			{
				yadif_filter *yadif = init_yadif( *width, *height );
				if ( yadif )
				{
					const int order = mlt_properties_get_int( properties, "top_field_first" );
					const int pitch = *width << 1;
					const int parity = 0;

					// Convert packed to planar
					YUY2ToPlanes( *image, pitch, *width, *height, yadif->ysrc,
						yadif->ypitch, yadif->usrc, yadif->vsrc, yadif->uvpitch, yadif->cpu );
					YUY2ToPlanes( previous_image, pitch, *width, *height, yadif->yprev,
						yadif->ypitch, yadif->uprev, yadif->vprev, yadif->uvpitch, yadif->cpu );
					YUY2ToPlanes( next_image, pitch, *width, *height, yadif->ynext,
						yadif->ypitch, yadif->unext, yadif->vnext, yadif->uvpitch, yadif->cpu );

					// Deinterlace each plane
					filter_plane( mode, yadif->ydest, yadif->ypitch, yadif->yprev, yadif->ysrc,
						yadif->ynext, yadif->ypitch, *width, *height, parity, order, yadif->cpu);
					filter_plane( mode, yadif->udest, yadif->uvpitch,yadif->uprev, yadif->usrc,
						yadif->unext, yadif->uvpitch, *width >> 1, *height, parity, order, yadif->cpu);
					filter_plane( mode, yadif->vdest, yadif->uvpitch, yadif->vprev, yadif->vsrc,
						yadif->vnext, yadif->uvpitch, *width >> 1, *height, parity, order, yadif->cpu);

					// Convert planar to packed
					YUY2FromPlanes( *image, pitch, *width, *height, yadif->ydest,
						yadif->ypitch, yadif->udest, yadif->vdest, yadif->uvpitch, yadif->cpu);

					close_yadif( yadif );
				}
			}
		}
	}
	else
	{
		mlt_service_unlock( MLT_FILTER_SERVICE(filter) );

		// Get the current frame's image
		error = mlt_frame_get_image( frame, image, format, width, height, 0 );
	}
	return error;
}

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	int deinterlace = mlt_properties_get_int( properties, "consumer_deinterlace" );
	// The progressive var should only represent the frame's original state and not the state
	// as modified by this filter!
	int progressive = mlt_properties_get_int( properties, "progressive" );
	// At this point - before image was requested - (progressive == 0) cannot be trusted because
	// some producers (avformat) do not yet know.

	if ( deinterlace && !mlt_properties_get_int( properties, "test_image" ) )
	{
		// Determine deinterlace method
		char *method_str = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "method" );
		int method = DEINTERLACE_NONE;
		char *frame_method_str = mlt_properties_get( properties, "deinterlace_method" );
		
		if ( frame_method_str )
			method_str = frame_method_str;
		
		if ( !method_str || strcmp( method_str, "yadif" ) == 0 )
			method = DEINTERLACE_YADIF;
		else if ( strcmp( method_str, "yadif-nospatial" ) == 0 )
			method = DEINTERLACE_YADIF_NOSPATIAL;
		else if ( strcmp( method_str, "onefield" ) == 0 )
			method = DEINTERLACE_ONEFIELD;
		else if ( strcmp( method_str, "linearblend" ) == 0 )
			method = DEINTERLACE_LINEARBLEND;
		else if ( strcmp( method_str, "bob" ) == 0 )
			method = DEINTERLACE_BOB;
		else if ( strcmp( method_str, "weave" ) == 0 )
			method = DEINTERLACE_BOB;
		else if ( strcmp( method_str, "greedy" ) == 0 )
			method = DEINTERLACE_GREEDY;

		// Some producers like pixbuf want rescale_width & _height, but will not get them if you request
		// the previous image first. So, on the first iteration, we use linearblend.
		if ( ( method == DEINTERLACE_YADIF || method == DEINTERLACE_YADIF_NOSPATIAL ) &&
			!mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "_notfirst" ) )
		{
			mlt_properties_set_int( MLT_FILTER_PROPERTIES(filter), "_notfirst", 1 );
			method = DEINTERLACE_LINEARBLEND;
			error = 1;
		}

		if ( method == DEINTERLACE_YADIF )
		{
			error = deinterlace_yadif( frame, filter, image, format, width, height, YADIF_MODE_TEMPORAL_SPATIAL );
		}
		else if ( method == DEINTERLACE_YADIF_NOSPATIAL )
		{
			error = deinterlace_yadif( frame, filter, image, format, width, height, YADIF_MODE_TEMPORAL );
		}
		if ( error || ( method > DEINTERLACE_NONE && method < DEINTERLACE_YADIF ) )
		{
			mlt_service service = mlt_properties_get_data( MLT_FILTER_PROPERTIES(filter), "service", NULL );

			// Get the current frame's image
			int error2 = mlt_frame_get_image( frame, image, format, width, height, writable );
			progressive = mlt_properties_get_int( properties, "progressive" );

			if ( error )
			{
				method = DEINTERLACE_LINEARBLEND;
				// If YADIF requested, prev/next cancelled because some previous frames were progressive,
				// but new frames are interlaced, then turn prev/next frames back on.
				if ( !progressive )
					mlt_properties_set_int( MLT_SERVICE_PROPERTIES(service), "_need_previous_next", 1 );
			}
			else
			{
				// Signal that we no longer need previous and next frames
				mlt_properties_set_int( MLT_SERVICE_PROPERTIES(service), "_need_previous_next", 0 );
			}
			error = error2;
			
			if ( !error && !progressive )
			{
				// OK, now we know we have work to do and can request the image in our format
				error = frame->convert_image( frame, image, format, mlt_image_yuv422 );

				// Check that we aren't already progressive
				if ( !error && *image && *format == mlt_image_yuv422 )
				{
					// Deinterlace the image using one of the Xine deinterlacers
					int image_size = mlt_image_format_size( *format, *width, *height, NULL );
					uint8_t *new_image = mlt_pool_alloc( image_size );

					deinterlace_yuv( new_image, image, *width * 2, *height, method );
					mlt_frame_set_image( frame, new_image, image_size, mlt_pool_release );
					*image = new_image;
				}
			}
		}
		else if ( method == DEINTERLACE_NONE )
		{
			error = mlt_frame_get_image( frame, image, format, width, height, writable );
		}

		// update progressive flag after having obtained image
		progressive = mlt_properties_get_int( properties, "progressive" );

		mlt_log_debug( MLT_FILTER_SERVICE( filter ), "error %d deint %d prog %d fmt %s method %s\n",
			error, deinterlace, progressive, mlt_image_format_name( *format ), method_str ? method_str : "yadif" );

		if ( !error )
		{
			// Make sure that others know the frame is deinterlaced
			mlt_properties_set_int( properties, "progressive", 1 );
		}
	}
	else
	{
		// Pass through
		error = mlt_frame_get_image( frame, image, format, width, height, writable );
	}

	if ( !deinterlace || progressive )
	{
		// Signal that we no longer need previous and next frames
		mlt_service service = mlt_properties_get_data( MLT_FILTER_PROPERTIES(filter), "service", NULL );
		if ( service )
			mlt_properties_set_int( MLT_SERVICE_PROPERTIES(service), "_need_previous_next", 0 );
	}

	return error;
}

/** Deinterlace filter processing - this should be lazy evaluation here...
*/

static mlt_frame deinterlace_process( mlt_filter filter, mlt_frame frame )
{
	// Push filter on to the service stack
	mlt_frame_push_service( frame, filter );

	// Push the get_image method on to the stack
	mlt_frame_push_get_image( frame, filter_get_image );
	
	return frame;
}

static void on_service_changed( mlt_service owner, mlt_service filter )
{
	mlt_service service = mlt_properties_get_data( MLT_SERVICE_PROPERTIES(filter), "service", NULL );
	mlt_properties_set_int( MLT_SERVICE_PROPERTIES(service), "_need_previous_next", 1 );
}

/** Constructor for the filter.
*/

mlt_filter filter_deinterlace_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = deinterlace_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "method", arg );
		mlt_events_listen( MLT_FILTER_PROPERTIES( filter ), filter, "service-changed", (mlt_listener) on_service_changed );
	}
	return filter;
}

