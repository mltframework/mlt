 /*
 * Copyright (C) 2001 the xine project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Deinterlace routines by Miguel Freitas
 * based of DScaler project sources (deinterlace.sourceforge.net)
 *
 * Currently only available for Xv driver and MMX extensions
 *
 * small todo list:
 * - implement non-MMX versions for all methods
 * - support MMX2 instructions
 * - move some generic code from xv driver to this file
 * - make it also work for yuy2 frames
 *
 */

#include <stdio.h>
#include <string.h>
#include "deinterlace.h"
#include "xineutils.h"

#define xine_fast_memcpy memcpy
#define xine_fast_memmove memmove

/*
   DeinterlaceFieldBob algorithm
   Based on Virtual Dub plugin by Gunnar Thalin
   MMX asm version from dscaler project (deinterlace.sourceforge.net)
   Linux version for Xine player by Miguel Freitas
*/
static void deinterlace_bob_yuv_mmx( uint8_t *pdst, uint8_t *psrc[],
    int width, int height )
{
#ifdef USE_MMX
  int Line;
  uint64_t *YVal1;
  uint64_t *YVal2;
  uint64_t *YVal3;
  uint64_t *Dest;
  uint8_t* pEvenLines = psrc[0];
  uint8_t* pOddLines = psrc[0]+width;
  int LineLength = width;
  int SourcePitch = width * 2;
  int IsOdd = 1;
  long EdgeDetect = 625;
  long JaggieThreshold = 73;

  int n;

  uint64_t qwEdgeDetect;
  uint64_t qwThreshold;

  static mmx_t YMask = {.ub={0xff,0,0xff,0,0xff,0,0xff,0}};
  static mmx_t Mask = {.ub={0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe}};

  qwEdgeDetect = EdgeDetect;
  qwEdgeDetect += (qwEdgeDetect << 48) + (qwEdgeDetect << 32) + (qwEdgeDetect << 16);
  qwThreshold = JaggieThreshold;
  qwThreshold += (qwThreshold << 48) + (qwThreshold << 32) + (qwThreshold << 16);


  // copy first even line no matter what, and the first odd line if we're
  // processing an odd field.
  xine_fast_memcpy(pdst, pEvenLines, LineLength);
  if (IsOdd)
    xine_fast_memcpy(pdst + LineLength, pOddLines, LineLength);

  height = height / 2;
  for (Line = 0; Line < height - 1; ++Line)
  {
    if (IsOdd)
    {
      YVal1 = (uint64_t *)(pOddLines + Line * SourcePitch);
      YVal2 = (uint64_t *)(pEvenLines + (Line + 1) * SourcePitch);
      YVal3 = (uint64_t *)(pOddLines + (Line + 1) * SourcePitch);
      Dest = (uint64_t *)(pdst + (Line * 2 + 2) * LineLength);
    }
    else
    {
      YVal1 = (uint64_t *)(pEvenLines + Line * SourcePitch);
      YVal2 = (uint64_t *)(pOddLines + Line * SourcePitch);
      YVal3 = (uint64_t *)(pEvenLines + (Line + 1) * SourcePitch);
      Dest = (uint64_t *)(pdst + (Line * 2 + 1) * LineLength);
    }

    // For ease of reading, the comments below assume that we're operating on an odd
    // field (i.e., that bIsOdd is true).  The exact same processing is done when we
    // operate on an even field, but the roles of the odd and even fields are reversed.
    // It's just too cumbersome to explain the algorithm in terms of "the next odd
    // line if we're doing an odd field, or the next even line if we're doing an
    // even field" etc.  So wherever you see "odd" or "even" below, keep in mind that
    // half the time this function is called, those words' meanings will invert.

    // Copy the odd line to the overlay verbatim.
    xine_fast_memcpy((char *)Dest + LineLength, YVal3, LineLength);

    n = LineLength >> 3;
    while( n-- )
    {
      movq_m2r (*YVal1++, mm0);
      movq_m2r (*YVal2++, mm1);
      movq_m2r (*YVal3++, mm2);

      // get intensities in mm3 - 4
      movq_r2r ( mm0, mm3 );
      pand_m2r ( YMask, mm3 );
      movq_r2r ( mm1, mm4 );
      pand_m2r ( YMask, mm4 );
      movq_r2r ( mm2, mm5 );
      pand_m2r ( YMask, mm5 );

      // get average in mm0
      pand_m2r ( Mask, mm0 );
      pand_m2r ( Mask, mm2 );
      psrlw_i2r ( 01, mm0 );
      psrlw_i2r ( 01, mm2 );
      paddw_r2r ( mm2, mm0 );

      // work out (O1 - E) * (O2 - E) / 2 - EdgeDetect * (O1 - O2) ^ 2 >> 12
      // result will be in mm6

      psrlw_i2r ( 01, mm3 );
      psrlw_i2r ( 01, mm4 );
      psrlw_i2r ( 01, mm5 );

      movq_r2r ( mm3, mm6 );
      psubw_r2r ( mm4, mm6 );	//mm6 = O1 - E

      movq_r2r ( mm5, mm7 );
      psubw_r2r ( mm4, mm7 );	//mm7 = O2 - E

      pmullw_r2r ( mm7, mm6 );		// mm6 = (O1 - E) * (O2 - E)

      movq_r2r ( mm3, mm7 );
      psubw_r2r ( mm5, mm7 );		// mm7 = (O1 - O2)
      pmullw_r2r ( mm7, mm7 );	// mm7 = (O1 - O2) ^ 2
      psrlw_i2r ( 12, mm7 );		// mm7 = (O1 - O2) ^ 2 >> 12
      pmullw_m2r ( *&qwEdgeDetect, mm7 );// mm7  = EdgeDetect * (O1 - O2) ^ 2 >> 12

      psubw_r2r ( mm7, mm6 );      // mm6 is what we want

      pcmpgtw_m2r ( *&qwThreshold, mm6 );

      movq_r2r ( mm6, mm7 );

      pand_r2r ( mm6, mm0 );

      pandn_r2r ( mm1, mm7 );

      por_r2r ( mm0, mm7 );

      movq_r2m ( mm7, *Dest++ );
    }
  }

  // Copy last odd line if we're processing an even field.
  if (! IsOdd)
  {
    xine_fast_memcpy(pdst + (height * 2 - 1) * LineLength,
                      pOddLines + (height - 1) * SourcePitch,
                      LineLength);
  }

  // clear out the MMX registers ready for doing floating point
  // again
  emms();
#endif
}

