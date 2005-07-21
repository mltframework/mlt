/*
 *	/brief fast motion estimation filter
 *	/author Zachary Drew, Copyright 2005
 *
 *	Currently only uses Gamma data for comparisonon (bug or feature?)
 *	Vector optimization coming soon. 
 *
 *	Vector orientation: The vector data that is generated for the current frame specifies
 *	the motion from the previous frame to the current frame. Thus, to know how a macroblock
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include "filter_motion_est.h"
#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>

#include "sad_sse.h"


#undef DEBUG
#undef DEBUG_ASM
#undef BENCHMARK
#undef COUNT_COMPARES

#define DIAMOND_SEARCH 0x0
#define FULL_SEARCH 0x1
#define SHIFT 8 
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define ABS(a) ((a) >= 0 ? (a) : (-(a)))

#ifdef COUNT_COMPARES
	int compares;
#endif

typedef struct motion_vector_s motion_vector;

struct yuv_data
{
	uint8_t *y;
	uint8_t *u;
	uint8_t *v;

};

struct motion_est_context_s
{
	int initialized;				//<! true if filter has been initialized

	/* same as mlt_frame's parameters */
	int width, height;

	/* Operational details */
	int macroblock_width, macroblock_height;
	int xstride, ystride;
	//uint8_t *former_image;			//<! Copy of the previous frame's image
	struct yuv_data former_image, current_image;
	int search_method, skip_prediction, shot_change;
	int limit_x, limit_y;			//<! max x and y of a motion vector
	int edge_blocks_x, edge_blocks_y;
	int initial_thresh;
	int check_chroma;			// if check_chroma == 1 then compare chroma

	/* bounds */
	struct mlt_geometry_item_s prev_bounds;	// Cache last frame's bounds (needed for predictor vectors validity)
	struct mlt_geometry_item_s *bounds;	// Current bounds

	/* bounds in macroblock units */
	int left_mb, prev_left_mb, right_mb, prev_right_mb;
	int top_mb, prev_top_mb, bottom_mb, prev_bottom_mb;

	/* size of our vector buffers */
	int mv_buffer_height, mv_buffer_width, mv_size;

	/* vector buffers */
	int former_vectors_valid;		//<! true if the previous frame's buffered motion vector data is valid
	motion_vector *former_vectors, *current_vectors;
	motion_vector *denoise_vectors;
	mlt_position former_frame_position, current_frame_position;

	/* two metrics for diagnostics. lower is a better estimation but beware of local minima  */
	float predictive_misses;		// How often do the prediction metion vectors fail?
	int comparison_average;			// How far does the best estimation deviate from a perfect comparison?
	int bad_comparisons;
	int average_length;
	int average_x, average_y;

	/* run-time configurable comparison functions */
	int (*compare_reference)(uint8_t *, uint8_t *, int, int, int, int);
	int (*compare_optimized)(uint8_t *, uint8_t *, int, int, int, int);
	//int (*vert_deviation_reference)(uint8_t *, int, int, int, int);
	//int (*horiz_deviation_reference)(uint8_t *, int, int, int, int);

};


// Clip the macroblocks as required. Only used for blocks at the edge of the picture
// "from" is assumed to be unclipped
inline static int clip( int *from_x,
			int *from_y,
			int *to_x,
			int *to_y,
			int *w,			//<! macroblock width
			int *h,			//<! macroblock height
			int width,		//<! image width
			int height)		//<! image height
{

	uint32_t penalty = 1 << SHIFT;	// Retain a few extra bits of precision minus floating-point's blemishes
	int diff;

	// Origin of macroblock moves left of absolute boundy
	if( *to_x < 0 ) {
		if( *to_x + *w <= 0) return 0;			// Clipped out of existance
		penalty = (*w * penalty) / (*w + *to_x);	 // Recipricol of the fraction of the block that remains
		*from_x -= *to_x;
		*w += *to_x; 
		*to_x = 0;
	}
	// Portion of macroblock moves right of absolute boundry
	else if( *to_x + *w > width ) {
		if(*to_x >= width) return 0;			// Clipped out of existance
		diff  = *to_x + *w - width;			// Width of area clipped (0 < diff <  macroblock width)
		penalty = (*w * penalty) / (*w - diff);		// Recipricol of the fraction of the block that remains
		*w -= diff;
	}
	// Origin of macroblock moves above absolute boundy
	if( *to_y < 0 ) {
		if( *to_y + *h <= 0) return 0;			// Clipped out of existance
		penalty = (*h * penalty) / (*h + *to_y);	// Recipricol of the fraction of the block that remains
		*from_y -= *to_y;
		*h += *to_y;
		*to_y = 0;
	}
	// Portion of macroblock moves bellow absolute boundry
	else if( *to_y + *h > height ) {
		if(*to_y >= height) return 0;			// Clipped out of existance
		diff = *to_y + *h - height;			// Height of area clipped (0 < diff < macroblock height)
		penalty = (*h * penalty) / (*h - diff);		// Recipricol of the fraction of the block that is clipped
		*h -= diff;
	}
	return penalty;
}


