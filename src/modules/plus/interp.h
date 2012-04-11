//interp.c
/*
 * Copyright (C) 2010 Marko Cebokli   http://lea.hamradio.si/~s57uuu
 * This file is a part of the Frei0r plugin "c0rners"
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*******************************************************************
 * The remapping functions use a map aray, which contains a pair
 * of floating values fo each pixel of the output image. These
 * represent the location in the input image, from where the value
 * of the given output pixel should be interpolated.
 * They are given in pixels of the input image.
 * If the output image is wo pixels wide, then the x coordinate
 * of the pixel in row r and column c is at  2*(r*wo+c) in the map
 * array, and y at 2*(r*wo+c)+1
 *
 * The map array is usually computation intensive to generate, and
 * he purpose of the map array is to allow fast remapping of
 * several images (video) using the same map array.
 ******************************************************************/


//compile:   gcc -c -O2 -Wall -std=c99 -fPIC interp.c -o interp.o

//	-std=c99 za rintf()
//	-fPIC da lahko linkas v .so (za frei0r)

#include <math.h>
#include <stdio.h>	/* za debug printoute */
#include <inttypes.h>


//#define TEST_XY_LIMITS

//--------------------------------------------------------
//pointer to an interpolating function
//parameters:
//  source image
//  source width
//  source height
//  X coordinate
//  Y coordinate
//  opacity
//  destination image
//  flag to overwrite alpha channel
typedef int (*interpp)(unsigned char*, int, int, float, float, float, unsigned char*, int);

//**************************************
//HERE BEGIN THE INTERPOLATION FUNCTIONS

//------------------------------------------------------
//za debugging - z izpisovanjem
//interpolacija "najblizji sosed" (ni prava interpolacija)
//za byte (char) vrednosti
//	*sl vhodni array (slika)
//	w,h dimenzija slike je wxh
//	x,y tocka, za katero izracuna interpolirano vrednost
//  o opacity
//	*v interpolirana vrednost
int interpNNpr_b(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	//printf("u=%5.2f v=%5.2f   ",x,y);
	printf("u=%5.3f v=%5.3f     ",x/(w-1),y/(h-1));
	//printf("U=%2d V=%2d   ",(int)rintf(x),(int)rintf(y));

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	*v=sl[(int)rintf(x)+(int)rintf(y)*w];
	return 0;
}

//------------------------------------------------------
//interpolacija "najblizji sosed" (ni prava interpolacija)
//za byte (char) vrednosti
//	*sl vhodni array (slika)
//	w,h dimenzija slike je wxh
//	x,y tocka, za katero izracuna interpolirano vrednost
//  o opacity
//	*v interpolirana vrednost
int interpNN_b(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	*v=sl[(int)rintf(x)+(int)rintf(y)*w];
	return 0;
}

//------------------------------------------------------
//interpolacija "najblizji sosed" (ni prava interpolacija)
//za byte (char) vrednosti  v packed color 32 bitnem formatu
//little endian !!
//	*sl vhodni array (slika)
//	w,h dimenzija slike je wxh
//	x,y tocka, za katero izracuna interpolirano vrednost
//  o opacity
//	*v interpolirana vrednost
int interpNN_b32(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif
	int p = (int) rintf(x) * 4 + (int) rintf(y) * 4 * w;
	float alpha = (float) sl[p + 3] / 255.0 * o;
	v[0]= v[0] * (1.0 - alpha) + sl[p] * alpha;
	v[1]= v[1] * (1.0 - alpha) + sl[p + 1] * alpha;
	v[2]= v[2] * (1.0 - alpha) + sl[p + 2] * alpha;
	if (is_alpha) v[3] = sl[p +3];

	return 0;
}

//------------------------------------------------------
//bilinearna interpolacija
//za byte (char) vrednosti
//	*sl vhodni array (slika)
//	w,h dimenzija slike je wxh
//	x,y tocka, za katero izracuna interpolirano vrednost
//  o opacity
//	*v interpolirana vrednost
int interpBL_b(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int m,n,k,l;
	float a,b;

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)floorf(x); n=(int)floorf(y);
	k=n*w+m; l=(n+1)*w+m;
	a=sl[k]+(sl[k+1]-sl[k])*(x-(float)m);
	b=sl[l]+(sl[l+1]-sl[l])*(x-(float)m);
	*v=a+(b-a)*(y-(float)n);
	return 0;
}

