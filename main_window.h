/*
Love Text is licensed under MIT license.
This file is part of Love Text. 

The MIT License (MIT)

Copyright (c) 2015 Felipe Ferreira da Silva

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _MAIN_WINDOW_H_
#define _MAIN_WINDOW_H_
#include <stdio.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcestylescheme.h>
#include <gtksourceview/gtksourcestyleschememanager.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <glib.h>
#include <glib/gstring.h>
#include <glib/glist.h>
#include "main_window_preferences.h"
#include "module.h"

struct cwindow_handler
{
	struct capplication_handler *application_handler;
	
	GMenu *menu_model;
	GtkWidget *window;
	gboolean window_fullscreen;
	gboolean decorated;
	GtkAccelGroup *accel_group;
	GtkWidget *box;
	GtkWidget *tool_bar;
	GtkWidget *action_bar;
	GtkWidget *header_bar;
	GtkWidget *main_button;
	GtkWidget *main_popover;
	GtkWidget *menu_bar;
	GtkWidget *box_client;
	GtkWidget *notebook;
	GtkWidget *action_widget_box;
	GtkWidget *revealer;
	
	GtkWidget *stack_switcher;
	GtkWidget *stack;
	
	GtkWidget *label_search_replace;
	GtkWidget *search_and_replace_bar;
	GtkWidget *search_bar;
	GtkWidget *replace_bar;
	GtkWidget *search_entry;
	GtkWidget *replace_with_entry;
	
	GtkWidget *window_new;
	
	gboolean drag_and_drop;
	
	GtkClipboard *clipboard;
	
	GtkAccelGroup *accelerator_group;
	
	GString *program_folder;
};

void initialize_lua(struct cwindow_handler *window_handler, struct capplication_handler *application_handler);
void update_editor(struct cwindow_handler *window_handler);
struct cwindow_handler *alloc_window_handler(struct capplication_handler *application_handler);
struct cbuffer_ref *create_page(struct cwindow_handler *window_handler, const gchar *file_name, const gchar *text);

#endif

// End of file.