/* Deinterlace the latest field, with a tendency to weave rather than bob.
   Good for high detail on low-movement scenes.
   Seems to produce bad output in general case, need to check if this
   is normal or if the code is broken.
*/
static int deinterlace_weave_yuv_mmx( uint8_t *pdst, uint8_t *psrc[],
    int width, int height )
{
#ifdef USE_MMX

  int Line;
  uint64_t *YVal1;
  uint64_t *YVal2;
  uint64_t *YVal3;
  uint64_t *YVal4;
  uint64_t *Dest;
  uint8_t* pEvenLines = psrc[0];
  uint8_t* pOddLines = psrc[0]+width;
  uint8_t* pPrevLines;

  int LineLength = width;
  int SourcePitch = width * 2;
  int IsOdd = 1;

  long TemporalTolerance = 300;
  long SpatialTolerance = 600;
  long SimilarityThreshold = 25;

  int n;

  uint64_t qwSpatialTolerance;
  uint64_t qwTemporalTolerance;
  uint64_t qwThreshold;

  static mmx_t YMask = {.ub={0xff,0,0xff,0,0xff,0,0xff,0}};
  static mmx_t Mask = {.ub={0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe}};


  // Make sure we have all the data we need.
  if ( psrc[0] == NULL || psrc[1] == NULL )
    return 0;

  if (IsOdd)
    pPrevLines = psrc[1] + width;
  else
    pPrevLines = psrc[1];

  // Since the code uses MMX to process 4 pixels at a time, we need our constants
  // to be represented 4 times per quadword.
  qwSpatialTolerance = SpatialTolerance;
  qwSpatialTolerance += (qwSpatialTolerance << 48) + (qwSpatialTolerance << 32) + (qwSpatialTolerance << 16);
  qwTemporalTolerance = TemporalTolerance;
  qwTemporalTolerance += (qwTemporalTolerance << 48) + (qwTemporalTolerance << 32) + (qwTemporalTolerance << 16);
  qwThreshold = SimilarityThreshold;
  qwThreshold += (qwThreshold << 48) + (qwThreshold << 32) + (qwThreshold << 16);

  // copy first even line no matter what, and the first odd line if we're
  // processing an even field.
  xine_fast_memcpy(pdst, pEvenLines, LineLength);
  if (!IsOdd)
    xine_fast_memcpy(pdst + LineLength, pOddLines, LineLength);

  height = height / 2;
  for (Line = 0; Line < height - 1; ++Line)
  {
    if (IsOdd)
    {
      YVal1 = (uint64_t *)(pEvenLines + Line * SourcePitch);
      YVal2 = (uint64_t *)(pOddLines + Line * SourcePitch);
      YVal3 = (uint64_t *)(pEvenLines + (Line + 1) * SourcePitch);
      YVal4 = (uint64_t *)(pPrevLines + Line * SourcePitch);
      Dest = (uint64_t *)(pdst + (Line * 2 + 1) * LineLength);
    }
    else
    {
      YVal1 = (uint64_t *)(pOddLines + Line * SourcePitch);
      YVal2 = (uint64_t *)(pEvenLines + (Line + 1) * SourcePitch);
      YVal3 = (uint64_t *)(pOddLines + (Line + 1) * SourcePitch);
      YVal4 = (uint64_t *)(pPrevLines + (Line + 1) * SourcePitch);
      Dest = (uint64_t *)(pdst + (Line * 2 + 2) * LineLength);
    }

    // For ease of reading, the comments below assume that we're operating on an odd
    // field (i.e., that bIsOdd is true).  The exact same processing is done when we
    // operate on an even field, but the roles of the odd and even fields are reversed.
    // It's just too cumbersome to explain the algorithm in terms of "the next odd
    // line if we're doing an odd field, or the next even line if we're doing an
    // even field" etc.  So wherever you see "odd" or "even" below, keep in mind that
    // half the time this function is called, those words' meanings will invert.

    // Copy the even scanline below this one to the overlay buffer, since we'll be
    // adapting the current scanline to the even lines surrounding it.  The scanline
    // above has already been copied by the previous pass through the loop.
    xine_fast_memcpy((char *)Dest + LineLength, YVal3, LineLength);

    n = LineLength >> 3;
    while( n-- )
    {
      movq_m2r ( *YVal1++, mm0 );    // mm0 = E1
      movq_m2r ( *YVal2++, mm1 );    // mm1 = O
      movq_m2r ( *YVal3++, mm2 );    // mm2 = E2

      movq_r2r ( mm0, mm3 );       // mm3 = intensity(E1)
      movq_r2r ( mm1, mm4 );       // mm4 = intensity(O)
      movq_r2r ( mm2, mm6 );       // mm6 = intensity(E2)

      pand_m2r ( YMask, mm3 );
      pand_m2r ( YMask, mm4 );
      pand_m2r ( YMask, mm6 );

      // Average E1 and E2 for interpolated bobbing.
      // leave result in mm0
      pand_m2r ( Mask, mm0 ); // mm0 = E1 with lower chroma bit stripped off
      pand_m2r ( Mask, mm2 ); // mm2 = E2 with lower chroma bit stripped off
      psrlw_i2r ( 01, mm0 );    // mm0 = E1 / 2
      psrlw_i2r ( 01, mm2 );    // mm2 = E2 / 2
      paddb_r2r ( mm2, mm0 );

      // The meat of the work is done here.  We want to see whether this pixel is
      // close in luminosity to ANY of: its top neighbor, its bottom neighbor,
      // or its predecessor.  To do this without branching, we use MMX's
      // saturation feature, which gives us Z(x) = x if x>=0, or 0 if x<0.
      //
      // The formula we're computing here is
      //		Z(ST - (E1 - O) ^ 2) + Z(ST - (E2 - O) ^ 2) + Z(TT - (Oold - O) ^ 2)
      // where ST is spatial tolerance and TT is temporal tolerance.  The idea
      // is that if a pixel is similar to none of its neighbors, the resulting
      // value will be pretty low, probably zero.  A high value therefore indicates
      // that the pixel had a similar neighbor.  The pixel in the same position
      // in the field before last (Oold) is considered a neighbor since we want
      // to be able to display 1-pixel-high horizontal lines.

      movq_m2r ( *&qwSpatialTolerance, mm7 );
      movq_r2r ( mm3, mm5 );     // mm5 = E1
      psubsw_r2r ( mm4, mm5 );   // mm5 = E1 - O
      psraw_i2r ( 1, mm5 );
      pmullw_r2r ( mm5, mm5 );   // mm5 = (E1 - O) ^ 2
      psubusw_r2r ( mm5, mm7 );  // mm7 = ST - (E1 - O) ^ 2, or 0 if that's negative

      movq_m2r ( *&qwSpatialTolerance, mm3 );
      movq_r2r ( mm6, mm5 );    // mm5 = E2
      psubsw_r2r ( mm4, mm5 );  // mm5 = E2 - O
      psraw_i2r ( 1, mm5 );
      pmullw_r2r ( mm5, mm5 );  // mm5 = (E2 - O) ^ 2
      psubusw_r2r ( mm5, mm3 ); // mm0 = ST - (E2 - O) ^ 2, or 0 if that's negative
      paddusw_r2r ( mm3, mm7 ); // mm7 = (ST - (E1 - O) ^ 2) + (ST - (E2 - O) ^ 2)

      movq_m2r ( *&qwTemporalTolerance, mm3 );
      movq_m2r ( *YVal4++, mm5 ); // mm5 = Oold
      pand_m2r ( YMask, mm5 );
      psubsw_r2r ( mm4, mm5 );  // mm5 = Oold - O
      psraw_i2r ( 1, mm5 ); // XXX
      pmullw_r2r ( mm5, mm5 );  // mm5 = (Oold - O) ^ 2
      psubusw_r2r ( mm5, mm3 ); /* mm0 = TT - (Oold - O) ^ 2, or 0 if that's negative */
      paddusw_r2r ( mm3, mm7 ); // mm7 = our magic number

      /*
       * Now compare the similarity totals against our threshold.  The pcmpgtw
       * instruction will populate the target register with a bunch of mask bits,
       * filling words where the comparison is true with 1s and ones where it's
       * false with 0s.  A few ANDs and NOTs and an OR later, we have bobbed
       * values for pixels under the similarity threshold and weaved ones for
       * pixels over the threshold.
       */

      pcmpgtw_m2r( *&qwThreshold, mm7 ); // mm7 = 0xffff where we're greater than the threshold, 0 elsewhere
      movq_r2r ( mm7, mm6 );  // mm6 = 0xffff where we're greater than the threshold, 0 elsewhere
      pand_r2r ( mm1, mm7 );  // mm7 = weaved data where we're greater than the threshold, 0 elsewhere
      pandn_r2r ( mm0, mm6 ); // mm6 = bobbed data where we're not greater than the threshold, 0 elsewhere
      por_r2r ( mm6, mm7 );   // mm7 = bobbed and weaved data

      movq_r2m ( mm7, *Dest++ );
    }
  }

  // Copy last odd line if we're processing an odd field.
  if (IsOdd)
  {
    xine_fast_memcpy(pdst + (height * 2 - 1) * LineLength,
                      pOddLines + (height - 1) * SourcePitch,
                      LineLength);
  }

  // clear out the MMX registers ready for doing floating point
  // again
  emms();

#endif

  return 1;
}


