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

#ifndef __JR_UI_H__
#define __JR_UI_H__

#include <glib.h>

#include "jack_rack.h"
#include "plugin_mgr.h"
#include "process.h"
#include "lock_free_fifo.h"

typedef struct _ui ui_t;

enum _ui_state
{
  /* nothing's happening */
  STATE_NORMAL,
  
  /* the gui is waiting for the process callback to do something */
  STATE_RACK_CHANGE,
  
  /* we're closing down */
  STATE_QUITTING
};
typedef enum _ui_state ui_state_t;


struct _jack_rack;
	
struct _ui
{
  plugin_mgr_t *    plugin_mgr;
  process_info_t *  procinfo;
  struct _jack_rack *     jack_rack;

  lff_t *           ui_to_process;
  lff_t *           process_to_ui;
  
  lff_t             *ui_to_midi;
  lff_t             *midi_to_ui;

  char *            filename;
  
  gboolean          shutdown;
  ui_state_t        state;

};

ui_t * ui_new     (const char * client_name, unsigned long channels, 
	gboolean connect_inputs, gboolean connect_outputs);
void   ui_quit    (ui_t * ui);
void   ui_destroy (ui_t * ui);

void ui_set_state    (ui_t * ui, ui_state_t state);
ui_state_t ui_get_state (ui_t * ui);

void jack_shutdown_cb (void * data);
gboolean ui_loop_iterate (ui_t * ui);

#endif /* __JR_UI_H__ */
