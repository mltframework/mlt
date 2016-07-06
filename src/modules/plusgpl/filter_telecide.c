/*
 * filter_telecide.c -- Donald Graft's Inverse Telecine Filter
 * Copyright (C) 2003 Donald A. Graft
 * Copyright (C) 2008 Dan Dennedy <dan@dennedy.org>
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CYCLE 6
#define BLKSIZE 24
#define BLKSIZE_TIMES2 (2 * BLKSIZE)
#define GUIDE_32 1
#define GUIDE_22 2
#define GUIDE_32322 3
#define AHEAD 0
#define BEHIND 1
#define POST_METRICS 1
#define POST_FULL 2
#define POST_FULL_MAP 3
#define POST_FULL_NOMATCH 4
#define POST_FULL_NOMATCH_MAP 5
#define CACHE_SIZE 100000
#define P 0
#define C 1
#define N 2
#define PBLOCK 3
#define CBLOCK 4
#define NO_BACK 0
#define BACK_ON_COMBED 1
#define ALWAYS_BACK 2

struct CACHE_ENTRY
{
	unsigned int frame;
	unsigned int metrics[5];
	unsigned int chosen;
};

struct PREDICTION
{
	unsigned int metric;
	unsigned int phase;
	unsigned int predicted;
	unsigned int predicted_metric;
};

struct context_s {
	int is_configured;
	mlt_properties image_cache;
	int out;
	
	int tff, chroma, blend, hints, show, debug;
	float dthresh, gthresh, vthresh, vthresh_saved, bthresh;
	int y0, y1, nt, guide, post, back, back_saved;
	int pitch, dpitch, pitchover2, pitchtimes4;
	int w, h, wover2, hover2, hplus1over2, hminus2;
  	int xblocks, yblocks;
#ifdef WINDOWED_MATCH
	unsigned int *matchc, *matchp, highest_matchc, highest_matchp;
#endif
	unsigned int *sumc, *sump, highest_sumc, highest_sump;
	int vmetric;
	unsigned int *overrides, *overrides_p;
	int film, override, inpattern, found;
	int force;

	// Used by field matching.
	unsigned char *fprp, *fcrp, *fcrp_saved, *fnrp;
	unsigned char *dstp, *finalp;
	int chosen;
	unsigned int p, c, pblock, cblock, lowest, predicted, predicted_metric;
	unsigned int np, nc, npblock, ncblock, nframe;
	float mismatch;
	int pframe, x, y;
	unsigned char *crp, *prp;
	unsigned char *crpU, *prpU;
	unsigned char *crpV, *prpV;
	int hard;
	char status[80];

	// Metrics cache.
	struct CACHE_ENTRY *cache;

	// Pattern guidance data.
	int cycle;
	struct PREDICTION pred[MAX_CYCLE+1];
};
typedef struct context_s *context;


static inline
void BitBlt(uint8_t* dstp, int dst_pitch, const uint8_t* srcp,
            int src_pitch, int row_size, int height)
{
	uint32_t y;
	for(y=0;y<height;y++)
	{
		memcpy(dstp,srcp,row_size);
		dstp+=dst_pitch;
		srcp+=src_pitch;
	}
}

static void Show(context cx, int frame, mlt_properties properties)
{
	char use;
	char buf[512];

	if (cx->chosen == P) use = 'p';
	else if (cx->chosen == C) use = 'c';
	else use = 'n';
	snprintf(buf, sizeof(buf), "Telecide: frame %d: matches: %d %d %d\n", frame, cx->p, cx->c, cx->np);
	if ( cx->post )
		snprintf(buf, sizeof(buf), "%sTelecide: frame %d: vmetrics: %d %d %d [chosen=%d]\n", buf, frame, cx->pblock, cx->cblock, cx->npblock, cx->vmetric);
	if ( cx->guide )
		snprintf(buf, sizeof(buf), "%spattern mismatch=%0.2f%%\n", buf, cx->mismatch);
	snprintf(buf, sizeof(buf), "%sTelecide: frame %d: [%s %c]%s %s\n", buf, frame, cx->found ? "forcing" : "using", use,
		cx->post ? (cx->film ? " [progressive]" : " [interlaced]") : "",
		cx->guide ? cx->status : "");
	mlt_properties_set( properties, "meta.attr.telecide.markup", buf );
}

static void Debug(context cx, int frame)
{
	char use;

	if (cx->chosen == P) use = 'p';
	else if (cx->chosen == C) use = 'c';
	else use = 'n';
	fprintf(stderr, "Telecide: frame %d: matches: %d %d %d\n", frame, cx->p, cx->c, cx->np);
	if ( cx->post )
		fprintf(stderr, "Telecide: frame %d: vmetrics: %d %d %d [chosen=%d]\n", frame, cx->pblock, cx->cblock, cx->npblock, cx->vmetric);
	if ( cx->guide )
		fprintf(stderr, "pattern mismatch=%0.2f%%\n", cx->mismatch); 
	fprintf(stderr, "Telecide: frame %d: [%s %c]%s %s\n", frame, cx->found ? "forcing" : "using", use,
		cx->post ? (cx->film ? " [progressive]" : " [interlaced]") : "",
		cx->guide ? cx->status : "");
}

static void WriteHints(int film, int inpattern, mlt_properties frame_properties)
{
	mlt_properties_set_int( frame_properties, "telecide.progressive", film);
	mlt_properties_set_int( frame_properties, "telecide.in_pattern", inpattern);
}

static void PutChosen(context cx, int frame, unsigned int chosen)
{
	int f = frame % CACHE_SIZE;
	if (frame < 0 || frame > cx->out || cx->cache[f].frame != frame)
		return;
	cx->cache[f].chosen = chosen;
}

static void CacheInsert(context cx, int frame, unsigned int p, unsigned int pblock,
					 unsigned int c, unsigned int cblock)
{
	int f = frame % CACHE_SIZE;
	if (frame < 0 || frame > cx->out)
		fprintf( stderr, "%s: internal error: invalid frame %d for CacheInsert", __FUNCTION__, frame);
	cx->cache[f].frame = frame;
	cx->cache[f].metrics[P] = p;
	if (f) cx->cache[f-1].metrics[N] = p;
	cx->cache[f].metrics[C] = c;
	cx->cache[f].metrics[PBLOCK] = pblock;
	cx->cache[f].metrics[CBLOCK] = cblock;
	cx->cache[f].chosen = 0xff;
}

static int CacheQuery(context cx, int frame, unsigned int *p, unsigned int *pblock,
					unsigned int *c, unsigned int *cblock)
{
	int f;

	f = frame % CACHE_SIZE;
	if (frame < 0 || frame > cx->out)
		fprintf( stderr, "%s: internal error: invalid frame %d for CacheQuery", __FUNCTION__, frame);
	if (cx->cache[f].frame != frame)
	{
		return 0;
	}
	*p = cx->cache[f].metrics[P];
	*c = cx->cache[f].metrics[C];
	*pblock = cx->cache[f].metrics[PBLOCK];
	*cblock = cx->cache[f].metrics[CBLOCK];
	return 1;
}

static int PredictHardYUY2(context cx, int frame, unsigned int *predicted, unsigned int *predicted_metric)
{
	// Look for pattern in the actual delivered matches of the previous cycle of frames.
	// If a pattern is found, use that to predict the current match.
	if ( cx->guide == GUIDE_22 )
	{
		if (cx->cache[(frame- cx->cycle)%CACHE_SIZE  ].chosen == 0xff ||
			cx->cache[(frame- cx->cycle+1)%CACHE_SIZE].chosen == 0xff)
			return 0;
		switch ((cx->cache[(frame- cx->cycle)%CACHE_SIZE  ].chosen << 4) +
				(cx->cache[(frame- cx->cycle+1)%CACHE_SIZE].chosen))
		{
		case 0x11:
			*predicted = C;
			*predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C];
			break;
		case 0x22:
			*predicted = N;
			*predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[N];
			break;
		default: return 0;
		}
	}
	else if ( cx->guide == GUIDE_32 )
	{
		if (cx->cache[(frame-cx->cycle)%CACHE_SIZE  ].chosen == 0xff ||
			cx->cache[(frame-cx->cycle+1)%CACHE_SIZE].chosen == 0xff ||
			cx->cache[(frame-cx->cycle+2)%CACHE_SIZE].chosen == 0xff ||
			cx->cache[(frame-cx->cycle+3)%CACHE_SIZE].chosen == 0xff ||
			cx->cache[(frame-cx->cycle+4)%CACHE_SIZE].chosen == 0xff)
			return 0;

		switch ((cx->cache[(frame-cx->cycle)%CACHE_SIZE  ].chosen << 16) +
				(cx->cache[(frame-cx->cycle+1)%CACHE_SIZE].chosen << 12) +
				(cx->cache[(frame-cx->cycle+2)%CACHE_SIZE].chosen <<  8) +
				(cx->cache[(frame-cx->cycle+3)%CACHE_SIZE].chosen <<  4) +
				(cx->cache[(frame-cx->cycle+4)%CACHE_SIZE].chosen))
		{
		case 0x11122:
		case 0x11221:
		case 0x12211:
		case 0x12221: 
		case 0x21122: 
		case 0x11222: 
			*predicted = C;
			*predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C];
			break;
		case 0x22111:
		case 0x21112:
		case 0x22112: 
		case 0x22211: 
			*predicted = N;
			*predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[N];
			break;
		default: return 0;
		}
	}
	else if ( cx->guide == GUIDE_32322 )
	{
		if (cx->cache[(frame- cx->cycle)%CACHE_SIZE  ].chosen == 0xff ||
			cx->cache[(frame- cx->cycle +1)%CACHE_SIZE].chosen == 0xff ||
			cx->cache[(frame- cx->cycle +2)%CACHE_SIZE].chosen == 0xff ||
			cx->cache[(frame- cx->cycle +3)%CACHE_SIZE].chosen == 0xff ||
			cx->cache[(frame- cx->cycle +4)%CACHE_SIZE].chosen == 0xff ||
			cx->cache[(frame- cx->cycle +5)%CACHE_SIZE].chosen == 0xff)
			return 0;

		switch ((cx->cache[(frame- cx->cycle)%CACHE_SIZE  ].chosen << 20) +
				(cx->cache[(frame- cx->cycle +1)%CACHE_SIZE].chosen << 16) +
				(cx->cache[(frame- cx->cycle +2)%CACHE_SIZE].chosen << 12) +
				(cx->cache[(frame- cx->cycle +3)%CACHE_SIZE].chosen <<  8) +
				(cx->cache[(frame- cx->cycle +4)%CACHE_SIZE].chosen <<  4) +
				(cx->cache[(frame- cx->cycle +5)%CACHE_SIZE].chosen))
		{
		case 0x111122:
		case 0x111221:
		case 0x112211:
		case 0x122111:
		case 0x111222: 
		case 0x112221:
		case 0x122211:
		case 0x222111: 
			*predicted = C;
			*predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C];
			break;
		case 0x221111:
		case 0x211112:

		case 0x221112: 
		case 0x211122: 
			*predicted = N;
			*predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[N];
			break;
		default: return 0;
		}
	}
#ifdef DEBUG_PATTERN_GUIDANCE
	fprintf( stderr, "%s: pos=%d HARD: predicted=%d\n", __FUNCTION__, frame, *predicted);
#endif
	return 1;
}

static struct PREDICTION *PredictSoftYUY2(context cx, int frame )
{
	// Use heuristics to look forward for a match.
	int i, j, y, c, n, phase;
	unsigned int metric;

	cx->pred[0].metric = 0xffffffff;
	if (frame < 0 || frame > cx->out - cx->cycle) return cx->pred;

	// Look at the next cycle of frames.
	for (y = frame + 1; y <= frame + cx->cycle; y++)
	{
		// Look for a frame where the current and next match values are
		// very close. Those are candidates to predict the phase, because
		// that condition should occur only once per cycle. Store the candidate
		// phases and predictions in a list sorted by goodness. The list will
		// be used by the caller to try the phases in order.
		c = cx->cache[y%CACHE_SIZE].metrics[C]; 
		n = cx->cache[y%CACHE_SIZE].metrics[N];
		if (c == 0) c = 1;
		metric = (100 * abs (c - n)) / c;
		phase = y % cx->cycle;
		if (metric < 5)
		{
			// Place the new candidate phase in sorted order in the list.
			// Find the insertion point.
			i = 0;
			while (metric > cx->pred[i].metric) i++;
			// Find the end-of-list marker.
			j = 0;
			while (cx->pred[j].metric != 0xffffffff) j++;
			// Shift all items below the insertion point down by one to make
			// room for the insertion.
			j++;
			for (; j > i; j--)
			{
				cx->pred[j].metric = cx->pred[j-1].metric;
				cx->pred[j].phase = cx->pred[j-1].phase;
				cx->pred[j].predicted = cx->pred[j-1].predicted;
				cx->pred[j].predicted_metric = cx->pred[j-1].predicted_metric;
			}
			// Insert the new candidate data.
			cx->pred[j].metric = metric;
			cx->pred[j].phase = phase;
			if ( cx->guide == GUIDE_32 )
			{
				switch ((frame % cx->cycle) - phase)
				{
				case -4: cx->pred[j].predicted = N; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[N]; break; 
				case -3: cx->pred[j].predicted = N; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[N]; break; 
				case -2: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				case -1: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				case  0: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				case +1: cx->pred[j].predicted = N; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[N]; break; 
				case +2: cx->pred[j].predicted = N; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[N]; break; 
				case +3: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				case +4: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				}
			}
			else if ( cx->guide == GUIDE_32322 )
			{
				switch ((frame % cx->cycle) - phase)
				{
				case -5: cx->pred[j].predicted = N; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[N]; break; 
				case -4: cx->pred[j].predicted = N; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[N]; break; 
				case -3: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				case -2: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				case -1: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				case  0: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				case +1: cx->pred[j].predicted = N; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[N]; break; 
				case +2: cx->pred[j].predicted = N; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[N]; break; 
				case +3: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				case +4: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				case +5: cx->pred[j].predicted = C; cx->pred[j].predicted_metric = cx->cache[frame%CACHE_SIZE].metrics[C]; break; 
				}
			}
		}
#ifdef DEBUG_PATTERN_GUIDANCE
		fprintf( stderr, "%s: pos=%d metric=%d phase=%d\n", __FUNCTION__, frame, metric, phase);
#endif
	}
	return cx->pred;
}

static
void CalculateMetrics(context cx, int frame, unsigned char *fcrp, unsigned char *fcrpU, unsigned char *fcrpV,
					unsigned char *fprp, unsigned char *fprpU, unsigned char *fprpV)
{
	int x, y, p, c, tmp1, tmp2, skip;
	int vc;
    unsigned char *currbot0, *currbot2, *prevbot0, *prevbot2;
	unsigned char *prevtop0, *prevtop2, *prevtop4, *currtop0, *currtop2, *currtop4;
	unsigned char *a0, *a2, *b0, *b2, *b4;
	unsigned int diff, index;
#	define T 4

	/* Clear the block sums. */
 	for (y = 0; y < cx->yblocks; y++)
	{
 		for (x = 0; x < cx->xblocks; x++)
		{
#ifdef WINDOWED_MATCH
			matchp[y*xblocks+x] = 0;
			matchc[y*xblocks+x] = 0;
#endif
			cx->sump[y * cx->xblocks + x] = 0;
			cx->sumc[y * cx->xblocks + x] = 0;
		}
	}

	/* Find the best field match. Subsample the frames for speed. */
	currbot0  = fcrp + cx->pitch;
	currbot2  = fcrp + 3 * cx->pitch;
	currtop0 = fcrp;
	currtop2 = fcrp + 2 * cx->pitch;
	currtop4 = fcrp + 4 * cx->pitch;
	prevbot0  = fprp + cx->pitch;
	prevbot2  = fprp + 3 * cx->pitch;
	prevtop0 = fprp;
	prevtop2 = fprp + 2 * cx->pitch;
	prevtop4 = fprp + 4 * cx->pitch;
	if ( cx->tff )
	{
		a0 = prevbot0;
		a2 = prevbot2;
		b0 = currtop0;
		b2 = currtop2;
		b4 = currtop4;
	}
	else
	{
		a0 = currbot0;
		a2 = currbot2;
		b0 = prevtop0;
		b2 = prevtop2;
		b4 = prevtop4;
	}
	p = c = 0;

	// Calculate the field match and film/video metrics.
	skip = 1 + ( !cx->chroma );
	for (y = 0, index = 0; y < cx->h - 4; y+=4)
	{
		/* Exclusion band. Good for ignoring subtitles. */
		if (cx->y0 == cx->y1 || y < cx->y0 || y > cx->y1)
		{
			for (x = 0; x < cx->w;)
			{
				index = (y/BLKSIZE) * cx->xblocks + x/BLKSIZE_TIMES2;

				// Test combination with current frame.
				tmp1 = ((long)currbot0[x] + (long)currbot2[x]);
				diff = labs((((long)currtop0[x] + (long)currtop2[x] + (long)currtop4[x])) - (tmp1 >> 1) - tmp1);
				if (diff > cx->nt)
				{
					c += diff;
#ifdef WINDOWED_MATCH
					matchc[index] += diff;
#endif
				}

				tmp1 = currbot0[x] + T;
				tmp2 = currbot0[x] - T;
				vc = (tmp1 < currtop0[x] && tmp1 < currtop2[x]) ||
					 (tmp2 > currtop0[x] && tmp2 > currtop2[x]);
				if (vc)
				{
					cx->sumc[index]++;
				}

				// Test combination with previous frame.
				tmp1 = ((long)a0[x] + (long)a2[x]);
				diff = labs((((long)b0[x] + (long)b2[x] + (long)b4[x])) - (tmp1 >> 1) - tmp1);
				if (diff > cx->nt)
				{
					p += diff;
#ifdef WINDOWED_MATCH
					matchp[index] += diff;
#endif
				}

				tmp1 = a0[x] + T;
				tmp2 = a0[x] - T;
				vc = (tmp1 < b0[x] && tmp1 < b2[x]) ||
					 (tmp2 > b0[x] && tmp2 > b2[x]);
				if (vc)
				{
					cx->sump[index]++;
				}

				x += skip;
				if (!(x&3)) x += 4;
			}
		}
		currbot0 += cx->pitchtimes4;
		currbot2 += cx->pitchtimes4;
		currtop0 += cx->pitchtimes4;
		currtop2 += cx->pitchtimes4;
		currtop4 += cx->pitchtimes4;
		a0		 += cx->pitchtimes4;
		a2		 += cx->pitchtimes4;
		b0		 += cx->pitchtimes4;
		b2		 += cx->pitchtimes4;
		b4		 += cx->pitchtimes4;
	}


	if ( cx->post )
	{
		cx->highest_sump = 0;
		for (y = 0; y < cx->yblocks; y++)
		{
			for (x = 0; x < cx->xblocks; x++)
			{
				if (cx->sump[y * cx->xblocks + x] > cx->highest_sump)
				{
					cx->highest_sump = cx->sump[y * cx->xblocks + x];
				}
			}
		}
		cx->highest_sumc = 0;
		for (y = 0; y < cx->yblocks; y++)
		{
			for (x = 0; x < cx->xblocks; x++)
			{
				if (cx->sumc[y * cx->xblocks + x] > cx->highest_sumc)
				{
					cx->highest_sumc = cx->sumc[y * cx->xblocks + x];
				}
			}
		}
	}