//------------------------------------------------------
//bilinearna interpolacija
//za byte (char) vrednosti  v packed color 32 bitnem formatu
int interpBL_b32(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int m,n,k,l,n1,l1,k1;
	float a,b;

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)floorf(x); n=(int)floorf(y);
	k=n*w+m; l=(n+1)*w+m;
	k1=4*(k+1); l1=4*(l+1); n1=4*((n+1)*w+m);
	l=4*l; k=4*k;

	a=sl[k+3]+(sl[k1+3]-sl[k+3])*(x-(float)m);
	b=sl[l+3]+(sl[l1+3]-sl[n1+3])*(x-(float)m);
	float alpha = a+(b-a)*(y-(float)n);
	if (is_alpha) v[3] = alpha;
	alpha = alpha / 255.0 * o;

	a=sl[k]+(sl[k1]-sl[k])*(x-(float)m);
	b=sl[l]+(sl[l1]-sl[n1])*(x-(float)m);
	v[0]= v[0] * (1.0 - alpha) + (a+(b-a)*(y-(float)n)) * alpha;

	a=sl[k+1]+(sl[k1+1]-sl[k+1])*(x-(float)m);
	b=sl[l+1]+(sl[l1+1]-sl[n1+1])*(x-(float)m);
	v[1]= v[1] * (1.0 - alpha) + (a+(b-a)*(y-(float)n)) * alpha;

	a=sl[k+2]+(sl[k1+2]-sl[k+2])*(x-(float)m);
	b=sl[l+2]+(sl[l1+2]-sl[n1+2])*(x-(float)m);
	v[2]= v[2] * (1.0 - alpha) + (a+(b-a)*(y-(float)n)) * alpha;

	return 0;
}

//------------------------------------------------------
//bikubicna interpolacija  "smooth"
//za byte (char) vrednosti
//kar Aitken-Neville formula iz Bronstajna
//	*sl vhodni array (slika)
//	w,h dimenzija slike je wxh
//	x,y tocka, za katero izracuna interpolirano vrednost
//  o opacity
//	*v interpolirana vrednost
int interpBC_b(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int i,j,l,m,n;
	float k;
	float p[4],p1[4],p2[4],p3[4],p4[4];

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)ceilf(x)-2; if (m<0) m=0; if ((m+5)>w) m=w-4;
	n=(int)ceilf(y)-2; if (n<0) n=0; if ((n+5)>h) n=h-4;

	//njaprej po y  (stiri stolpce)
	for (i=0;i<4;i++)
	{
		l=m+(i+n)*w;
		p1[i]=sl[l];
		p2[i]=sl[l+1];
		p3[i]=sl[l+2];
		p4[i]=sl[l+3];
	}
	for (j=1;j<4;j++)
		for (i=3;i>=j;i--)
		{
		k=(y-i-n)/j;
		p1[i]=p1[i]+k*(p1[i]-p1[i-1]);
		p2[i]=p2[i]+k*(p2[i]-p2[i-1]);
		p3[i]=p3[i]+k*(p3[i]-p3[i-1]);
		p4[i]=p4[i]+k*(p4[i]-p4[i-1]);
	}

	//zdaj pa po x
	p[0]=p1[3]; p[1]=p2[3]; p[2]=p3[3]; p[3]=p4[3];
	for (j=1;j<4;j++)
		for (i=3;i>=j;i--)
			p[i]=p[i]+(x-i-m)/j*(p[i]-p[i-1]);

	if (p[3]<0.0) p[3]=0.0;			//printf("p=%f ",p[3]);
	if (p[3]>256.0) p[3]=255.0;		//printf("p=%f ",p[3]);

	*v=p[3];

	return 0;
}