/** /brief Reference Sum of Absolute Differences comparison function
*
*/
inline static int sad_reference( uint8_t *block1, uint8_t *block2, int xstride, int ystride, int w, int h )
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

inline static void change_422_to_444_planar_rep( uint8_t *image, struct yuv_data yuv, struct motion_est_context_s *c )
{
	register uint8_t *p = image;
	register uint8_t *q = image + c->width * c->height * 2;
	while ( *p != *q ) {
		*(yuv.y++) = *(p ++);
		*(yuv.u++) = *p;
		*(yuv.u++) = *(p ++);
		*(yuv.y++) = *(p ++);
		*(yuv.v++) = *p;
		*(yuv.v++) = *(p ++);
	}
}

// broken
inline static void change_420p_to_444_planar_rep( uint8_t *image, struct yuv_data yuv, struct motion_est_context_s *c )
{
	uint8_t *p = image + c->width * c->height;
	uint8_t *q = p + c->width*c->height/2;
	uint8_t *u2, *v2;
	while( *p != *q ) {
		u2 = yuv.u + c->width;
		*yuv.u  ++ = *p;
		*yuv.u  ++ = *p;
		*u2 ++ = *p;
		*u2 ++ = *p ++;
	}

	*q += c->width*c->height/2;
	while( *p != *q ) {
		v2 = yuv.v + c->width;
		*yuv.v  ++ = *p;
		*yuv.v  ++ = *p;
		*v2 ++ = *p;
		*v2 ++ = *p ++;
	}

}

/** /brief Abstracted block comparison function
*/
inline static int compare( uint8_t *from,
			   uint8_t *to,
			   int from_x,
			   int from_y,
			   int to_x,
			   int to_y,
			   struct motion_est_context_s *c)
{
#ifdef COUNT_COMPARES
	compares++;
#endif

	if( ABS(from_x - to_x) >= c->limit_x || ABS(from_y - to_y) >= c->limit_y )
		return MAX_MSAD;

	int score;
	int (*cmp)(uint8_t *, uint8_t *, int, int, int, int) = c->compare_optimized;

	int mb_w = c->macroblock_width;
	int mb_h = c->macroblock_height;

	int penalty = clip(&from_x, &from_y, &to_x, &to_y, &mb_w, &mb_h, c->width, c->height);
	if ( penalty == 1<<SHIFT)
	    penalty = clip(&to_x, &to_y, &from_x, &from_y, &mb_w, &mb_h, c->width, c->height);

	if( penalty == 0 )			// Clipped out of existance
		return MAX_MSAD;
	else if( penalty != 1<<SHIFT )		// SIMD optimized comparison won't work
		cmp = c->compare_reference;

	uint8_t *from_block = from + from_x * c->xstride + from_y * c->ystride; 
	uint8_t *to_block = to + to_x * c->xstride + to_y * c->ystride; 

	#ifdef DEBUG_ASM	
	if( penalty == 1<<SHIFT ){
		score = c->compare_reference( from_block, to_block, c->xstride, c->ystride, mb_w, mb_h );
		int score2 = c->compare_optimized( from_block, to_block, c->xstride, c->ystride, mb_w, mb_h );
		if ( score != score2 )
			fprintf(stderr, "Your assembly doesn't work! Reference: %d Asm: %d\n", score, score2);
	}
	else
	#endif

