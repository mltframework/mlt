/*
 * Copyright (C) 2000-2004 the xine project
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
 */
#ifndef XINEUTILS_H
#define XINEUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <pthread.h>
#if HAVE_LIBGEN_H
#  include <libgen.h>
#endif

//#ifdef XINE_COMPILE
#  include "attributes.h"
//#  include "compat.h"
//#  include "xmlparser.h"
//#  include "xine_buffer.h"
//#  include "configfile.h"
//#else
//#  include <xine/attributes.h>
//#  include <xine/compat.h>
//#  include <xine/xmlparser.h>
//#  include <xine/xine_buffer.h>
//#  include <xine/configfile.h>
//#endif

//#ifdef HAVE_CONFIG_H
//#include "config.h"
//#endif

#include <stdio.h>
#include <string.h>

  /*
   * debugable mutexes
   */

  typedef struct {
    pthread_mutex_t  mutex;
    char             id[80];
    char            *locked_by;
  } xine_mutex_t;

  int xine_mutex_init    (xine_mutex_t *mutex, const pthread_mutexattr_t *mutexattr,
			  char *id);

  int xine_mutex_lock    (xine_mutex_t *mutex, char *who);
  int xine_mutex_unlock  (xine_mutex_t *mutex, char *who);
  int xine_mutex_destroy (xine_mutex_t *mutex);



			/* CPU Acceleration */

/*
 * The type of an value that fits in an MMX register (note that long
 * long constant values MUST be suffixed by LL and unsigned long long
 * values by ULL, lest they be truncated by the compiler)
 */

/* generic accelerations */
#define MM_ACCEL_MLIB           0x00000001

/* x86 accelerations */
#define MM_ACCEL_X86_MMX        0x80000000
#define MM_ACCEL_X86_3DNOW      0x40000000
#define MM_ACCEL_X86_MMXEXT     0x20000000
#define MM_ACCEL_X86_SSE	0x10000000
#define MM_ACCEL_X86_SSE2	0x08000000
/* powerpc accelerations */
#define MM_ACCEL_PPC_ALTIVEC    0x04000000
/* x86 compat defines */
#define MM_MMX                  MM_ACCEL_X86_MMX
#define MM_3DNOW                MM_ACCEL_X86_3DNOW
#define MM_MMXEXT               MM_ACCEL_X86_MMXEXT
#define MM_SSE                  MM_ACCEL_X86_SSE
#define MM_SSE2                 MM_ACCEL_X86_SSE2

uint32_t xine_mm_accel (void);

#ifdef USE_MMX

typedef	union {
	int64_t			q;	/* Quadword (64-bit) value */
	uint64_t		uq;	/* Unsigned Quadword */
	int			d[2];	/* 2 Doubleword (32-bit) values */
	unsigned int		ud[2];	/* 2 Unsigned Doubleword */
	short			w[4];	/* 4 Word (16-bit) values */
	unsigned short		uw[4];	/* 4 Unsigned Word */
	char			b[8];	/* 8 Byte (8-bit) values */
	unsigned char		ub[8];	/* 8 Unsigned Byte */
	float			s[2];	/* Single-precision (32-bit) value */
} ATTR_ALIGN(8) mmx_t;	/* On an 8-byte (64-bit) boundary */



#define	mmx_i2r(op,imm,reg) \
	__asm__ __volatile__ (#op " %0, %%" #reg \
			      : /* nothing */ \
			      : "i" (imm) )

#define	mmx_m2r(op,mem,reg) \
	__asm__ __volatile__ (#op " %0, %%" #reg \
			      : /* nothing */ \
			      : "m" (mem))

#define	mmx_r2m(op,reg,mem) \
	__asm__ __volatile__ (#op " %%" #reg ", %0" \
			      : "=m" (mem) \
			      : /* nothing */ )