//------------------------------------------------------
//bikubicna interpolacija  "smooth"
//za byte (char) vrednosti  v packed color 32 bitnem formatu
int interpBC_b32(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int i,j,b,l,m,n;
	float k;
	float p[4],p1[4],p2[4],p3[4],p4[4];
	float alpha = 1.0;

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)ceilf(x)-2; if (m<0) m=0; if ((m+5)>w) m=w-4;
	n=(int)ceilf(y)-2; if (n<0) n=0; if ((n+5)>h) n=h-4;


	for (b=3;b>-1;b--)
	{
		//njaprej po y  (stiri stolpce)
		for (i=0;i<4;i++)
		{
			l=m+(i+n)*w;
			p1[i]=sl[4*l+b];
			p2[i]=sl[4*(l+1)+b];
			p3[i]=sl[4*(l+2)+b];
			p4[i]=sl[4*(l+3)+b];
		}
		for (j=1;j<4;j++)
			for (i=3;i>=j;i--)
			{
				k=(y-i-n)/j;
				p1[i]=p1[i]+k*(p1[i]-p1[i-1]);
				p2[i]=p2[i]+k*(p2[i]-p2[i-1]);
				p3[i]=p3[i]+k*(p3[i]-p3[i-1]);
				p4[i]=p4[i]+k*(p4[i]-p4[i-1]);
			}

		//zdaj pa po x
		p[0]=p1[3]; p[1]=p2[3]; p[2]=p3[3]; p[3]=p4[3];
		for (j=1;j<4;j++)
			for (i=3;i>=j;i--)
				p[i]=p[i]+(x-i-m)/j*(p[i]-p[i-1]);

		if (p[3]<0.0) p[3]=0.0;
		if (p[3]>255.0) p[3]=255.0;

		if (b == 3) {
			alpha = p[3] / 255.0 * o;
			if (is_alpha) v[3] = p[3];
		} else {
			v[b] = v[b] * (1.0 - alpha) + p[3] * alpha;
		}
	}

	return 0;
}

//------------------------------------------------------
//bikubicna interpolacija  "sharp"
//za byte (char) vrednosti
//Helmut Dersch polinom
//	*sl vhodni array (slika)
//	w,h dimenzija slike je wxh
//	x,y tocka, za katero izracuna interpolirano vrednost
//  o opacity
//	*v interpolirana vrednost
//!!! ODKOD SUM???  (ze po eni rotaciji v interp_test !!)
//!!! v defish tega suma ni???
int interpBC2_b(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int i,k,l,m,n;
	float pp,p[4],wx[4],wy[4],xx;

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)ceilf(x)-2; if (m<0) m=0; if ((m+5)>w) m=w-4;
	n=(int)ceilf(y)-2; if (n<0) n=0; if ((n+5)>h) n=h-4;


	//najprej po y (stiri stolpce)
	xx=y-n;    wy[0]=(-0.75*(xx-5.0)*xx-6.0)*xx+3.0;
	xx=xx-1.0; wy[1]=(1.25*xx-2.25)*xx*xx+1.0;
	xx=1.0-xx; wy[2]=(1.25*xx-2.25)*xx*xx+1.0;
	xx=xx+1.0; wy[3]=(-0.75*(xx-5.0)*xx-6.0)*xx+3.0;
	//se po x
	xx=x-m;    wx[0]=(-0.75*(xx-5.0)*xx-6.0)*xx+3.0;
	xx=xx-1.0; wx[1]=(1.25*xx-2.25)*xx*xx+1.0;
	xx=1.0-xx; wx[2]=(1.25*xx-2.25)*xx*xx+1.0;
	xx=xx+1.0; wx[3]=(-0.75*(xx-5.0)*xx-6.0)*xx+3.0;

	k=n*w+m;
	for (i=0;i<4;i++)
	{
		p[i]=0.0;
		l=k+i;
		p[i]=wy[0]*sl[l]; l+=w;
		p[i]+=wy[1]*sl[l]; l+=w;
		p[i]+=wy[2]*sl[l]; l+=w;
		p[i]+=wy[3]*sl[l];
	}

	pp=wx[0]*p[0];
	pp+=wx[1]*p[1];
	pp+=wx[2]*p[2];
	pp+=wx[3]*p[3];

	if (pp<0.0) pp=0.0;
	if (pp>256.0) pp=255.0;

	*v=pp;
	return 0;
}

