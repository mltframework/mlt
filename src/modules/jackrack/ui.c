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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <unistd.h>

#include <glib.h>
#include <ladspa.h>

#include "ui.h"
#include "control_message.h"


#define PROCESS_FIFO_SIZE 64
#define MIDI_FIFO_SIZE 256

ui_t *
ui_new (const char * client_name, unsigned long channels, 
	gboolean connect_inputs, gboolean connect_outputs)
{
  ui_t * ui;

  ui = g_malloc (sizeof (ui_t));
  ui->filename = NULL;
  ui->shutdown = FALSE;
  ui->state = STATE_NORMAL;

  ui->ui_to_process = lff_new (PROCESS_FIFO_SIZE, sizeof (ctrlmsg_t));
  ui->process_to_ui = lff_new (PROCESS_FIFO_SIZE, sizeof (ctrlmsg_t));

  ui->procinfo = process_info_new (ui, client_name, channels, connect_inputs, connect_outputs);
  if (!ui->procinfo)
    return NULL;

  ui->plugin_mgr = plugin_mgr_new (ui);
  plugin_mgr_set_plugins (ui->plugin_mgr, channels);
  ui->jack_rack = jack_rack_new (ui, channels);
  
  return ui;
}


void
ui_quit (ui_t * ui)
{
  ctrlmsg_t ctrlmsg;

  ui_set_state (ui, STATE_QUITTING);
  ctrlmsg.type = CTRLMSG_QUIT;
  lff_write (ui->ui_to_process, &ctrlmsg);
}

void
ui_destroy (ui_t * ui)
{
  if (!ui->shutdown)
    ui_quit (ui);
  jack_rack_destroy (ui->jack_rack);  
  
  g_free (ui);
}

void
ui_set_state    (ui_t * ui, ui_state_t state)
{
  ui->state = state;
}

ui_state_t
ui_get_state (ui_t * ui)
{
  return ui->state;
}

void
jack_shutdown_cb (void * data)
{
  ui_t * ui = data;
  
  ui->shutdown = TRUE;
}

/* do the process->gui message processing */
gboolean
ui_loop_iterate (ui_t * ui)
{
  ctrlmsg_t ctrlmsg;
  jack_rack_t * jack_rack = ui->jack_rack;

  while (lff_read (ui->process_to_ui, &ctrlmsg) == 0)
    {
    switch (ctrlmsg.type)
      {
      case CTRLMSG_ADD:
        jack_rack_add_plugin (jack_rack, ctrlmsg.data.add.plugin);
        break;

      case CTRLMSG_REMOVE:
        plugin_destroy (ctrlmsg.data.remove.plugin, ui);
        break;

      case CTRLMSG_QUIT:
        return TRUE;
        break;
        
      }
    }
 
  usleep (100000);
  
  return FALSE;
}

/* EOF */
