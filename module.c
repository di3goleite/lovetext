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

struct cpreferences *alloc_preferences()
{
	g_printf("[MESSAGE] Allocating preferences.\n");
	struct cpreferences *preferences = malloc(sizeof(struct cpreferences));
	g_printf("[MESSAGE] Structure allocated.\n");
	
	g_printf("[MESSAGE] Receiving \"HOME\".\n");
	//const gchar *home_env = getenv("HOME");
	const gchar *home_env = g_get_home_dir();
	g_printf("[MESSAGE] \"HOME\" received.\n");
	if (home_env) {
		preferences->home_path = g_string_new(home_env);
		g_printf("[MESSAGE] Your home path is \"%s\".\n", preferences->home_path->str);
	} else {
		g_printf("[ERROR] Your home path is NULL, failed to recieve enviroment variable \"HOME\".\n");
		preferences->home_path = NULL;
	}
	preferences->last_path = g_string_new("/");
	
	if (preferences->home_path) {
		g_printf("[MESSAGE] Setting configuration path.\n");
		preferences->program_path = g_string_new(preferences->home_path->str);
		preferences->program_path = g_string_append(preferences->program_path, "/.lovetext");
		
		if (g_file_test(preferences->program_path->str, G_FILE_TEST_IS_DIR)) {
			g_printf("[MESSAGE] Configuration path found at \"%s\".\n", preferences->program_path->str);
		} else {
			g_printf("[ERROR] Configuration path not found. Creating at \"%s\".\n", preferences->program_path->str);
			mkdir(preferences->program_path->str, 0777);
		}
	} else {
		g_printf("[ERROR] Your home path is not set. Igoring configuration path.\n");
		preferences->program_path = NULL;
	}
	
	if (preferences->program_path) {
		g_printf("[MESSAGE] Setting extension path.\n");
		preferences->extension_path = g_string_new(preferences->program_path->str);
		preferences->extension_path = g_string_append(preferences->extension_path, "/plugins");
		
		if (g_file_test(preferences->extension_path->str, G_FILE_TEST_IS_DIR)) {
			g_printf("[MESSAGE] Extensions path found at \"%s\".\n", preferences->extension_path->str);
		} else {
			g_printf("[ERROR] Extensions path not found. Creating at \"%s\".\n", preferences->extension_path->str);
			mkdir(preferences->extension_path->str, 0777);
		}
	} else {
		g_printf("[ERROR] Your configuration path is not set. Igoring extension path.\n");
		preferences->extension_path = NULL;
	}
	
	if (preferences->program_path) {
		g_printf("[MESSAGE] Setting configuration file path.\n");
		preferences->configuration_file_path = g_string_new(preferences->program_path->str);
		preferences->configuration_file_path = g_string_append(preferences->configuration_file_path, "/lovetextrc");
	} else {
		g_printf("[ERROR] Your configuration path is not set. Igoring configuration file path.\n");
		preferences->configuration_file_path = NULL;
	}
	
	preferences->configuration_file = g_key_file_new();
	
	preferences->gtk_theme = NULL;
	gchar *scheme_id = NULL;
	gchar *font_name = NULL;
	preferences->start_new_page = FALSE;
	preferences->use_decoration = FALSE;
	preferences->show_menu_bar = TRUE;
	preferences->show_action_bar = FALSE;
	preferences->show_tool_bar = FALSE;
	preferences->use_custom_gtk_theme = FALSE;
	
	preferences->tabs_position = 2;
	preferences->show_line_numbers = TRUE;
	preferences->show_line_marks = TRUE;
	preferences->show_right_margin = TRUE;
	preferences->auto_indent = TRUE;
	preferences->highlight_current_line = TRUE;
	preferences->highlight_matching_brackets = TRUE;
	preferences->tab_width = 8;
	if (preferences->configuration_file_path) {
		g_printf("[MESSAGE] Trying to load configuration file from \"%s\".\n", preferences->configuration_file_path->str);
		if (g_key_file_load_from_file(preferences->configuration_file,
		preferences->configuration_file_path->str, G_KEY_FILE_NONE, NULL)) {
			g_printf("[MESSAGE] Configuration file loaded.\n");
			scheme_id = g_key_file_get_string(preferences->configuration_file,
				"editor",
				"scheme_id",
				NULL);
			font_name = g_key_file_get_string(preferences->configuration_file,
				"editor",
				"font",
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
			preferences->use_decoration = g_key_file_get_boolean(preferences->configuration_file,
				"general",
				"use_decoration",
				NULL);
			preferences->show_menu_bar = g_key_file_get_boolean(preferences->configuration_file,
				"general",
				"show_menu_bar",
				NULL);
			preferences->show_tool_bar = g_key_file_get_boolean(preferences->configuration_file,
				"general",
				"show_tool_bar",
				NULL);
			preferences->show_action_bar = g_key_file_get_boolean(preferences->configuration_file,
				"general",
				"show_action_bar",
				NULL);
			preferences->tabs_position = g_key_file_get_integer(preferences->configuration_file,
				"general",
				"tabs_position",
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
			preferences->highlight_matching_brackets = g_key_file_get_boolean(preferences->configuration_file,
				"editor",
				"highlight_matching_brackets",
				NULL);
			preferences->auto_indent = g_key_file_get_boolean(preferences->configuration_file,
				"editor",
				"auto_indent",
				NULL);
			preferences->show_line_marks = g_key_file_get_boolean(preferences->configuration_file,
				"editor",
				"show_line_marks",
				NULL);
			preferences->tab_width = g_key_file_get_integer(preferences->configuration_file,
				"editor",
				"tab_width",
				NULL);
		} else {
			g_printf("[ERROR] Failed to open \"%s\"\n", preferences->configuration_file_path->str);
		}
	} else {
		g_printf("[ERROR] Your configuration file path is not set. Igoring configuration file.\n");
	}
	preferences->style_scheme_manager = gtk_source_style_scheme_manager_get_default();
	preferences->scheme_ids = gtk_source_style_scheme_manager_get_scheme_ids(preferences->style_scheme_manager);
	preferences->scheme = NULL;
	
	if (scheme_id) {
		preferences->scheme = gtk_source_style_scheme_manager_get_scheme(preferences->style_scheme_manager, scheme_id);
	}
	
	if (font_name) {
		preferences->editor_font = g_string_new(font_name);
	} else {
		preferences->editor_font = g_string_new("Fixed");
	}
	preferences->language_manager = gtk_source_language_manager_get_default();
	
	return preferences;
}

struct capplication_handler *alloc_application_handler(void)
{
	struct capplication_handler *application_handler = (struct capplication_handler *)malloc(sizeof(struct capplication_handler));
	application_handler->version = FALSE;
	application_handler->help = FALSE;
	application_handler->file_name_input = NULL;
	application_handler->lua = NULL;
	return application_handler;
}

// End of file.