#define	mmx_r2r(op,regs,regd) \
	__asm__ __volatile__ (#op " %" #regs ", %" #regd)


#define	emms() __asm__ __volatile__ ("emms")

#define	movd_m2r(var,reg)	mmx_m2r (movd, var, reg)
#define	movd_r2m(reg,var)	mmx_r2m (movd, reg, var)
#define	movd_r2r(regs,regd)	mmx_r2r (movd, regs, regd)

#define	movq_m2r(var,reg)	mmx_m2r (movq, var, reg)
#define	movq_r2m(reg,var)	mmx_r2m (movq, reg, var)
#define	movq_r2r(regs,regd)	mmx_r2r (movq, regs, regd)

#define	packssdw_m2r(var,reg)	mmx_m2r (packssdw, var, reg)
#define	packssdw_r2r(regs,regd) mmx_r2r (packssdw, regs, regd)
#define	packsswb_m2r(var,reg)	mmx_m2r (packsswb, var, reg)
#define	packsswb_r2r(regs,regd) mmx_r2r (packsswb, regs, regd)

#define	packuswb_m2r(var,reg)	mmx_m2r (packuswb, var, reg)
#define	packuswb_r2r(regs,regd) mmx_r2r (packuswb, regs, regd)

#define	paddb_m2r(var,reg)	mmx_m2r (paddb, var, reg)
#define	paddb_r2r(regs,regd)	mmx_r2r (paddb, regs, regd)
#define	paddd_m2r(var,reg)	mmx_m2r (paddd, var, reg)
#define	paddd_r2r(regs,regd)	mmx_r2r (paddd, regs, regd)
#define	paddw_m2r(var,reg)	mmx_m2r (paddw, var, reg)
#define	paddw_r2r(regs,regd)	mmx_r2r (paddw, regs, regd)

#define	paddsb_m2r(var,reg)	mmx_m2r (paddsb, var, reg)
#define	paddsb_r2r(regs,regd)	mmx_r2r (paddsb, regs, regd)
#define	paddsw_m2r(var,reg)	mmx_m2r (paddsw, var, reg)
#define	paddsw_r2r(regs,regd)	mmx_r2r (paddsw, regs, regd)

#define	paddusb_m2r(var,reg)	mmx_m2r (paddusb, var, reg)
#define	paddusb_r2r(regs,regd)	mmx_r2r (paddusb, regs, regd)
#define	paddusw_m2r(var,reg)	mmx_m2r (paddusw, var, reg)
#define	paddusw_r2r(regs,regd)	mmx_r2r (paddusw, regs, regd)

#define	pand_m2r(var,reg)	mmx_m2r (pand, var, reg)
#define	pand_r2r(regs,regd)	mmx_r2r (pand, regs, regd)

#define	pandn_m2r(var,reg)	mmx_m2r (pandn, var, reg)
#define	pandn_r2r(regs,regd)	mmx_r2r (pandn, regs, regd)

#define	pcmpeqb_m2r(var,reg)	mmx_m2r (pcmpeqb, var, reg)
#define	pcmpeqb_r2r(regs,regd)	mmx_r2r (pcmpeqb, regs, regd)
#define	pcmpeqd_m2r(var,reg)	mmx_m2r (pcmpeqd, var, reg)
#define	pcmpeqd_r2r(regs,regd)	mmx_r2r (pcmpeqd, regs, regd)
#define	pcmpeqw_m2r(var,reg)	mmx_m2r (pcmpeqw, var, reg)
#define	pcmpeqw_r2r(regs,regd)	mmx_r2r (pcmpeqw, regs, regd)

#define	pcmpgtb_m2r(var,reg)	mmx_m2r (pcmpgtb, var, reg)
#define	pcmpgtb_r2r(regs,regd)	mmx_r2r (pcmpgtb, regs, regd)
#define	pcmpgtd_m2r(var,reg)	mmx_m2r (pcmpgtd, var, reg)
#define	pcmpgtd_r2r(regs,regd)	mmx_r2r (pcmpgtd, regs, regd)
#define	pcmpgtw_m2r(var,reg)	mmx_m2r (pcmpgtw, var, reg)
#define	pcmpgtw_r2r(regs,regd)	mmx_r2r (pcmpgtw, regs, regd)

#define	pmaddwd_m2r(var,reg)	mmx_m2r (pmaddwd, var, reg)
#define	pmaddwd_r2r(regs,regd)	mmx_r2r (pmaddwd, regs, regd)

#define	pmulhw_m2r(var,reg)	mmx_m2r (pmulhw, var, reg)
#define	pmulhw_r2r(regs,regd)	mmx_r2r (pmulhw, regs, regd)

#define	pmullw_m2r(var,reg)	mmx_m2r (pmullw, var, reg)
#define	pmullw_r2r(regs,regd)	mmx_r2r (pmullw, regs, regd)

#define	por_m2r(var,reg)	mmx_m2r (por, var, reg)
#define	por_r2r(regs,regd)	mmx_r2r (por, regs, regd)

#define	pslld_i2r(imm,reg)	mmx_i2r (pslld, imm, reg)
#define	pslld_m2r(var,reg)	mmx_m2r (pslld, var, reg)
#define	pslld_r2r(regs,regd)	mmx_r2r (pslld, regs, regd)
#define	psllq_i2r(imm,reg)	mmx_i2r (psllq, imm, reg)
#define	psllq_m2r(var,reg)	mmx_m2r (psllq, var, reg)
#define	psllq_r2r(regs,regd)	mmx_r2r (psllq, regs, regd)
#define	psllw_i2r(imm,reg)	mmx_i2r (psllw, imm, reg)
#define	psllw_m2r(var,reg)	mmx_m2r (psllw, var, reg)
#define	psllw_r2r(regs,regd)	mmx_r2r (psllw, regs, regd)

#define	psrad_i2r(imm,reg)	mmx_i2r (psrad, imm, reg)
#define	psrad_m2r(var,reg)	mmx_m2r (psrad, var, reg)
#define	psrad_r2r(regs,regd)	mmx_r2r (psrad, regs, regd)
#define	psraw_i2r(imm,reg)	mmx_i2r (psraw, imm, reg)
#define	psraw_m2r(var,reg)	mmx_m2r (psraw, var, reg)
#define	psraw_r2r(regs,regd)	mmx_r2r (psraw, regs, regd)

#define	psrld_i2r(imm,reg)	mmx_i2r (psrld, imm, reg)
#define	psrld_m2r(var,reg)	mmx_m2r (psrld, var, reg)
#define	psrld_r2r(regs,regd)	mmx_r2r (psrld, regs, regd)
#define	psrlq_i2r(imm,reg)	mmx_i2r (psrlq, imm, reg)
#define	psrlq_m2r(var,reg)	mmx_m2r (psrlq, var, reg)
#define	psrlq_r2r(regs,regd)	mmx_r2r (psrlq, regs, regd)
#define	psrlw_i2r(imm,reg)	mmx_i2r (psrlw, imm, reg)
#define	psrlw_m2r(var,reg)	mmx_m2r (psrlw, var, reg)
#define	psrlw_r2r(regs,regd)	mmx_r2r (psrlw, regs, regd)

#define	psubb_m2r(var,reg)	mmx_m2r (psubb, var, reg)
#define	psubb_r2r(regs,regd)	mmx_r2r (psubb, regs, regd)
#define	psubd_m2r(var,reg)	mmx_m2r (psubd, var, reg)
#define	psubd_r2r(regs,regd)	mmx_r2r (psubd, regs, regd)
#define	psubw_m2r(var,reg)	mmx_m2r (psubw, var, reg)
#define	psubw_r2r(regs,regd)	mmx_r2r (psubw, regs, regd)

#define	psubsb_m2r(var,reg)	mmx_m2r (psubsb, var, reg)
#define	psubsb_r2r(regs,regd)	mmx_r2r (psubsb, regs, regd)
#define	psubsw_m2r(var,reg)	mmx_m2r (psubsw, var, reg)
#define	psubsw_r2r(regs,regd)	mmx_r2r (psubsw, regs, regd)

#define	psubusb_m2r(var,reg)	mmx_m2r (psubusb, var, reg)
#define	psubusb_r2r(regs,regd)	mmx_r2r (psubusb, regs, regd)
#define	psubusw_m2r(var,reg)	mmx_m2r (psubusw, var, reg)
#define	psubusw_r2r(regs,regd)	mmx_r2r (psubusw, regs, regd)

#define	punpckhbw_m2r(var,reg)		mmx_m2r (punpckhbw, var, reg)
#define	punpckhbw_r2r(regs,regd)	mmx_r2r (punpckhbw, regs, regd)
#define	punpckhdq_m2r(var,reg)		mmx_m2r (punpckhdq, var, reg)
#define	punpckhdq_r2r(regs,regd)	mmx_r2r (punpckhdq, regs, regd)
#define	punpckhwd_m2r(var,reg)		mmx_m2r (punpckhwd, var, reg)
#define	punpckhwd_r2r(regs,regd)	mmx_r2r (punpckhwd, regs, regd)

#define	punpcklbw_m2r(var,reg) 		mmx_m2r (punpcklbw, var, reg)
#define	punpcklbw_r2r(regs,regd)	mmx_r2r (punpcklbw, regs, regd)
#define	punpckldq_m2r(var,reg)		mmx_m2r (punpckldq, var, reg)
#define	punpckldq_r2r(regs,regd)	mmx_r2r (punpckldq, regs, regd)
#define	punpcklwd_m2r(var,reg)		mmx_m2r (punpcklwd, var, reg)
#define	punpcklwd_r2r(regs,regd)	mmx_r2r (punpcklwd, regs, regd)

#define	pxor_m2r(var,reg)	mmx_m2r (pxor, var, reg)
#define	pxor_r2r(regs,regd)	mmx_r2r (pxor, regs, regd)


/* 3DNOW extensions */

#define pavgusb_m2r(var,reg)	mmx_m2r (pavgusb, var, reg)
#define pavgusb_r2r(regs,regd)	mmx_r2r (pavgusb, regs, regd)


/* AMD MMX extensions - also available in intel SSE */


#define mmx_m2ri(op,mem,reg,imm) \
        __asm__ __volatile__ (#op " %1, %0, %%" #reg \
                              : /* nothing */ \
                              : "X" (mem), "X" (imm))
#define mmx_r2ri(op,regs,regd,imm) \
        __asm__ __volatile__ (#op " %0, %%" #regs ", %%" #regd \
                              : /* nothing */ \
                              : "X" (imm) )

#define	mmx_fetch(mem,hint) \
	__asm__ __volatile__ ("prefetch" #hint " %0" \
			      : /* nothing */ \
			      : "X" (mem))


#define	maskmovq(regs,maskreg)		mmx_r2ri (maskmovq, regs, maskreg)

#define	movntq_r2m(mmreg,var)		mmx_r2m (movntq, mmreg, var)

#define	pavgb_m2r(var,reg)		mmx_m2r (pavgb, var, reg)
#define	pavgb_r2r(regs,regd)		mmx_r2r (pavgb, regs, regd)
#define	pavgw_m2r(var,reg)		mmx_m2r (pavgw, var, reg)
#define	pavgw_r2r(regs,regd)		mmx_r2r (pavgw, regs, regd)

#define	pextrw_r2r(mmreg,reg,imm)	mmx_r2ri (pextrw, mmreg, reg, imm)

#define	pinsrw_r2r(reg,mmreg,imm)	mmx_r2ri (pinsrw, reg, mmreg, imm)

#define	pmaxsw_m2r(var,reg)		mmx_m2r (pmaxsw, var, reg)
#define	pmaxsw_r2r(regs,regd)		mmx_r2r (pmaxsw, regs, regd)

#define	pmaxub_m2r(var,reg)		mmx_m2r (pmaxub, var, reg)
#define	pmaxub_r2r(regs,regd)		mmx_r2r (pmaxub, regs, regd)

#define	pminsw_m2r(var,reg)		mmx_m2r (pminsw, var, reg)
#define	pminsw_r2r(regs,regd)		mmx_r2r (pminsw, regs, regd)

#define	pminub_m2r(var,reg)		mmx_m2r (pminub, var, reg)
#define	pminub_r2r(regs,regd)		mmx_r2r (pminub, regs, regd)

#define	pmovmskb(mmreg,reg) \
	__asm__ __volatile__ ("movmskps %" #mmreg ", %" #reg)

#define	pmulhuw_m2r(var,reg)		mmx_m2r (pmulhuw, var, reg)
#define	pmulhuw_r2r(regs,regd)		mmx_r2r (pmulhuw, regs, regd)

#define	prefetcht0(mem)			mmx_fetch (mem, t0)
#define	prefetcht1(mem)			mmx_fetch (mem, t1)
#define	prefetcht2(mem)			mmx_fetch (mem, t2)
#define	prefetchnta(mem)		mmx_fetch (mem, nta)

#define	psadbw_m2r(var,reg)		mmx_m2r (psadbw, var, reg)
#define	psadbw_r2r(regs,regd)		mmx_r2r (psadbw, regs, regd)

#define	pshufw_m2r(var,reg,imm)		mmx_m2ri(pshufw, var, reg, imm)
#define	pshufw_r2r(regs,regd,imm)	mmx_r2ri(pshufw, regs, regd, imm)

#define	sfence() __asm__ __volatile__ ("sfence\n\t")

typedef	union {
	float			sf[4];	/* Single-precision (32-bit) value */
} ATTR_ALIGN(16) sse_t;	/* On a 16 byte (128-bit) boundary */


#define	sse_i2r(op, imm, reg) \
	__asm__ __volatile__ (#op " %0, %%" #reg \
			      : /* nothing */ \
			      : "X" (imm) )

#define	sse_m2r(op, mem, reg) \
	__asm__ __volatile__ (#op " %0, %%" #reg \
			      : /* nothing */ \
			      : "X" (mem))

#define	sse_r2m(op, reg, mem) \
	__asm__ __volatile__ (#op " %%" #reg ", %0" \
			      : "=X" (mem) \
			      : /* nothing */ )

#define	sse_r2r(op, regs, regd) \
	__asm__ __volatile__ (#op " %" #regs ", %" #regd)

#define	sse_r2ri(op, regs, regd, imm) \
	__asm__ __volatile__ (#op " %0, %%" #regs ", %%" #regd \
			      : /* nothing */ \
			      : "X" (imm) )

#define	sse_m2ri(op, mem, reg, subop) \
	__asm__ __volatile__ (#op " %0, %%" #reg ", " #subop \
			      : /* nothing */ \
			      : "X" (mem))


#define	movaps_m2r(var, reg)	sse_m2r(movaps, var, reg)
#define	movaps_r2m(reg, var)	sse_r2m(movaps, reg, var)
#define	movaps_r2r(regs, regd)	sse_r2r(movaps, regs, regd)

#define	movntps_r2m(xmmreg, var)	sse_r2m(movntps, xmmreg, var)

#define	movups_m2r(var, reg)	sse_m2r(movups, var, reg)
#define	movups_r2m(reg, var)	sse_r2m(movups, reg, var)
#define	movups_r2r(regs, regd)	sse_r2r(movups, regs, regd)

#define	movhlps_r2r(regs, regd)	sse_r2r(movhlps, regs, regd)

#define	movlhps_r2r(regs, regd)	sse_r2r(movlhps, regs, regd)

#define	movhps_m2r(var, reg)	sse_m2r(movhps, var, reg)
#define	movhps_r2m(reg, var)	sse_r2m(movhps, reg, var)

#define	movlps_m2r(var, reg)	sse_m2r(movlps, var, reg)
#define	movlps_r2m(reg, var)	sse_r2m(movlps, reg, var)

#define	movss_m2r(var, reg)	sse_m2r(movss, var, reg)
#define	movss_r2m(reg, var)	sse_r2m(movss, reg, var)
#define	movss_r2r(regs, regd)	sse_r2r(movss, regs, regd)

#define	shufps_m2r(var, reg, index)	sse_m2ri(shufps, var, reg, index)
#define	shufps_r2r(regs, regd, index)	sse_r2ri(shufps, regs, regd, index)

#define	cvtpi2ps_m2r(var, xmmreg)	sse_m2r(cvtpi2ps, var, xmmreg)
#define	cvtpi2ps_r2r(mmreg, xmmreg)	sse_r2r(cvtpi2ps, mmreg, xmmreg)

#define	cvtps2pi_m2r(var, mmreg)	sse_m2r(cvtps2pi, var, mmreg)
#define	cvtps2pi_r2r(xmmreg, mmreg)	sse_r2r(cvtps2pi, mmreg, xmmreg)

#define	cvttps2pi_m2r(var, mmreg)	sse_m2r(cvttps2pi, var, mmreg)
#define	cvttps2pi_r2r(xmmreg, mmreg)	sse_r2r(cvttps2pi, mmreg, xmmreg)

#define	cvtsi2ss_m2r(var, xmmreg)	sse_m2r(cvtsi2ss, var, xmmreg)
#define	cvtsi2ss_r2r(reg, xmmreg)	sse_r2r(cvtsi2ss, reg, xmmreg)

#define	cvtss2si_m2r(var, reg)		sse_m2r(cvtss2si, var, reg)
#define	cvtss2si_r2r(xmmreg, reg)	sse_r2r(cvtss2si, xmmreg, reg)

#define	cvttss2si_m2r(var, reg)		sse_m2r(cvtss2si, var, reg)
#define	cvttss2si_r2r(xmmreg, reg)	sse_r2r(cvtss2si, xmmreg, reg)

#define	movmskps(xmmreg, reg) \
	__asm__ __volatile__ ("movmskps %" #xmmreg ", %" #reg)

#define	addps_m2r(var, reg)		sse_m2r(addps, var, reg)
#define	addps_r2r(regs, regd)		sse_r2r(addps, regs, regd)

#define	addss_m2r(var, reg)		sse_m2r(addss, var, reg)
#define	addss_r2r(regs, regd)		sse_r2r(addss, regs, regd)

#define	subps_m2r(var, reg)		sse_m2r(subps, var, reg)
#define	subps_r2r(regs, regd)		sse_r2r(subps, regs, regd)

#define	subss_m2r(var, reg)		sse_m2r(subss, var, reg)
#define	subss_r2r(regs, regd)		sse_r2r(subss, regs, regd)

#define	mulps_m2r(var, reg)		sse_m2r(mulps, var, reg)
#define	mulps_r2r(regs, regd)		sse_r2r(mulps, regs, regd)

#define	mulss_m2r(var, reg)		sse_m2r(mulss, var, reg)
#define	mulss_r2r(regs, regd)		sse_r2r(mulss, regs, regd)

#define	divps_m2r(var, reg)		sse_m2r(divps, var, reg)
#define	divps_r2r(regs, regd)		sse_r2r(divps, regs, regd)

#define	divss_m2r(var, reg)		sse_m2r(divss, var, reg)
#define	divss_r2r(regs, regd)		sse_r2r(divss, regs, regd)

#define	rcpps_m2r(var, reg)		sse_m2r(rcpps, var, reg)
#define	rcpps_r2r(regs, regd)		sse_r2r(rcpps, regs, regd)

#define	rcpss_m2r(var, reg)		sse_m2r(rcpss, var, reg)
#define	rcpss_r2r(regs, regd)		sse_r2r(rcpss, regs, regd)

#define	rsqrtps_m2r(var, reg)		sse_m2r(rsqrtps, var, reg)
#define	rsqrtps_r2r(regs, regd)		sse_r2r(rsqrtps, regs, regd)

#define	rsqrtss_m2r(var, reg)		sse_m2r(rsqrtss, var, reg)
#define	rsqrtss_r2r(regs, regd)		sse_r2r(rsqrtss, regs, regd)

#define	sqrtps_m2r(var, reg)		sse_m2r(sqrtps, var, reg)
#define	sqrtps_r2r(regs, regd)		sse_r2r(sqrtps, regs, regd)

#define	sqrtss_m2r(var, reg)		sse_m2r(sqrtss, var, reg)
#define	sqrtss_r2r(regs, regd)		sse_r2r(sqrtss, regs, regd)

#define	andps_m2r(var, reg)		sse_m2r(andps, var, reg)
#define	andps_r2r(regs, regd)		sse_r2r(andps, regs, regd)

#define	andnps_m2r(var, reg)		sse_m2r(andnps, var, reg)
#define	andnps_r2r(regs, regd)		sse_r2r(andnps, regs, regd)

#define	orps_m2r(var, reg)		sse_m2r(orps, var, reg)
#define	orps_r2r(regs, regd)		sse_r2r(orps, regs, regd)

#define	xorps_m2r(var, reg)		sse_m2r(xorps, var, reg)
#define	xorps_r2r(regs, regd)		sse_r2r(xorps, regs, regd)

#define	maxps_m2r(var, reg)		sse_m2r(maxps, var, reg)
#define	maxps_r2r(regs, regd)		sse_r2r(maxps, regs, regd)

#define	maxss_m2r(var, reg)		sse_m2r(maxss, var, reg)
#define	maxss_r2r(regs, regd)		sse_r2r(maxss, regs, regd)

#define	minps_m2r(var, reg)		sse_m2r(minps, var, reg)
#define	minps_r2r(regs, regd)		sse_r2r(minps, regs, regd)

#define	minss_m2r(var, reg)		sse_m2r(minss, var, reg)
#define	minss_r2r(regs, regd)		sse_r2r(minss, regs, regd)

#define	cmpps_m2r(var, reg, op)		sse_m2ri(cmpps, var, reg, op)
#define	cmpps_r2r(regs, regd, op)	sse_r2ri(cmpps, regs, regd, op)

#define	cmpeqps_m2r(var, reg)		sse_m2ri(cmpps, var, reg, 0)
#define	cmpeqps_r2r(regs, regd)		sse_r2ri(cmpps, regs, regd, 0)

#define	cmpltps_m2r(var, reg)		sse_m2ri(cmpps, var, reg, 1)
#define	cmpltps_r2r(regs, regd)		sse_r2ri(cmpps, regs, regd, 1)

#define	cmpleps_m2r(var, reg)		sse_m2ri(cmpps, var, reg, 2)
#define	cmpleps_r2r(regs, regd)		sse_r2ri(cmpps, regs, regd, 2)

#define	cmpunordps_m2r(var, reg)	sse_m2ri(cmpps, var, reg, 3)
#define	cmpunordps_r2r(regs, regd)	sse_r2ri(cmpps, regs, regd, 3)

#define	cmpneqps_m2r(var, reg)		sse_m2ri(cmpps, var, reg, 4)
#define	cmpneqps_r2r(regs, regd)	sse_r2ri(cmpps, regs, regd, 4)

#define	cmpnltps_m2r(var, reg)		sse_m2ri(cmpps, var, reg, 5)
#define	cmpnltps_r2r(regs, regd)	sse_r2ri(cmpps, regs, regd, 5)

#define	cmpnleps_m2r(var, reg)		sse_m2ri(cmpps, var, reg, 6)
#define	cmpnleps_r2r(regs, regd)	sse_r2ri(cmpps, regs, regd, 6)

#define	cmpordps_m2r(var, reg)		sse_m2ri(cmpps, var, reg, 7)
#define	cmpordps_r2r(regs, regd)	sse_r2ri(cmpps, regs, regd, 7)

#define	cmpss_m2r(var, reg, op)		sse_m2ri(cmpss, var, reg, op)
#define	cmpss_r2r(regs, regd, op)	sse_r2ri(cmpss, regs, regd, op)

#define	cmpeqss_m2r(var, reg)		sse_m2ri(cmpss, var, reg, 0)
#define	cmpeqss_r2r(regs, regd)		sse_r2ri(cmpss, regs, regd, 0)

#define	cmpltss_m2r(var, reg)		sse_m2ri(cmpss, var, reg, 1)
#define	cmpltss_r2r(regs, regd)		sse_r2ri(cmpss, regs, regd, 1)

#define	cmpless_m2r(var, reg)		sse_m2ri(cmpss, var, reg, 2)
#define	cmpless_r2r(regs, regd)		sse_r2ri(cmpss, regs, regd, 2)

#define	cmpunordss_m2r(var, reg)	sse_m2ri(cmpss, var, reg, 3)
#define	cmpunordss_r2r(regs, regd)	sse_r2ri(cmpss, regs, regd, 3)

#define	cmpneqss_m2r(var, reg)		sse_m2ri(cmpss, var, reg, 4)
#define	cmpneqss_r2r(regs, regd)	sse_r2ri(cmpss, regs, regd, 4)

#define	cmpnltss_m2r(var, reg)		sse_m2ri(cmpss, var, reg, 5)
#define	cmpnltss_r2r(regs, regd)	sse_r2ri(cmpss, regs, regd, 5)

#define	cmpnless_m2r(var, reg)		sse_m2ri(cmpss, var, reg, 6)
#define	cmpnless_r2r(regs, regd)	sse_r2ri(cmpss, regs, regd, 6)

#define	cmpordss_m2r(var, reg)		sse_m2ri(cmpss, var, reg, 7)
#define	cmpordss_r2r(regs, regd)	sse_r2ri(cmpss, regs, regd, 7)

#define	comiss_m2r(var, reg)		sse_m2r(comiss, var, reg)
#define	comiss_r2r(regs, regd)		sse_r2r(comiss, regs, regd)

#define	ucomiss_m2r(var, reg)		sse_m2r(ucomiss, var, reg)
#define	ucomiss_r2r(regs, regd)		sse_r2r(ucomiss, regs, regd)

#define	unpcklps_m2r(var, reg)		sse_m2r(unpcklps, var, reg)
#define	unpcklps_r2r(regs, regd)	sse_r2r(unpcklps, regs, regd)

#define	unpckhps_m2r(var, reg)		sse_m2r(unpckhps, var, reg)
#define	unpckhps_r2r(regs, regd)	sse_r2r(unpckhps, regs, regd)

#define	fxrstor(mem) \
	__asm__ __volatile__ ("fxrstor %0" \
			      : /* nothing */ \
			      : "X" (mem))

#define	fxsave(mem) \
	__asm__ __volatile__ ("fxsave %0" \
			      : /* nothing */ \
			      : "X" (mem))

#define	stmxcsr(mem) \
	__asm__ __volatile__ ("stmxcsr %0" \
			      : /* nothing */ \
			      : "X" (mem))

#define	ldmxcsr(mem) \
	__asm__ __volatile__ ("ldmxcsr %0" \
			      : /* nothing */ \
			      : "X" (mem))
#endif /* USE_MMX */



		     /* Optimized/fast memcpy */

/*
   TODO : fix dll linkage problem for xine_fast_memcpy on win32

   xine_fast_memcpy dll linkage is screwy here.
   declaring as dllimport seems to fix the problem
   but causes compiler warning with libxineutils
*/
#ifdef _MSC_VER
__declspec( dllimport ) extern void *(* xine_fast_memcpy)(void *to, const void *from, size_t len);
#else
extern void *(* xine_fast_memcpy)(void *to, const void *from, size_t len);
#endif

#ifdef HAVE_XINE_INTERNAL_H
/* Benchmark available memcpy methods */
void xine_probe_fast_memcpy(xine_t *xine);
#endif


/*
 * Debug stuff
 */
/*
 * profiling (unworkable in non DEBUG isn't defined)
 */
void xine_profiler_init (void);
int xine_profiler_allocate_slot (char *label);
void xine_profiler_start_count (int id);
void xine_profiler_stop_count (int id);
void xine_profiler_print_results (void);

/*
 * Allocate and clean memory size_t 'size', then return the pointer
 * to the allocated memory.
 */
#if !defined(__GNUC__) || __GNUC__ < 3
void *xine_xmalloc(size_t size);
#else
void *xine_xmalloc(size_t size) __attribute__ ((__malloc__));
#endif

/*
 * Same as above, but memory is aligned to 'alignement'.
 * **base is used to return pointer to un-aligned memory, use
 * this to free the mem chunk
 */
void *xine_xmalloc_aligned(size_t alignment, size_t size, void **base);

/*
 * Get user home directory.
 */
const char *xine_get_homedir(void);

/*
 * Clean a string (remove spaces and '=' at the begin,
 * and '\n', '\r' and spaces at the end.
 */
char *xine_chomp (char *str);

/*
 * A thread-safe usecond sleep
 */
void xine_usec_sleep(unsigned usec);


  /*
   * Some string functions
   */


void xine_strdupa(char *dest, char *src);
#define xine_strdupa(d, s) do {                                             \
                                (d) = NULL;                                 \
                                if((s) != NULL) {                           \
                                  (d) = (char *) alloca(strlen((s)) + 1);   \
                                  strcpy((d), (s));                         \
                                }                                           \
                              } while(0)

/* Shamefully copied from glibc 2.2.3 */
#ifdef HAVE_STRPBRK
#define xine_strpbrk strpbrk
#else
static inline char *_private_strpbrk(char *s, const char *accept) {

  while(*s != '\0') {
    const char *a = accept;
    while(*a != '\0')
      if(*a++ == *s)
	return s;
    ++s;
  }

  return NULL;
}
#define xine_strpbrk _private_strpbrk
#endif

#if defined HAVE_STRSEP && !defined(_MSC_VER)
#define xine_strsep strsep
#else
static inline char *_private_strsep(char **stringp, const char *delim) {
  char *begin, *end;

  begin = *stringp;
  if(begin == NULL)
    return NULL;

  if(delim[0] == '\0' || delim[1] == '\0') {
    char ch = delim[0];

    if(ch == '\0')
      end = NULL;
    else {
      if(*begin == ch)
	end = begin;
      else if(*begin == '\0')
	end = NULL;
      else
	end = strchr(begin + 1, ch);
    }
  }
  else
    end = xine_strpbrk(begin, delim);

  if(end) {
    *end++ = '\0';
    *stringp = end;
  }
  else
    *stringp = NULL;

  return begin;
}
#define xine_strsep _private_strsep
#endif


#ifdef HAVE_SETENV
#define	xine_setenv	setenv
#else
static inline void _private_setenv(const char *name, const char *val, int _xx) {
  int  len  = strlen(name) + strlen(val) + 2;
  char env[len];

  sprintf(env, "%s%c%s", name, '=', val);
  putenv(env);
}
#define	xine_setenv	_private_setenv
#endif

/*
 * Color Conversion Utility Functions
 * The following data structures and functions facilitate the conversion
 * of RGB images to packed YUV (YUY2) images. There are also functions to
 * convert from YUV9 -> YV12. All of the meaty details are written in
 * color.c.
 */

typedef struct yuv_planes_s {

  unsigned char *y;
  unsigned char *u;
  unsigned char *v;
  unsigned int row_width;    /* frame width */
  unsigned int row_count;    /* frame height */

} yuv_planes_t;

void init_yuv_conversion(void);
void init_yuv_planes(yuv_planes_t *yuv_planes, int width, int height);
void free_yuv_planes(yuv_planes_t *yuv_planes);

extern void (*yuv444_to_yuy2)
  (yuv_planes_t *yuv_planes, unsigned char *yuy2_map, int pitch);
extern void (*yuv9_to_yv12)
  (unsigned char *y_src, int y_src_pitch, unsigned char *y_dest, int y_dest_pitch,
   unsigned char *u_src, int u_src_pitch, unsigned char *u_dest, int u_dest_pitch,
   unsigned char *v_src, int v_src_pitch, unsigned char *v_dest, int v_dest_pitch,
   int width, int height);
extern void (*yuv411_to_yv12)
  (unsigned char *y_src, int y_src_pitch, unsigned char *y_dest, int y_dest_pitch,
   unsigned char *u_src, int u_src_pitch, unsigned char *u_dest, int u_dest_pitch,
   unsigned char *v_src, int v_src_pitch, unsigned char *v_dest, int v_dest_pitch,
   int width, int height);
extern void (*yv12_to_yuy2)
  (unsigned char *y_src, int y_src_pitch,
   unsigned char *u_src, int u_src_pitch,
   unsigned char *v_src, int v_src_pitch,
   unsigned char *yuy2_map, int yuy2_pitch,
   int width, int height, int progressive);
extern void (*yuy2_to_yv12)
  (unsigned char *yuy2_map, int yuy2_pitch,
   unsigned char *y_dst, int y_dst_pitch,
   unsigned char *u_dst, int u_dst_pitch,
   unsigned char *v_dst, int v_dst_pitch,
   int width, int height);

#define SCALEFACTOR 65536
#define CENTERSAMPLE 128

#define COMPUTE_Y(r, g, b) \
  (unsigned char) \
  ((y_r_table[r] + y_g_table[g] + y_b_table[b]) / SCALEFACTOR)
#define COMPUTE_U(r, g, b) \
  (unsigned char) \
  ((u_r_table[r] + u_g_table[g] + u_b_table[b]) / SCALEFACTOR + CENTERSAMPLE)
#define COMPUTE_V(r, g, b) \
  (unsigned char) \
  ((v_r_table[r] + v_g_table[g] + v_b_table[b]) / SCALEFACTOR + CENTERSAMPLE)

#define UNPACK_BGR15(packed_pixel, r, g, b) \
  b = (packed_pixel & 0x7C00) >> 7; \
  g = (packed_pixel & 0x03E0) >> 2; \
  r = (packed_pixel & 0x001F) << 3;

#define UNPACK_BGR16(packed_pixel, r, g, b) \
  b = (packed_pixel & 0xF800) >> 8; \
  g = (packed_pixel & 0x07E0) >> 3; \
  r = (packed_pixel & 0x001F) << 3;

#define UNPACK_RGB15(packed_pixel, r, g, b) \
  r = (packed_pixel & 0x7C00) >> 7; \
  g = (packed_pixel & 0x03E0) >> 2; \
  b = (packed_pixel & 0x001F) << 3;

#define UNPACK_RGB16(packed_pixel, r, g, b) \
  r = (packed_pixel & 0xF800) >> 8; \
  g = (packed_pixel & 0x07E0) >> 3; \
  b = (packed_pixel & 0x001F) << 3;

extern int y_r_table[256];
extern int y_g_table[256];
extern int y_b_table[256];

extern int u_r_table[256];
extern int u_g_table[256];
extern int u_b_table[256];

extern int v_r_table[256];
extern int v_g_table[256];
extern int v_b_table[256];

/* frame copying functions */
extern void yv12_to_yv12
  (unsigned char *y_src, int y_src_pitch, unsigned char *y_dst, int y_dst_pitch,
   unsigned char *u_src, int u_src_pitch, unsigned char *u_dst, int u_dst_pitch,
   unsigned char *v_src, int v_src_pitch, unsigned char *v_dst, int v_dst_pitch,
   int width, int height);
extern void yuy2_to_yuy2
  (unsigned char *src, int src_pitch,
   unsigned char *dst, int dst_pitch,
   int width, int height);

/* print a hexdump of the given data */
void xine_hexdump (const char *buf, int length);

/*
 * Optimization macros for conditions
 * Taken from the FIASCO L4 microkernel sources
 */
#if !defined(__GNUC__) || __GNUC__ < 3
#  define EXPECT_TRUE(x)  (x)
#  define EXPECT_FALSE(x) (x)
#else
#  define EXPECT_TRUE(x)  __builtin_expect((x),1)
#  define EXPECT_FALSE(x) __builtin_expect((x),0)
#endif

#ifdef NDEBUG
#define _x_assert(exp) \
  do {                                                                \
    if (!(exp))                                                       \
      fprintf(stderr, "assert: %s:%d: %s: Assertion `%s' failed.\n",  \
              __FILE__, __LINE__, __XINE_FUNCTION__, #exp);           \
  } while(0)
#else
#define _x_assert(exp) \
  do {                                                                \
    if (!(exp)) {                                                     \
      fprintf(stderr, "assert: %s:%d: %s: Assertion `%s' failed.\n",  \
              __FILE__, __LINE__, __XINE_FUNCTION__, #exp);           \
      abort();                                                        \
    }                                                                 \
  } while(0)
#endif

#define _x_abort()                                                    \
  do {                                                                \
    fprintf(stderr, "abort: %s:%d: %s: Aborting.\n",                  \
            __FILE__, __LINE__, __XINE_FUNCTION__);                   \
    abort();                                                          \
  } while(0)


/****** logging with xine **********************************/

#ifndef LOG_MODULE
  #define LOG_MODULE __FILE__
#endif /* LOG_MODULE */

#define LOG_MODULE_STRING printf("%s: ", LOG_MODULE );

#ifdef LOG_VERBOSE
  #define LONG_LOG_MODULE_STRING                                            \
    printf("%s: (%s:%d) ", LOG_MODULE, __XINE_FUNCTION__, __LINE__ );
#else
  #define LONG_LOG_MODULE_STRING  LOG_MODULE_STRING
#endif /* LOG_VERBOSE */

#ifdef LOG
  #ifdef __GNUC__
    #define lprintf(fmt, args...)                                           \
      do {                                                                  \
        LONG_LOG_MODULE_STRING                                              \
        printf(fmt, ##args);                                                \
      } while(0)
  #else /* __GNUC__ */
    #ifdef _MSC_VER
      #define lprintf(fmtargs)                                              \
        do {                                                                \
          LONG_LOG_MODULE_STRING                                            \
          printf("%s", fmtargs);                                            \
        } while(0)
    #else /* _MSC_VER */
      #define lprintf(fmt, ...)                                             \
        do {                                                                \
          LONG_LOG_MODULE_STRING                                            \
          printf(__VA_ARGS__);                                              \
        } while(0)
    #endif  /* _MSC_VER */
  #endif /* __GNUC__ */
#else /* LOG */
  #ifdef __GNUC__
    #define lprintf(fmt, args...)     do {} while(0)
  #else
  #ifdef _MSC_VER
    #define lprintf
  #else
    #define lprintf(...)              do {} while(0)
  #endif /* _MSC_VER */
  #endif /* __GNUC__ */
#endif /* LOG */

#ifdef __GNUC__
  #define llprintf(cat, fmt, args...)                                       \
    do{                                                                     \
      if(cat){                                                              \
        LONG_LOG_MODULE_STRING                                              \
        printf( fmt, ##args );                                              \
      }                                                                     \
    }while(0)
#else
#ifdef _MSC_VER
  #define llprintf(cat, fmtargs)                                            \
    do{                                                                     \
      if(cat){                                                              \
        LONG_LOG_MODULE_STRING                                              \
        printf( "%s", fmtargs );                                            \
      }                                                                     \
    }while(0)
#else
  #define llprintf(cat, ...)                                                \
    do{                                                                     \
      if(cat){                                                              \
        LONG_LOG_MODULE_STRING                                              \
        printf( __VA_ARGS__ );                                              \
      }                                                                     \
    }while(0)
#endif /* _MSC_VER */
#endif /* __GNUC__ */

#ifdef  __GNUC__
  #define xprintf(xine, verbose, fmt, args...)                              \
    do {                                                                    \
      if((xine) && (xine)->verbosity >= verbose){                           \
        xine_log(xine, XINE_LOG_TRACE, fmt, ##args);                        \
      }                                                                     \
    } while(0)
#else
#ifdef _MSC_VER
  #define xprintf(xine, verbose, fmtargs)                                   \
    do {                                                                    \
      if((xine) && (xine)->verbosity >= verbose){                           \
        xine_log(xine, XINE_LOG_TRACE, fmtargs);                            \
      }                                                                     \
    } while(0)
#else
  #define xprintf(xine, verbose, ...)                                       \
    do {                                                                    \
      if((xine) && (xine)->verbosity >= verbose){                           \
        xine_log(xine, XINE_LOG_TRACE, __VA_ARGS__);                        \
      }                                                                     \
    } while(0)
#endif /* _MSC_VER */
#endif /* __GNUC__ */

/* time measuring macros for profiling tasks */

#ifdef DEBUG
#  define XINE_PROFILE(function)                                            \
     do {                                                                   \
       struct timeval current_time;                                         \
       double dtime;                                                        \
       gettimeofday(&current_time, NULL);                                   \
       dtime = -(current_time.tv_sec + (current_time.tv_usec / 1000000.0)); \
       function;                                                            \
       gettimeofday(&current_time, NULL);                                   \
       dtime += current_time.tv_sec + (current_time.tv_usec / 1000000.0);   \
       printf("%s: (%s:%d) took %lf seconds\n",                             \
              LOG_MODULE, __XINE_FUNCTION__, __LINE__, dtime);              \
     } while(0)
#  define XINE_PROFILE_ACCUMULATE(function)                                 \
     do {                                                                   \
       struct timeval current_time;                                         \
       static double dtime = 0;                                             \
       gettimeofday(&current_time, NULL);                                   \
       dtime -= current_time.tv_sec + (current_time.tv_usec / 1000000.0);   \
       function;                                                            \
       gettimeofday(&current_time, NULL);                                   \
       dtime += current_time.tv_sec + (current_time.tv_usec / 1000000.0);   \
       printf("%s: (%s:%d) took %lf seconds\n",                             \
              LOG_MODULE, __XINE_FUNCTION__, __LINE__, dtime);              \
     } while(0)
#else
#  define XINE_PROFILE(function) function
#  define XINE_PROFILE_ACCUMULATE(function) function
#endif /* LOG */


/******** double chained lists with builtin iterator *******/

typedef struct xine_node_s {

  struct xine_node_s    *next, *prev;

  void                  *content;

  int                    priority;

} xine_node_t;


typedef struct {

  xine_node_t    *first, *last, *cur;

} xine_list_t;



xine_list_t *xine_list_new (void);


/**
 * dispose the whole list.
 * note: disposes _only_ the list structure, content must be free()d elsewhere
 */
void xine_list_free(xine_list_t *l);


/**
 * returns: Boolean
 */
int xine_list_is_empty (xine_list_t *l);

/**
 * return content of first entry in list.
 */
void *xine_list_first_content (xine_list_t *l);

/**
 * return next content in list.
 */
void *xine_list_next_content (xine_list_t *l);

/**
 * Return last content of list.
 */
void *xine_list_last_content (xine_list_t *l);

/**
 * Return previous content of list.
 */
void *xine_list_prev_content (xine_list_t *l);

/**
 * Append content to list, sorted by decreasing priority.
 */
void xine_list_append_priority_content (xine_list_t *l, void *content, int priority);

/**
 * Append content to list.
 */
void xine_list_append_content (xine_list_t *l, void *content);

/**
 * Insert content in list.
 */
void xine_list_insert_content (xine_list_t *l, void *content);

/**
 * Remove current content in list.
 * note: removes only the list entry; content must be free()d elsewhere.
 */
void xine_list_delete_current (xine_list_t *l);

#ifndef HAVE_BASENAME
/*
 * get base name
 */
char *basename (char const *name);
#endif


#ifdef __cplusplus
}
#endif

#endif
