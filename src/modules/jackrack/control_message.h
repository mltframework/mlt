/*
 *   JACK Rack
 *    
 *   Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
 *    
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __JR_CONTROL_MESSAGE_H__
#define __JR_CONTROL_MESSAGE_H__

#include <glib.h>
#include <ladspa.h>

/** These are for messages between the gui and the process() callback */
typedef enum _ctrlmsg_type
{
  CTRLMSG_ADD,
  CTRLMSG_REMOVE,
  CTRLMSG_QUIT,

} ctrlmsg_type_t;

typedef struct _ctrlmsg ctrlmsg_t;

struct _plugin;
struct _plugin_desc;

struct _ctrlmsg
{
  ctrlmsg_type_t type;
  union
  {
    struct
    {
      struct _plugin      * plugin;
    } add;
    
    struct
    {
      struct _plugin      * plugin;
    } remove;
    
  } data;
 
};

#endif /* __JR_CONTROL_MESSAGE_H__ */
