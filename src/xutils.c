/*
 * Copyright (C) 2001 Havoc Pennington
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "xutils.h"
#include <string.h>
#include <stdio.h>

static GHashTable *atom_hash = NULL;
static GHashTable *reverse_atom_hash = NULL;

Atom
my_wnck_atom_get (const char *atom_name)
{
  Atom retval;
  
  g_return_val_if_fail (atom_name != NULL, None);

  if (!atom_hash)
    {
      atom_hash = g_hash_table_new (g_str_hash, g_str_equal);
      reverse_atom_hash = g_hash_table_new (NULL, NULL);
    }
      
  retval = GPOINTER_TO_UINT (g_hash_table_lookup (atom_hash, atom_name));
  if (!retval)
    {
      retval = XInternAtom (gdk_display, atom_name, FALSE);

      if (retval != None)
        {
          char *name_copy;

          name_copy = g_strdup (atom_name);
          
          g_hash_table_insert (atom_hash,
                               name_copy,
                               GUINT_TO_POINTER (retval));
          g_hash_table_insert (reverse_atom_hash,
                               GUINT_TO_POINTER (retval),
                               name_copy);
        }
    }

  return retval;
}

void
my_wnck_change_state (Screen  *screen,
		    Window   xwindow,
                    gboolean add,
                    Atom     state1,
                    Atom     state2)
{
  XEvent xev;

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */  
  
  xev.xclient.type = ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event = True;
  xev.xclient.display = gdk_display;
  xev.xclient.window = xwindow;
  xev.xclient.message_type = my_wnck_atom_get ("_NET_WM_STATE");
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = add ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
  xev.xclient.data.l[1] = state1;
  xev.xclient.data.l[2] = state2;

  XSendEvent (gdk_display,
	      RootWindowOfScreen (screen),
              False,
	      SubstructureRedirectMask | SubstructureNotifyMask,
	      &xev);
}

void
my_wnck_error_trap_push (void)
{
  gdk_error_trap_push ();
}

int
my_wnck_error_trap_pop (void)
{
  XSync (gdk_display, False);
  return gdk_error_trap_pop ();
}

char*
my_wnck_get_string_property_latin1 (Window  xwindow,
                                    Atom    atom)
{
  Atom type;
  int format;
  gulong nitems;
  gulong bytes_after;
  unsigned char *property;
  int err, result;
  char *retval;
  int i;
  long *pp;
  char* prop_name;
  char** prop_names;
  
  my_wnck_error_trap_push ();
  property = NULL;
  result = XGetWindowProperty (gdk_display,
			       xwindow, atom,
			       0, G_MAXLONG,
			       False, AnyPropertyType, &type, &format, &nitems,
			       &bytes_after, &property);

  err = my_wnck_error_trap_pop ();
  if (err != Success ||
      result != Success)
    return NULL;
  
  retval = NULL;
  
  if (type == XA_STRING)
    {
      retval = g_strdup ((char*)property);
    }
  else if (type == XA_ATOM && nitems > 0 && format == 32)
    {
      pp = (long *)property;  // we can assume (long *) since format == 32
      if (nitems == 1)
        {
          prop_name = XGetAtomName (gdk_display, *pp);
          if (prop_name)
            {
              retval = g_strdup (prop_name);
              XFree (prop_name);
            }
        }
      else
        {
          prop_names = g_new (char *, nitems + 1);
          prop_names[nitems] = NULL;
          for (i=0; i < nitems; i++)
            {
              prop_names[i] = XGetAtomName (gdk_display, *pp++);
            }
          retval = g_strjoinv (", ", prop_names);
          for (i=0; i < nitems; i++)
            {
              if (prop_names[i]) XFree (prop_names[i]);
            }
          g_free (prop_names);
        }
    }
  
  XFree (property);
  return retval;
}

Screen*
my_wnck_window_get_xscreen (WnckWindow *window)
{
   Window   xid;
   XWindowAttributes attrs;

   xid = wnck_window_get_xid (window);
   XGetWindowAttributes(gdk_display, xid, &attrs);

   return attrs.screen;
}