	score = cmp( from_block, to_block, c->xstride, c->ystride, mb_w, mb_h );

	return ( score * penalty ) >> SHIFT;			// The extra precision is no longer wanted
}

static inline void check_candidates (   struct yuv_data *from, struct yuv_data *to,
					int from_x, int from_y,
					motion_vector *candidates, int count, int unique,
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
			score = compare( from->y, to->y, from_x, from_y,
					 from_x + candidates[i].dx,	/* to x */
					 from_y + candidates[i].dy,	/* to y */
					 c);

			if ( c->check_chroma ) {
				if ( score >= result->msad )	// Early term
					continue;

				// Chroma - U
				score += compare( from->u, to->u, from_x, from_y,
						 from_x + candidates[i].dx,	/* to x */
						 from_y + candidates[i].dy,	/* to y */
						 c);
	
				if ( score >= result->msad )	// Early term
					continue;
	
				// Chroma - V
				score += compare( from->v, to->v, from_x, from_y,
						 from_x + candidates[i].dx,	/* to x */
						 from_y + candidates[i].dy,	/* to y */
						 c);
			}

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
			struct yuv_data *from,			//<! Image data from previous frame
			struct yuv_data *to,			//<! Image data in current frame
			int mb_x,			//<! X upper left corner of macroblock
			int mb_y,			//<! U upper left corner of macroblock
			struct motion_vector_s *result,	//<! Best predicted mv and eventual result
			struct motion_est_context_s *c)	//<! motion estimation context
{

	// diamond search pattern
	motion_vector candidates[4];

	// Keep track of best and former best candidates
	motion_vector best, former;

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
	
		check_candidates ( from, to, mb_x, mb_y, candidates, i, 1, result, c ); 
		best.dx = result->dx - current.dx;
		best.dy = result->dy - current.dy;

		if ( best.dx == 0 && best.dy == 0 )
			return;

		if ( first == 1 ){
			first = 0;
			former.dx = best.dx; former.dy = best.dy; // First iteration, sensible value for former_d*
		}
	}
}

/* /brief Full (brute) search 
* Operates on a single macroblock
*/
static void full_search(
			struct yuv_data *from,			//<! Image data from previous frame
			struct yuv_data *to,			//<! Image data in current frame
			int mb_x,			//<! X upper left corner of macroblock
			int mb_y,			//<! U upper left corner of macroblock
			struct motion_vector_s *result,	//<! Best predicted mv and eventual result
			struct motion_est_context_s *c)	//<! motion estimation context
{
	// Keep track of best candidate
	int i,j,score;