//------------------------------------------------------
//bikubicna interpolacija  "sharp"
//za byte (char) vrednosti  v packed color 32 bitnem formatu
//!!! ODKOD SUM???  (ze po eni rotaciji v interp_test !!)
//!!! v defish tega suma ni???
int interpBC2_b32(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int b,i,k,l,m,n,u;
	float pp,p[4],wx[4],wy[4],xx;

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)ceilf(x)-2; if (m<0) m=0; if ((m+5)>w) m=w-4;
	n=(int)ceilf(y)-2; if (n<0) n=0; if ((n+5)>h) n=h-4;

	//najprej po y (stiri stolpce)
	xx=y-n;    wy[0]=(-0.75*(xx-5.0)*xx-6.0)*xx+3.0;
	xx=xx-1.0; wy[1]=(1.25*xx-2.25)*xx*xx+1.0;
	xx=1.0-xx; wy[2]=(1.25*xx-2.25)*xx*xx+1.0;
	xx=xx+1.0; wy[3]=(-0.75*(xx-5.0)*xx-6.0)*xx+3.0;
	//se po x
	xx=x-m;    wx[0]=(-0.75*(xx-5.0)*xx-6.0)*xx+3.0;
	xx=xx-1.0; wx[1]=(1.25*xx-2.25)*xx*xx+1.0;
	xx=1.0-xx; wx[2]=(1.25*xx-2.25)*xx*xx+1.0;
	xx=xx+1.0; wx[3]=(-0.75*(xx-5.0)*xx-6.0)*xx+3.0;

	k=4*(n*w+m); u=4*w;
	for (b=0;b<4;b++)
	{
		for (i=0;i<4;i++)
		{
			p[i]=0.0;
			l=k+4*i;
			p[i]=wy[0]*sl[l]; l+=u;
			p[i]+=wy[1]*sl[l]; l+=u;
			p[i]+=wy[2]*sl[l]; l+=u;
			p[i]+=wy[3]*sl[l];
		}
		k++;

		pp=wx[0]*p[0];
		pp+=wx[1]*p[1];
		pp+=wx[2]*p[2];
		pp+=wx[3]*p[3];

		if (pp<0.0) pp=0.0;
		if (pp>256.0) pp=255.0;

		v[b]=pp;
	}

	return 0;
}

//------------------------------------------------------
//spline 4x4 interpolacija
//za byte (char) vrednosti
//Helmut Dersch polinom
//	*sl vhodni array (slika)
//	w,h dimenzija slike je wxh
//	x,y tocka, za katero izracuna interpolirano vrednost
//  o opacity
//	*v interpolirana vrednost
int interpSP4_b(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int i,j,m,n;
	float pp,p[4],wx[4],wy[4],xx;

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)ceilf(x)-2; if (m<0) m=0; if ((m+5)>w) m=w-4;
	n=(int)ceilf(y)-2; if (n<0) n=0; if ((n+5)>h) n=h-4;

	//najprej po y (stiri stolpce)
	xx=y-n;    wy[0]=((-0.333333*(xx-1.0)+0.8)*(xx-1.0)-0.466667)*(xx-1.0);
	xx=xx-1.0; wy[1]=((xx-1.8)*xx-0.2)*xx+1.0;
	xx=1.0-xx; wy[2]=((xx-1.8)*xx-0.2)*xx+1.0;
	xx=xx+1.0; wy[3]=((-0.333333*(xx-1.0)+0.8)*(xx-1.0)-0.466667)*(xx-1.0);
	//se po x
	xx=x-m;    wx[0]=((-0.333333*(xx-1.0)+0.8)*(xx-1.0)-0.466667)*(xx-1.0);
	xx=xx-1.0; wx[1]=((xx-1.8)*xx-0.2)*xx+1.0;
	xx=1.0-xx; wx[2]=((xx-1.8)*xx-0.2)*xx+1.0;
	xx=xx+1.0; wx[3]=((-0.333333*(xx-1.0)+0.8)*(xx-1.0)-0.466667)*(xx-1.0);

	for (i=0;i<4;i++)
	{
		p[i]=0.0;
		for (j=0;j<4;j++)
		{
			p[i]=p[i]+wy[j]*sl[(j+n)*w+i+m];
		}
	}

	pp=0.0;
	for (i=0;i<4;i++)
		pp=pp+wx[i]*p[i];

	if (pp<0.0) pp=0.0;
	if (pp>256.0) pp=255.0;

	*v=pp;
	return 0;
}

