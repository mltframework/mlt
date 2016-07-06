/*
 *	/brief fast motion estimation filter
 *	/author Zachary Drew, Copyright 2005
 *
 *	Currently only uses Gamma data for comparisonon (bug or feature?)
 *	SSE optimized where available.
 *
 *	Vector orientation: The vector data that is generated for the current frame specifies
 *	the motion from the previous frame to the current frame. To know how a macroblock
 *	in the current frame will move in the future, the next frame is needed.
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


#include "filter_motion_est.h"
#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef USE_SSE
#include "sad_sse.h"
#endif

#define NDEBUG
#include <assert.h>

#undef DEBUG
#undef DEBUG_ASM
#undef BENCHMARK
#undef COUNT_COMPARES

#define DIAMOND_SEARCH 0x0
#define FULL_SEARCH 0x1
#define SHIFT 8
#define ABS(a) ((a) >= 0 ? (a) : (-(a)))


struct motion_est_context_s
{
	int initialized;				// true if filter has been initialized

#ifdef COUNT_COMPARES
	int compares;
#endif

	/* same as mlt_frame's parameters */
	int width, height;

	/* Operational details */
	int mb_w, mb_h;
	int xstride, ystride;
	uint8_t *cache_image;			// Copy of current frame
	uint8_t *former_image;			// Copy of former frame
	int search_method;
	int skip_prediction;
	int shot_change;
	int limit_x, limit_y;			// max x and y of a motion vector
	int initial_thresh;
	int check_chroma;			// if check_chroma == 1 then compare chroma
	int denoise;
	int previous_msad;
	int show_reconstruction;
	int toggle_when_paused;
	int show_residual;

	/* bounds */
	struct mlt_geometry_item_s bounds;	// Current bounds (from filters crop_detect, autotrack rectangle, or other)

	/* bounds in macroblock units; macroblocks are completely contained within the boundry */
	int left_mb, prev_left_mb, right_mb, prev_right_mb;
	int top_mb, prev_top_mb, bottom_mb, prev_bottom_mb;

	/* size of our vector buffers */
	int mv_buffer_height, mv_buffer_width, mv_size;

	/* vector buffers */
	int former_vectors_valid;		//<! true if the previous frame's buffered motion vector data is valid
	motion_vector *former_vectors;
	motion_vector *current_vectors;
	motion_vector *denoise_vectors;
	mlt_position former_frame_position, current_frame_position;

	/* diagnostic metrics */
	float predictive_misses;		// How often do the prediction motion vectors fail?
	int comparison_average;			// How far does the best estimation deviate from a perfect comparison?
	int bad_comparisons;
	int average_length;
	int average_x, average_y;

	/* run-time configurable comparison functions */
	int (*compare_reference)(uint8_t *, uint8_t *, int, int, int, int);
	int (*compare_optimized)(uint8_t *, uint8_t *, int, int, int, int);

};

// This is used to constrains pixel operations between two blocks to be within the image boundry
inline static int constrain(	int *x, int *y, int *w,	int *h,
				const int dx, const int dy,
				const int left, const int right,
				const int top, const int bottom)
{
	uint32_t penalty = 1 << SHIFT;			// Retain a few extra bits of precision
	int x2 = *x + dx;
	int y2 = *y + dy;
	int w_remains = *w;
	int h_remains = *h;

	// Origin of macroblock moves left of image boundy
	if( *x < left || x2 < left ) {
		w_remains = *w - left + ((*x < x2) ?  *x : x2);
		*x += *w - w_remains;
	}
	// Portion of macroblock moves right of image boundry
	else if( *x + *w > right || x2 + *w > right )
		w_remains = right - ((*x > x2) ? *x : x2);

	// Origin of macroblock moves above image boundy
	if( *y < top || y2 < top ) {
		h_remains = *h - top + ((*y < y2) ? *y : y2);
		*y += *h - h_remains;
	}
	// Portion of macroblock moves bellow image boundry
	else if( *y + *h > bottom || y2 + *h > bottom )
		h_remains = bottom - ((*y > y2) ?  *y : y2);

	if( w_remains == *w && h_remains == *h ) return penalty;
	if( w_remains <= 0 || h_remains <= 0) return 0;	// Block is clipped out of existance
	penalty = (*w * *h * penalty)
		/ ( w_remains * h_remains);		// Recipricol of the fraction of the block that remains

	assert(*x >= left); assert(x2 + *w - w_remains >= left);
	assert(*y >= top); assert(y2 + *h - h_remains >= top);
	assert(*x + w_remains <= right); assert(x2 + w_remains <= right);
	assert(*y + h_remains <= bottom); assert(y2 + h_remains <= bottom);

	*w = w_remains;					// Update the width and height
	*h = h_remains;

	return penalty;
}

