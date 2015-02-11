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

#ifndef _MODULE_H_
#define _MODULE_H_
#include <stdio.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcestylescheme.h>
#include <gtksourceview/gtksourcestyleschememanager.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/completion-providers/words/gtksourcecompletionwords.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <glib.h>
#include <glib/gstring.h>
#include <glib/glist.h>

struct cbuffer_ref
{
	GString *file_name;
	int modified;
	GtkWidget *source_view;
	GtkWidget *tab;
	GtkWidget *label;
	
	
	GtkSourceSearchContext *search_context;
	GtkSourceSearchSettings *search_settings;
	GtkTextIter current_search_start;
	GtkTextIter current_search_end;
	GtkWidget *scrolled_window;
	GtkSourceCompletionWords *provider_words;
};

struct cpreferences
{
	GString *home_path;
	GString *program_path;
	GString *last_path;
	GString *extension_path;
	GString *configuration_file_path;
	GKeyFile *configuration_file;
	
	// [general]
	gchar *gtk_theme;
	gboolean use_custom_gtk_theme;
	gboolean start_new_page;
	gboolean show_menu_bar;
	gboolean show_status_bar;
	gboolean show_tool_bar;
	
	// [editor]
	gboolean show_line_numbers;
	gboolean show_right_margin;
	gboolean highlight_current_line;
	gboolean highlight_maching_brackets;
	
	GtkSourceStyleScheme *scheme;
	GtkSourceStyleSchemeManager *style_scheme_manager;
	GtkSourceLanguageManager *language_manager;
	gchar **scheme_ids;
	PangoFontDescription *font_desc;
	GString *editor_font;
};

struct capplication_handler {
	GtkApplication *application;
	GOptionEntry *option_entries;
	GApplicationCommandLine *command_line;
	gchar *GUID;
	gboolean version;
	gchar *file_name_input;
	
	GMenu *menu_model;
	
	gboolean help;
	GOptionContext *option_context;
	gchar **args;
	gint argc;
};

struct capplication_handler *alloc_application_handler(void);

struct cpreferences *alloc_preferences();

#endif

// End of file.