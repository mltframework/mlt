/*
 *	/brief Draw arrows
 *	/author Zachary Drew, Copyright 2004
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

#include <framework/mlt_frame.h>
#include "arrow_code.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define ROUNDED_DIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))
#define ABS(a) ((a) >= 0 ? (a) : (-(a)))


static int w;
static int h;
static int xstride;
static int ystride;
static mlt_image_format format;

int init_arrows( mlt_image_format *image_format, int width, int height )
{
	w = width;
	h = height;
	format = *image_format;
	switch( *image_format ) {
		case mlt_image_yuv422:
			xstride = 2;
			ystride = xstride * w;
			break; 
		default:
			// I don't know
			return 0;
	}
	return 1;

}

// ffmpeg borrowed
static inline int clip(int a, int amin, int amax)
{
    if (a < amin)
        return amin;
    else if (a > amax)
        return amax;
    else
        return a;
}


/**
 * draws an line from (ex, ey) -> (sx, sy).
 * Credits: modified from ffmpeg project
 * @param ystride stride/linesize of the image
 * @param xstride stride/element size of the image
 * @param color color of the arrow
 */
void draw_line(uint8_t *buf, int sx, int sy, int ex, int ey, int color)
{
    int t, x, y, fr, f;


    sx= clip(sx, 0, w-1);
    sy= clip(sy, 0, h-1);
    ex= clip(ex, 0, w-1);
    ey= clip(ey, 0, h-1);

    buf[sy*ystride + sx*xstride]+= color;

    if(ABS(ex - sx) > ABS(ey - sy)){
        if(sx > ex){
            t=sx; sx=ex; ex=t;
            t=sy; sy=ey; ey=t;
        }
        buf+= sx*xstride + sy*ystride;
        ex-= sx;
        f= ((ey-sy)<<16)/ex;
        for(x= 0; x <= ex; x++){
            y = (x*f)>>16;
            fr= (x*f)&0xFFFF;
            buf[ y   *ystride + x*xstride]+= (color*(0x10000-fr))>>16;
            buf[(y+1)*ystride + x*xstride]+= (color*         fr )>>16;
        }
    }else{
        if(sy > ey){
            t=sx; sx=ex; ex=t;
            t=sy; sy=ey; ey=t;
        }
        buf+= sx*xstride + sy*ystride;
        ey-= sy;
        if(ey) f= ((ex-sx)<<16)/ey;
        else   f= 0;
        for(y= 0; y <= ey; y++){
            x = (y*f)>>16;
            fr= (y*f)&0xFFFF;
            buf[y*ystride + x    *xstride]+= (color*(0x10000-fr))>>16;;
            buf[y*ystride + (x+1)*xstride]+= (color*         fr )>>16;;
        }
    }
}

void draw_rectangle_fill(uint8_t *buf, int x, int y, int w, int h, int color)
{
	int i,j;
	for ( i = 0; i < w; i++ ) 
		for ( j = 0; j < h; j++ )
			buf[ (y+j)*ystride + (x+i)*xstride] = color; 
}

void draw_rectangle_outline(uint8_t *buf, int x, int y, int w, int h, int color)
{
	int i,j;
	for ( i = 0; i < w; i++ ) {
		buf[ y*ystride + (x+i)*xstride ] += color; 
		buf[ (y+h)*ystride + (x+i)*xstride ] += color; 
	}
	for ( j = 1; j < h+1; j++ ) {
		buf[ (y+j)*ystride + x*xstride ] += color;
		buf[ (y+j)*ystride + (x+w)*xstride ] += color; 
	}
}
/**
 * draws an arrow from (ex, ey) -> (sx, sy).
 * Credits: modified from ffmpeg project
 * @param stride stride/linesize of the image
 * @param color color of the arrow
 */
void draw_arrow(uint8_t *buf, int sx, int sy, int ex, int ey, int color){

	int dx,dy;
	dx= ex - sx;
	dy= ey - sy;

	if(dx*dx + dy*dy > 3*3){
		int rx=  dx + dy;
		int ry= -dx + dy;
		int length= sqrt((rx*rx + ry*ry)<<8);

		rx= ROUNDED_DIV(rx*3<<4, length);
		ry= ROUNDED_DIV(ry*3<<4, length);

		draw_line(buf, sx, sy, sx + rx, sy + ry, color);
		draw_line(buf, sx, sy, sx - ry, sy + rx, color);
	}
	draw_line(buf, sx, sy, ex, ey, color);
}