// This is a simple lightweight DeInterlace method that uses little CPU time
// but gives very good results for low or intermedite motion. (MORE CPU THAN BOB)
// It defers frames by one field, but that does not seem to produce noticeable
// lip sync problems.
//
// The method used is to take either the older or newer weave pixel depending
// upon which give the smaller comb factor, and then clip to avoid large damage
// when wrong.
//
// I'd intended this to be part of a larger more elaborate method added to
// Blended Clip but this give too good results for the CPU to ignore here.
static int deinterlace_greedy_yuv_mmx( uint8_t *pdst, uint8_t *psrc[],
    int width, int height )
{
#ifdef USE_MMX
  int Line;
  int	LoopCtr;
  uint64_t *L1;					// ptr to Line1, of 3
  uint64_t *L2;					// ptr to Line2, the weave line
  uint64_t *L3;					// ptr to Line3
  uint64_t *LP2;					// ptr to prev Line2
  uint64_t *Dest;
  uint8_t* pEvenLines = psrc[0];
  uint8_t* pOddLines = psrc[0]+width;
  uint8_t* pPrevLines;

  static mmx_t ShiftMask = {.ub={0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe}};

  int LineLength = width;
  int SourcePitch = width * 2;
  int IsOdd = 1;
  long GreedyMaxComb = 15;
  static mmx_t MaxComb;
  int i;

  if ( psrc[0] == NULL || psrc[1] == NULL )
    return 0;

  if (IsOdd)
    pPrevLines = psrc[1] + width;
  else
    pPrevLines = psrc[1];


  for( i = 0; i < 8; i++ )
    MaxComb.ub[i] = GreedyMaxComb; // How badly do we let it weave? 0-255


  // copy first even line no matter what, and the first odd line if we're
  // processing an EVEN field. (note diff from other deint rtns.)
  xine_fast_memcpy(pdst, pEvenLines, LineLength); //DL0
  if (!IsOdd)
    xine_fast_memcpy(pdst + LineLength, pOddLines, LineLength); //DL1

  height = height / 2;
  for (Line = 0; Line < height - 1; ++Line)
  {
    LoopCtr = LineLength / 8;				// there are LineLength / 8 qwords per line

    if (IsOdd)
    {
      L1 = (uint64_t *)(pEvenLines + Line * SourcePitch);
      L2 = (uint64_t *)(pOddLines + Line * SourcePitch);
      L3 = (uint64_t *)(pEvenLines + (Line + 1) * SourcePitch);
      LP2 = (uint64_t *)(pPrevLines + Line * SourcePitch); // prev Odd lines
      Dest = (uint64_t *)(pdst + (Line * 2 + 1) * LineLength);
    }
    else
    {
      L1 = (uint64_t *)(pOddLines + Line * SourcePitch);
      L2 = (uint64_t *)(pEvenLines + (Line + 1) * SourcePitch);
      L3 = (uint64_t *)(pOddLines + (Line + 1) * SourcePitch);
      LP2 = (uint64_t *)(pPrevLines + (Line + 1) * SourcePitch); //prev even lines
      Dest = (uint64_t *)(pdst + (Line * 2 + 2) * LineLength);
    }

    xine_fast_memcpy((char *)Dest + LineLength, L3, LineLength);

// For ease of reading, the comments below assume that we're operating on an odd
// field (i.e., that info->IsOdd is true).  Assume the obvious for even lines..

    while( LoopCtr-- )
    {
      movq_m2r ( *L1++, mm1 );
      movq_m2r ( *L2++, mm2 );
      movq_m2r ( *L3++, mm3 );
      movq_m2r ( *LP2++, mm0 );

      // average L1 and L3 leave result in mm4
      movq_r2r ( mm1, mm4 );	// L1

      pand_m2r ( ShiftMask, mm4 );
      psrlw_i2r ( 01, mm4 );
      movq_r2r ( mm3, mm5 );  // L3
      pand_m2r ( ShiftMask, mm5 );
      psrlw_i2r ( 01, mm5 );
      paddb_r2r ( mm5, mm4 );  // the average, for computing comb

      // get abs value of possible L2 comb
      movq_r2r	( mm2, mm7 );				// L2
      psubusb_r2r ( mm4, mm7 );				// L2 - avg
      movq_r2r ( mm4, mm5 );				// avg
      psubusb_r2r ( mm2, mm5 );				// avg - L2
      por_r2r ( mm7, mm5 );				// abs(avg-L2)
      movq_r2r ( mm4, mm6 );     // copy of avg for later

      // get abs value of possible LP2 comb
      movq_r2r ( mm0, mm7 );				// LP2
      psubusb_r2r ( mm4, mm7 );				// LP2 - avg
      psubusb_r2r ( mm0, mm4 );				// avg - LP2
      por_r2r ( mm7, mm4 );				// abs(avg-LP2)

      // use L2 or LP2 depending upon which makes smaller comb
      psubusb_r2r ( mm5, mm4 );				// see if it goes to zero
      psubusb_r2r ( mm5, mm5 );				// 0
      pcmpeqb_r2r ( mm5, mm4 );				// if (mm4=0) then FF else 0
      pcmpeqb_r2r ( mm4, mm5 );				// opposite of mm4

      // if Comb(LP2) <= Comb(L2) then mm4=ff, mm5=0 else mm4=0, mm5 = 55
      pand_r2r ( mm2, mm5 );				// use L2 if mm5 == ff, else 0
      pand_r2r ( mm0, mm4 );				// use LP2 if mm4 = ff, else 0
      por_r2r ( mm5, mm4 );				// may the best win

      // Now lets clip our chosen value to be not outside of the range
      // of the high/low range L1-L3 by more than abs(L1-L3)
      // This allows some comb but limits the damages and also allows more
      // detail than a boring oversmoothed clip.

      movq_r2r ( mm1, mm2 );				// copy L1
      psubusb_r2r ( mm3, mm2 );				// - L3, with saturation
      paddusb_r2r ( mm3, mm2 );                // now = Max(L1,L3)

      pcmpeqb_r2r ( mm7, mm7 );				// all ffffffff
      psubusb_r2r ( mm1, mm7 );				// - L1
      paddusb_r2r ( mm7, mm3 );				// add, may sat at fff..
      psubusb_r2r ( mm7, mm3 );				// now = Min(L1,L3)

      // allow the value to be above the high or below the low by amt of MaxComb
      paddusb_m2r ( MaxComb, mm2 );			// increase max by diff
      psubusb_m2r ( MaxComb, mm3 );			// lower min by diff

      psubusb_r2r ( mm3, mm4 );				// best - Min
      paddusb_r2r ( mm3, mm4 );				// now = Max(best,Min(L1,L3)

      pcmpeqb_r2r ( mm7, mm7 );				// all ffffffff
      psubusb_r2r ( mm4, mm7 );				// - Max(best,Min(best,L3)
      paddusb_r2r ( mm7, mm2 );				// add may sat at FFF..
      psubusb_r2r ( mm7, mm2 );				// now = Min( Max(best, Min(L1,L3), L2 )=L2 clipped

      movq_r2m ( mm2, *Dest++ );        // move in our clipped best

    }
  }

  /* Copy last odd line if we're processing an Odd field. */
  if (IsOdd)
  {
    xine_fast_memcpy(pdst + (height * 2 - 1) * LineLength,
                      pOddLines + (height - 1) * SourcePitch,
                      LineLength);
  }

  /* clear out the MMX registers ready for doing floating point again */
  emms();

#endif

  return 1;
}

