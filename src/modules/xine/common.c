/*
 * common.c
 * Copyright (C) 2023 Meltytech, LLC
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

#include "common.h"
#include "deinterlace.h"
#include "yadif.h"

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

mlt_deinterlacer supported_method( mlt_deinterlacer method )
{
	// Round the method down to the next one that is supported
	if ( method <= mlt_deinterlacer_bob ) method = mlt_deinterlacer_bob;
	else if ( method <= mlt_deinterlacer_weave ) method = mlt_deinterlacer_weave;
	else if ( method <= mlt_deinterlacer_onefield ) method = mlt_deinterlacer_onefield;
	else if ( method <= mlt_deinterlacer_linearblend ) method = mlt_deinterlacer_linearblend;
	else if ( method <= mlt_deinterlacer_greedy ) method = mlt_deinterlacer_greedy;
	else if ( method <= mlt_deinterlacer_yadif_nospatial ) method = mlt_deinterlacer_yadif_nospatial;
	else if ( method <= mlt_deinterlacer_invalid ) method = mlt_deinterlacer_yadif;
	return method;
}

int deinterlace_image( mlt_image dst, mlt_image src, mlt_image prev, mlt_image next, int tff, mlt_deinterlacer method )
{
	if ( !dst || !src || !dst->data || !src->data )
	{
		return 1;
	}

	if ( method >= mlt_deinterlacer_yadif_nospatial &&
		 ( !prev || !next || !prev->data || !next->data ) )
	{
		method = mlt_deinterlacer_linearblend;
	}

	if ( method == mlt_deinterlacer_bob ) {
		deinterlace_yuv( dst->data, (uint8_t**)&src->data, src->width * 2, src->height, DEINTERLACE_BOB );
	} else if ( method == mlt_deinterlacer_weave ) {
		deinterlace_yuv( dst->data, (uint8_t**)&src->data, src->width * 2, src->height, DEINTERLACE_WEAVE );
	} else if ( method == mlt_deinterlacer_onefield ) {
		deinterlace_yuv( dst->data, (uint8_t**)&src->data, src->width * 2, src->height, DEINTERLACE_ONEFIELD );
	} else if ( method == mlt_deinterlacer_linearblend ) {
		deinterlace_yuv( dst->data, (uint8_t**)&src->data, src->width * 2, src->height, DEINTERLACE_LINEARBLEND );
	} else if ( method == mlt_deinterlacer_greedy ) {
		deinterlace_yuv( dst->data, (uint8_t**)&src->data, src->width * 2, src->height, DEINTERLACE_GREEDY );
	} else if ( method >= mlt_deinterlacer_yadif_nospatial ) {
		yadif_filter *yadif = init_yadif( src->width, src->height );
		if ( yadif )
		{
			const int pitch = src->width << 1;
			const int parity = 0;
			const int YADIF_MODE_TEMPORAL_SPATIAL = 0;
			const int YADIF_MODE_TEMPORAL = 2;
			int mode = YADIF_MODE_TEMPORAL_SPATIAL;
			if ( method == mlt_deinterlacer_yadif_nospatial )
			{
				mode = YADIF_MODE_TEMPORAL;
			}

			// Convert packed to planar
			YUY2ToPlanes( src->data, pitch, src->width, src->height, yadif->ysrc,
				yadif->ypitch, yadif->usrc, yadif->vsrc, yadif->uvpitch, yadif->cpu );
			YUY2ToPlanes( prev->data, pitch, src->width, src->height, yadif->yprev,
				yadif->ypitch, yadif->uprev, yadif->vprev, yadif->uvpitch, yadif->cpu );
			YUY2ToPlanes( next->data, pitch, src->width, src->height, yadif->ynext,
				yadif->ypitch, yadif->unext, yadif->vnext, yadif->uvpitch, yadif->cpu );

			// Deinterlace each plane
			filter_plane( mode, yadif->ydest, yadif->ypitch, yadif->yprev, yadif->ysrc,
				yadif->ynext, yadif->ypitch, src->width, src->height, parity, tff, yadif->cpu);
			filter_plane( mode, yadif->udest, yadif->uvpitch,yadif->uprev, yadif->usrc,
				yadif->unext, yadif->uvpitch, src->width >> 1, src->height, parity, tff, yadif->cpu);
			filter_plane( mode, yadif->vdest, yadif->uvpitch, yadif->vprev, yadif->vsrc,
				yadif->vnext, yadif->uvpitch, src->width >> 1, src->height, parity, tff, yadif->cpu);

			// Convert planar to packed
			YUY2FromPlanes( dst->data, pitch, src->width, src->height, yadif->ydest,
				yadif->ypitch, yadif->udest, yadif->vdest, yadif->uvpitch, yadif->cpu);

			close_yadif( yadif );
		}
	}
	else
	{
		// If all else fails, default to linear blend
		deinterlace_yuv( dst->data, (uint8_t**)&src->data, src->width * 2, src->height, DEINTERLACE_LINEARBLEND );
	}
	return 0;
}