/** /brief Reference Sum of Absolute Differences comparison function
*
*/
static int sad_reference( uint8_t *block1, uint8_t *block2, const int xstride, const int ystride, const int w, const int h )
{
	int i, j, score = 0;
	for ( j = 0; j < h; j++ ){
		for ( i = 0; i < w; i++ ){
			score += ABS( block1[i*xstride] - block2[i*xstride] );
		}
		block1 += ystride;
		block2 += ystride;
	}

	return score;
}


/** /brief Abstracted block comparison function
*/
inline static int block_compare( uint8_t *block1,
			   uint8_t *block2,
			   int x,
			   int y,
			   int dx,
			   int dy,
			   struct motion_est_context_s *c)
{

#ifdef COUNT_COMPARES
	c->compares++;
#endif

	int score;

	// Default comparison may be overridden by the slower, more capable reference comparison
	int (*cmp)(uint8_t *, uint8_t *, int, int, int, int) = c->compare_optimized;

	// vector displacement limited has been exceeded
	if( ABS( dx ) >= c->limit_x || ABS( dy ) >= c->limit_y )
		return MAX_MSAD;

	int mb_w = c->mb_w;	// Some writeable local copies
	int mb_h = c->mb_h;

	// Determine if either macroblock got clipped
	int penalty = constrain( &x, &y, &mb_w,	&mb_h, dx, dy, 0, c->width, 0, c->height);

	// Some gotchas
	if( penalty == 0 )			// Clipped out of existance: Return worst score
		return MAX_MSAD;
	else if( penalty != 1<<SHIFT )		// Nonstandard macroblock dimensions: Disable SIMD optimizizations.
		cmp = c->compare_reference;

	// Calculate the memory locations of the macroblocks
	block1 += x	 * c->xstride + y 	* c->ystride;
	block2 += (x+dx) * c->xstride + (y+dy)  * c->ystride;

	#ifdef DEBUG_ASM
	if( penalty == 1<<SHIFT ){
		score = c->compare_reference( block1, block2, c->xstride, c->ystride, mb_w, mb_h );
		int score2 = c->compare_optimized( block1, block2, c->xstride, c->ystride, mb_w, mb_h );
		if ( score != score2 )
			fprintf(stderr, "Your assembly doesn't work! Reference: %d Asm: %d\n", score, score2);
	}
	else
	#endif

	score = cmp( block1, block2, c->xstride, c->ystride, mb_w, mb_h );

	return ( score * penalty ) >> SHIFT;			// Ditch the extra precision
}

static inline void check_candidates (   uint8_t *ref,
					uint8_t *candidate_base,
					const int x,
					const int y,
					const motion_vector *candidates,// Contains to_x & to_y
					const int count,		// Number of candidates
					const int unique,		// Sometimes we know the candidates are unique
					motion_vector *result,
					struct motion_est_context_s *c )
{
		int score, i, j;
		/* Scan for the best candidate */
		for ( i = 0; i < count; i++ )
		{
			// this little dohicky ignores duplicate candidates, if they are possible
			if ( unique == 0 ) {
				j = 0;
				while ( j < i )
				{
					if ( candidates[j].dx == candidates[i].dx &&
					     candidates[j].dy == candidates[i].dy )
							goto next_for_loop;

					j++;
				}
			}

			// Luma
			score = block_compare( ref, candidate_base,
						x, y,
						candidates[i].dx,	// from
						candidates[i].dy,
						c);

			if ( score < result->msad ) {	// New minimum
				result->dx = candidates[i].dx;
				result->dy = candidates[i].dy;
				result->msad = score;
			}
			next_for_loop:;
		}
}

/* /brief Diamond search
* Operates on a single macroblock
*/
static inline void diamond_search(
			uint8_t *ref,				//<! Image data from previous frame
			uint8_t *candidate_base,		//<! Image data in current frame
			const int x,				//<! X upper left corner of macroblock
			const int y,				//<! U upper left corner of macroblock
			struct motion_vector_s *result,		//<! Best predicted mv and eventual result
			struct motion_est_context_s *c)		//<! motion estimation context
{

	// diamond search pattern
	motion_vector candidates[4];

	// Keep track of best and former best candidates
	motion_vector best, former;
	best.dx = 0;
	best.dy = 0;
	former.dx = 0;
	former.dy = 0;

	// The direction of the refinement needs to be known
	motion_vector current;

	int i, first = 1;