	// Go loopy
	for( i = -c->macroblock_width; i <= c->macroblock_width; i++ ){
		for( j = -c->macroblock_height; j <=  c->macroblock_height; j++ ){

			score = compare( from->y, to->y,
					 mb_x,		/* from x */
					 mb_y,		/* from y */
					 mb_x + i,	/* to x */
					 mb_y + j,	/* to y */
					 c);		/* context */

			if ( score < result->msad ) {
				result->dx = i;
				result->dy = j;
				result->msad = score;
			}
		}
	}
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

#if 0
inline static int vertical_gradient_reference( uint8_t *block, int xstride, int ystride, int w, int h )
{
	int i, j, average, deviation = 0;
	for ( i = 0; i < w; i++ ){
		average = 0;
		for ( j = 0; j < h; j++ ){
			average += *(block + i*xstride + j*ystride);
		}
		average /= h;
		for ( j = 0; j < h; j++ ){
			deviation += ABS(average - block[i*xstride + j*ystride]);
		}
	}

	return deviation;
}
#endif

#if 0
inline static int horizontal_gradient_reference( uint8_t *block, int xstride, int ystride, int w, int h )
{
	int i, j, average, deviation = 0;
	for ( j = 0; j < h; j++ ){
		average = 0;
		for ( i = 0; i < w; i++ ){
			average += block[i*xstride + j*ystride];
		}
		average /= w;
		for ( i = 0; i < w; i++ ){
			deviation += ABS(average - block[i*xstride + j*ystride]);
		}
	}

	return deviation;
}
#endif

// Macros for pointer calculations
#define CURRENT(i,j)	( c->current_vectors + (j)*c->mv_buffer_width + (i) )
#define FORMER(i,j)	( c->former_vectors + (j)*c->mv_buffer_width + (i) )

#if 0
void collect_pre_statistics( struct motion_est_context_s *c, uint8_t *image ) {

	int i, j, count = 0;
	uint8_t *p;

	for ( i = c->left_mb; i <= c->right_mb; i++ ){
	 for ( j = c->top_mb; j <= c->bottom_mb; j++ ){  
		count++;
		p = image + i * c->macroblock_width * c->xstride + j * c->macroblock_height * c->ystride;
		CURRENT(i,j)->vert_dev = c->vert_deviation_reference( p, c->xstride, c->ystride, c->macroblock_width, c->macroblock_height );
		CURRENT(i,j)->horiz_dev = c->horiz_deviation_reference( p, c->xstride, c->ystride, c->macroblock_width, c->macroblock_height );
	 }
	}
}
#endif

static void median_denoise( motion_vector *v, struct motion_est_context_s *c )
{
//	for ( int i = 0; i++

}

/** /brief Motion search
*
*
* Search for the Vector that best represents the motion *from the last frame *to the current frame
* Vocab: Colocated - the pixel in the previous frame at the current position
*
* Based on enhanced predictive zonal search. [Tourapis 2002]
*/
static void search( struct yuv_data from,			//<! Image data. Motion vector source in previous frame
		    struct yuv_data to,			//<! Image data. Motion vector destination current
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

		int from_x = i * c->macroblock_width;
		int from_y = j * c->macroblock_height;
		check_candidates ( &from, &to, from_x, from_y, candidates, n, 0, here, c ); 


#ifndef FULLSEARCH
		diamond_search( &from, &to, from_x, from_y, here, c); 
#else
		full_search( from, to, from_x, from_y, here, c); 
#endif


		/* Do things in Reverse
		 * Check for occlusions. A block from last frame becomes obscured this frame.
		 * A bogus motion vector will result. To look for this, run the search in reverse
		 * and see if the vector is good backwards and forwards. Most occlusions won't be.
		 * The new source block may not correspond exactly to blocks in the vector buffer
		 * The opposite case, a block being revealed is inherently ignored.
		 */
#if 0
		if ( here->msad < c->initial_thresh )		// The vector is probably good.
			continue;

		struct motion_vector_s reverse;
		reverse.dx = -here->dx;	
		reverse.dy = -here->dy;
		reverse.msad = here->msad;

		// calculate psuedo block coordinates
		from_x += here->dx;
		from_y += here->dy;

		n = 0;
#endif

		// Calculate the real block closest to our psuedo block
#if 0
		int ri = ( ABS( here->dx ) + c->macroblock_width/2 ) / c->macroblock_width;
		if ( ri != 0 ) ri *= here->dx / ABS(here->dx);	// Recover sign
		ri += i;
		if ( ri < 0 ) ri = 0;
		else if ( ri >= c->mv_buffer_width ) ri = c->mv_buffer_width;

		int rj = ( ABS( here->dy ) + c->macroblock_height/2 ) / c->macroblock_height;
		if ( rj != 0 ) rj *= here->dy / ABS(here->dy);	// Recover sign
		rj += j;
		if ( rj < 0 ) rj = 0;
		else if ( rj >= c->mv_buffer_height ) rj = c->mv_buffer_height;

		/* Adjacent to collocated */
		if( c->former_vectors_valid )
		{
			// Top of colocated
			if( rj > c->prev_top_mb ){// && COL_TOP->valid ){
				candidates[n  ].dx = -FORMER(ri,rj-1)->dx;
				candidates[n++].dy = -FORMER(ri,rj-1)->dy;
			}
	
			// Left of colocated
			if( ri > c->prev_left_mb ){// && COL_LEFT->valid ){
				candidates[n  ].dx = -FORMER(ri-1,rj)->dx;
				candidates[n++].dy = -FORMER(ri-1,rj)->dy;
			}
	
			// Right of colocated
			if( ri < c->prev_right_mb ){// && COL_RIGHT->valid ){
				candidates[n  ].dx = -FORMER(ri+1,rj)->dx;
				candidates[n++].dy = -FORMER(ri+1,rj)->dy;
			}
	
			// Bottom of colocated
			if( rj < c->prev_bottom_mb ){// && COL_BOTTOM->valid ){
				candidates[n  ].dx = -FORMER(ri,rj+1)->dx;
				candidates[n++].dy = -FORMER(ri,rj+1)->dy;
			}

			// And finally, colocated
			candidates[n  ].dx = -FORMER(ri,rj)->dx;
			candidates[n++].dy = -FORMER(ri,rj)->dy;
		}
#endif
#if 0
		// Zero vector
		candidates[n].dx = 0;
		candidates[n++].dy = 0;

		check_candidates ( &to, &from, from_x, from_y, candidates, 1, 1, &reverse, c ); 

		/* Scan for the best candidate */
		while( n ) {
			n--;

			score = compare( to, from, from_x, from_y,	/* to and from are reversed */
					 from_x + candidates[n].dx,	/* to x */
					 from_y + candidates[n].dy,	/* to y */
					 c);				/* context */

			if ( score < reverse.msad ) {
				reverse.dx = candidates[n].dx;
				reverse.dy = candidates[n].dy;
				reverse.msad = score;
				if ( score < c->initial_thresh )
					n=0;		// Simplified version of early termination thresh
			}
		}

//		if ( reverse.msad == here->msad)	// If nothing better was found
//		{					// this is an opportunity
//							// to skip 4 block comparisons
//			continue;			// in the diamond search
//		}


		diamond_search( &to, &from, from_x, from_y, &reverse, c); /* to and from are reversed */

		if ( ABS( reverse.dx + here->dx ) + ABS( reverse.dy + here->dy ) > 5  )
//		if ( here->msad > reverse.msad + c->initial_thresh*10   )
		{
			here->valid = 2;
		}

#endif
	 } /* End column loop */
	} /* End row loop */

	asm volatile ( "emms" );

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
	if ( c->check_chroma ) {
		switch(c->macroblock_width){
			case 8:  if(c->macroblock_height == 8)  c->compare_optimized = sad_sse_8x8;
				 else				c->compare_optimized = sad_sse_8w;
				 break;
			case 16: if(c->macroblock_height == 16) c->compare_optimized = sad_sse_16x16;
				 else				c->compare_optimized = sad_sse_16w;
				 break;
			case 32: if(c->macroblock_height == 32) c->compare_optimized = sad_sse_32x32;
				 else				c->compare_optimized = sad_sse_32w;
				 break;
			case 64: c->compare_optimized = sad_sse_64w;
				 break;
			default: c->compare_optimized = sad_reference;
				 break;
		}
	}
	else
	{
		switch(c->macroblock_width){
			case 4:  if(c->macroblock_height == 4)	c->compare_optimized = sad_sse_422_luma_4x4;
				 else				c->compare_optimized = sad_sse_422_luma_4w;
				 break;
			case 8:  if(c->macroblock_height == 8)  c->compare_optimized = sad_sse_422_luma_8x8;
				 else				c->compare_optimized = sad_sse_422_luma_8w;
				 break;
			case 16: if(c->macroblock_height == 16) c->compare_optimized = sad_sse_422_luma_16x16;
				 else				c->compare_optimized = sad_sse_422_luma_16w;
				 break;
			case 32: if(c->macroblock_height == 32) c->compare_optimized = sad_sse_422_luma_32x32;
				 else				c->compare_optimized = sad_sse_422_luma_32w;
				 break;
			case 64: c->compare_optimized = sad_sse_422_luma_64w;
				 break;
			default: c->compare_optimized = sad_reference;
				 break;
		}
	}
}
	
