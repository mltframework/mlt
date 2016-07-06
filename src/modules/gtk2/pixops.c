/* GdkPixbuf library - Scaling and compositing functions
 *
 * Original:
 * Copyright (C) 2000 Red Hat, Inc
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 * Modification for MLT:
 * Copyright (C) 2003-2014 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc.,  51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <math.h>
#include <glib.h>
#include <stdio.h>

#include "pixops.h"

#define SUBSAMPLE_BITS 4
#define SUBSAMPLE (1 << SUBSAMPLE_BITS)
#define SUBSAMPLE_MASK ((1 << SUBSAMPLE_BITS)-1)
#define SCALE_SHIFT 16

typedef struct _PixopsFilter PixopsFilter;
typedef struct _PixopsFilterDimension PixopsFilterDimension;

struct _PixopsFilterDimension
{
	int n;
	double offset;
	double *weights;
};

struct _PixopsFilter
{
	PixopsFilterDimension x;
	PixopsFilterDimension y;
	double overall_alpha;
};

typedef guchar *( *PixopsLineFunc ) ( int *weights, int n_x, int n_y,
                                      guchar *dest, int dest_x, guchar *dest_end,
                                      guchar **src,
                                      int x_init, int x_step, int src_width );

typedef void ( *PixopsPixelFunc ) ( guchar *dest, guint y1, guint cr, guint y2, guint cb );


/* mmx function declarations */
#if defined(USE_MMX) && !defined(ARCH_X86_64)
guchar *pixops_scale_line_22_yuv_mmx ( guint32 weights[ 16 ][ 8 ], guchar *p, guchar *q1, guchar *q2, int x_step, guchar *p_stop, int x_init, int destx );
int _pixops_have_mmx ( void );
#endif

static inline int
get_check_shift ( int check_size )
{
	int check_shift = 0;
	g_return_val_if_fail ( check_size >= 0, 4 );

	while ( !( check_size & 1 ) )
	{
		check_shift++;
		check_size >>= 1;
	}

	return check_shift;
}

static inline void
pixops_scale_nearest ( guchar *dest_buf,
                       int render_x0,
                       int render_y0,
                       int render_x1,
                       int render_y1,
                       int dest_rowstride,
                       const guchar *src_buf,
                       int src_width,
                       int src_height,
                       int src_rowstride,
                       double scale_x,
                       double scale_y )
{
	register int i, j;
	register int x_step = ( 1 << SCALE_SHIFT ) / scale_x;
	register int y_step = ( 1 << SCALE_SHIFT ) / scale_y;
	register int x, x_scaled;

	for ( i = 0; i < ( render_y1 - render_y0 ); i++ )
	{
		const guchar *src = src_buf + ( ( ( i + render_y0 ) * y_step + ( y_step >> 1 ) ) >> SCALE_SHIFT ) * src_rowstride;
		guchar *dest = dest_buf + i * dest_rowstride;
		x = render_x0 * x_step + ( x_step >> 1 );

		for ( j = 0; j < ( render_x1 - render_x0 ); j++ )
		{
			x_scaled = x >> SCALE_SHIFT;
			*dest++ = src[ x_scaled << 1 ];
			*dest++ = src[ ( ( x_scaled >> 1 ) << 2 ) + ( ( j & 1 ) << 1 ) + 1 ];
			x += x_step;
		}
	}
}


static inline guchar *
scale_line ( int *weights, int n_x, int n_y,
             guchar *dest, int dest_x, guchar *dest_end,
             guchar **src,
             int x_init, int x_step, int src_width )
{
	register int x = x_init;
	register int i, j, x_scaled, y_index, uv_index;

	while ( dest < dest_end )
	{
		unsigned int y = 0, uv = 0;
		int *pixel_weights = weights + ( ( x >> ( SCALE_SHIFT - SUBSAMPLE_BITS ) ) & SUBSAMPLE_MASK ) * n_x * n_y;

		x_scaled = x >> SCALE_SHIFT;
		y_index = x_scaled << 1;
		uv_index = ( ( x_scaled >> 1 ) << 2 ) + ( ( dest_x & 1 ) << 1 ) + 1;

		for ( i = 0; i < n_y; i++ )
		{
			int *line_weights = pixel_weights + n_x * i;
			guchar *q = src[ i ];

			for ( j = 0; j < n_x; j ++ )
			{
				unsigned int ta = line_weights[ j ];

				y  += ta * q[ y_index ];
				uv += ta * q[ uv_index ];
			}
		}

		*dest++ = ( y  + 0xffff ) >> SCALE_SHIFT;
		*dest++ = ( uv + 0xffff ) >> SCALE_SHIFT;

		x += x_step;
		dest_x++;
	}

	return dest;
}