	// Loop through the search pattern
	while( 1 ) {

		current.dx = result->dx;
		current.dy = result->dy;

		if ( first == 1 )	// Set the initial pattern
		{
			candidates[0].dx = result->dx + 1; candidates[0].dy = result->dy + 0;
			candidates[1].dx = result->dx + 0; candidates[1].dy = result->dy + 1;
			candidates[2].dx = result->dx - 1; candidates[2].dy = result->dy + 0;
			candidates[3].dx = result->dx + 0; candidates[3].dy = result->dy - 1;
			i = 4;
		}
		else	 // Construct the next portion of the search pattern
		{
			candidates[0].dx = result->dx + best.dx;
			candidates[0].dy = result->dy + best.dy;
			if (best.dx == former.dx && best.dy == former.dy) {
				candidates[1].dx = result->dx + best.dy;
				candidates[1].dy = result->dy + best.dx;		//	Yes, the wires
				candidates[2].dx = result->dx - best.dy;		// 	are crossed
				candidates[2].dy = result->dy - best.dx;
				i = 3;
			} else {
				candidates[1].dx = result->dx + former.dx;
				candidates[1].dy = result->dy + former.dy;
				i = 2;
			}

			former.dx = best.dx; former.dy = best.dy; 	// Keep track of new former best
		}

		check_candidates ( ref, candidate_base, x, y, candidates, i, 1, result, c );

		// Which candidate was the best?
		best.dx = result->dx - current.dx;
		best.dy = result->dy - current.dy;

		// A better canidate was not found
		if ( best.dx == 0 && best.dy == 0 )
			return;

		if ( first == 1 ){
			first = 0;
			former.dx = best.dx; former.dy = best.dy; // First iteration, sensible value for former.d*
		}
	}
}

/* /brief Full (brute) search
* Operates on a single macroblock
*/
__attribute__((used))
static void full_search(
			uint8_t *ref,				//<! Image data from previous frame
			uint8_t *candidate_base,		//<! Image data in current frame
			int x,					//<! X upper left corner of macroblock
			int y,					//<! U upper left corner of macroblock
			struct motion_vector_s *result,		//<! Best predicted mv and eventual result
			struct motion_est_context_s *c)		//<! motion estimation context
{
	// Keep track of best candidate
	int i,j,score;

	// Go loopy
	for( i = -c->mb_w; i <= c->mb_w; i++ ){
		for( j = -c->mb_h; j <=  c->mb_h; j++ ){

			score = block_compare( ref, candidate_base,
					 	x,
					 	y,
					 	x + i,
					 	y + j,
					 	c);

			if ( score < result->msad ) {
				result->dx = i;
				result->dy = j;
				result->msad = score;
			}
		}
	}
}

// Macros for pointer calculations
#define CURRENT(i,j)	( c->current_vectors + (j)*c->mv_buffer_width + (i) )
#define FORMER(i,j)	( c->former_vectors + (j)*c->mv_buffer_width + (i) )
#define DENOISE(i,j)	( c->denoise_vectors + (j)*c->mv_buffer_width + (i) )

int ncompare (const void * a, const void * b)
{
	return ( *(const int*)a - *(const int*)b );
}

// motion vector denoising
// for x and y components seperately,
// change the vector to be the median value of the 9 adjacent vectors
static void median_denoise( motion_vector *v, struct motion_est_context_s *c )
{
	int xvalues[9], yvalues[9];

	int i,j,n;
	for( j = c->top_mb; j <= c->bottom_mb; j++ )
		for( i = c->left_mb; i <= c->right_mb; i++ ){
		{
			n = 0;

			xvalues[n  ] = CURRENT(i,j)->dx; // Center
			yvalues[n++] = CURRENT(i,j)->dy;

			if( i > c->left_mb ) // Not in First Column
			{
				xvalues[n  ] = CURRENT(i-1,j)->dx; // Left
				yvalues[n++] = CURRENT(i-1,j)->dy;

				if( j > c->top_mb ) {
					xvalues[n  ] = CURRENT(i-1,j-1)->dx; // Upper Left
					yvalues[n++] = CURRENT(i-1,j-1)->dy;
				}

				if( j < c->bottom_mb ) {
					xvalues[n  ] = CURRENT(i-1,j+1)->dx; // Bottom Left
					yvalues[n++] = CURRENT(i-1,j+1)->dy;
				}
			}
			if( i < c->right_mb ) // Not in Last Column
			{
				xvalues[n  ] = CURRENT(i+1,j)->dx; // Right
				yvalues[n++] = CURRENT(i+1,j)->dy;


				if( j > c->top_mb ) {
					xvalues[n  ] = CURRENT(i+1,j-1)->dx; // Upper Right
					yvalues[n++] = CURRENT(i+1,j-1)->dy;
				}

				if( j < c->bottom_mb ) {
					xvalues[n  ] = CURRENT(i+1,j+1)->dx; // Bottom Right
					yvalues[n++] = CURRENT(i+1,j+1)->dy;
				}
			}
			if( j > c->top_mb ) // Not in First Row
			{
				xvalues[n  ] = CURRENT(i,j-1)->dx; // Top
				yvalues[n++] = CURRENT(i,j-1)->dy;
			}

			if( j < c->bottom_mb ) // Not in Last Row
			{
				xvalues[n  ] = CURRENT(i,j+1)->dx; // Bottom
				yvalues[n++] = CURRENT(i,j+1)->dy;
			}

			qsort (xvalues, n, sizeof(int), ncompare);
			qsort (yvalues, n, sizeof(int), ncompare);

			if( n % 2 == 1 ) {
				DENOISE(i,j)->dx = xvalues[n/2];
				DENOISE(i,j)->dy = yvalues[n/2];
			}
			else {
				DENOISE(i,j)->dx = (xvalues[n/2] + xvalues[n/2+1])/2;
				DENOISE(i,j)->dy = (yvalues[n/2] + yvalues[n/2+1])/2;
			}
		}
	}

	motion_vector *t = c->current_vectors;
	c->current_vectors = c->denoise_vectors;
	c->denoise_vectors = t;

}

