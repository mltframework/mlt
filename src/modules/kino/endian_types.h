/* <endian_types.h>
 *
 * Quick hack to handle endianness and word length issues.
 * Defines _le, _be, and _ne variants to standard ISO types
 * like int32_t, that are stored in little-endian, big-endian,
 * and native-endian byteorder in memory, respectively.
 * Caveat: int32_le_t and friends cannot be used in vararg
 * functions like printf() without an explicit cast.
 *
 * Copyright (c) 2003-2005 Daniel Kobras <kobras@debian.org>
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

#ifndef _ENDIAN_TYPES_H
#define _ENDIAN_TYPES_H

/* Needed for BYTE_ORDER and BIG/LITTLE_ENDIAN macros. */
#if !defined(__FreeBSD__) && !defined(__NetBSD__)
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
# include <endian.h>
# undef  _BSD_SOURCE
#else
# include <endian.h>
#endif
#else
# include <sys/endian.h>
#endif /* !defined(__FreeBSD__) && !defined(__NetBSD__) */

#include <sys/types.h>
#if !defined(__FreeBSD__) && !defined(__NetBSD__)
#include <byteswap.h>
#else
#define bswap_16(x) bswap16(x)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif /* !defined(__FreeBSD__) && !defined(__NetBSD__) */

static inline int8_t bswap(const int8_t& x)
{
	return x;
}

static inline u_int8_t bswap(const u_int8_t& x)
{
	return x;
}

static inline int16_t bswap(const int16_t& x)
{
	return bswap_16(x);
}

static inline u_int16_t bswap(const u_int16_t& x)
{
	return bswap_16(x);
}

static inline int32_t bswap(const int32_t& x)
{
	return bswap_32(x);
}

static inline u_int32_t bswap(const u_int32_t& x)
{
	return bswap_32(x);
}

static inline int64_t bswap(const int64_t& x)
{
	return bswap_64(x);
}

static inline u_int64_t bswap(const u_int64_t& x)
{
	return bswap_64(x);
}

#define le_to_cpu	cpu_to_le
#define be_to_cpu	cpu_to_be

template <class T> static inline T cpu_to_le(const T& x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	return x;
#else
	return bswap(x);
#endif
}

template <class T> static inline T cpu_to_be(const T& x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	return bswap(x);
#else
	return x;
#endif
}

template <class T> class le_t {
	T	m;
	T	read() const {
		return le_to_cpu(m);
	};
	void	write(const T& n) {
		m = cpu_to_le(n);
	};
public:
	le_t(void) {
		m = 0;
	};
	le_t(const T& o) {
		write(o);
	};
	operator T() const {
		return read();
	};
	le_t<T> operator++() {
		write(read() + 1);
		return *this;
	};
	le_t<T> operator++(int) {
		write(read() + 1);
		return *this;
	};
	le_t<T> operator--() {
		write(read() - 1);
		return *this;
	};
	le_t<T> operator--(int) {
		write(read() - 1);
		return *this;
	};
	le_t<T>& operator+=(const T& t) {
		write(read() + t);
		return *this;
	};
	le_t<T>& operator-=(const T& t) {
		write(read() - t);
		return *this;
	};
	le_t<T>& operator&=(const le_t<T>& t) {
		m &= t.m;
		return *this;
	};
	le_t<T>& operator|=(const le_t<T>& t) {
		m |= t.m;
		return *this;
	};
} __attribute__((packed));

/* Just copy-and-pasted from le_t. Too lazy to do it right. */

template <class T> class be_t {
	T	m;
	T	read() const {
		return be_to_cpu(m);
	};
	void	write(const T& n) {
		m = cpu_to_be(n);
	};
public:
	be_t(void) {
		m = 0;
	};
	be_t(const T& o) {
		write(o);
	};
	operator T() const {
		return read();
	};
	be_t<T> operator++() {
		write(read() + 1);
		return *this;
	};
	be_t<T> operator++(int) {
		write(read() + 1);
		return *this;
	};
	be_t<T> operator--() {
		write(read() - 1);
		return *this;
	};
	be_t<T> operator--(int) {
		write(read() - 1);
		return *this;
	};
	be_t<T>& operator+=(const T& t) {
		write(read() + t);
		return *this;
	};
	be_t<T>& operator-=(const T& t) {
		write(read() - t);
		return *this;
	};
	be_t<T>& operator&=(const be_t<T>& t) {
		m &= t.m;
		return *this;
	};
	be_t<T>& operator|=(const be_t<T>& t) {
		m |= t.m;
		return *this;
	};
} __attribute__((packed));

/* Define types of native endianness similar to the little and big endian
 * versions below. Not really necessary but useful occasionally to emphasize
 * endianness of data.
 */

typedef	int8_t		int8_ne_t;
typedef	int16_t		int16_ne_t;
typedef	int32_t		int32_ne_t;
typedef	int64_t		int64_ne_t;
typedef	u_int8_t	u_int8_ne_t;
typedef	u_int16_t	u_int16_ne_t;
typedef	u_int32_t	u_int32_ne_t;
typedef	u_int64_t	u_int64_ne_t;


/* The classes work on their native endianness as well, but obviously
 * introduce some overhead.  Use the faster typedefs to native types
 * therefore, unless you're debugging.
 */

#if BYTE_ORDER == LITTLE_ENDIAN
typedef	int8_ne_t	int8_le_t;
typedef	int16_ne_t	int16_le_t;
typedef	int32_ne_t	int32_le_t;
typedef	int64_ne_t	int64_le_t;
typedef	u_int8_ne_t	u_int8_le_t;
typedef	u_int16_ne_t	u_int16_le_t;
typedef	u_int32_ne_t	u_int32_le_t;
typedef	u_int64_ne_t	u_int64_le_t;
typedef int8_t		int8_be_t;
typedef be_t<int16_t>	int16_be_t;
typedef be_t<int32_t>	int32_be_t;
typedef be_t<int64_t>	int64_be_t;
typedef u_int8_t	u_int8_be_t;
typedef be_t<u_int16_t>	u_int16_be_t;
typedef be_t<u_int32_t>	u_int32_be_t;
typedef be_t<u_int64_t>	u_int64_be_t;
#else
typedef	int8_ne_t	int8_be_t;
typedef	int16_ne_t	int16_be_t;
typedef	int32_ne_t	int32_be_t;
typedef	int64_ne_t	int64_be_t;
typedef	u_int8_ne_t	u_int8_be_t;
typedef	u_int16_ne_t	u_int16_be_t;
typedef	u_int32_ne_t	u_int32_be_t;
typedef	u_int64_ne_t	u_int64_be_t;
typedef int8_t		int8_le_t;
typedef le_t<int16_t>	int16_le_t;
typedef le_t<int32_t>	int32_le_t;
typedef le_t<int64_t>	int64_le_t;
typedef u_int8_t	u_int8_le_t;
typedef le_t<u_int16_t>	u_int16_le_t;
typedef le_t<u_int32_t>	u_int32_le_t;
typedef le_t<u_int64_t>	u_int64_le_t;
#endif

#endif
