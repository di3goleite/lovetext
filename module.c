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
#include "module.h"

struct cpreferences *alloc_preferences()
{
	struct cpreferences *preferences = (struct cpreferences *)malloc(sizeof(struct cpreferences));
	
	preferences->home_path = g_string_new(getenv("HOME"));
	preferences->last_path = g_string_new("");
	preferences->program_path = g_string_new(preferences->home_path->str);
	preferences->program_path = g_string_append(preferences->program_path, "/.lovetext");
	if (g_file_test(preferences->program_path->str, G_FILE_TEST_IS_DIR)) {
	} else {
		g_printf("[ERROR] Configuration path not found. Creating at \"%s\".\n", preferences->program_path->str);
		mkdir(preferences->program_path->str, 0777);
	}
	
	preferences->extension_path = g_string_new(preferences->program_path->str);
	preferences->extension_path = g_string_append(preferences->extension_path, "/plugins");
	if (g_file_test(preferences->extension_path->str, G_FILE_TEST_IS_DIR)) {
	} else {
		g_printf("[ERROR] Extensions path not found. Creating at \"%s\".\n", preferences->extension_path->str);
		mkdir(preferences->extension_path->str, 0777);
	}
	
	preferences->configuration_file_path = g_string_new(preferences->program_path->str);
	preferences->configuration_file_path = g_string_append(preferences->configuration_file_path, "/lovetextrc");
	
	preferences->configuration_file = g_key_file_new();
	
	preferences->gtk_theme = NULL;
	gchar *scheme_id = NULL;
	preferences->start_new_page = FALSE;
	preferences->show_menu_bar = FALSE;
	preferences->show_status_bar = FALSE;
	preferences->show_tool_bar = FALSE;
	if (g_key_file_load_from_file(preferences->configuration_file,
		preferences->configuration_file_path->str, G_KEY_FILE_NONE, NULL)) {
		scheme_id = g_key_file_get_string(preferences->configuration_file,
			"editor",
			"scheme_id",
			NULL);
		preferences->start_new_page = g_key_file_get_boolean(preferences->configuration_file,
			"general",
			"start_new_page",
			NULL);
		preferences->use_custom_gtk_theme = g_key_file_get_boolean(preferences->configuration_file,
			"general",
			"use_custom_gtk_theme",
			NULL);
		preferences->gtk_theme = g_key_file_get_string(preferences->configuration_file,
			"general",
			"gtk_theme",
			NULL);
		preferences->show_menu_bar = g_key_file_get_boolean(preferences->configuration_file,
			"general",
			"show_menu_bar",
			NULL);
		preferences->show_tool_bar = g_key_file_get_boolean(preferences->configuration_file,
			"general",
			"show_tool_bar",
			NULL);
		preferences->show_status_bar = g_key_file_get_boolean(preferences->configuration_file,
			"general",
			"show_status_bar",
			NULL);
		preferences->show_line_numbers = g_key_file_get_boolean(preferences->configuration_file,
			"editor",
			"show_line_numbers",
			NULL);
		preferences->show_right_margin = g_key_file_get_boolean(preferences->configuration_file,
			"editor",
			"show_right_margin",
			NULL);
		preferences->highlight_current_line = g_key_file_get_boolean(preferences->configuration_file,
			"editor",
			"highlight_current_line",
			NULL);
		preferences->highlight_maching_brackets = g_key_file_get_boolean(preferences->configuration_file,
			"editor",
			"highlight_maching_brackets",
			NULL);
	} else {
		g_printf("[ERROR] Failed to open \"%s\"\n", preferences->configuration_file_path->str);
	}
	
	preferences->style_scheme_manager = gtk_source_style_scheme_manager_get_default();
	preferences->scheme_ids = gtk_source_style_scheme_manager_get_scheme_ids(preferences->style_scheme_manager);
	preferences->scheme = NULL;
	
	if (scheme_id) {
		preferences->scheme = gtk_source_style_scheme_manager_get_scheme(preferences->style_scheme_manager, scheme_id);
	}
	
	preferences->editor_font = g_string_new("Fixed");
	preferences->language_manager = gtk_source_language_manager_get_default();
	
	return preferences;
}

struct capplication_handler *alloc_application_handler(void)
{
	struct capplication_handler *application_handler = (struct capplication_handler *)malloc(sizeof(struct capplication_handler));
	application_handler->version = FALSE;
	application_handler->help = FALSE;
	application_handler->file_name_input = NULL;
	return application_handler;
}

// End of file.