#if defined(USE_MMX) && !defined(ARCH_X86_64)
static inline guchar *
scale_line_22_yuv_mmx_stub ( int *weights, int n_x, int n_y,
                            guchar *dest, int dest_x, guchar *dest_end,
                            guchar **src,
                            int x_init, int x_step, int src_width )
{
	guint32 mmx_weights[ 16 ][ 8 ];
	int j;

	for ( j = 0; j < 16; j++ )
	{
		mmx_weights[ j ][ 0 ] = 0x00010001 * ( weights[ 4 * j ] >> 8 );
		mmx_weights[ j ][ 1 ] = 0x00010001 * ( weights[ 4 * j ] >> 8 );
		mmx_weights[ j ][ 2 ] = 0x00010001 * ( weights[ 4 * j + 1 ] >> 8 );
		mmx_weights[ j ][ 3 ] = 0x00010001 * ( weights[ 4 * j + 1 ] >> 8 );
		mmx_weights[ j ][ 4 ] = 0x00010001 * ( weights[ 4 * j + 2 ] >> 8 );
		mmx_weights[ j ][ 5 ] = 0x00010001 * ( weights[ 4 * j + 2 ] >> 8 );
		mmx_weights[ j ][ 6 ] = 0x00010001 * ( weights[ 4 * j + 3 ] >> 8 );
		mmx_weights[ j ][ 7 ] = 0x00010001 * ( weights[ 4 * j + 3 ] >> 8 );
	}

	return pixops_scale_line_22_yuv_mmx ( mmx_weights, dest, src[ 0 ], src[ 1 ], x_step, dest_end, x_init, dest_x );
}
#endif /* USE_MMX */

static inline guchar *
scale_line_22_yuv ( int *weights, int n_x, int n_y,
                   guchar *dest, int dest_x, guchar *dest_end,
                   guchar **src,
                   int x_init, int x_step, int src_width )
{
	register int x = x_init;
	register guchar *src0 = src[ 0 ];
	register guchar *src1 = src[ 1 ];
	register unsigned int p;
	register guchar *q0, *q1;
	register int w1, w2, w3, w4;
	register int x_scaled, x_aligned, uv_index;

	while ( dest < dest_end )
	{
		int *pixel_weights = weights + ( ( x >> ( SCALE_SHIFT - SUBSAMPLE_BITS ) ) & SUBSAMPLE_MASK ) * 4;

		x_scaled = x >> SCALE_SHIFT;

		w1 = pixel_weights[ 0 ];
		w2 = pixel_weights[ 1 ];
		w3 = pixel_weights[ 2 ];
		w4 = pixel_weights[ 3 ];

		/* process Y */
		q0 = src0 + ( x_scaled << 1 );
		q1 = src1 + ( x_scaled << 1 );
		p  = w1 * q0[ 0 ];
		p += w2 * q0[ 2 ];
		p += w3 * q1[ 0 ];
		p += w4 * q1[ 2 ];
		*dest++ = ( p + 0x8000 ) >> SCALE_SHIFT;

		/* process U/V */
		x_aligned = ( ( x_scaled >> 1 ) << 2 );
		uv_index = ( ( dest_x & 1 ) << 1 ) + 1;

		q0 = src0 + x_aligned;
		q1 = src1 + x_aligned;
		p  = w1 * q0[ uv_index ];
		p += w3 * q1[ uv_index ];
		p += w2 * q0[ uv_index ];
		p += w4 * q1[ uv_index ];

		x += x_step;
		dest_x ++;

		*dest++ = ( p + 0x8000 ) >> SCALE_SHIFT;
	}

	return dest;
}