/* Use one field to interpolate the other (low cpu utilization)
   Will lose resolution but does not produce weaving effect
   (good for fast moving scenes) also know as "linear interpolation"
*/
static void deinterlace_onefield_yuv_mmx( uint8_t *pdst, uint8_t *psrc[],
    int width, int height )
{
#ifdef USE_MMX
  int Line;
  uint64_t *YVal1;
  uint64_t *YVal3;
  uint64_t *Dest;
  uint8_t* pEvenLines = psrc[0];
  uint8_t* pOddLines = psrc[0]+width;
  int LineLength = width;
  int SourcePitch = width * 2;
  int IsOdd = 1;

  int n;

  static mmx_t Mask = {.ub={0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe}};

  /*
   * copy first even line no matter what, and the first odd line if we're
   * processing an odd field.
   */

  xine_fast_memcpy(pdst, pEvenLines, LineLength);
  if (IsOdd)
    xine_fast_memcpy(pdst + LineLength, pOddLines, LineLength);

  height = height / 2;
  for (Line = 0; Line < height - 1; ++Line)
  {
    if (IsOdd)
    {
      YVal1 = (uint64_t *)(pOddLines + Line * SourcePitch);
      YVal3 = (uint64_t *)(pOddLines + (Line + 1) * SourcePitch);
      Dest = (uint64_t *)(pdst + (Line * 2 + 2) * LineLength);
    }
    else
    {
      YVal1 = (uint64_t *)(pEvenLines + Line * SourcePitch);
      YVal3 = (uint64_t *)(pEvenLines + (Line + 1) * SourcePitch);
      Dest = (uint64_t *)(pdst + (Line * 2 + 1) * LineLength);
    }

    // Copy the odd line to the overlay verbatim.
    xine_fast_memcpy((char *)Dest + LineLength, YVal3, LineLength);

    n = LineLength >> 3;
    while( n-- )
    {
      movq_m2r (*YVal1++, mm0);
      movq_m2r (*YVal3++, mm2);

      // get average in mm0
      pand_m2r ( Mask, mm0 );
      pand_m2r ( Mask, mm2 );
      psrlw_i2r ( 01, mm0 );
      psrlw_i2r ( 01, mm2 );
      paddw_r2r ( mm2, mm0 );

      movq_r2m ( mm0, *Dest++ );
    }
  }

  /* Copy last odd line if we're processing an even field. */
  if (! IsOdd)
  {
    xine_fast_memcpy(pdst + (height * 2 - 1) * LineLength,
                      pOddLines + (height - 1) * SourcePitch,
                      LineLength);
  }

  /* clear out the MMX registers ready for doing floating point
   * again
   */
  emms();
#endif
}

