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

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib/gstring.h>
#include <glib/glist.h>
#include <glib/gprintf.h>
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
#include "module.h"

struct capplication_handler *alloc_application_handler(GtkApplication *application)
{
	struct capplication_handler *application_handler = (struct capplication_handler *)malloc(sizeof(struct capplication_handler));
	application_handler->application = application;
	
	// GLib.
	application_handler->option_context = g_option_context_new(_PROGRAM_NAME_);
	application_handler->GUID = g_dbus_generate_guid();
	
	application_handler->version = FALSE;
	application_handler->help = FALSE;
	application_handler->file_name = NULL;
	application_handler->lua = NULL;
	
	g_printf("MM Loading application_handler.\n");
	g_printf("MM Receiving \"HOME\".\n");
	const gchar *home_env = g_get_home_dir();
	g_printf("MM \"HOME\" received.\n");
	if (home_env) {
		application_handler->home_path = g_string_new(home_env);
		g_printf("MM Your home path is \"%s\".\n", application_handler->home_path->str);
	} else {
		g_printf("EE Your home path is NULL, failed to recieve enviroment variable \"HOME\".\n");
		application_handler->home_path = NULL;
	}
	application_handler->last_path = g_string_new("/");
	
	if (application_handler->home_path) {
		g_printf("MM Setting configuration path.\n");
		application_handler->program_path = g_string_new(application_handler->home_path->str);
		application_handler->program_path = g_string_append(application_handler->program_path, "/.lovetext");
		
		if (g_file_test(application_handler->program_path->str, G_FILE_TEST_IS_DIR)) {
			g_printf("MM Configuration path found at \"%s\".\n", application_handler->program_path->str);
		} else {
			g_printf("EE Configuration path not found. Creating at \"%s\".\n", application_handler->program_path->str);
			mkdir(application_handler->program_path->str, 0777);
		}
	} else {
		g_printf("EE Your home path is not set. Igoring configuration path.\n");
		application_handler->program_path = NULL;
	}
	
	if (application_handler->program_path) {
		g_printf("MM Setting extension path.\n");
		application_handler->extension_path = g_string_new(application_handler->program_path->str);
		application_handler->extension_path = g_string_append(application_handler->extension_path, "/plugins");
		
		if (g_file_test(application_handler->extension_path->str, G_FILE_TEST_IS_DIR)) {
			g_printf("MM Extensions path found at \"%s\".\n", application_handler->extension_path->str);
		} else {
			g_printf("EE Extensions path not found. Creating at \"%s\".\n", application_handler->extension_path->str);
			mkdir(application_handler->extension_path->str, 0777);
		}
	} else {
		g_printf("EE Your configuration path is not set. Igoring extension path.\n");
		application_handler->extension_path = NULL;
	}
	
	if (application_handler->program_path) {
		g_printf("MM Setting configuration file path.\n");
		application_handler->configuration_file_path = g_string_new(application_handler->program_path->str);
		application_handler->configuration_file_path = g_string_append(application_handler->configuration_file_path, "/lovetextrc");
	} else {
		g_printf("EE Your configuration path is not set. Igoring configuration file path.\n");
		application_handler->configuration_file_path = NULL;
	}
	
	application_handler->configuration_file = g_key_file_new();
	
	application_handler->gtk_theme = NULL;
	gchar *scheme_id = NULL;
	gchar *font_name = NULL;
	application_handler->start_new_page = FALSE;
	application_handler->use_decoration = FALSE;
	application_handler->show_menu_bar = TRUE;
	application_handler->show_action_bar = FALSE;
	application_handler->show_tool_bar = FALSE;
	application_handler->use_custom_gtk_theme = FALSE;
	application_handler->show_map = FALSE;
	application_handler->wrap_lines = FALSE;
	application_handler->show_grid = FALSE;
	
	application_handler->draw_spaces_space = FALSE;
	application_handler->draw_spaces_leading = FALSE;
	application_handler->draw_spaces_nbsp = FALSE;
	application_handler->draw_spaces_newline = FALSE;
	application_handler->draw_spaces_tab = FALSE;
	application_handler->draw_spaces_text = FALSE;
	application_handler->draw_spaces_trailing = FALSE;
	