// Image stack(able) method
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the filter
	mlt_filter filter = mlt_frame_pop_service( frame );

	// Get the motion_est context object
	struct motion_est_context_s *c = mlt_properties_get_data( MLT_FILTER_PROPERTIES( filter ), "context", NULL);

	// Get the new image and frame number
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

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
			c->macroblock_width = mlt_properties_get_int( properties, "macroblock_width");

		if( mlt_properties_get( properties, "macroblock_height") != NULL )
			c->macroblock_height = mlt_properties_get_int( properties, "macroblock_height");

		if( mlt_properties_get( properties, "prediction_thresh") != NULL )
			c->initial_thresh = mlt_properties_get_int( properties, "prediction_thresh" );
		else
			c->initial_thresh = c->macroblock_width * c->macroblock_height;

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

		init_optimizations( c );

		// Calculate the dimensions in macroblock units
		c->mv_buffer_width = (*width / c->macroblock_width);
		c->mv_buffer_height = (*height / c->macroblock_height);

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

		// Figure out how many blocks should be considered edge blocks
		c->edge_blocks_x = (c->limit_x + c->macroblock_width - 1) / c->macroblock_width;
		c->edge_blocks_y = (c->limit_y + c->macroblock_height - 1) / c->macroblock_height;

		// Calculate the size of our steps (the number of bytes that seperate adjacent pixels in X and Y direction)
		switch( *format ) {
			case mlt_image_yuv422:
				if ( c->check_chroma )
					c->xstride = 1;
				else
					c->xstride = 2;
				c->ystride = c->xstride * *width;
				break; 
/*			case mlt_image_yuv420p:
				c->xstride = 1;
				c->ystride = c->xstride * *width;
				break;
*/			default:
				// I don't know
				fprintf(stderr, "\"I am unfamiliar with your new fangled pixel format!\" -filter_motion_est\n");
				return -1;
		}

		if ( c->check_chroma ) {
			// Allocate memory for the 444 images
			c->former_image.y = mlt_pool_alloc( *width * *height * 3 );
			c->current_image.y = mlt_pool_alloc( *width * *height * 3 );
			c->current_image.u = c->current_image.y + *width * *height;
			c->current_image.v = c->current_image.u + *width * *height;
			c->former_image.u = c->former_image.y + *width * *height;
			c->former_image.v = c->former_image.u + *width * *height;
			// Register for destruction
			mlt_properties_set_data( properties, "current_image", (void *)c->current_image.y, 0, mlt_pool_release, NULL );
		}
		else
		{
			c->former_image.y = mlt_pool_alloc( *width * *height * 2 );
		}
		// Register for destruction
		mlt_properties_set_data( properties, "former_image", (void *)c->former_image.y, 0, mlt_pool_release, NULL );


		c->former_frame_position = c->current_frame_position;

		c->initialized = 1;
	}

	/* Check to see if somebody else has given us bounds */
	c->bounds = mlt_properties_get_data( MLT_FRAME_PROPERTIES( frame ), "bounds", NULL );

	/* no bounds were given, they won't change next frame, so use a convient storage place */
	if( c->bounds == NULL ) {
		c->bounds = &c->prev_bounds;
		c->bounds->x = 0;
		c->bounds->y = 0;
		c->bounds->w = *width - 1;	// Zero indexed
		c->bounds->h = *height - 1;	// Zero indexed
	}

	// translate pixel units (from bounds) to macroblock units
	// make sure whole macroblock stays within bounds
	c->left_mb = ( c->bounds->x + c->macroblock_width - 1 ) / c->macroblock_width;
	c->top_mb = ( c->bounds->y + c->macroblock_height - 1 ) / c->macroblock_height;
	c->right_mb = ( c->bounds->x + c->bounds->w ) / c->macroblock_width - 1;
	c->bottom_mb = ( c->bounds->y + c->bounds->h ) / c->macroblock_height - 1;

	// Do the same thing for the previous frame's geometry
	// This will be used for determining validity of predictors
	c->prev_left_mb = ( c->prev_bounds.x + c->macroblock_width - 1) / c->macroblock_width;
	c->prev_top_mb = ( c->prev_bounds.y + c->macroblock_height - 1) / c->macroblock_height;
	c->prev_right_mb = ( c->prev_bounds.x + c->prev_bounds.w ) / c->macroblock_width - 1;
	c->prev_bottom_mb = ( c->prev_bounds.y + c->prev_bounds.h ) / c->macroblock_height - 1;

	
	// If video is advancing, run motion vector algorithm and etc...	
	if( c->former_frame_position + 1 == c->current_frame_position )
	{
		#ifdef BENCHMARK
		struct timeval start; gettimeofday(&start, NULL );
		#endif

		// Swap the motion vector buffers and reuse allocated memory
		struct motion_vector_s *temp = c->current_vectors;
		c->current_vectors = c->former_vectors;
		c->former_vectors = temp;

		// Swap the image buffers
		if ( c->check_chroma ) {
			uint8_t *temp_yuv;
			temp_yuv = c->current_image.y;
			c->current_image.y = c->former_image.y;
			c->former_image.y = temp_yuv;
			temp_yuv = c->current_image.u;
			c->current_image.u = c->former_image.u;
			c->former_image.u = temp_yuv;
			temp_yuv = c->current_image.v;
			c->current_image.v = c->former_image.v;
			c->former_image.v = temp_yuv;

			switch ( *format ) {
				case mlt_image_yuv422:
					change_422_to_444_planar_rep( *image, c->current_image, c );
					break;
				case mlt_image_yuv420p:
					change_420p_to_444_planar_rep( *image, c->current_image, c );
					break;
				default:
					break;
			}
		}
		else
			c->current_image.y = *image;

		// Find a better place for this
		memset( c->current_vectors, 0, c->mv_size );

		// Perform the motion search

		//collect_pre_statistics( context, *image );
		search( c->current_image, c->former_image, c );

		//median_denoise( c->current_vectors, c );

		collect_post_statistics( c );

		#ifdef BENCHMARK
		struct timeval finish; gettimeofday(&finish, NULL ); int difference = (finish.tv_sec - start.tv_sec) * 1000000 + (finish.tv_usec - start.tv_usec);
		fprintf(stderr, " in frame %d:%d usec\n", c->current_frame_position, difference);
		#endif



		// Detect shot changes
		if( c->comparison_average > 12 * c->macroblock_width * c->macroblock_height ) {
			//fprintf(stderr, " - SAD: %d   <<Shot change>>\n", c->comparison_average);
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "shot_change", 1);
		//	c->former_vectors_valid = 0; // Invalidate the previous frame's predictors
			c->shot_change = 1;
		}
		else {
			c->former_vectors_valid = 1;
			c->shot_change = 0;
			//fprintf(stderr, " - SAD: %d\n", c->comparison_average);
		}

		if( c->comparison_average != 0 ) {
			// Pass the new vector data into the frame
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "motion_est.vectors",
					 (void*)c->current_vectors, c->mv_size, NULL, NULL );

		}
		else {
			// This fixes the ugliness caused by a duplicate frame
			temp = c->current_vectors;
			c->current_vectors = c->former_vectors;
			c->former_vectors = temp;
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "motion_est.vectors",
					 (void*)c->former_vectors, c->mv_size, NULL, NULL );
		}

	}
	// paused
	else if( c->former_frame_position == c->current_frame_position )
	{
		// Pass the old vector data into the frame if it's valid
		if( c->former_vectors_valid == 1 )
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "motion_est.vectors",
					 (void*)c->current_vectors, c->mv_size, NULL, NULL );

		mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "shot_change", c->shot_change);
	}
	// there was jump in frame number
	else
		c->former_vectors_valid = 0;


	// Cache our bounding geometry for the next frame's processing
	if( c->bounds != &c->prev_bounds )
		memcpy( &c->prev_bounds, c->bounds, sizeof( struct mlt_geometry_item_s ) );

	// Remember which frame this is
	c->former_frame_position = c->current_frame_position;

	if ( c->check_chroma == 0 )
		memcpy( c->former_image.y, *image, *width * *height * c->xstride );

	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "motion_est.macroblock_width", c->macroblock_width );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "motion_est.macroblock_height", c->macroblock_height );

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
mlt_filter filter_motion_est_init( char *arg )
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
		context->macroblock_width = 16;
		context->macroblock_height = 16;
		context->skip_prediction = 0;
		context->limit_x = 64;
		context->limit_y = 64;
		context->search_method = DIAMOND_SEARCH;
		context->check_chroma = 0;

		/* reference functions that may have optimized versions */
		context->compare_reference = sad_reference;
		//context->vert_deviation_reference = vertical_gradient_reference;
		//context->horiz_deviation_reference = horizontal_gradient_reference;

		// The rest of the buffers will be initialized when the filter is first processed
		context->initialized = 0;
	}
	return this;
}

/** This source code will self destruct in 5...4...3... */