static inline void
process_pixel ( int *weights, int n_x, int n_y,
                guchar *dest, int dest_x, int dest_channels,
                guchar **src, int src_channels,
                int x_start, int src_width )
{
	register unsigned int y = 0, uv = 0;
	register int i, j;
	int uv_index = ( ( dest_x & 1 ) << 1 ) + 1;

	for ( i = 0; i < n_y; i++ )
	{
		int *line_weights = weights + n_x * i;

		for ( j = 0; j < n_x; j++ )
		{
			unsigned int ta = 0xff * line_weights[ j ];

			if ( x_start + j < 0 )
			{
				y  += ta * src[ i ][ 0 ];
				uv += ta * src[ i ][ uv_index ];
			}
			else if ( x_start + j < src_width )
			{
				y  += ta * src[ i ][ ( x_start + j ) << 1 ];
				uv += ta * src[ i ][ ( ( ( x_start + j ) >> 1 ) << 2) + uv_index ];
			}
			else
			{
				y  += ta * src[ i ][ ( src_width - 1 ) << 1 ];
				uv += ta * src[ i ][ ( ( ( src_width - 1 ) >> 1 ) << 2) + uv_index ];
			}
		}
	}

	*dest++ = ( y  + 0xffffff ) >> 24;
	*dest++ = ( uv + 0xffffff ) >> 24;
}


static inline void
correct_total ( int *weights,
                int n_x,
                int n_y,
                int total,
                double overall_alpha )
{
	int correction = ( int ) ( 0.5 + 65536 * overall_alpha ) - total;
	int remaining, c, d, i;

	if ( correction != 0 )
	{
		remaining = correction;
		for ( d = 1, c = correction; c != 0 && remaining != 0; d++, c = correction / d )
			for ( i = n_x * n_y - 1; i >= 0 && c != 0 && remaining != 0; i-- )
				if ( *( weights + i ) + c >= 0 )
				{
					*( weights + i ) += c;
					remaining -= c;
					if ( ( 0 < remaining && remaining < c ) ||
					        ( 0 > remaining && remaining > c ) )
						c = remaining;
				}
	}
}


static inline int *
make_filter_table ( PixopsFilter *filter )
{
	int i_offset, j_offset;
	int n_x = filter->x.n;
	int n_y = filter->y.n;
	int *weights = g_new ( int, SUBSAMPLE * SUBSAMPLE * n_x * n_y );

	for ( i_offset = 0; i_offset < SUBSAMPLE; i_offset++ )
		for ( j_offset = 0; j_offset < SUBSAMPLE; j_offset++ )
		{
			double weight;
			int *pixel_weights = weights + ( ( i_offset * SUBSAMPLE ) + j_offset ) * n_x * n_y;
			int total = 0;
			int i, j;

			for ( i = 0; i < n_y; i++ )
				for ( j = 0; j < n_x; j++ )
				{
					weight = filter->x.weights[ ( j_offset * n_x ) + j ] *
					         filter->y.weights[ ( i_offset * n_y ) + i ] *
					         filter->overall_alpha * 65536 + 0.5;

					total += ( int ) weight;

					*( pixel_weights + n_x * i + j ) = weight;
				}

			correct_total ( pixel_weights, n_x, n_y, total, filter->overall_alpha );
		}

	return weights;
}


