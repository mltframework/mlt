/*
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

#include <stdint.h>

#define SAD_SSE_INIT \
	asm volatile ( "pxor %%mm6,%%mm6\n\t" ::  );\

// Sum two 8x1 pixel blocks
#define SAD_SSE_SUM_8(OFFSET) \
			"movq " #OFFSET "(%0),%%mm0		\n\t"\
			"movq " #OFFSET "(%1),%%mm1		\n\t"\
			"psadbw %%mm1,%%mm0			\n\t"\
			"paddw %%mm0,%%mm6			\n\t"\

#define SAD_SSE_FINISH(RESULT) \
	asm volatile( "movd %%mm6,%0" : "=r" (RESULT) : );

// Advance by ystride
#define SAD_SSE_NEXTROW \
			"add %2,%0				\n\t"\
			"add %2,%1				\n\t"\

// BROKEN!
inline static int sad_sse_4x4( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 
	SAD_SSE_INIT
	#define ROW	SAD_SSE_SUM_8(0) SAD_SSE_NEXTROW
	asm volatile (  ROW ROW ROW ROW
			:: "r" (block1), "r" (block2), "r" ((intptr_t)(ystride)));
	
	SAD_SSE_FINISH(result)
	return result;
	#undef ROW

}

inline static int sad_sse_8x8( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 
	SAD_SSE_INIT
	#define ROW	SAD_SSE_SUM_8(0) SAD_SSE_NEXTROW
	asm volatile (  ROW ROW ROW ROW ROW ROW ROW ROW
			:: "r" (block1), "r" (block2), "r" ((intptr_t)(ystride)));
	
	SAD_SSE_FINISH(result)
	return result;
	#undef ROW

}

inline static int sad_sse_16x16( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 
	SAD_SSE_INIT
	#define ROW	SAD_SSE_SUM_8(0) SAD_SSE_SUM_8(8) SAD_SSE_NEXTROW
	asm volatile (	ROW ROW ROW ROW ROW ROW ROW ROW
			ROW ROW ROW ROW ROW ROW ROW ROW
			:: "r" (block1), "r" (block2), "r" ((intptr_t)(ystride)));
	
	SAD_SSE_FINISH(result)
	return result;
	#undef ROW

}

inline static int sad_sse_32x32( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 
	SAD_SSE_INIT
	#define ROW	SAD_SSE_SUM_8(0) SAD_SSE_SUM_8(8) SAD_SSE_SUM_8(16) SAD_SSE_SUM_8(24)\
			SAD_SSE_NEXTROW

	asm volatile (	ROW ROW ROW ROW ROW ROW ROW ROW
			ROW ROW ROW ROW ROW ROW ROW ROW
			ROW ROW ROW ROW ROW ROW ROW ROW
			ROW ROW ROW ROW ROW ROW ROW ROW
			:: "r" (block1), "r" (block2), "r" ((intptr_t)(ystride)));
	
	SAD_SSE_FINISH(result)
	return result;
	#undef ROW

}
// BROKEN!
inline static int sad_sse_4w( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 

	SAD_SSE_INIT

	while( h != 0 ) {
		asm volatile (
			SAD_SSE_SUM_8(0)
			:: "r" (block1), "r" (block2)
		);
	
		h--;
		block1 += ystride;
		block2 += ystride;
	}
	SAD_SSE_FINISH(result)
	return result;

}

inline static int sad_sse_8w( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 

	SAD_SSE_INIT

	while( h != 0 ) {
		asm volatile (
			SAD_SSE_SUM_8(0)

			:: "r" (block1), "r" (block2)
		);
	
		h--;
		block1 += ystride;
		block2 += ystride;
	}
	SAD_SSE_FINISH(result)
	return result;

}

inline static int sad_sse_16w( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 

	SAD_SSE_INIT

	while( h != 0 ) {
		asm volatile (
			SAD_SSE_SUM_8(0)
			SAD_SSE_SUM_8(8)

			:: "r" (block1), "r" (block2)
		);
	
		h--;
		block1 += ystride;
		block2 += ystride;
	}
	SAD_SSE_FINISH(result)
	return result;

}

inline static int sad_sse_32w( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 

	SAD_SSE_INIT

	while( h != 0 ) {
		asm volatile (
			SAD_SSE_SUM_8(0)
			SAD_SSE_SUM_8(8)
			SAD_SSE_SUM_8(16)
			SAD_SSE_SUM_8(24)

			:: "r" (block1), "r" (block2)
		);
	
		h--;
		block1 += ystride;
		block2 += ystride;
	}
	SAD_SSE_FINISH(result)
	return result;

}

inline static int sad_sse_64w( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 

	SAD_SSE_INIT

	while( h != 0 ) {
		asm volatile (
			SAD_SSE_SUM_8(0)
			SAD_SSE_SUM_8(8)
			SAD_SSE_SUM_8(16)
			SAD_SSE_SUM_8(24)
			SAD_SSE_SUM_8(32)
			SAD_SSE_SUM_8(40)
			SAD_SSE_SUM_8(48)
			SAD_SSE_SUM_8(56)

			:: "r" (block1), "r" (block2)
		);
	
		h--;
		block1 += ystride;
		block2 += ystride;
	}
	SAD_SSE_FINISH(result)
	return result;

}
static __attribute__((used)) __attribute__((aligned(8))) uint64_t sad_sse_422_mask_chroma = 0x00ff00ff00ff00ffULL;

#define SAD_SSE_422_LUMA_INIT \
	asm volatile (  "movq %0,%%mm7\n\t"\
			"pxor %%mm6,%%mm6\n\t" :: "m" (sad_sse_422_mask_chroma) );\

// Sum two 4x1 pixel blocks
#define SAD_SSE_422_LUMA_SUM_4(OFFSET) \
			"movq " #OFFSET "(%0),%%mm0		\n\t"\
			"movq " #OFFSET "(%1),%%mm1		\n\t"\
			"pand %%mm7,%%mm0			\n\t"\
			"pand %%mm7,%%mm1			\n\t"\
			"psadbw %%mm1,%%mm0			\n\t"\
			"paddw %%mm0,%%mm6			\n\t"\

static int sad_sse_422_luma_4x4( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 
	SAD_SSE_422_LUMA_INIT
	#define ROW	SAD_SSE_422_LUMA_SUM_4(0) SAD_SSE_NEXTROW
	asm volatile (  ROW ROW ROW ROW
			:: "r" (block1), "r" (block2), "r" ((intptr_t)(ystride)));
	
	SAD_SSE_FINISH(result)
	return result;
	#undef ROW

}

static int sad_sse_422_luma_8x8( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 
	SAD_SSE_422_LUMA_INIT
	#define ROW	SAD_SSE_422_LUMA_SUM_4(0) SAD_SSE_422_LUMA_SUM_4(8) SAD_SSE_NEXTROW
	asm volatile (  ROW ROW ROW ROW ROW ROW ROW ROW
			:: "r" (block1), "r" (block2), "r" ((intptr_t)(ystride)));
	
	SAD_SSE_FINISH(result)
	return result;
	#undef ROW

}

static int sad_sse_422_luma_16x16( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 
	SAD_SSE_422_LUMA_INIT
	#define ROW	SAD_SSE_422_LUMA_SUM_4(0) SAD_SSE_422_LUMA_SUM_4(8) SAD_SSE_422_LUMA_SUM_4(16) SAD_SSE_422_LUMA_SUM_4(24) SAD_SSE_NEXTROW
	asm volatile (	ROW ROW ROW ROW ROW ROW ROW ROW
			ROW ROW ROW ROW ROW ROW ROW ROW
			:: "r" (block1), "r" (block2), "r" ((intptr_t)(ystride)));
	
	SAD_SSE_FINISH(result)
	return result;
	#undef ROW

}

static int sad_sse_422_luma_32x32( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 
	SAD_SSE_422_LUMA_INIT
	#define ROW	SAD_SSE_422_LUMA_SUM_4(0) SAD_SSE_422_LUMA_SUM_4(8) SAD_SSE_422_LUMA_SUM_4(16) SAD_SSE_422_LUMA_SUM_4(24)\
			SAD_SSE_422_LUMA_SUM_4(32) SAD_SSE_422_LUMA_SUM_4(40) SAD_SSE_422_LUMA_SUM_4(48) SAD_SSE_422_LUMA_SUM_4(56)\
			SAD_SSE_NEXTROW

	asm volatile (	ROW ROW ROW ROW ROW ROW ROW ROW
			ROW ROW ROW ROW ROW ROW ROW ROW
			ROW ROW ROW ROW ROW ROW ROW ROW
			ROW ROW ROW ROW ROW ROW ROW ROW
			:: "r" (block1), "r" (block2), "r" ((intptr_t)(ystride)));
	
	SAD_SSE_FINISH(result)
	return result;
	#undef ROW

}

static int sad_sse_422_luma_4w( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 

	SAD_SSE_422_LUMA_INIT

	while( h != 0 ) {
		asm volatile (
			SAD_SSE_422_LUMA_SUM_4(0)
			:: "r" (block1), "r" (block2)
		);
	
		h--;
		block1 += ystride;
		block2 += ystride;
	}
	SAD_SSE_FINISH(result)
	return result;

}

static int sad_sse_422_luma_8w( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 

	SAD_SSE_422_LUMA_INIT

	while( h != 0 ) {
		asm volatile (
			SAD_SSE_422_LUMA_SUM_4(0)
			SAD_SSE_422_LUMA_SUM_4(8)

			:: "r" (block1), "r" (block2)
		);
	
		h--;
		block1 += ystride;
		block2 += ystride;
	}
	SAD_SSE_FINISH(result)
	return result;

}

static int sad_sse_422_luma_16w( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 

	SAD_SSE_422_LUMA_INIT

	while( h != 0 ) {
		asm volatile (
			SAD_SSE_422_LUMA_SUM_4(0)
			SAD_SSE_422_LUMA_SUM_4(8)
			SAD_SSE_422_LUMA_SUM_4(16)
			SAD_SSE_422_LUMA_SUM_4(24)

			:: "r" (block1), "r" (block2)
		);
	
		h--;
		block1 += ystride;
		block2 += ystride;
	}
	SAD_SSE_FINISH(result)
	return result;

}

static int sad_sse_422_luma_32w( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 

	SAD_SSE_422_LUMA_INIT

	while( h != 0 ) {
		asm volatile (
			SAD_SSE_422_LUMA_SUM_4(0)
			SAD_SSE_422_LUMA_SUM_4(8)
			SAD_SSE_422_LUMA_SUM_4(16)
			SAD_SSE_422_LUMA_SUM_4(24)
			SAD_SSE_422_LUMA_SUM_4(32)
			SAD_SSE_422_LUMA_SUM_4(40)
			SAD_SSE_422_LUMA_SUM_4(48)
			SAD_SSE_422_LUMA_SUM_4(56)

			:: "r" (block1), "r" (block2)
		);
	
		h--;
		block1 += ystride;
		block2 += ystride;
	}
	SAD_SSE_FINISH(result)
	return result;

}

static int sad_sse_422_luma_64w( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
{
	int result; 

	SAD_SSE_422_LUMA_INIT

	while( h != 0 ) {
		asm volatile (
			SAD_SSE_422_LUMA_SUM_4(0)
			SAD_SSE_422_LUMA_SUM_4(8)
			SAD_SSE_422_LUMA_SUM_4(16)
			SAD_SSE_422_LUMA_SUM_4(24)
			SAD_SSE_422_LUMA_SUM_4(32)
			SAD_SSE_422_LUMA_SUM_4(40)
			SAD_SSE_422_LUMA_SUM_4(48)
			SAD_SSE_422_LUMA_SUM_4(56)
			SAD_SSE_422_LUMA_SUM_4(64)
			SAD_SSE_422_LUMA_SUM_4(72)
			SAD_SSE_422_LUMA_SUM_4(80)
			SAD_SSE_422_LUMA_SUM_4(88)
			SAD_SSE_422_LUMA_SUM_4(96)
			SAD_SSE_422_LUMA_SUM_4(104)
			SAD_SSE_422_LUMA_SUM_4(112)
			SAD_SSE_422_LUMA_SUM_4(120)

			:: "r" (block1), "r" (block2)
		);
	
		h--;
		block1 += ystride;
		block2 += ystride;
	}
	SAD_SSE_FINISH(result)
	return result;
}