//------------------------------------------------------
//spline 4x4 interpolacija
//za byte (char) vrednosti  v packed color 32 bitnem formatu
int interpSP4_b32(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int i,j,m,n,b;
	float pp,p[4],wx[4],wy[4],xx;

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)ceilf(x)-2; if (m<0) m=0; if ((m+5)>w) m=w-4;
	n=(int)ceilf(y)-2; if (n<0) n=0; if ((n+5)>h) n=h-4;

	//najprej po y (stiri stolpce)
	xx=y-n;    wy[0]=((-0.333333*(xx-1.0)+0.8)*(xx-1.0)-0.466667)*(xx-1.0);
	xx=xx-1.0; wy[1]=((xx-1.8)*xx-0.2)*xx+1.0;
	xx=1.0-xx; wy[2]=((xx-1.8)*xx-0.2)*xx+1.0;
	xx=xx+1.0; wy[3]=((-0.333333*(xx-1.0)+0.8)*(xx-1.0)-0.466667)*(xx-1.0);
	//se po x
	xx=x-m;    wx[0]=((-0.333333*(xx-1.0)+0.8)*(xx-1.0)-0.466667)*(xx-1.0);
	xx=xx-1.0; wx[1]=((xx-1.8)*xx-0.2)*xx+1.0;
	xx=1.0-xx; wx[2]=((xx-1.8)*xx-0.2)*xx+1.0;
	xx=xx+1.0; wx[3]=((-0.333333*(xx-1.0)+0.8)*(xx-1.0)-0.466667)*(xx-1.0);

	for (b=0;b<4;b++)
	{
		for (i=0;i<4;i++)
		{
			p[i]=0.0;
			for (j=0;j<4;j++)
			{
				p[i]=p[i]+wy[j]*sl[4*((j+n)*w+i+m)+b];
			}
		}

		pp=0.0;
		for (i=0;i<4;i++)
			pp=pp+wx[i]*p[i];

		if (pp<0.0) pp=0.0;
		if (pp>256.0) pp=255.0;

		v[b]=pp;
	}

	return 0;
}

//------------------------------------------------------
//spline 6x6 interpolacija
//za byte (char) vrednosti
//Helmut Dersch polinom
//	*sl vhodni array (slika)
//	w,h dimenzija slike je wxh
//	x,y tocka, za katero izracuna interpolirano vrednost
//  o opacity
//	*v interpolirana vrednost
//!!! PAZI, TOLE NE DELA CISTO PRAV ???   belina se siri
//!!! zaenkrat sem dodal fudge factor...
int interpSP6_b(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int i,j,m,n;
	float pp,p[6],wx[6],wy[6],xx;

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)ceilf(x)-3; if (m<0) m=0; if ((m+7)>w) m=w-6;
	n=(int)ceilf(y)-3; if (n<0) n=0; if ((n+7)>h) n=h-6;

	//najprej po y (sest stolpcev)
	xx=y-n;
	wy[0]=((0.090909*(xx-2.0)-0.215311)*(xx-2.0)+0.124402)*(xx-2.0);
	xx=xx-1.0;
	wy[1]=((-0.545455*(xx-1.0)+1.291866)*(xx-1.0)-0.746411)*(xx-1.0);
	xx=xx-1.0;
	wy[2]=((1.181818*xx-2.167464)*xx+0.014354)*xx+1.0;
	xx=1.0-xx;
	wy[3]=((1.181818*xx-2.167464)*xx+0.014354)*xx+1.0;
	xx=xx+1.0;
	wy[4]=((-0.545455*(xx-1.0)+1.291866)*(xx-1.0)-0.746411)*(xx-1.0);
	xx=xx+1.0;
	wy[5]=((0.090909*(xx-2.0)-0.215311)*(xx-2.0)+0.124402)*(xx-2.0);
	//se po x
	xx=x-m;
	wx[0]=((0.090909*(xx-2.0)-0.215311)*(xx-2.0)+0.124402)*(xx-2.0);
	xx=xx-1.0;
	wx[1]=((-0.545455*(xx-1.0)+1.291866)*(xx-1.0)-0.746411)*(xx-1.0);
	xx=xx-1.0;
	wx[2]=((1.181818*xx-2.167464)*xx+0.014354)*xx+1.0;
	xx=1.0-xx;
	wx[3]=((1.181818*xx-2.167464)*xx+0.014354)*xx+1.0;
	xx=xx+1.0;
	wx[4]=((-0.545455*(xx-1.0)+1.291866)*(xx-1.0)-0.746411)*(xx-1.0);
	xx=xx+1.0;
	wx[5]=((0.090909*(xx-2.0)-0.215311)*(xx-2.0)+0.124402)*(xx-2.0);


	for (i=0;i<6;i++)
	{
		p[i]=0.0;
		for (j=0;j<6;j++)
		{
			p[i]=p[i]+wy[j]*sl[(j+n)*w+i+m];
		}
	}

	pp=0.0;
	for (i=0;i<6;i++)
		pp=pp+wx[i]*p[i];

	pp=0.947*pp;	//fudge factor...!!!    cca 0.947...
	if (pp<0.0) pp=0.0;
	if (pp>256.0) pp=255.0;

	*v=pp;
	return 0;
}