static inline void
pixops_process ( guchar *dest_buf,
                 int render_x0,
                 int render_y0,
                 int render_x1,
                 int render_y1,
                 int dest_rowstride,
                 int dest_channels,
                 gboolean dest_has_alpha,
                 const guchar *src_buf,
                 int src_width,
                 int src_height,
                 int src_rowstride,
                 int src_channels,
                 gboolean src_has_alpha,
                 double scale_x,
                 double scale_y,
                 int check_x,
                 int check_y,
                 int check_size,
                 guint32 color1,
                 guint32 color2,
                 PixopsFilter *filter,
                 PixopsLineFunc line_func )
{
	int i, j;
	int x, y;			/* X and Y position in source (fixed_point) */

	guchar **line_bufs = g_new ( guchar *, filter->y.n );
	int *filter_weights = make_filter_table ( filter );

	int x_step = ( 1 << SCALE_SHIFT ) / scale_x; /* X step in source (fixed point) */
	int y_step = ( 1 << SCALE_SHIFT ) / scale_y; /* Y step in source (fixed point) */

	int check_shift = check_size ? get_check_shift ( check_size ) : 0;

	int scaled_x_offset = floor ( filter->x.offset * ( 1 << SCALE_SHIFT ) );

	/* Compute the index where we run off the end of the source buffer. The furthest
	 * source pixel we access at index i is:
	 *
	 *  ((render_x0 + i) * x_step + scaled_x_offset) >> SCALE_SHIFT + filter->x.n - 1
	 *
	 * So, run_end_index is the smallest i for which this pixel is src_width, i.e, for which:
	 *
	 *  (i + render_x0) * x_step >= ((src_width - filter->x.n + 1) << SCALE_SHIFT) - scaled_x_offset
	 *
	 */
#define MYDIV(a,b) ((a) > 0 ? (a) / (b) : ((a) - (b) + 1) / (b))    /* Division so that -1/5 = -1 */

	int run_end_x = ( ( ( src_width - filter->x.n + 1 ) << SCALE_SHIFT ) - scaled_x_offset );
	int run_end_index = MYDIV ( run_end_x + x_step - 1, x_step ) - render_x0;
	run_end_index = MIN ( run_end_index, render_x1 - render_x0 );

	y = render_y0 * y_step + floor ( filter->y.offset * ( 1 << SCALE_SHIFT ) );
	for ( i = 0; i < ( render_y1 - render_y0 ); i++ )
	{
		int dest_x;
		int y_start = y >> SCALE_SHIFT;
		int x_start;
		int *run_weights = filter_weights +
		                   ( ( y >> ( SCALE_SHIFT - SUBSAMPLE_BITS ) ) & SUBSAMPLE_MASK ) *
		                   filter->x.n * filter->y.n * SUBSAMPLE;
		guchar *new_outbuf;
		guint32 tcolor1, tcolor2;

		guchar *outbuf = dest_buf + dest_rowstride * i;
		guchar *outbuf_end = outbuf + dest_channels * ( render_x1 - render_x0 );

		if ( ( ( i + check_y ) >> check_shift ) & 1 )
		{
			tcolor1 = color2;
			tcolor2 = color1;
		}
		else
		{
			tcolor1 = color1;
			tcolor2 = color2;
		}

		for ( j = 0; j < filter->y.n; j++ )
		{
			if ( y_start < 0 )
				line_bufs[ j ] = ( guchar * ) src_buf;
			else if ( y_start < src_height )
				line_bufs[ j ] = ( guchar * ) src_buf + src_rowstride * y_start;
			else
				line_bufs[ j ] = ( guchar * ) src_buf + src_rowstride * ( src_height - 1 );

			y_start++;
		}

		dest_x = check_x;
		x = render_x0 * x_step + scaled_x_offset;
		x_start = x >> SCALE_SHIFT;

		while ( x_start < 0 && outbuf < outbuf_end )
		{
			process_pixel ( run_weights + ( ( x >> ( SCALE_SHIFT - SUBSAMPLE_BITS ) ) & SUBSAMPLE_MASK ) * ( filter->x.n * filter->y.n ),
			                filter->x.n, filter->y.n,
			                outbuf, dest_x, dest_channels,
			                line_bufs, src_channels,
			                x >> SCALE_SHIFT, src_width );

			x += x_step;
			x_start = x >> SCALE_SHIFT;
			dest_x++;
			outbuf += dest_channels;
		}

		new_outbuf = ( *line_func ) ( run_weights, filter->x.n, filter->y.n,
		                              outbuf, dest_x,
		                              dest_buf + dest_rowstride * i + run_end_index * dest_channels,
		                              line_bufs,
		                              x, x_step, src_width );

		dest_x += ( new_outbuf - outbuf ) / dest_channels;

		x = ( dest_x - check_x + render_x0 ) * x_step + scaled_x_offset;
		outbuf = new_outbuf;

		while ( outbuf < outbuf_end )
		{
			process_pixel ( run_weights + ( ( x >> ( SCALE_SHIFT - SUBSAMPLE_BITS ) ) & SUBSAMPLE_MASK ) * ( filter->x.n * filter->y.n ),
			                filter->x.n, filter->y.n,
			                outbuf, dest_x, dest_channels,
			                line_bufs, src_channels,
			                x >> SCALE_SHIFT, src_width );

			x += x_step;
			dest_x++;
			outbuf += dest_channels;
		}

		y += y_step;
	}

	g_free ( line_bufs );
	g_free ( filter_weights );
}


