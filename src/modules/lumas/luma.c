/*
 * luma.c -- image generator for transition_luma
 * Copyright (C) 2003-2014 Meltytech, LLC
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

void luma_init( luma *this )
{
	memset( this, 0, sizeof( luma ) );
	this->type = 0;
	this->w = 720;
	this->h = 576;
	this->bands = 1;
	this->rband = 0;
	this->vmirror = 0;
	this->hmirror = 0;
	this->dmirror = 0;
	this->invert = 0;
	this->offset = 0;
	this->flip = 0;
	this->flop = 0;
	this->quart = 0;
	this->pflop = 0;
	this->pflip = 0;
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

uint16_t *luma_render( luma *this )
{
	int i = 0;
	int j = 0;
	int k = 0;

	if ( this->quart )
	{
		this->w *= 2;
		this->h *= 2;
	}

	if ( this->rotate )
	{
		int t = this->w;
		this->w = this->h;
		this->h = t;
	}

	int max = ( 1 << 16 ) - 1;
	uint16_t *image = malloc( this->w * this->h * sizeof( uint16_t ) );
	uint16_t *end = image + this->w * this->h;
	uint16_t *p = image;
	uint16_t *r = image;
	int lower = 0;
	int lpb = this->h / this->bands;
	int rpb = max / this->bands;
	int direction = 1;

	int half_w = this->w / 2;
	int half_h = this->h / 2;

	if ( !this->dmirror && ( this->hmirror || this->vmirror ) )
		rpb *= 2;

	for ( i = 0; i < this->bands; i ++ )
	{
		lower = i * rpb;
		direction = 1;

		if ( this->rband && i % 2 == 1 )
		{
			direction = -1;
			lower += rpb;
		}

		switch( this->type )
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
						for ( k = 0; k < this->w; k ++ )
						{
							x = k - half_w;
							value = sqrti( x * x + y * y );
							*p ++ = lower + ( direction * rpb * ( ( max * value ) / length ) / max ) + ( j * this->offset * 2 / lpb ) + ( j * this->offset / lpb );
						}
					}
				}
				break;

			case 2:
				{
					for ( j = 0; j < lpb; j ++ )
					{
						int value = ( ( j * this->w ) / lpb ) - half_w;
						if ( value > 0 )
							value = - value;
						for ( k = - half_w; k < value; k ++ )
							*p ++ = lower + ( direction * rpb * ( ( max * abs( k ) ) / half_w ) / max );
						for ( k = value; k < abs( value ); k ++ )
							*p ++ = lower + ( direction * rpb * ( ( max * abs( value ) ) / half_w ) / max ) + ( j * this->offset * 2 / lpb ) + ( j * this->offset / lpb );
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
					for ( k = 0; k < this->w; k ++ )
						*p ++ = lower + ( direction * ( rpb * ( ( k * max ) / this->w ) / max ) ) + ( j * this->offset * 2 / lpb );
				break;
		}
	}

	if ( this->quart )
	{
		this->w /= 2;
		this->h /= 2;
		for ( i = 1; i < this->h; i ++ )
		{
			p = image + i * this->w;
			r = image + i * 2 * this->w;
			j = this->w;
			while ( j -- > 0 )
				*p ++ = *r ++;
		}
	}

	if ( this->dmirror )
	{
		for ( i = 0; i < this->h; i ++ )
		{
			p = image + i * this->w;
			r = end - i * this->w;
			j = ( this->w * ( this->h - i ) ) / this->h;
			while ( j -- )
				*( -- r ) = *p ++;
		}
	}

	if ( this->flip )
	{
		uint16_t t;
		for ( i = 0; i < this->h; i ++ )
		{
			p = image + i * this->w;
			r = p + this->w;
			while( p != r )
			{
				t = *p;
				*p ++ = *( -- r );
				*r = t;
			}
		}
	}

	if ( this->flop )
	{
		uint16_t t;
		r = end;
		for ( i = 1; i < this->h / 2; i ++ )
		{
			p = image + i * this->w;
			j = this->w;
			while( j -- )
			{
				t = *( -- p );
				*p = *( -- r );
				*r = t;
			}
		}
	}

	if ( this->hmirror )
	{
		p = image;
		while ( p < end )
		{
			r = p + this->w;
			while ( p != r )
				*( -- r ) = *p ++;
			p += this->w / 2;
		}
	}

	if ( this->vmirror )
	{
		p = image;
		r = end;
		while ( p != r )
			*( -- r ) = *p ++;
	}

	if ( this->invert )
	{
		p = image;
		r = image;
		while ( p < end )
			*p ++ = max - *r ++;
	}

	if ( this->pflip )
	{
		uint16_t t;
		for ( i = 0; i < this->h; i ++ )
		{
			p = image + i * this->w;
			r = p + this->w;
			while( p != r )
			{
				t = *p;
				*p ++ = *( -- r );
				*r = t;
			}
		}
	}

	if ( this->pflop )
	{
		uint16_t t;
		end = image + this->w * this->h;
		r = end;
		for ( i = 1; i < this->h / 2; i ++ )
		{
			p = image + i * this->w;
			j = this->w;
			while( j -- )
			{
				t = *( -- p );
				*p = *( -- r );
				*r = t;
			}
		}
	}

	if ( this->rotate )
	{
		uint16_t *image2 = malloc( this->w * this->h * sizeof( uint16_t ) );
		for ( i = 0; i < this->h; i ++ )
		{
			p = image + i * this->w;
			r = image2 + this->h - i - 1;
			for ( j = 0; j < this->w; j ++ )
			{
				*r = *( p ++ );
				r += this->h;
			}
		}
		i = this->w;
		this->w = this->h;
		this->h = i;
		free( image );
		image = image2;
	}

	return image;
}

int main( int argc, char **argv )
{
	int arg = 1;
	int bpp = 8;

	luma this;
	uint16_t *image = NULL;

	luma_init( &this );

	for ( arg = 1; arg < argc - 1; arg ++ )
	{
		if ( !strcmp( argv[ arg ], "-bpp" ) )
			bpp = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-type" ) )
			this.type = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-w" ) )
		{
			int tmp = atoi( argv[ ++ arg ] );
			// TODO: is there an upper bound?
			if ( tmp )
				this.w = tmp;
			else
				return 1;
		}
		else if ( !strcmp( argv[ arg ], "-h" ) )
		{
			int tmp = atoi( argv[ ++ arg ] );
			// TODO: is there an upper bound?
			if ( tmp )
				this.h = tmp;
			else return 1;
		}
		else if ( !strcmp( argv[ arg ], "-bands" ) )
		{
			int tmp = atoi( argv[ ++ arg ] );
			// TODO: is there an upper bound?
			if ( tmp >= 0 )
				this.bands = tmp;
			else
				return 1;
		}
		else if ( !strcmp( argv[ arg ], "-rband" ) )
			this.rband = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-hmirror" ) )
			this.hmirror = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-vmirror" ) )
			this.vmirror = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-dmirror" ) )
			this.dmirror = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-offset" ) )
			this.offset = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-invert" ) )
			this.invert = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-flip" ) )
			this.flip = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-flop" ) )
			this.flop = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-pflip" ) )
			this.pflip = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-pflop" ) )
			this.pflop = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-quart" ) )
			this.quart = atoi( argv[ ++ arg ] );
		else if ( !strcmp( argv[ arg ], "-rotate" ) )
			this.rotate = atoi( argv[ ++ arg ] );
		else
			fprintf( stderr, "ignoring %s\n", argv[ arg ] );
	}

	if ( bpp != 8 && bpp != 16 )
	{
		fprintf( stderr, "Invalid bpp %d\n", bpp );
		return 1;
	}

	image = luma_render( &this );

	if ( bpp == 16 )
	{
		uint16_t *end = image + this.w * this.h;
		uint16_t *p = image;
		uint8_t *q = ( uint8_t * )image;
		while ( p < end )
		{
			*p ++ = ( *q << 8 ) + *( q + 1 );
			q += 2;
		}
		printf( "P5\n" );
		printf( "%d %d\n", this.w, this.h );
		printf( "65535\n" );
		fwrite( image, this.w * this.h * sizeof( uint16_t ), 1, stdout );
	}
	else
	{
		uint16_t *end = image + this.w * this.h;
		uint16_t *p = image;
		uint8_t *q = ( uint8_t * )image;
		while ( p < end )
			*q ++ = ( uint8_t )( *p ++ >> 8 );
		printf( "P5\n" );
		printf( "%d %d\n", this.w, this.h );
		printf( "255\n" );
		fwrite( image, this.w * this.h, 1, stdout );
	}

	return 0;
}