	application_handler->wrap_mode = 0;
	application_handler->tabs_position = 2;
	application_handler->show_right_margin = TRUE;
	application_handler->right_margin_position = 80;
	application_handler->show_line_numbers = TRUE;
	application_handler->show_line_marks = TRUE;
	application_handler->show_right_margin = TRUE;
	application_handler->auto_indent = TRUE;
	application_handler->highlight_current_line = TRUE;
	application_handler->highlight_matching_brackets = TRUE;
	application_handler->tab_width = 8;
	if (application_handler->configuration_file_path) {
		g_printf("MM Trying to load configuration file from \"%s\".\n", application_handler->configuration_file_path->str);
		if (g_key_file_load_from_file(application_handler->configuration_file,
		application_handler->configuration_file_path->str, G_KEY_FILE_NONE, NULL)) {
			g_printf("MM Configuration file loaded.\n");
			application_handler->start_new_page = g_key_file_get_boolean(application_handler->configuration_file,
				"general",
				"start_new_page",
				NULL);
			application_handler->use_custom_gtk_theme = g_key_file_get_boolean(application_handler->configuration_file,
				"general",
				"use_custom_gtk_theme",
				NULL);
			application_handler->gtk_theme = g_key_file_get_string(application_handler->configuration_file,
				"general",
				"gtk_theme",
				NULL);
			application_handler->use_decoration = g_key_file_get_boolean(application_handler->configuration_file,
				"general",
				"use_decoration",
				NULL);
			application_handler->show_menu_bar = g_key_file_get_boolean(application_handler->configuration_file,
				"general",
				"show_menu_bar",
				NULL);
			application_handler->show_tool_bar = g_key_file_get_boolean(application_handler->configuration_file,
				"general",
				"show_tool_bar",
				NULL);
			application_handler->show_action_bar = g_key_file_get_boolean(application_handler->configuration_file,
				"general",
				"show_action_bar",
				NULL);
			application_handler->tabs_position = g_key_file_get_integer(application_handler->configuration_file,
				"general",
				"tabs_position",
				NULL);
			scheme_id = g_key_file_get_string(application_handler->configuration_file,
				"editor",
				"scheme_id",
				NULL);
			font_name = g_key_file_get_string(application_handler->configuration_file,
				"editor",
				"font",
				NULL);
			application_handler->show_line_numbers = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"show_line_numbers",
				NULL);
			application_handler->wrap_lines = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"wrap_lines",
				NULL);
			application_handler->show_map = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"show_map",
				NULL);
			application_handler->show_grid = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"show_grid",
				NULL);
			application_handler->show_right_margin = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"show_right_margin",
				NULL);
			application_handler->highlight_current_line = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"highlight_current_line",
				NULL);
			application_handler->highlight_matching_brackets = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"highlight_matching_brackets",
				NULL);
			application_handler->auto_indent = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"auto_indent",
				NULL);
			application_handler->show_line_marks = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"show_line_marks",
				NULL);
			application_handler->tab_width = g_key_file_get_integer(application_handler->configuration_file,
				"editor",
				"tab_width",
				NULL);
			application_handler->draw_spaces_nbsp = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"draw_spaces_nbsp",
				NULL);
			application_handler->draw_spaces_space = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"draw_spaces_space",
				NULL);
			application_handler->draw_spaces_newline = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"draw_spaces_newline",
				NULL);
			application_handler->draw_spaces_tab = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"draw_spaces_tab",
				NULL);
			application_handler->draw_spaces_text = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"draw_spaces_text",
				NULL);
			application_handler->draw_spaces_leading = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"draw_spaces_leading",
				NULL);
			application_handler->draw_spaces_trailing = g_key_file_get_boolean(application_handler->configuration_file,
				"editor",
				"draw_spaces_trailing",
				NULL);
		} else {
			g_printf("EE Failed to open \"%s\"\n", application_handler->configuration_file_path->str);
		}
	} else {
		g_printf("EE Your configuration file path is not set. Igoring configuration file.\n");
	}
	application_handler->style_scheme_manager = gtk_source_style_scheme_manager_get_default();
	application_handler->scheme_ids = gtk_source_style_scheme_manager_get_scheme_ids(application_handler->style_scheme_manager);
	application_handler->scheme = NULL;
	
	if (scheme_id) {
		application_handler->scheme = gtk_source_style_scheme_manager_get_scheme(application_handler->style_scheme_manager, scheme_id);
	}
	
	if (font_name) {
		application_handler->editor_font = g_string_new(font_name);
	} else {
		application_handler->editor_font = g_string_new("Fixed");
	}
	application_handler->language_manager = gtk_source_language_manager_get_default();
	
	g_printf("MM application_handler loaded.\n");
	return application_handler;
}

// End of file.