/* Compute weights for reconstruction by replication followed by
 * sampling with a box filter
 */
static inline void
tile_make_weights ( PixopsFilterDimension *dim,
                    double scale )
{
	int n = ceil ( 1 / scale + 1 );
	double *pixel_weights = g_new ( double, SUBSAMPLE * n );
	int offset;
	int i;

	dim->n = n;
	dim->offset = 0;
	dim->weights = pixel_weights;

	for ( offset = 0; offset < SUBSAMPLE; offset++ )
	{
		double x = ( double ) offset / SUBSAMPLE;
		double a = x + 1 / scale;

		for ( i = 0; i < n; i++ )
		{
			if ( i < x )
			{
				if ( i + 1 > x )
					* ( pixel_weights++ ) = ( MIN ( i + 1, a ) - x ) * scale;
				else
					*( pixel_weights++ ) = 0;
			}
			else
			{
				if ( a > i )
					* ( pixel_weights++ ) = ( MIN ( i + 1, a ) - i ) * scale;
				else
					*( pixel_weights++ ) = 0;
			}
		}
	}
}

/* Compute weights for a filter that, for minification
 * is the same as 'tiles', and for magnification, is bilinear
 * reconstruction followed by a sampling with a delta function.
 */
static inline void
bilinear_magnify_make_weights ( PixopsFilterDimension *dim,
                                double scale )
{
	double * pixel_weights;
	int n;
	int offset;
	int i;

	if ( scale > 1.0 )              /* Linear */
	{
		n = 2;
		dim->offset = 0.5 * ( 1 / scale - 1 );
	}
	else                          /* Tile */
	{
		n = ceil ( 1.0 + 1.0 / scale );
		dim->offset = 0.0;
	}

	dim->n = n;
	dim->weights = g_new ( double, SUBSAMPLE * n );

	pixel_weights = dim->weights;

	for ( offset = 0; offset < SUBSAMPLE; offset++ )
	{
		double x = ( double ) offset / SUBSAMPLE;

		if ( scale > 1.0 )        /* Linear */
		{
			for ( i = 0; i < n; i++ )
				*( pixel_weights++ ) = ( ( ( i == 0 ) ? ( 1 - x ) : x ) / scale ) * scale;
		}
		else                  /* Tile */
		{
			double a = x + 1 / scale;

			/*           x
			 * ---------|--.-|----|--.-|-------  SRC
			 * ------------|---------|---------  DEST
			 */
			for ( i = 0; i < n; i++ )
			{
				if ( i < x )
				{
					if ( i + 1 > x )
						* ( pixel_weights++ ) = ( MIN ( i + 1, a ) - x ) * scale;
					else
						*( pixel_weights++ ) = 0;
				}
				else
				{
					if ( a > i )
						* ( pixel_weights++ ) = ( MIN ( i + 1, a ) - i ) * scale;
					else
						*( pixel_weights++ ) = 0;
				}
			}
		}
	}
}

/* Computes the integral from b0 to b1 of
 *
 * f(x) = x; 0 <= x < 1
 * f(x) = 0; otherwise
 *
 * We combine two of these to compute the convolution of
 * a box filter with a triangular spike.
 */
static inline double
linear_box_half ( double b0, double b1 )
{
	double a0, a1;
	double x0, x1;

	a0 = 0.;
	a1 = 1.;

	if ( a0 < b0 )
	{
		if ( a1 > b0 )
		{
			x0 = b0;
			x1 = MIN ( a1, b1 );
		}
		else
			return 0;
	}
	else
	{
		if ( b1 > a0 )
		{
			x0 = a0;
			x1 = MIN ( a1, b1 );
		}
		else
			return 0;
	}

	return 0.5 * ( x1 * x1 - x0 * x0 );
}