#ifdef WINDOWED_MATCH
	CacheInsert(frame, highest_matchp, highest_sump, highest_matchc, highest_sumc);
#else
	CacheInsert( cx, frame, p, cx->highest_sump, c, cx->highest_sumc);
#endif
}

/** Process the image.
*/

static int get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the filter service
	mlt_filter filter = mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties frame_properties = mlt_frame_properties( frame );
	context cx = mlt_properties_get_data( properties, "context", NULL );
	mlt_service producer = mlt_service_producer( mlt_filter_service( filter ) );
	cx->out = producer? mlt_producer_get_playtime( MLT_PRODUCER( producer ) ) : 999999;

	if ( ! cx->is_configured )
	{
		cx->back = mlt_properties_get_int( properties, "back" );
		cx->chroma = mlt_properties_get_int( properties, "chroma" );
		cx->guide = mlt_properties_get_int( properties, "guide" );
		cx->gthresh = mlt_properties_get_double( properties, "gthresh" );
		cx->post = mlt_properties_get_int( properties, "post" );
		cx->vthresh = mlt_properties_get_double( properties, "vthresh" );
		cx->bthresh = mlt_properties_get_double( properties, "bthresh" );
		cx->dthresh = mlt_properties_get_double( properties, "dthresh" );
		cx->blend = mlt_properties_get_int( properties, "blend" );
		cx->nt = mlt_properties_get_int( properties, "nt" );
		cx->y0 = mlt_properties_get_int( properties, "y0" );
		cx->y1 = mlt_properties_get_int( properties, "y1" );
		cx->hints = mlt_properties_get_int( properties, "hints" );
		cx->debug = mlt_properties_get_int( properties, "debug" );
		cx->show = mlt_properties_get_int( properties, "show" );
	}

	// Get the image
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	if ( ! cx->sump )
	{
		int guide = mlt_properties_get_int( properties, "guide" );
		cx->cycle = 0;
		if ( guide == GUIDE_32 )
		{
			// 24fps to 30 fps telecine.
			cx->cycle = 5;
		}
		else if ( guide == GUIDE_22 )
		{
			// PAL guidance (expect the current match to be continued).
			cx->cycle = 2;
		}
		else if ( guide == GUIDE_32322 )
		{
			// 25fps to 30 fps telecine.
			cx->cycle = 6;
		}

		cx->xblocks = (*width+BLKSIZE-1) / BLKSIZE;
		cx->yblocks = (*height+BLKSIZE-1) / BLKSIZE;
		cx->sump = (unsigned int *) mlt_pool_alloc( cx->xblocks * cx->yblocks * sizeof(unsigned int) );
		cx->sumc = (unsigned int *) mlt_pool_alloc( cx->xblocks * cx->yblocks * sizeof(unsigned int) );
		mlt_properties_set_data( properties, "sump", cx->sump, cx->xblocks * cx->yblocks * sizeof(unsigned int), (mlt_destructor)mlt_pool_release, NULL );
		mlt_properties_set_data( properties, "sumc", cx->sumc, cx->xblocks * cx->yblocks * sizeof(unsigned int), (mlt_destructor)mlt_pool_release, NULL );
		cx->tff = mlt_properties_get_int( frame_properties, "top_field_first" );
	}

	// Only process if we have no error and a valid colour space
	if ( error == 0 && *format == mlt_image_yuv422 )
	{
		// Put the current image into the image cache, keyed on position
		size_t image_size = (*width * *height) << 1;
		mlt_position pos = mlt_filter_get_position( filter, frame );
		uint8_t *image_copy = mlt_pool_alloc( image_size );
		memcpy( image_copy, *image, image_size );
		char key[20];
		sprintf( key, MLT_POSITION_FMT, pos );
		mlt_properties_set_data( cx->image_cache, key, image_copy, image_size, (mlt_destructor)mlt_pool_release, NULL );
		
		// Only if we have enough frame images cached
		if ( pos > 1 && pos > cx->cycle + 1 )
		{
			pos -= cx->cycle + 1;
			// Get the current frame image
			sprintf( key, MLT_POSITION_FMT, pos );
			cx->fcrp = mlt_properties_get_data( cx->image_cache, key, NULL );
			if (!cx->fcrp) return error;
			 
			// Get the previous frame image
			cx->pframe = pos == 0 ? 0 : pos - 1;
			sprintf( key, "%d", cx->pframe );
			cx->fprp = mlt_properties_get_data( cx->image_cache, key, NULL );
			if (!cx->fprp) return error;
			
			// Get the next frame image
			cx->nframe = pos > cx->out ? cx->out : pos + 1;
			sprintf( key, "%d", cx->nframe );
			cx->fnrp = mlt_properties_get_data( cx->image_cache, key, NULL );
			if (!cx->fnrp) return error;
			
			cx->pitch = *width << 1;
			cx->pitchover2 = cx->pitch >> 1;
			cx->pitchtimes4 = cx->pitch << 2;
			cx->w = *width << 1;
			cx->h = *height;
			if ((cx->w/2) & 1)
				fprintf( stderr, "%s: width must be a multiple of 2\n", __FUNCTION__ );
			if (cx->h & 1)
				fprintf( stderr, "%s: height must be a multiple of 2\n", __FUNCTION__ );
			cx->wover2 = cx->w/2;
			cx->hover2 = cx->h/2;
			cx->hplus1over2 = (cx->h+1)/2;
			cx->hminus2 = cx->h - 2;
			cx->dpitch = cx->pitch;
			
			// Ensure that the metrics for the frames
			// after the current frame are in the cache. They will be used for
			// pattern guidance.
			if ( cx->guide )
			{
				for ( cx->y = pos + 1; (cx->y <= pos + cx->cycle + 1) && (cx->y <= cx->out); cx->y++ )
				{
					if ( ! CacheQuery( cx, cx->y, &cx->p, &cx->pblock, &cx->c, &cx->cblock ) )
					{
						sprintf( key, "%d", cx->y );
						cx->crp = (unsigned char *) mlt_properties_get_data( cx->image_cache, key, NULL );
						sprintf( key, "%d", cx->y ? cx->y - 1 : 1 );
						cx->prp = (unsigned char *) mlt_properties_get_data( cx->image_cache, key, NULL );
						CalculateMetrics( cx, cx->y, cx->crp, NULL, NULL, cx->prp, NULL, NULL );
					}
				}
			}
			
			// Check for manual overrides of the field matching.
			cx->found = 0;
			cx->film = 1;
			cx->override = 0;
			cx->inpattern = 0;
			cx->back = cx->back_saved;
			
			// Get the metrics for the current-previous (p), current-current (c), and current-next (n) match candidates.
			if ( ! CacheQuery( cx, pos, &cx->p, &cx->pblock, &cx->c, &cx->cblock ) )
			{
				CalculateMetrics( cx, pos, cx->fcrp, NULL, NULL, cx->fprp, NULL, NULL );
				CacheQuery( cx, pos, &cx->p, &cx->pblock, &cx->c, &cx->cblock );
			}				
			if ( ! CacheQuery( cx, cx->nframe, &cx->np, &cx->npblock, &cx->nc, &cx->ncblock ) )
			{
				CalculateMetrics( cx, cx->nframe, cx->fnrp, NULL, NULL, cx->fcrp, NULL, NULL );
				CacheQuery( cx, cx->nframe, &cx->np, &cx->npblock, &cx->nc, &cx->ncblock );
			}				
			
			// Determine the best candidate match.
			if ( !cx->found )
			{
				cx->lowest = cx->c;
				cx->chosen = C;
				if ( cx->back == ALWAYS_BACK && cx->p < cx->lowest )
				{
					cx->lowest = cx->p;
					cx->chosen = P;
				}
				if ( cx->np < cx->lowest )
				{
					cx->lowest = cx->np;
					cx->chosen = N;
				}
			}
			if ((pos == 0 && cx->chosen == P) || (pos == cx->out && cx->chosen == N))
			{
				cx->chosen = C;
				cx->lowest = cx->c;
			}

			// See if we can apply pattern guidance.
			cx->mismatch = 100.0;
			if ( cx->guide )
			{
				cx->hard = 0;
				if ( pos >= cx->cycle && PredictHardYUY2( cx, pos, &cx->predicted, &cx->predicted_metric) )
				{
					cx->inpattern = 1;
					cx->mismatch = 0.0;
					cx->hard = 1;
					if ( cx->chosen != cx->predicted )
					{
						// The chosen frame doesn't match the prediction.
						if ( cx->predicted_metric == 0 )
							cx->mismatch = 0.0;
						else
							cx->mismatch = (100.0 * ( cx->predicted_metric - cx->lowest ) ) / cx->predicted_metric;
						if ( cx->mismatch < cx->gthresh )
						{
							// It's close enough, so use the predicted one.
							if ( !cx->found )
							{
								cx->chosen = cx->predicted;
								cx->override = 1;
							}
						}
						else
						{
							cx->hard = 0;
							cx->inpattern = 0;
						}
					}
				}
		
				if ( !cx->hard && cx->guide != GUIDE_22 )
				{
					int i;
					struct PREDICTION *pred = PredictSoftYUY2( cx, pos );
		
					if ( ( pos <= cx->out - cx->cycle) && ( pred[0].metric != 0xffffffff ) )
					{
						// Apply pattern guidance.
						// If the predicted match metric is within defined percentage of the
						// best calculated one, then override the calculated match with the
						// predicted match.
						i = 0;
						while ( pred[i].metric != 0xffffffff )
						{
							cx->predicted = pred[i].predicted;
							cx->predicted_metric = pred[i].predicted_metric;
#ifdef DEBUG_PATTERN_GUIDANCE
							fprintf(stderr, "%s: pos=%d predicted=%d\n", __FUNCTION__, pos, cx->predicted);
#endif
							if ( cx->chosen != cx->predicted )
							{
								// The chosen frame doesn't match the prediction.
								if ( cx->predicted_metric == 0 )
									cx->mismatch = 0.0;
								else
									cx->mismatch = (100.0 * ( cx->predicted_metric - cx->lowest )) / cx->predicted_metric;
								if ( (int) cx->mismatch <= cx->gthresh )
								{
									// It's close enough, so use the predicted one.
									if ( !cx->found )
									{
										cx->chosen = cx->predicted;
										cx->override = 1;
									}
									cx->inpattern = 1;
									break;
								}
								else
								{
									// Looks like we're not in a predictable pattern.
									cx->inpattern = 0;
								}
							}
							else
							{
								cx->inpattern = 1;
								cx->mismatch = 0.0;
								break;
							}
							i++;
						}
					}
				}
			}

			// Check the match for progressive versus interlaced.
			if ( cx->post )
			{
				if (cx->chosen == P) cx->vmetric = cx->pblock;
				else if (cx->chosen == C) cx->vmetric = cx->cblock;
				else if (cx->chosen == N) cx->vmetric = cx->npblock;
		
				if ( !cx->found && cx->back == BACK_ON_COMBED && cx->vmetric > cx->bthresh && cx->p < cx->lowest )
				{
					// Backward match.
					cx->vmetric = cx->pblock;
					cx->chosen = P;
					cx->inpattern = 0;
					cx->mismatch = 100;
				}
				if ( cx->vmetric > cx->vthresh )
				{
					// After field matching and pattern guidance the frame is still combed.
					cx->film = 0;
					if ( !cx->found && ( cx->post == POST_FULL_NOMATCH || cx->post == POST_FULL_NOMATCH_MAP ) )
					{
						cx->chosen = C;
						cx->vmetric = cx->cblock;
						cx->inpattern = 0;
						cx->mismatch = 100;
					}
				}
			}
			cx->vthresh = cx->vthresh_saved;
		
			// Setup strings for debug info.
			if ( cx->inpattern && !cx->override ) strcpy( cx->status, "[in-pattern]" );
			else if ( cx->inpattern && cx->override ) strcpy( cx->status, "[in-pattern*]" );
			else strcpy( cx->status, "[out-of-pattern]" );
	
			// Assemble and output the reconstructed frame according to the final match.
			cx->dstp = *image;
			if ( cx->chosen == N )
			{
				// The best match was with the next frame.
				if ( cx->tff )
				{
					BitBlt( cx->dstp, 2 * cx->dpitch, cx->fnrp, 2 * cx->pitch, cx->w, cx->hover2 );
					BitBlt( cx->dstp + cx->dpitch, 2 * cx->dpitch, cx->fcrp + cx->pitch, 2 * cx->pitch,	cx->w, cx->hover2 );
				}
				else
				{
					BitBlt( cx->dstp, 2 * cx->dpitch, cx->fcrp, 2 * cx->pitch, cx->w, cx->hplus1over2 );
					BitBlt( cx->dstp + cx->dpitch, 2 * cx->dpitch, cx->fnrp + cx->pitch, 2 * cx->pitch,	cx->w, cx->hover2 );
				}
			}
			else if ( cx->chosen == C )
			{
				// The best match was with the current frame.
				BitBlt( cx->dstp, 2 * cx->dpitch, cx->fcrp, 2 * cx->pitch, cx->w, cx->hplus1over2 );
				BitBlt( cx->dstp + cx->dpitch, 2 * cx->dpitch, cx->fcrp + cx->pitch, 2 * cx->pitch,	cx->w, cx->hover2 );
			}
			else if ( ! cx->tff )
			{
				// The best match was with the previous frame.
				BitBlt( cx->dstp, 2 * cx->dpitch, cx->fprp, 2 * cx->pitch, cx->w, cx->hplus1over2 );
				BitBlt( cx->dstp + cx->dpitch, 2 * cx->dpitch, cx->fcrp + cx->pitch, 2 * cx->pitch, cx->w, cx->hover2 );
			}
			else
			{
				// The best match was with the previous frame.
				BitBlt( cx->dstp, 2 * cx->dpitch, cx->fcrp, 2 * cx->pitch, cx->w, cx->hplus1over2 );
				BitBlt( cx->dstp + cx->dpitch, 2 * cx->dpitch, cx->fprp + cx->pitch, 2 * cx->pitch,	cx->w, cx->hover2 );
			}
			if ( cx->guide )
				PutChosen( cx, pos, cx->chosen );

			if ( !cx->post || cx->post == POST_METRICS )
			{
				if ( cx->force == '+') cx->film = 0;
				else if ( cx->force == '-' ) cx->film = 1;
			}
			else if ((cx->force == '+') ||
				((cx->post == POST_FULL || cx->post == POST_FULL_MAP || cx->post == POST_FULL_NOMATCH || cx->post == POST_FULL_NOMATCH_MAP)
				         && (cx->film == 0 && cx->force != '-')))
			{
				unsigned char *dstpp, *dstpn;
				int v1, v2;
		
				if ( cx->blend )
				{
					// Do first and last lines.
					uint8_t *final = mlt_pool_alloc( image_size );
					cx->finalp = final;
					mlt_frame_set_image( frame, final, image_size, mlt_pool_release );
					dstpn = cx->dstp + cx->dpitch;
					for ( cx->x = 0; cx->x < cx->w; cx->x++ )
					{
						cx->finalp[cx->x] = (((int)cx->dstp[cx->x] + (int)dstpn[cx->x]) >> 1);
					}
					cx->finalp = final + (cx->h-1)*cx->dpitch;
					cx->dstp = *image + (cx->h-1)*cx->dpitch;
					dstpp = cx->dstp - cx->dpitch;
					for ( cx->x = 0; cx->x < cx->w; cx->x++ )
					{
						cx->finalp[cx->x] = (((int)cx->dstp[cx->x] + (int)dstpp[cx->x]) >> 1);
					}
					// Now do the rest.
					cx->dstp = *image + cx->dpitch;
					dstpp = cx->dstp - cx->dpitch;
					dstpn = cx->dstp + cx->dpitch;
					cx->finalp = final + cx->dpitch;
					for ( cx->y = 1; cx->y < cx->h - 1; cx->y++ )
					{
						for ( cx->x = 0; cx->x < cx->w; cx->x++ )
						{
							v1 = (int) cx->dstp[cx->x] - cx->dthresh;
							if ( v1 < 0 )
								v1 = 0; 
							v2 = (int) cx->dstp[cx->x] + cx->dthresh;
							if (v2 > 235) v2 = 235; 
							if ((v1 > dstpp[cx->x] && v1 > dstpn[cx->x]) || (v2 < dstpp[cx->x] && v2 < dstpn[cx->x]))
							{
								if ( cx->post == POST_FULL_MAP || cx->post == POST_FULL_NOMATCH_MAP )
								{
									if (cx->x & 1) cx->finalp[cx->x] = 128;
									else cx->finalp[cx->x] = 235;
								}
								else
									cx->finalp[cx->x] = ((int)dstpp[cx->x] + (int)dstpn[cx->x] + (int)cx->dstp[cx->x] + (int)cx->dstp[cx->x]) >> 2;
							}
							else cx->finalp[cx->x] = cx->dstp[cx->x];
						}
						cx->finalp += cx->dpitch;
						cx->dstp += cx->dpitch;
						dstpp += cx->dpitch;
						dstpn += cx->dpitch;
					}
		
		
					if (cx->show ) Show( cx, pos, frame_properties);
					if (cx->debug) Debug(cx, pos);
					if (cx->hints) WriteHints(cx->film, cx->inpattern, frame_properties);
					goto final;
				}
		
				// Interpolate mode.
				cx->dstp = *image + cx->dpitch;
				dstpp = cx->dstp - cx->dpitch;
				dstpn = cx->dstp + cx->dpitch;
				for ( cx->y = 1; cx->y < cx->h - 1; cx->y+=2 )
				{
					for ( cx->x = 0; cx->x < cx->w; cx->x++ )
					{
						v1 = (int) cx->dstp[cx->x] - cx->dthresh;
						if (v1 < 0) v1 = 0; 
						v2 = (int) cx->dstp[cx->x] + cx->dthresh;
						if (v2 > 235) v2 = 235; 
						if ((v1 > dstpp[cx->x] && v1 > dstpn[cx->x]) || (v2 < dstpp[cx->x] && v2 < dstpn[cx->x]))
						{
							if ( cx->post == POST_FULL_MAP || cx->post == POST_FULL_NOMATCH_MAP )
							{
								if (cx->x & 1) cx->dstp[cx->x] = 128;
								else cx->dstp[cx->x] = 235;
							}
							else
								cx->dstp[cx->x] = (dstpp[cx->x] + dstpn[cx->x]) >> 1;
						}
					}
					cx->dstp += 2 * cx->dpitch;
					dstpp += 2 * cx->dpitch;
					dstpn += 2 * cx->dpitch;
				}
			}
			if (cx->show ) Show( cx, pos, frame_properties);
			if (cx->debug) Debug(cx, pos);
			if (cx->hints) WriteHints(cx->film, cx->inpattern, frame_properties);

final:			
			// Flush frame at tail of period from the cache
			sprintf( key, MLT_POSITION_FMT, pos - 1 );
			mlt_properties_set_data( cx->image_cache, key, NULL, 0, NULL, NULL );
		}
		else
		{
			// Signal the first {cycle} frames as invalid
			mlt_properties_set_int( frame_properties, "garbage", 1 );
		}
	}
	else if ( error == 0 && *format == mlt_image_yuv420p )
	{
		fprintf(stderr,"%s: %d pos " MLT_POSITION_FMT "\n", __FUNCTION__, *width * *height * 3/2, mlt_frame_get_position(frame) );
	}

	return error;
}

