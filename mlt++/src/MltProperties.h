/**
 * MltProperties.h - MLT Wrapper
 * Copyright (C) 2004-2005 Charles Yates
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
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

#ifndef _MLTPP_PROPERTIES_H_
#define _MLTPP_PROPERTIES_H_

#include <framework/mlt.h>

namespace Mlt 
{
	/** Abstract Properties class.
	 */

	class Properties 
	{
		public:
			virtual mlt_properties get_properties( ) = 0;
			int count( );
			char *get( char *name );
			int get_int( char *name );
			double get_double( char *name );
			void *get_data( char *name, int &size );
			int set( char *name, char *value );
			int set( char *name, int value );
			int set( char *name, double value );
			int set( char *name, void *value, int size, mlt_destructor destroy = NULL, mlt_serialiser serial = NULL );
	};
	
	/** Instance class.
	 */
	
	class PropertiesInstance : public Properties
	{
		private:
			bool destroy;
			mlt_properties instance;
		public:
			mlt_properties get_properties( );
			PropertiesInstance( );
			PropertiesInstance( Properties &properties );
			PropertiesInstance( mlt_properties properties );
			virtual ~PropertiesInstance( );
	};
}

#endif
