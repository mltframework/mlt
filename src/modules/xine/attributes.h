/*
 * attributes.h
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* use gcc attribs to align critical data structures */

#ifndef ATTRIBUTE_H_
#define ATTRIBUTE_H_

#ifdef ATTRIBUTE_ALIGNED_MAX
#define ATTR_ALIGN(align) __attribute__ ((__aligned__ ((ATTRIBUTE_ALIGNED_MAX < align) ? ATTRIBUTE_ALIGNED_MAX : align)))
#else
#define ATTR_ALIGN(align)
#endif

/* disable GNU __attribute__ extension, when not compiling with GNU C */
#if defined(__GNUC__) || defined (__ICC)
#ifndef ATTRIBUTE_PACKED
#define	ATTRIBUTE_PACKED 1
#endif 
#else
#undef	ATTRIBUTE_PACKED
#ifndef __attribute__
#define	__attribute__(x)	/**/
#endif /* __attribute __*/
#endif

#endif /* ATTRIBUTE_H_ */

