/*
 * luma.c -- image generator for transition_luma
 * Copyright (C) 2003-2018 Meltytech, LLC
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct 
{
	int type;
	int w;
	int h;
	int bands;
	int rband;
	int vmirror;
	int hmirror;
	int dmirror;
	int invert;
	int offset;
	int flip;
	int flop;
	int pflip;
	int pflop;
	int quart;
	int rotate;
}
luma;

void luma_init( luma *self )
{
	memset( self, 0, sizeof( luma ) );
	self->type = 0;
	self->w = 720;
	self->h = 576;
	self->bands = 1;
	self->rband = 0;
	self->vmirror = 0;
	self->hmirror = 0;
	self->dmirror = 0;
	self->invert = 0;
	self->offset = 0;
	self->flip = 0;
	self->flop = 0;
	self->quart = 0;
	self->pflop = 0;
	self->pflip = 0;
}

static inline int sqrti( int n )
{
    int p = 0;
	int q = 1;
	int r = n;
	int h = 0;

    while( q <= n ) 
		q = 4 * q;

    while( q != 1 )
    {
        q = q / 4;
        h = p + q;
        p = p / 2;
        if ( r >= h )
        {
            p = p + q;
            r = r - h;
        } 
    }

    return p;
}

uint16_t *luma_render( luma *self )
{
	int i = 0;
	int j = 0;
	int k = 0;

	if ( self->quart )
	{
		self->w *= 2;
		self->h *= 2;
	}

	if ( self->rotate )
	{
		int t = self->w;
		self->w = self->h;
		self->h = t;
	}

	int max = ( 1 << 16 ) - 1;
	uint16_t *image = malloc( self->w * self->h * sizeof( uint16_t ) );
	uint16_t *end = image + self->w * self->h;
	uint16_t *p = image;
	uint16_t *r = image;
	int lower = 0;
	int lpb = self->h / self->bands;
	int rpb = max / self->bands;
	int direction = 1;

	int half_w = self->w / 2;
	int half_h = self->h / 2;

	if ( !self->dmirror && ( self->hmirror || self->vmirror ) )
		rpb *= 2;

	for ( i = 0; i < self->bands; i ++ )
	{
		lower = i * rpb;
		direction = 1;

		if ( self->rband && i % 2 == 1 )
		{
			direction = -1;
			lower += rpb;
		}

		switch( self->type )
		{
			case 1:
				{
					int length = sqrti( half_w * half_w + lpb * lpb / 4 );
					int value;
					int x = 0;
					int y = 0;
					for ( j = 0; j < lpb; j ++ )
					{
						y = j - lpb / 2;
						for ( k = 0; k < self->w; k ++ )
						{
							x = k - half_w;
							value = sqrti( x * x + y * y );
							*p ++ = lower + ( direction * rpb * ( ( max * value ) / length ) / max ) + ( j * self->offset * 2 / lpb ) + ( j * self->offset / lpb );
						}
					}
				}
				break;

			case 2:
				{
					for ( j = 0; j < lpb; j ++ )
					{
						int value = ( ( j * self->w ) / lpb ) - half_w;
						if ( value > 0 )
							value = - value;
						for ( k = - half_w; k < value; k ++ )
							*p ++ = lower + ( direction * rpb * ( ( max * abs( k ) ) / half_w ) / max );
						for ( k = value; k < abs( value ); k ++ )
							*p ++ = lower + ( direction * rpb * ( ( max * abs( value ) ) / half_w ) / max ) + ( j * self->offset * 2 / lpb ) + ( j * self->offset / lpb );
						for ( k = abs( value ); k < half_w; k ++ )
							*p ++ = lower + ( direction * rpb * ( ( max * abs( k ) ) / half_w ) / max );
					}
				}
				break;

			case 3:
				{
					int length;
					for ( j = -half_h; j < half_h; j ++ )
					{
						if ( j < 0 )
						{
							for ( k = - half_w; k < half_w; k ++ )
							{
								length = sqrti( k * k + j * j );
								*p ++ = ( max / 4 * k ) / ( length + 1 );
							}
						}
						else
						{
							for ( k = half_w; k > - half_w; k -- )
							{
								length = sqrti( k * k + j * j );
								*p ++ = ( max / 2 ) + ( max / 4 * k ) / ( length + 1 );
							}
						}
					}
				}
				break;

			default:
				for ( j = 0; j < lpb; j ++ )
					for ( k = 0; k < self->w; k ++ )
						*p ++ = lower + ( direction * ( rpb * ( ( k * max ) / self->w ) / max ) ) + ( j * self->offset * 2 / lpb );
				break;
		}
	}

	if ( self->quart )
	{
		self->w /= 2;
		self->h /= 2;
		for ( i = 1; i < self->h; i ++ )
		{
			p = image + i * self->w;
			r = image + i * 2 * self->w;
			j = self->w;
			while ( j -- > 0 )
				*p ++ = *r ++;
		}
	}

	if ( self->dmirror )
	{
		for ( i = 0; i < self->h; i ++ )
		{
			p = image + i * self->w;
			r = end - i * self->w;
			j = ( self->w * ( self->h - i ) ) / self->h;
			while ( j -- )
				*( -- r ) = *p ++;
		}
	}

	if ( self->flip )
	{
		uint16_t t;
		for ( i = 0; i < self->h; i ++ )
		{
			p = image + i * self->w;
			r = p + self->w;
			while( p != r )
			{
				t = *p;
				*p ++ = *( -- r );
				*r = t;
			}
		}
	}

	if ( self->flop )
	{
		uint16_t t;
		r = end;
		for ( i = 1; i < self->h / 2; i ++ )
		{
			p = image + i * self->w;
			j = self->w;
			while( j -- )
			{
				t = *( -- p );
				*p = *( -- r );
				*r = t;
			}
		}
	}

	if ( self->hmirror )
	{
		p = image;
		while ( p < end )
		{
			r = p + self->w;
			while ( p != r )
				*( -- r ) = *p ++;
			p += self->w / 2;
		}
	}

	if ( self->vmirror )
	{
		p = image;
		r = end;
		while ( p != r )
			*( -- r ) = *p ++;
	}

	if ( self->invert )
	{
		p = image;
		r = image;
		while ( p < end )
			*p ++ = max - *r ++;
	}

	if ( self->pflip )
	{
		uint16_t t;
		for ( i = 0; i < self->h; i ++ )
		{
			p = image + i * self->w;
			r = p + self->w;
			while( p != r )
			{
				t = *p;
				*p ++ = *( -- r );
				*r = t;
			}
		}
	}

	if ( self->pflop )
	{
		uint16_t t;
		end = image + self->w * self->h;
		r = end;
		for ( i = 1; i < self->h / 2; i ++ )
		{
			p = image + i * self->w;
			j = self->w;
			while( j -- )
			{
				t = *( -- p );
				*p = *( -- r );
				*r = t;
			}
		}
	}

	if ( self->rotate )
	{
		uint16_t *image2 = malloc( self->w * self->h * sizeof( uint16_t ) );
		for ( i = 0; i < self->h; i ++ )
		{
			p = image + i * self->w;
			r = image2 + self->h - i - 1;
			for ( j = 0; j < self->w; j ++ )
			{
				*r = *( p ++ );
				r += self->h;
			}
		}
		i = self->w;
		self->w = self->h;
		self->h = i;
		free( image );
		image = image2;
	}

	return image;
}

int main( int argc, char **argv )
{
	int arg = 1;
	int bpp = 8;

	luma self;
	uint16_t *image = NULL;

	luma_init( &self );

	for ( arg = 1; arg < argc - 1; arg ++ )
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
			fprintf( stderr, "ignoring %s\n", argv[ arg ] );
	}

	if ( bpp != 8 && bpp != 16 )
	{
		fprintf( stderr, "Invalid bpp %d\n", bpp );
		return 1;
	}

	image = luma_render( &self );

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
		printf( "P5\n" );
		printf( "%d %d\n", self.w, self.h );
		printf( "65535\n" );
		fwrite( image, self.w * self.h * sizeof( uint16_t ), 1, stdout );
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