// Credits: ffmpeg
// return the median
static inline int median_predictor(int a, int b, int c) {
	if ( a > b ){
		if ( c > b ){
			if ( c > a  ) b = a;
			else	  b = c;
		}
	} else {
		if ( b > c ){
			if ( c > a ) b = c;
			else	 b = a;
		}
	}
	return b;
}


/** /brief Motion search
*
* For each macroblock in the current frame, estimate the block from the last frame that
* matches best.
*
* Vocab: Colocated - the pixel in the previous frame at the current position
*
* Based on enhanced predictive zonal search. [Tourapis 2002]
*/
static void motion_search( uint8_t *from,			//<! Image data.
		   	   uint8_t *to,				//<! Image data. Rigid grid.
			   struct motion_est_context_s *c)	//<! The context
{

#ifdef COUNT_COMPARES
	compares = 0;
#endif

	motion_vector candidates[10];
	motion_vector *here;		// This one gets used alot (about 30 times per macroblock)
	int n = 0;

	int i, j, count=0;

	// For every macroblock, perform motion vector estimation
	for( i = c->left_mb; i <= c->right_mb; i++ ){
	 for( j = c->top_mb; j <= c->bottom_mb; j++ ){

		here = CURRENT(i,j);
		here->valid = 1;
		here->color = 100;
		here->msad = MAX_MSAD;
		count++;
		n = 0;


		/* Stack the predictors [i.e. checked in reverse order] */

		/* Adjacent to collocated */
		if( c->former_vectors_valid )
		{
			// Top of colocated
			if( j > c->prev_top_mb ){// && COL_TOP->valid ){
				candidates[n  ].dx = FORMER(i,j-1)->dx;
				candidates[n++].dy = FORMER(i,j-1)->dy;
			}

			// Left of colocated
			if( i > c->prev_left_mb ){// && COL_LEFT->valid ){
				candidates[n  ].dx = FORMER(i-1,j)->dx;
				candidates[n++].dy = FORMER(i-1,j)->dy;
			}

			// Right of colocated
			if( i < c->prev_right_mb ){// && COL_RIGHT->valid ){
				candidates[n  ].dx = FORMER(i+1,j)->dx;
				candidates[n++].dy = FORMER(i+1,j)->dy;
			}

			// Bottom of colocated
			if( j < c->prev_bottom_mb ){// && COL_BOTTOM->valid ){
				candidates[n  ].dx = FORMER(i,j+1)->dx;
				candidates[n++].dy = FORMER(i,j+1)->dy;
			}

			// And finally, colocated
			candidates[n  ].dx = FORMER(i,j)->dx;
			candidates[n++].dy = FORMER(i,j)->dy;
		}

		// For macroblocks not in the top row
		if ( j > c->top_mb) {

			// Top if ( TOP->valid ) {
				candidates[n  ].dx = CURRENT(i,j-1)->dx;
				candidates[n++].dy = CURRENT(i,j-1)->dy;
			//}

			// Top-Right, macroblocks not in the right row
			if ( i < c->right_mb ){// && TOP_RIGHT->valid ) {
				candidates[n  ].dx = CURRENT(i+1,j-1)->dx;
				candidates[n++].dy = CURRENT(i+1,j-1)->dy;
			}
		}

		// Left, Macroblocks not in the left column
		if ( i > c->left_mb ){// && LEFT->valid ) {
			candidates[n  ].dx = CURRENT(i-1,j)->dx;
			candidates[n++].dy = CURRENT(i-1,j)->dy;
		}

		/* Median predictor vector (median of left, top, and top right adjacent vectors) */
		if ( i > c->left_mb && j > c->top_mb && i < c->right_mb
			 )//&& LEFT->valid && TOP->valid && TOP_RIGHT->valid )
		{
			candidates[n  ].dx = median_predictor( CURRENT(i-1,j)->dx, CURRENT(i,j-1)->dx, CURRENT(i+1,j-1)->dx);
			candidates[n++].dy = median_predictor( CURRENT(i-1,j)->dy, CURRENT(i,j-1)->dy, CURRENT(i+1,j-1)->dy);
		}

		// Zero vector
		candidates[n  ].dx = 0;
		candidates[n++].dy = 0;

		int x = i * c->mb_w;
		int y = j * c->mb_h;
		check_candidates ( to, from, x, y, candidates, n, 0, here, c );


#ifndef FULLSEARCH
		diamond_search( to, from, x, y, here, c);
#else
		full_search( to, from, x, y, here, c);
#endif

		assert( x + c->mb_w + here->dx > 0 );	// All macroblocks must have area > 0
		assert( y + c->mb_h + here->dy > 0 );
		assert( x + here->dx < c->width );
		assert( y + here->dy < c->height );

	 } /* End column loop */
	} /* End row loop */

#ifdef USE_SSE
	asm volatile ( "emms" );
#endif

#ifdef COUNT_COMPARES
	fprintf(stderr, "%d comparisons per block were made", compares/count);
#endif
	return;
}