/* Linear Blend filter - does a kind of vertical blurring on the image.
   (idea borrowed from mplayer's sources)
*/
static void deinterlace_linearblend_yuv_mmx( uint8_t *pdst, uint8_t *psrc[],
    int width, int height )
{
#ifdef USE_MMX
  int Line;
  uint64_t *YVal1;
  uint64_t *YVal2;
  uint64_t *YVal3;
  uint64_t *Dest;
  int LineLength = width;

  int n;

  /* Copy first line */
  xine_fast_memmove(pdst, psrc[0], LineLength);

  for (Line = 1; Line < height - 1; ++Line)
  {
    YVal1 = (uint64_t *)(psrc[0] + (Line - 1) * LineLength);
    YVal2 = (uint64_t *)(psrc[0] + (Line) * LineLength);
    YVal3 = (uint64_t *)(psrc[0] + (Line + 1) * LineLength);
    Dest = (uint64_t *)(pdst + Line * LineLength);

    n = LineLength >> 3;
    while( n-- )
    {
      /* load data from 3 lines */
      movq_m2r (*YVal1++, mm0);
      movq_m2r (*YVal2++, mm1);
      movq_m2r (*YVal3++, mm2);

      /* expand bytes to words */
      punpckhbw_r2r (mm0, mm3);
      punpckhbw_r2r (mm1, mm4);
      punpckhbw_r2r (mm2, mm5);
      punpcklbw_r2r (mm0, mm0);
      punpcklbw_r2r (mm1, mm1);
      punpcklbw_r2r (mm2, mm2);

      /*
       * deinterlacing:
       * deint_line = (line0 + 2*line1 + line2) / 4
       */
      psrlw_i2r (07, mm0);
      psrlw_i2r (06, mm1);
      psrlw_i2r (07, mm2);
      psrlw_i2r (07, mm3);
      psrlw_i2r (06, mm4);
      psrlw_i2r (07, mm5);
      paddw_r2r (mm1, mm0);
      paddw_r2r (mm2, mm0);
      paddw_r2r (mm4, mm3);
      paddw_r2r (mm5, mm3);
      psrlw_i2r (03, mm0);
      psrlw_i2r (03, mm3);

      /* pack 8 words to 8 bytes in mm0 */
      packuswb_r2r (mm3, mm0);

      movq_r2m ( mm0, *Dest++ );
    }
  }

  /* Copy last line */
  xine_fast_memmove(pdst + Line * LineLength,
                   psrc[0] + Line * LineLength, LineLength);

  /* clear out the MMX registers ready for doing floating point
   * again
   */
  emms();
#endif
}

