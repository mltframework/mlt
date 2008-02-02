/*
 * filter_boxblur.c -- blur filter
 * Copyright (C) ?-2007 Leny Grisel <leny.grisel@laposte.net>
 * Copyright (C) 2007 Jean-Baptiste Mardelle <jb@ader.ch>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


static void PreCompute(uint8_t *yuv, int32_t *rgb, unsigned int width, unsigned int height)
{
	register int x, y, z;
	register int uneven = width % 2;
	int w = (width - uneven ) / 2;
	int yy, uu, vv;
	int r, g, b;
	int32_t pts[3];
	for (y=0; y<height; y++)
	{
		for (x=0; x<w; x++)
		{
			uu = yuv[1];
			vv = yuv[3];
			yy = yuv[0];
			YUV2RGB(yy, uu, vv, r, g, b);
			pts[0] = r;
			pts[1] = g;
			pts[2] = b;
			for (z = 0; z < 3; z++) 
			{
				if (x>0) pts[z]+=rgb[-3];
				if (y>0) pts[z]+=rgb[-(width*3)];
				if (x>0 && y>0) pts[z]-=rgb[-((width+1)*3)];
				*rgb++=pts[z];
            		}

			yy = yuv[2];
			YUV2RGB(yy, uu, vv, r, g, b);
			pts[0] = r;
			pts[1] = g;
			pts[2] = b;
			for (z = 0; z < 3; z++)
			{
				pts[z]+=rgb[-3];
				if (y>0)
				{
					pts[z]+=rgb[-(width*3)];
					pts[z]-=rgb[-((width+1)*3)];
				}
				*rgb++=pts[z];
			}
			yuv += 4;
		}
		if (uneven) 
		{
			uu = yuv[1];
			vv = yuv[3];
			yy = yuv[0];
			YUV2RGB(yy, uu, vv, r, g, b);
			pts[0] = r;
			pts[1] = g;
			pts[2] = b;
			for (z = 0; z < 3; z++)
			{
				pts[z]+=rgb[-3];
				if (y>0)
				{
					pts[z]+=rgb[-(width*3)];
					pts[z]-=rgb[-((width+1)*3)];
				}
				*rgb++=pts[z];
			}
			yuv += 2;
		}
	}
}

static int32_t GetRGB(int32_t *rgb, unsigned int w, unsigned int h, unsigned int x, int offsetx, unsigned int y, int offsety, unsigned int z)
{
	int xtheo = x * 2 + offsetx;
	int ytheo = y + offsety;
	if (xtheo < 0) xtheo = 0; else if (xtheo >= w) xtheo = w - 1;
	if (ytheo < 0) ytheo = 0; else if (ytheo >= h) ytheo = h - 1;
	return rgb[3*(xtheo+ytheo*w)+z];
}

static int32_t GetRGB2(int32_t *rgb, unsigned int w, unsigned int h, unsigned int x, int offsetx, unsigned int y, int offsety, unsigned int z)
{
	int xtheo = x * 2 + 1 + offsetx;
	int ytheo = y + offsety;
	if (xtheo < 0) xtheo = 0; else if (xtheo >= w) xtheo = w - 1;
	if (ytheo < 0) ytheo = 0; else if (ytheo >= h) ytheo = h - 1;
	return rgb[3*(xtheo+ytheo*w)+z];
}

static void DoBoxBlur(uint8_t *yuv, int32_t *rgb, unsigned int width, unsigned int height, unsigned int boxw, unsigned int boxh)
{
	register int x, y;
	int32_t r, g, b;
	register int uneven = width % 2;
	register int y0, y1, u0, u1, v0, v1;
	int w = (width - uneven ) / 2;
	float mul = 1.f / ((boxw*2) * (boxh*2));

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < w; x++)
		{
			r = GetRGB(rgb, width, height, x, +boxw, y, +boxh, 0) + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 0) - GetRGB(rgb, width, height, x, -boxw, y, + boxh, 0) - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 0);
			g = GetRGB(rgb, width, height, x, +boxw, y, +boxh, 1) + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 1) - GetRGB(rgb, width, height, x, -boxw, y, +boxh, 1) - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 1);
			b = GetRGB(rgb, width, height, x, +boxw, y, +boxh, 2) + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 2) - GetRGB(rgb, width, height, x, -boxw, y, +boxh, 2) - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 2);
            		r = (int32_t) (r * mul);
			g = (int32_t) (g * mul);
			b = (int32_t) (b * mul);
			RGB2YUV (r, g, b, y0, u0, v0);

			r = GetRGB2(rgb, width, height, x, +boxw, y, +boxh, 0) + GetRGB2(rgb, width, height, x, -boxw, y, -boxh, 0) - GetRGB2(rgb, width, height, x, -boxw, y, +boxh, 0) - GetRGB2(rgb, width, height, x, +boxw, y, -boxh, 0);
			g = GetRGB2(rgb, width, height, x, +boxw, y, +boxh, 1) + GetRGB2(rgb, width, height, x, -boxw, y, -boxh, 1) - GetRGB2(rgb, width, height, x, -boxw, y, +boxh, 1) - GetRGB2(rgb, width, height, x, +boxw, y, -boxh, 1);
			b = GetRGB2(rgb, width, height, x, +boxw, y, +boxh, 2) + GetRGB2(rgb, width, height, x, -boxw, y, -boxh, 2) - GetRGB2(rgb, width, height, x, -boxw, y, +boxh, 2) - GetRGB2(rgb, width, height, x, +boxw, y, -boxh, 2);
			r = (int32_t) (r * mul);
			g = (int32_t) (g * mul);
			b = (int32_t) (b * mul);
			RGB2YUV (r, g, b, y1, u1, v1);
			*yuv++ = y0;
			*yuv++ = (u0+u1) >> 1;
			*yuv++ = y1;
			*yuv++ = (v0+v1) >> 1;
		}
		if (uneven)
		{
			r =  GetRGB(rgb, width, height, x, +boxw, y, +boxh, 0) + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 0) - GetRGB(rgb, width, height, x, -boxw, y, +boxh, 0) - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 0);
			g =  GetRGB(rgb, width, height, x, +boxw, y, +boxh, 1) + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 1) - GetRGB(rgb, width, height, x, -boxw, y, +boxh, 1) - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 1);
			b =  GetRGB(rgb, width, height, x, +boxw, y, +boxh, 2) + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 2) - GetRGB(rgb, width, height, x, -boxw, y, +boxh, 2) - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 2);
			r = (int32_t) (r * mul);
			g = (int32_t) (g * mul);
			b = (int32_t) (b * mul);
			RGB2YUV (r, g, b, y0, u0, v0);
			*yuv++ = mul * y0;
			*yuv++ = mul * u0;
		}
	}
}

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the image
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );
	short hori = mlt_properties_get_int(MLT_FRAME_PROPERTIES( this ), "hori" );
	short vert = mlt_properties_get_int(MLT_FRAME_PROPERTIES( this ), "vert" );

	// Only process if we have no error and a valid colour space
	if ( error == 0 && *format == mlt_image_yuv422 )
	{
		double factor = mlt_properties_get_double( MLT_FRAME_PROPERTIES( this ), "boxblur" );
		if (factor != 0) {
			int h = *height + 1;
			int32_t *rgb = mlt_pool_alloc (3 * *width * h * sizeof(int32_t));
			PreCompute (*image, rgb, *width, h);
			DoBoxBlur (*image, rgb, *width, h, (int) factor*hori, (int) factor*vert);
			mlt_pool_release (rgb);
		}
    	}
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Get the starting blur level
	double blur = (double) mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "start" );
	short hori = mlt_properties_get_int(MLT_FILTER_PROPERTIES( this ), "hori" );
	short vert = mlt_properties_get_int(MLT_FILTER_PROPERTIES( this ), "vert" );

	// If there is an end adjust gain to the range
	if ( mlt_properties_get( MLT_FILTER_PROPERTIES( this ), "end" ) != NULL )
	{
		// Determine the time position of this frame in the transition duration
		mlt_position in = mlt_filter_get_in( this );
		mlt_position out = mlt_filter_get_out( this );
		mlt_position time = mlt_frame_get_position( frame );
		double position = (double) ( time - in ) / ( out - in + 1.0 );
		double end = (double) mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "end" );
		blur += ( end - blur ) * position;
	}

	// Push the frame filter
	mlt_properties_set_double( MLT_FRAME_PROPERTIES( frame ), "boxblur", blur );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "hori", hori );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "vert", vert );
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_boxblur_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "start", arg == NULL ? "10" : arg);
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "hori", arg == NULL ? "1" : arg);
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "vert", arg == NULL ? "1" : arg);
	}
	return this;
}



