/*
 * global_commands.h
 * Copyright (C) 2002-2003 Ushodaya Enterprises Limited
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef _GLOBAL_COMMANDS_H_
#define _GLOBAL_COMMANDS_H_

#include <dv1394status.h>
#include "dvunit.h"
#include "dvconnection.h"

#ifdef __cplusplus
extern "C"
{
#endif

dv_unit dv1394d_get_unit( int );
void dv1394d_delete_unit( int );
void dv1394d_delete_all_units( void );
int dv1394d_unit_status( int n, dv1394_status status, int root_offset );
void raw1394_start_service_threads( void );
void raw1394_stop_service_threads( void );

extern response_codes dv1394d_add_unit( command_argument );
extern response_codes dv1394d_list_nodes( command_argument );
extern response_codes dv1394d_list_units( command_argument );
extern response_codes dv1394d_list_clips( command_argument );
extern response_codes dv1394d_set_global_property( command_argument );
extern response_codes dv1394d_get_global_property( command_argument );

#ifdef __cplusplus
}
#endif

#endif