/* Linear Blend filter - C version contributed by Rogerio Brito.
   This algorithm has the same interface as the other functions.

   The destination "screen" (pdst) is constructed from the source
   screen (psrc[0]) line by line.

   The i-th line of the destination screen is the average of 3 lines
   from the source screen: the (i-1)-th, i-th and (i+1)-th lines, with
   the i-th line having weight 2 in the computation.

   Remarks:
   * each line on pdst doesn't depend on previous lines;
   * due to the way the algorithm is defined, the first & last lines of the
     screen aren't deinterlaced.

*/
static void deinterlace_linearblend_yuv( uint8_t *pdst, uint8_t *psrc[],
                                         int width, int height )
{
  register int x, y;
  register uint8_t *l0, *l1, *l2, *l3;

  l0 = pdst;		/* target line */
  l1 = psrc[0];		/* 1st source line */
  l2 = l1 + width;	/* 2nd source line = line that follows l1 */
  l3 = l2 + width;	/* 3rd source line = line that follows l2 */

  /* Copy the first line */
  xine_fast_memcpy(l0, l1, width);
  l0 += width;

  for (y = 1; y < height-1; ++y) {
    /* computes avg of: l1 + 2*l2 + l3 */

    for (x = 0; x < width; ++x) {
      l0[x] = (l1[x] + (l2[x]<<1) + l3[x]) >> 2;
    }

    /* updates the line pointers */
    l1 = l2; l2 = l3; l3 += width;
    l0 += width;
  }

  /* Copy the last line */
  xine_fast_memcpy(l0, l1, width);
}

