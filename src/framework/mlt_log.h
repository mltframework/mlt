/**
 * \file mlt_log.h
 * \brief logging functions
 *
 * Copyright (C) 2004-2014 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MLT_LOG_H
#define MLT_LOG_H

#include <stdarg.h>
#include <stdint.h>

#define MLT_LOG_QUIET    -8

/**
 * something went really wrong and we will crash now
 */
#define MLT_LOG_PANIC     0

/**
 * something went wrong and recovery is not possible
 * like no header in a format which depends on it or a combination
 * of parameters which are not allowed
 */
#define MLT_LOG_FATAL     8

/**
 * something went wrong and cannot losslessly be recovered
 * but not all future data is affected
 */
#define MLT_LOG_ERROR    16

/**
 * something somehow does not look correct / something which may or may not
 * lead to some problems
 */
#define MLT_LOG_WARNING  24

#define MLT_LOG_INFO     32
#define MLT_LOG_VERBOSE  40
#define MLT_LOG_TIMINGS  44

/**
 * stuff which is only useful for MLT developers
 */
#define MLT_LOG_DEBUG    48

/**
 * Send the specified message to the log if the level is less than or equal to
 * the current logging level. By default, all logging messages are sent to
 * stderr. This behavior can be altered by setting a different mlt_vlog callback
 * function.
 *
 * \param service An optional pointer to a \p mlt_service_s.
 * \param level The importance level of the message, lower values signifying
 * higher importance.
 * \param fmt The format string (printf-compatible) that specifies how
 * subsequent arguments are converted to output.
 * \see mlt_vlog
 */
#ifdef __GNUC__
void mlt_log( void *service, int level, const char *fmt, ... ) __attribute__ ((__format__ (__printf__, 3, 4)));
#else
void mlt_log( void *service, int level, const char *fmt, ... );
#endif

#define mlt_log_panic(service, format, args...) mlt_log((service), MLT_LOG_PANIC, (format), ## args)
#define mlt_log_fatal(service, format, args...) mlt_log((service), MLT_LOG_FATAL, (format), ## args)
#define mlt_log_error(service, format, args...) mlt_log((service), MLT_LOG_ERROR, (format), ## args)
#define mlt_log_warning(service, format, args...) mlt_log((service), MLT_LOG_WARNING, (format), ## args)
#define mlt_log_info(service, format, args...) mlt_log((service), MLT_LOG_INFO, (format), ## args)
#define mlt_log_verbose(service, format, args...) mlt_log((service), MLT_LOG_VERBOSE, (format), ## args)
#define mlt_log_timings(service, format, args...) mlt_log((service), MLT_LOG_TIMINGS, (format), ## args)
#define mlt_log_debug(service, format, args...) mlt_log((service), MLT_LOG_DEBUG, (format), ## args)

void mlt_vlog( void *service, int level, const char *fmt, va_list );
int mlt_log_get_level( void );
void mlt_log_set_level( int );
void mlt_log_set_callback( void (*)( void*, int, const char*, va_list ) );

#define mlt_log_timings_begin() \
{ \
	int64_t _mlt_log_timings_begin = mlt_log_timings_now(), _mlt_log_timings_end;

#define mlt_log_timings_end(service, msg) \
	_mlt_log_timings_end = mlt_log_timings_now(); \
	mlt_log_timings( service, "%s:%d: T(%s)=%" PRId64 " us\n", \
		__FILE__, __LINE__, msg, _mlt_log_timings_end - _mlt_log_timings_begin ); \
}

int64_t mlt_log_timings_now( void );

#endif /* MLT_LOG_H */
