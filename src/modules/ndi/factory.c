/*
 * factory.c -- output through NewTek NDI
 * Copyright (C) 2016 Maksym Veremeyenko <verem@m1stereo.tv>
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
 * License along with consumer library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define __STDC_FORMAT_MACROS  /* see inttypes.h */
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#define _XOPEN_SOURCE

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>
#include <pthread.h>

#include "factory.h"

#include "Processing.NDI.Lib.h"

void swab2( const void *from, void *to, int n )
{
#if defined(USE_SSE)
#define SWAB_STEP 16
	__asm__ volatile
	(
		"loop_start:                            \n\t"

		/* load */
		"movdqa         0(%[from]), %%xmm0      \n\t"
		"add            $0x10, %[from]          \n\t"

		/* duplicate to temp registers */
		"movdqa         %%xmm0, %%xmm1          \n\t"

		/* shift right temp register */
		"psrlw          $8, %%xmm1              \n\t"

		/* shift left main register */
		"psllw          $8, %%xmm0              \n\t"

		/* compose them back */
		"por           %%xmm0, %%xmm1           \n\t"

		/* save */
		"movdqa         %%xmm1, 0(%[to])        \n\t"
		"add            $0x10, %[to]            \n\t"

		"dec            %[cnt]                  \n\t"
		"jnz            loop_start              \n\t"

		:
		: [from]"r"(from), [to]"r"(to), [cnt]"r"(n / SWAB_STEP)
		//: "xmm0", "xmm1"
	);

	from = (unsigned char*) from + n - (n % SWAB_STEP);
	to = (unsigned char*) to + n - (n % SWAB_STEP);
	n = (n % SWAB_STEP);
#endif
	swab((char*) from, (char*) to, n);
};

#define SWAB_SLICED_ALIGN_POW 5
int swab_sliced( int id, int idx, int jobs, void* cookie )
{
	unsigned char** args = (unsigned char**)cookie;
	ssize_t sz = (ssize_t)args[2];
	ssize_t bsz = ( ( sz / jobs + ( 1 << SWAB_SLICED_ALIGN_POW ) - 1 ) >> SWAB_SLICED_ALIGN_POW ) << SWAB_SLICED_ALIGN_POW;
	ssize_t offset = bsz * idx;

	if ( offset < sz )
	{
		if ( ( offset + bsz ) > sz )
			bsz = sz - offset;

		swab2( args[0] + offset, args[1] + offset, bsz );
	}

	return 0;
};

static void *create_service( mlt_profile profile, mlt_service_type type, const char *id, void *arg )
{
	mlt_log_debug( NULL, "%s: entering id=[%s]\n", __FUNCTION__, id );

	if ( !NDIlib_initialize() )
	{
		mlt_log_error( NULL, "%s: NDIlib_initialize failed\n", __FUNCTION__ );
		return NULL;
	}

	if ( type == producer_type )
		return producer_ndi_init( profile, type, id, (char*)arg );
	else if ( type == consumer_type )
		return consumer_ndi_init( profile, type, id, (char*)arg );

	return NULL;
}

MLT_REPOSITORY
{
	MLT_REGISTER( consumer_type, "ndi", create_service );
	MLT_REGISTER( producer_type, "ndi", create_service );
}