void collect_post_statistics( struct motion_est_context_s *c ) {

	c->comparison_average = 0;
	c->average_length = 0;
	c->average_x = 0;
	c->average_y = 0;

	int i, j, count = 0;

	for ( i = c->left_mb; i <= c->right_mb; i++ ){
	 for ( j = c->top_mb; j <= c->bottom_mb; j++ ){

		count++;
		c->comparison_average += CURRENT(i,j)->msad;
		c->average_x += CURRENT(i,j)->dx;
		c->average_y += CURRENT(i,j)->dy;


	 }
	}

	if ( count > 0 )
	{
		c->comparison_average /= count;
		c->average_x /= count;
		c->average_y /= count;
		c->average_length = sqrt( c->average_x * c->average_x + c->average_y * c->average_y );
	}

}

static void init_optimizations( struct motion_est_context_s *c )
{
	switch(c->mb_w){
#ifdef USE_SSE
		case 4:  if(c->mb_h == 4)	c->compare_optimized = sad_sse_422_luma_4x4;
			 else				c->compare_optimized = sad_sse_422_luma_4w;
			 break;
		case 8:  if(c->mb_h == 8)  c->compare_optimized = sad_sse_422_luma_8x8;
			 else				c->compare_optimized = sad_sse_422_luma_8w;
			 break;
		case 16: if(c->mb_h == 16) c->compare_optimized = sad_sse_422_luma_16x16;
			 else				c->compare_optimized = sad_sse_422_luma_16w;
			 break;
		case 32: if(c->mb_h == 32) c->compare_optimized = sad_sse_422_luma_32x32;
			 else				c->compare_optimized = sad_sse_422_luma_32w;
			 break;
		case 64: c->compare_optimized = sad_sse_422_luma_64w;
			 break;
#endif
		default: c->compare_optimized = sad_reference;
			 break;
	}
}

inline static void set_red(uint8_t *image, struct motion_est_context_s *c)
{
	int n;
	for( n = 0; n < c->width * c->height * 2; n+=4 )
	{
		image[n]   = 79;
		image[n+1] = 91;
		image[n+2] = 79;
		image[n+3] = 237;
	}

}

static void show_residual( uint8_t *result,  struct motion_est_context_s *c )
{
	int i, j;
	int x,y,w,h;
	int dx, dy;
	int tx,ty;
	uint8_t *b, *r;

//	set_red(result,c);

	for( j = c->top_mb; j <= c->bottom_mb; j++ ){
	 for( i = c->left_mb; i <= c->right_mb; i++ ){

		dx = CURRENT(i,j)->dx;
		dy = CURRENT(i,j)->dy;
		w = c->mb_w;
		h = c->mb_h;
		x = i * w;
		y = j * h;

		// Denoise function caused some blocks to be completely clipped, ignore them
		if (constrain( &x, &y, &w, &h, dx, dy, 0, c->width, 0, c->height) == 0 )
			continue;

		for( ty = y; ty < y + h ; ty++ ){
		 for( tx = x; tx < x + w ; tx++ ){

			b = c->former_image +  (tx+dx)*c->xstride + (ty+dy)*c->ystride;
			r = result +		tx*c->xstride + ty*c->ystride;

			r[0] = 16 + ABS( r[0] - b[0] );

			if( dx % 2 == 0 )
				r[1] = 128 + ABS( r[1] - b[1] );
			else
				// FIXME: may exceed boundies
				r[1] = 128 + ABS( r[1] - ( *(b-1) + b[3] ) /2 );
		 }
		}
	 }
	}
}

