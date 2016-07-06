/*
* error.h Error handling
* Copyright (C) 2000 Arne Schirmacher <arne@schirmacher.de>
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

#ifndef _ERROR_H
#define _ERROR_H 1

#ifdef __cplusplus
extern "C"
{
#endif

	/*
	 * Should check for gcc/g++ and version > 2.6 I suppose
	 */
#ifndef        __ASSERT_FUNCTION
#   define __ASSERT_FUNCTION   __PRETTY_FUNCTION__
#endif

#define fail_neg(eval)  real_fail_neg  (eval, #eval, __ASSERT_FUNCTION, __FILE__, __LINE__)
#define fail_null(eval) real_fail_null (eval, #eval, __ASSERT_FUNCTION, __FILE__, __LINE__)
#define fail_if(eval)   real_fail_if   (eval, #eval, __ASSERT_FUNCTION, __FILE__, __LINE__)

	void real_fail_neg ( int eval, const char * eval_str, const char * func, const char * file, int line );
	void real_fail_null ( const void * eval, const char * eval_str, const char * func, const char * file, int line );
	void real_fail_if ( bool eval, const char * eval_str, const char * func, const char * file, int line );

	extern void sigpipe_clear( );
	extern int sigpipe_get( );

#ifdef __cplusplus
}
#endif

#endif