/* Compute weights for reconstructing with bilinear
 * interpolation, then sampling with a box filter
 */
static inline void
bilinear_box_make_weights ( PixopsFilterDimension *dim,
                            double scale )
{
	int n = ceil ( 1 / scale + 2.0 );
	double *pixel_weights = g_new ( double, SUBSAMPLE * n );
	double w;
	int offset, i;

	dim->offset = -1.0;
	dim->n = n;
	dim->weights = pixel_weights;

	for ( offset = 0 ; offset < SUBSAMPLE; offset++ )
	{
		double x = ( double ) offset / SUBSAMPLE;
		double a = x + 1 / scale;

		for ( i = 0; i < n; i++ )
		{
			w = linear_box_half ( 0.5 + i - a, 0.5 + i - x );
			w += linear_box_half ( 1.5 + x - i, 1.5 + a - i );

			*( pixel_weights++ ) = w * scale;
		}
	}
}


static inline void
make_weights ( PixopsFilter *filter,
               PixopsInterpType interp_type,
               double scale_x,
               double scale_y )
{
	switch ( interp_type )
	{
	case PIXOPS_INTERP_NEAREST:
		g_assert_not_reached ();
		break;

	case PIXOPS_INTERP_TILES:
		tile_make_weights ( &filter->x, scale_x );
		tile_make_weights ( &filter->y, scale_y );
		break;

	case PIXOPS_INTERP_BILINEAR:
		bilinear_magnify_make_weights ( &filter->x, scale_x );
		bilinear_magnify_make_weights ( &filter->y, scale_y );
		break;

	case PIXOPS_INTERP_HYPER:
		bilinear_box_make_weights ( &filter->x, scale_x );
		bilinear_box_make_weights ( &filter->y, scale_y );
		break;
	}
}


void
yuv422_scale ( guchar *dest_buf,
               int render_x0,
               int render_y0,
               int render_x1,
               int render_y1,
               int dest_rowstride,
               int dest_channels,
               gboolean dest_has_alpha,
               const guchar *src_buf,
               int src_width,
               int src_height,
               int src_rowstride,
               int src_channels,
               gboolean src_has_alpha,
               double scale_x,
               double scale_y,
               PixopsInterpType interp_type )
{
	PixopsFilter filter = { { 0, 0, 0}, { 0, 0, 0 }, 0 };
	PixopsLineFunc line_func;

#if defined(USE_MMX) && !defined(ARCH_X86_64)
	gboolean found_mmx = _pixops_have_mmx();
#endif

	//g_return_if_fail ( !( dest_channels == 3 && dest_has_alpha ) );
	//g_return_if_fail ( !( src_channels == 3 && src_has_alpha ) );
	//g_return_if_fail ( !( src_has_alpha && !dest_has_alpha ) );

	if ( scale_x == 0 || scale_y == 0 )
		return ;

	if ( interp_type == PIXOPS_INTERP_NEAREST )
	{
		pixops_scale_nearest ( dest_buf, render_x0, render_y0, render_x1, render_y1,
		                       dest_rowstride,
		                       src_buf, src_width, src_height, src_rowstride,
		                       scale_x, scale_y );
		return;
	}

	filter.overall_alpha = 1.0;
	make_weights ( &filter, interp_type, scale_x, scale_y );

	if ( filter.x.n == 2 && filter.y.n == 2 )
	{
#if defined(USE_MMX) && !defined(ARCH_X86_64)
		if ( found_mmx )
		{
			//fprintf( stderr, "rescale: using mmx\n" );
			line_func = scale_line_22_yuv_mmx_stub;
		}
		else
#endif

			line_func = scale_line_22_yuv;
	}
	else
		line_func = scale_line;

	pixops_process ( dest_buf, render_x0, render_y0, render_x1, render_y1,
	                 dest_rowstride, dest_channels, dest_has_alpha,
	                 src_buf, src_width, src_height, src_rowstride, src_channels,
	                 src_has_alpha, scale_x, scale_y, 0, 0, 0, 0, 0,
	                 &filter, line_func );

	g_free ( filter.x.weights );
	g_free ( filter.y.weights );
}