//------------------------------------------------------
//spline 6x6 interpolacija
//za byte (char) vrednosti  v packed color 32 bitnem formatu
//!!! PAZI, TOLE NE DELA CISTO PRAV ???   belina se siri
//!!! zaenkrat sem dodal fudge factor...
int interpSP6_b32(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int i,b,j,m,n;
	float pp,p[6],wx[6],wy[6],xx;

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)ceilf(x)-3; if (m<0) m=0; if ((m+7)>w) m=w-6;
	n=(int)ceilf(y)-3; if (n<0) n=0; if ((n+7)>h) n=h-6;

	//najprej po y (sest stolpcev)
	xx=y-n;
	wy[0]=((0.090909*(xx-2.0)-0.215311)*(xx-2.0)+0.124402)*(xx-2.0);
	xx=xx-1.0;
	wy[1]=((-0.545455*(xx-1.0)+1.291866)*(xx-1.0)-0.746411)*(xx-1.0);
	xx=xx-1.0;
	wy[2]=((1.181818*xx-2.167464)*xx+0.014354)*xx+1.0;
	xx=1.0-xx;
	wy[3]=((1.181818*xx-2.167464)*xx+0.014354)*xx+1.0;
	xx=xx+1.0;
	wy[4]=((-0.545455*(xx-1.0)+1.291866)*(xx-1.0)-0.746411)*(xx-1.0);
	xx=xx+1.0;
	wy[5]=((0.090909*(xx-2.0)-0.215311)*(xx-2.0)+0.124402)*(xx-2.0);
	//se po x
	xx=x-m;
	wx[0]=((0.090909*(xx-2.0)-0.215311)*(xx-2.0)+0.124402)*(xx-2.0);
	xx=xx-1.0;
	wx[1]=((-0.545455*(xx-1.0)+1.291866)*(xx-1.0)-0.746411)*(xx-1.0);
	xx=xx-1.0;
	wx[2]=((1.181818*xx-2.167464)*xx+0.014354)*xx+1.0;
	xx=1.0-xx;
	wx[3]=((1.181818*xx-2.167464)*xx+0.014354)*xx+1.0;
	xx=xx+1.0;
	wx[4]=((-0.545455*(xx-1.0)+1.291866)*(xx-1.0)-0.746411)*(xx-1.0);
	xx=xx+1.0;
	wx[5]=((0.090909*(xx-2.0)-0.215311)*(xx-2.0)+0.124402)*(xx-2.0);


	for (b=0;b<4;b++)
	{
		for (i=0;i<6;i++)
		{
			p[i]=0.0;
			for (j=0;j<6;j++)
			{
				p[i]=p[i]+wy[j]*sl[4*((j+n)*w+i+m)+b];
			}
		}

		pp=0.0;
		for (i=0;i<6;i++)
			pp=pp+wx[i]*p[i];

		pp=0.947*pp;	//fudge factor...!!!    cca 0.947...
		if (pp<0.0) pp=0.0;
		if (pp>256.0) pp=255.0;

		v[b]=pp;
	}

	return 0;
}