/** Process the frame object.
*/

static mlt_frame process( mlt_filter filter, mlt_frame frame )
{
	// Push the filter on to the stack
	mlt_frame_push_service( frame, filter );

	// Push the frame filter
	mlt_frame_push_get_image( frame, get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_telecide_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = process;

		// Allocate the context and set up for garbage collection		
		context cx = (context) mlt_pool_alloc( sizeof(struct context_s) );
		memset( cx, 0, sizeof( struct context_s ) );
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set_data( properties, "context", cx, sizeof(struct context_s), (mlt_destructor)mlt_pool_release, NULL );

		// Allocate the metrics cache and set up for garbage collection
		cx->cache = (struct CACHE_ENTRY *) mlt_pool_alloc(CACHE_SIZE * sizeof(struct CACHE_ENTRY ));
		mlt_properties_set_data( properties, "cache", cx->cache, CACHE_SIZE * sizeof(struct CACHE_ENTRY), (mlt_destructor)mlt_pool_release, NULL );
		int i;
		for (i = 0; i < CACHE_SIZE; i++)
		{
			cx->cache[i].frame = 0xffffffff;
			cx->cache[i].chosen = 0xff;
		}
		
		// Allocate the image cache and set up for garbage collection
		cx->image_cache = mlt_properties_new();
		mlt_properties_set_data( properties, "image_cache", cx->image_cache, 0, (mlt_destructor)mlt_properties_close, NULL );
		
		// Initialize the parameter defaults
		mlt_properties_set_int( properties, "guide", 0 );
		mlt_properties_set_int( properties, "back", 0 );
		mlt_properties_set_int( properties, "chroma", 0 );
		mlt_properties_set_int( properties, "post", POST_FULL );
		mlt_properties_set_double( properties, "gthresh", 10.0 );
		mlt_properties_set_double( properties, "vthresh", 50.0 );
		mlt_properties_set_double( properties, "bthresh", 50.0 );
		mlt_properties_set_double( properties, "dthresh", 7.0 );
		mlt_properties_set_int( properties, "blend", 0 );
		mlt_properties_set_int( properties, "nt", 10 );
		mlt_properties_set_int( properties, "y0", 0 );
		mlt_properties_set_int( properties, "y1", 0 );
		mlt_properties_set_int( properties, "hints", 1 );
	}
	return filter;
}