static void show_reconstruction( uint8_t *result, struct motion_est_context_s *c )
{
	int i, j;
	int x,y,w,h;
	int dx,dy;
	uint8_t *r, *s;
	int tx,ty;

	for( i = c->left_mb; i <= c->right_mb; i++ ){
	 for( j = c->top_mb; j <= c->bottom_mb; j++ ){

		dx = CURRENT(i,j)->dx;
		dy = CURRENT(i,j)->dy;
		w = c->mb_w;
		h = c->mb_h;
		x = i * w;
		y = j * h;

		// Denoise function caused some blocks to be completely clipped, ignore them
		if (constrain( &x, &y, &w, &h, dx, dy, 0, c->width, 0, c->height) == 0 )
			continue;

		for( ty = y; ty < y + h ; ty++ ){
		 for( tx = x; tx < x + w ; tx++ ){

			r = result +          tx*c->xstride + ty*c->ystride;
			s = c->former_image + (tx+dx)*c->xstride + (ty+dy)*c->ystride;

			r[0] = s[0];

			if( dx % 2 == 0 )
				r[1] = s[1];
			else
				// FIXME: may exceed boundies
				r[1] = ( *(s-1) + s[3] ) /2;
		 }
		}
	 }
	}
}

// Image stack(able) method
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the filter
	mlt_filter filter = mlt_frame_pop_service( frame );

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	// Get the motion_est context object
	struct motion_est_context_s *c = mlt_properties_get_data( MLT_FILTER_PROPERTIES( filter ), "context", NULL);

	// Get the new image and frame number
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	#ifdef BENCHMARK
	struct timeval start; gettimeofday(&start, NULL );
	#endif


	if( error != 0 )
		mlt_properties_debug( MLT_FRAME_PROPERTIES(frame), "error after mlt_frame_get_image() in motion_est", stderr );

	c->current_frame_position = mlt_frame_get_position( frame );

	/* Context Initialization */
	if ( c->initialized == 0 ) {

		// Get the filter properties object
		mlt_properties properties = mlt_filter_properties( filter );

		c->width = *width;
		c->height = *height;

		/* Get parameters that may have been overridden */
		if( mlt_properties_get( properties, "macroblock_width") != NULL )
			c->mb_w = mlt_properties_get_int( properties, "macroblock_width");

		if( mlt_properties_get( properties, "macroblock_height") != NULL )
			c->mb_h = mlt_properties_get_int( properties, "macroblock_height");

		if( mlt_properties_get( properties, "prediction_thresh") != NULL )
			c->initial_thresh = mlt_properties_get_int( properties, "prediction_thresh" );
		else
			c->initial_thresh = c->mb_w * c->mb_h;

		if( mlt_properties_get( properties, "search_method") != NULL )
			c->search_method = mlt_properties_get_int( properties, "search_method");

		if( mlt_properties_get( properties, "skip_prediction") != NULL )
			c->skip_prediction = mlt_properties_get_int( properties, "skip_prediction");

		if( mlt_properties_get( properties, "limit_x") != NULL )
			c->limit_x = mlt_properties_get_int( properties, "limit_x");

		if( mlt_properties_get( properties, "limit_y") != NULL )
			c->limit_y = mlt_properties_get_int( properties, "limit_y");

		if( mlt_properties_get( properties, "check_chroma" ) != NULL )
			c->check_chroma = mlt_properties_get_int( properties, "check_chroma" );

		if( mlt_properties_get( properties, "denoise" ) != NULL )
			c->denoise = mlt_properties_get_int( properties, "denoise" );

		if( mlt_properties_get( properties, "show_reconstruction" ) != NULL )
			c->show_reconstruction = mlt_properties_get_int( properties, "show_reconstruction" );

		if( mlt_properties_get( properties, "show_residual" ) != NULL )
			c->show_residual = mlt_properties_get_int( properties, "show_residual" );

		if( mlt_properties_get( properties, "toggle_when_paused" ) != NULL )
			c->toggle_when_paused = mlt_properties_get_int( properties, "toggle_when_paused" );

		init_optimizations( c );

		// Calculate the dimensions in macroblock units
		c->mv_buffer_width = (*width / c->mb_w);
		c->mv_buffer_height = (*height / c->mb_h);

		// Size of the motion vector buffer
		c->mv_size =  c->mv_buffer_width * c->mv_buffer_height * sizeof(struct motion_vector_s);

		// Allocate the motion vector buffers
		c->former_vectors = mlt_pool_alloc( c->mv_size );
		c->current_vectors = mlt_pool_alloc( c->mv_size );
		c->denoise_vectors = mlt_pool_alloc( c->mv_size );

		// Register motion buffers for destruction
		mlt_properties_set_data( properties, "current_motion_vectors", (void *)c->current_vectors, 0, mlt_pool_release, NULL );
		mlt_properties_set_data( properties, "former_motion_vectors", (void *)c->former_vectors, 0, mlt_pool_release, NULL );
		mlt_properties_set_data( properties, "denoise_motion_vectors", (void *)c->denoise_vectors, 0, mlt_pool_release, NULL );

		c->former_vectors_valid = 0;
		memset( c->former_vectors, 0, c->mv_size );

		c->xstride = 2;
		c->ystride = c->xstride * *width;

		// Allocate a cache for the previous frame's image
		c->former_image = mlt_pool_alloc( *width * *height * 2 );
		c->cache_image = mlt_pool_alloc( *width * *height * 2 );

		// Register for destruction
		mlt_properties_set_data( properties, "cache_image", (void *)c->cache_image, 0, mlt_pool_release, NULL );
		mlt_properties_set_data( properties, "former_image", (void *)c->former_image, 0, mlt_pool_release, NULL );

		c->former_frame_position = c->current_frame_position;
		c->previous_msad = 0;

		c->initialized = 1;
	}

	/* Check to see if somebody else has given us bounds */
	struct mlt_geometry_item_s *bounds = mlt_properties_get_data( MLT_FRAME_PROPERTIES( frame ), "bounds", NULL );

	if ( !bounds )
	{
		char *property = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "bounding" );
		if ( property )
		{
			mlt_geometry geometry = mlt_geometry_init( );
			mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE(filter) );
			if ( geometry )
			{
				mlt_geometry_parse( geometry, property, 0, profile->width, profile->height );
				bounds = calloc( 1, sizeof(*bounds) );
				mlt_properties_set_data( MLT_FILTER_PROPERTIES(filter), "bounds", bounds, sizeof(*bounds), free, NULL );
				mlt_geometry_fetch( geometry, bounds, 0 );
				mlt_geometry_close( geometry );
			}
		}
	}

	if( bounds != NULL ) {
		// translate pixel units (from bounds) to macroblock units
		// make sure whole macroblock stays within bounds
		c->left_mb = ( bounds->x + c->mb_w - 1 ) / c->mb_w;
		c->top_mb = ( bounds->y + c->mb_h - 1 ) / c->mb_h;
		c->right_mb = ( bounds->x + bounds->w ) / c->mb_w - 1;
		c->bottom_mb = ( bounds->y + bounds->h ) / c->mb_h - 1;
		c->bounds.x = bounds->x;
		c->bounds.y = bounds->y;
		c->bounds.w = bounds->w;
		c->bounds.h = bounds->h;
	} else {
		c->left_mb = c->prev_left_mb = 0;
		c->top_mb = c->prev_top_mb = 0;
		c->right_mb = c->prev_right_mb = c->mv_buffer_width - 1;	// Zero indexed
		c->bottom_mb = c->prev_bottom_mb = c->mv_buffer_height - 1;
		c->bounds.x = 0;
		c->bounds.y = 0;
		c->bounds.w = *width;
		c->bounds.h = *height;
	}

	// If video is advancing, run motion vector algorithm and etc...
	if( c->former_frame_position + 1 == c->current_frame_position )
	{

		// Swap the motion vector buffers and reuse allocated memory
		struct motion_vector_s *temp = c->current_vectors;
		c->current_vectors = c->former_vectors;
		c->former_vectors = temp;

		// This is done because filter_vismv doesn't pay attention to frame boundry
		memset( c->current_vectors, 0, c->mv_size );

		// Perform the motion search
		motion_search( c->cache_image, *image, c );

		collect_post_statistics( c );


		// Detect shot changes
		if( c->comparison_average > 10 * c->mb_w * c->mb_h &&
		    c->comparison_average > c->previous_msad * 2 )
		{
			mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
			mlt_log_verbose( MLT_FILTER_SERVICE(filter), "shot change: %d\n", c->comparison_average);
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "shot_change", 1);
			c->shot_change = 1;

			// Add the shot change to the list
			mlt_geometry key_frames = mlt_properties_get_data( properties, "shot_change_list", NULL );
			if ( !key_frames )
			{
				key_frames = mlt_geometry_init();
				mlt_properties_set_data( properties, "shot_change_list", key_frames, 0,
					(mlt_destructor) mlt_geometry_close, (mlt_serialiser) mlt_geometry_serialise );
				if ( key_frames )
					mlt_geometry_set_length( key_frames, mlt_filter_get_length2( filter, frame ) );
			}
			if ( key_frames )
			{
				struct mlt_geometry_item_s item;
				item.frame = (int) c->current_frame_position;
				item.x = c->comparison_average;
				item.f[0] = 1;
				item.f[1] = item.f[2] = item.f[3] = item.f[4] = 0;
				mlt_geometry_insert( key_frames, &item );
			}
		}
		else {
			c->former_vectors_valid = 1;
			c->shot_change = 0;
			//fprintf(stderr, " - SAD: %d\n", c->comparison_average);
		}

		c->previous_msad = c->comparison_average;

		if( c->comparison_average != 0 ) { // If the frame is not a duplicate of the previous frame

			// denoise the vector buffer
			if( c->denoise )
				median_denoise( c->current_vectors, c );

			// Pass the new vector data into the frame
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "motion_est.vectors",
					 (void*)c->current_vectors, c->mv_size, NULL, NULL );

			// Cache the frame's image. Save the old cache. Reuse memory.
			// After this block, exactly two unique frames will be cached
			uint8_t *timg = c->cache_image;
			c->cache_image = c->former_image;
			c->former_image = timg;
			memcpy( c->cache_image, *image, *width * *height * c->xstride );


		}
		else {
			// Undo the Swap, This fixes the ugliness caused by a duplicate frame
			temp = c->current_vectors;
			c->current_vectors = c->former_vectors;
			c->former_vectors = temp;
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "motion_est.vectors",
					 (void*)c->former_vectors, c->mv_size, NULL, NULL );
		}


		if( c->shot_change == 1)
			;
		else if( c->show_reconstruction )
			show_reconstruction( *image, c );
		else if( c->show_residual )
			show_residual( *image, c );

	}
	// paused
	else if( c->former_frame_position == c->current_frame_position )
	{
		// Pass the old vector data into the frame if it's valid
		if( c->former_vectors_valid == 1 ) {
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "motion_est.vectors",
					 (void*)c->current_vectors, c->mv_size, NULL, NULL );

			if( c->shot_change == 1)
				;
			else if( c->toggle_when_paused == 1 ) {
				if( c->show_reconstruction )
					show_reconstruction( *image, c );
				else if( c->show_residual )
					show_residual( *image, c );
				c->toggle_when_paused = 2;
			}
			else if( c->toggle_when_paused == 2 )
				c->toggle_when_paused = 1;
			else {
				if( c->show_reconstruction )
					show_reconstruction( *image, c );
				else if( c->show_residual )
					show_residual( *image, c );
			}

		}

		mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "shot_change", c->shot_change);
	}
	// there was jump in frame number
	else {
//		fprintf(stderr, "Warning: there was a frame number jumped from %d to %d.\n", c->former_frame_position, c->current_frame_position);
		c->former_vectors_valid = 0;
	}


	// Cache our bounding geometry for the next frame's processing
	c->prev_left_mb = c->left_mb;
	c->prev_top_mb = c->top_mb;
	c->prev_right_mb = c->right_mb;
	c->prev_bottom_mb = c->bottom_mb;

	// Remember which frame this is
	c->former_frame_position = c->current_frame_position;

	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "motion_est.macroblock_width", c->mb_w );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "motion_est.macroblock_height", c->mb_h );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "motion_est.left_mb", c->left_mb );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "motion_est.right_mb", c->right_mb );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "motion_est.top_mb", c->top_mb );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "motion_est.bottom_mb", c->bottom_mb );

	#ifdef BENCHMARK
	struct timeval finish; gettimeofday(&finish, NULL ); int difference = (finish.tv_sec - start.tv_sec) * 1000000 + (finish.tv_usec - start.tv_usec);
	fprintf(stderr, " in frame %d:%d usec\n", c->current_frame_position, difference);
	#endif

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

	return error;
}



/** filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{

	// Keeps tabs on the filter object
	mlt_frame_push_service( frame, this);

	// Push the frame filter
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/
mlt_filter filter_motion_est_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		// Get the properties object
		mlt_properties properties = MLT_FILTER_PROPERTIES( this );

		// Initialize the motion estimation context
		struct motion_est_context_s *context;
		context = mlt_pool_alloc( sizeof(struct motion_est_context_s) );
		mlt_properties_set_data( properties, "context", (void *)context, sizeof( struct motion_est_context_s ),
					mlt_pool_release, NULL );


		// Register the filter
		this->process = filter_process;

		/* defaults that may be overridden */
		context->mb_w = 16;
		context->mb_h = 16;
		context->skip_prediction = 0;
		context->limit_x = 64;
		context->limit_y = 64;
		context->search_method = DIAMOND_SEARCH;	// FIXME: not used
		context->check_chroma = 0;
		context->denoise = 1;
		context->show_reconstruction = 0;
		context->show_residual = 0;
		context->toggle_when_paused = 0;

		/* reference functions that may have optimized versions */
		context->compare_reference = sad_reference;

		// The rest of the buffers will be initialized when the filter is first processed
		context->initialized = 0;
	}
	return this;
}