static int check_for_mmx(void)
{
#ifdef USE_MMX
static int config_flags = -1;

  if ( config_flags == -1 )
    config_flags = xine_mm_accel();
  if (config_flags & MM_ACCEL_X86_MMX)
    return 1;
  return 0;
#else
  return 0;
#endif
}

/* generic YUV deinterlacer
   pdst -> pointer to destination bitmap
   psrc -> array of pointers to source bitmaps ([0] = most recent)
   width,height -> dimension for bitmaps
   method -> DEINTERLACE_xxx
*/

void deinterlace_yuv( uint8_t *pdst, uint8_t *psrc[],
    int width, int height, int method )
{
  switch( method ) {
    case DEINTERLACE_NONE:
      xine_fast_memcpy(pdst,psrc[0],width*height);
      break;
    case DEINTERLACE_BOB:
      if( check_for_mmx() )
        deinterlace_bob_yuv_mmx(pdst,psrc,width,height);
      else /* FIXME: provide an alternative? */
        deinterlace_linearblend_yuv(pdst,psrc,width,height);
      break;
    case DEINTERLACE_WEAVE:
      if( check_for_mmx() )
      {
        if( !deinterlace_weave_yuv_mmx(pdst,psrc,width,height) )
          xine_fast_memcpy(pdst,psrc[0],width*height);
      }
      else /* FIXME: provide an alternative? */
        deinterlace_linearblend_yuv(pdst,psrc,width,height);
      break;
    case DEINTERLACE_GREEDY:
      if( check_for_mmx() )
      {
        if( !deinterlace_greedy_yuv_mmx(pdst,psrc,width,height) )
          xine_fast_memcpy(pdst,psrc[0],width*height);
      }
      else /* FIXME: provide an alternative? */
        deinterlace_linearblend_yuv(pdst,psrc,width,height);
      break;
    case DEINTERLACE_ONEFIELD:
      if( check_for_mmx() )
        deinterlace_onefield_yuv_mmx(pdst,psrc,width,height);
      else /* FIXME: provide an alternative? */
        deinterlace_linearblend_yuv(pdst,psrc,width,height);
      break;
    case DEINTERLACE_ONEFIELDXV:
      lprintf("ONEFIELDXV must be handled by the video driver.\n");
      break;
    case DEINTERLACE_LINEARBLEND:
      if( check_for_mmx() )
        deinterlace_linearblend_yuv_mmx(pdst,psrc,width,height);
      else
        deinterlace_linearblend_yuv(pdst,psrc,width,height);
      break;
    default:
      lprintf("unknown method %d.\n",method);
      break;
  }
}

int deinterlace_yuv_supported ( int method )
{
  switch( method ) {
    case DEINTERLACE_NONE:
      return 1;
    case DEINTERLACE_BOB:
    case DEINTERLACE_WEAVE:
    case DEINTERLACE_GREEDY:
    case DEINTERLACE_ONEFIELD:
      return check_for_mmx();
    case DEINTERLACE_ONEFIELDXV:
      lprintf ("ONEFIELDXV must be handled by the video driver.\n");
      return 0;
    case DEINTERLACE_LINEARBLEND:
      return 1;
  }

  return 0;
}

const char *deinterlace_methods[] = {
  "none",
  "bob",
  "weave",
  "greedy",
  "onefield",
  "onefield_xv",
  "linearblend",
  NULL
};


