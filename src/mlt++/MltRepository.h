/**
 * MltRepository.h - MLT Wrapper
 * Copyright (C) 2008 Dan Dennedy <dan@dennedy.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MLTPP_REPOSITORY_H_
#define _MLTPP_REPOSITORY_H_

#include "config.h"

#ifdef SWIG
#define MLTPP_DECLSPEC
#endif

#include <framework/mlt.h>

namespace Mlt
{
	class Profile;
	class Properties;

	class MLTPP_DECLSPEC Repository
	{
		private:
			mlt_repository instance;
			Repository( ) { }
		public:
			Repository( const char* directory );
			Repository( mlt_repository repository );
			~Repository();

			void register_service( mlt_service_type service_type, const char *service, mlt_register_callback symbol );
			void *create( Profile& profile, mlt_service_type type, const char *service, void *arg );
			Properties *consumers( ) const;
			Properties *filters( ) const;
			Properties *producers( ) const;
			Properties *transitions( ) const;
			void register_metadata( mlt_service_type type, const char *service, mlt_metadata_callback, void *callback_data );
			Properties *metadata( mlt_service_type type, const char *service ) const;
			Properties *languages( ) const;
			static Properties *presets();
	};
}

#endif
