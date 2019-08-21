/*
 * luma.c -- image generator for transition_luma
 * Copyright (C) 2003-2019 Meltytech, LLC
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

#include <framework/mlt_pool.h>
#include <framework/mlt_luma_map.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int main( int argc, char **argv )
{
	int arg = 1;
	int bpp = 8;

	struct mlt_luma_map_s self;
	uint16_t *image = NULL;
	const char *filename = NULL;

	mlt_luma_map_init( &self );

	for ( arg = 1; arg < argc; arg ++ )
	{
		if ( !strcmp( argv[ arg ], "-bpp" ) )
			bpp = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-type" ) )
			self.type = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-w" ) )
		{
			int tmp = atoi( argv[ ++ arg ] );
			// TODO: is there an upper bound?
			if ( tmp )
				self.w = tmp;
			else
				return 1;
		}
		else if ( !strcmp( argv[ arg ], "-h" ) )
		{
			int tmp = atoi( argv[ ++ arg ] );
			// TODO: is there an upper bound?
			if ( tmp )
				self.h = tmp;
			else return 1;
		}
		else if ( !strcmp( argv[ arg ], "-bands" ) )
		{
			int tmp = atoi( argv[ ++ arg ] );
			// TODO: is there an upper bound?
			if ( tmp >= 0 )
				self.bands = tmp;
			else
				return 1;
		}
		else if ( !strcmp( argv[ arg ], "-rband" ) )
			self.rband = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-hmirror" ) )
			self.hmirror = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-vmirror" ) )
			self.vmirror = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-dmirror" ) )
			self.dmirror = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-offset" ) )
			self.offset = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-invert" ) )
			self.invert = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-flip" ) )
			self.flip = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-flop" ) )
			self.flop = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-pflip" ) )
			self.pflip = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-pflop" ) )
			self.pflop = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-quart" ) )
			self.quart = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-rotate" ) )
			self.rotate = atoi( argv[ ++ arg ] );
		else
			filename = argv[arg];
	}

	if ( bpp != 8 && bpp != 16 )
	{
		fprintf( stderr, "Invalid bpp %d\n", bpp );
		return 1;
	}

	mlt_pool_init();
	image = mlt_luma_map_render( &self );

	if ( bpp == 16 )
	{
		uint16_t *end = image + self.w * self.h;
		uint16_t *p = image;
		uint8_t *q = ( uint8_t * )image;
		while ( p < end )
		{
			*p ++ = ( *q << 8 ) + *( q + 1 );
			q += 2;
		}
		if (filename) {
			FILE *f = fopen(filename, "wb");
			if (f) {
				fprintf(f, "P5\x0a" );
				fprintf(f, "%d %d\x0a", self.w, self.h );
				fprintf(f, "65535\x0a" );
				fwrite( image, self.w * self.h * sizeof( uint16_t ), 1, f );
				fclose(f);
			}
		} else {
			printf( "P5\n" );
			printf( "%d %d\n", self.w, self.h );
			printf( "65535\n" );
			fwrite( image, self.w * self.h * sizeof( uint16_t ), 1, stdout );
		}
	}
	else
	{
		uint16_t *end = image + self.w * self.h;
		uint16_t *p = image;
		uint8_t *q = ( uint8_t * )image;
		while ( p < end )
			*q ++ = ( uint8_t )( *p ++ >> 8 );
		printf( "P5\n" );
		printf( "%d %d\n", self.w, self.h );
		printf( "255\n" );
		fwrite( image, self.w * self.h, 1, stdout );
	}

	return 0;
}