//------------------------------------------------------
//truncated sinc "lanczos" 16x16 interpolacija
//za byte (char) vrednosti
//	*sl vhodni array (slika)
//	w,h dimenzija slike je wxh
//	x,y tocka, za katero izracuna interpolirano vrednost
//  o opacity
//	*v interpolirana vrednost
int interpSC16_b(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int i,j,m,n;
	float pp,p[16],wx[16],wy[16],xx,xxx,x1;
	float PI=3.141592654;

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)ceilf(x)-8; if (m<0) m=0; if ((m+17)>w) m=w-16;
	n=(int)ceilf(y)-8; if (n<0) n=0; if ((n+17)>h) n=h-16;

	//najprej po y
	xx=y-n;
	for (i=7;i>=0;i--)
	{
		x1=xx*PI;
		wy[7-i]=(sin(x1)/(x1))*(sin(x1*0.125)/(x1*0.125));
		xxx=(float)(2*i+1)-xx;
		x1=xxx*PI;
		wy[8+i]=(sin(x1)/(x1))*(sin(x1*0.125)/(x1*0.125));
		xx=xx-1.0;
	}
	//se po x
	xx=x-m;
	for (i=7;i>=0;i--)
	{
		x1=xx*PI;
		wx[7-i]=(sin(x1)/(x1))*(sin(x1*0.125)/(x1*0.125));
		xxx=(float)(2*i+1)-xx;
		x1=xxx*PI;
		wx[8+i]=(sin(x1)/(x1))*(sin(x1*0.125)/(x1*0.125));
		xx=xx-1.0;
	}

	for (i=0;i<16;i++)
	{
		p[i]=0.0;
		for (j=0;j<16;j++)
		{
			p[i]=p[i]+wy[j]*sl[(j+n)*w+i+m];
		}
	}

	pp=0.0;
	for (i=0;i<16;i++)
		pp=pp+wx[i]*p[i];

	if (pp<0.0) pp=0.0;
	if (pp>256.0) pp=255.0;

	*v=pp;
	return 0;
}

//------------------------------------------------------
//truncated sinc "lanczos" 16x16 interpolacija
//za byte (char) vrednosti  v packed color 32 bitnem formatu
int interpSC16_b32(unsigned char *sl, int w, int h, float x, float y, float o, unsigned char *v, int is_alpha)
{
	int i,j,m,b,n;
	float pp,p[16],wx[16],wy[16],xx,xxx,x1;
	float PI=3.141592654;

#ifdef TEST_XY_LIMITS
	if ((x<0)||(x>=(w-1))||(y<0)||(y>=(h-1))) return -1;
#endif

	m=(int)ceilf(x)-8; if (m<0) m=0; if ((m+17)>w) m=w-16;
	n=(int)ceilf(y)-8; if (n<0) n=0; if ((n+17)>h) n=h-16;

	//najprej po y
	xx=y-n;
	for (i=7;i>=0;i--)
	{
		x1=xx*PI;
		wy[7-i]=(sin(x1)/(x1))*(sin(x1*0.125)/(x1*0.125));
		xxx=(float)(2*i+1)-xx;
		x1=xxx*PI;
		wy[8+i]=(sin(x1)/(x1))*(sin(x1*0.125)/(x1*0.125));
		xx=xx-1.0;
	}
	//se po x
	xx=x-m;
	for (i=7;i>=0;i--)
	{
		x1=xx*PI;
		wx[7-i]=(sin(x1)/(x1))*(sin(x1*0.125)/(x1*0.125));
		xxx=(float)(2*i+1)-xx;
		x1=xxx*PI;
		wx[8+i]=(sin(x1)/(x1))*(sin(x1*0.125)/(x1*0.125));
		xx=xx-1.0;
	}

	for (b=0;b<4;b++)
	{
		for (i=0;i<16;i++)
		{
			p[i]=0.0;
			for (j=0;j<16;j++)
			{
				p[i]=p[i]+wy[j]*sl[4*((j+n)*w+i+m)+b];
			}
		}

		pp=0.0;
		for (i=0;i<16;i++)
			pp=pp+wx[i]*p[i];

		if (pp<0.0) pp=0.0;
		if (pp>256.0) pp=255.0;

		v[b]=pp;
	}

	return 0;
}
