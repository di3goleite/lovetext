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
#include <glib/gi18n.h>
#include <locale.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <glib.h>
#include <glib/gstring.h>
#include <glib/glist.h>
#include "main_window.h"
#include "main_window_preferences.h"
#include "module.h"

int main(int argc, char *args[])
{
	GtkApplication *application = gtk_application_new(NULL, 0);
	g_application_register(G_APPLICATION(application), NULL, NULL);
	struct capplication_handler *application_handler = alloc_application_handler(application);
	
	// Handle option context.
	GOptionEntry option_entries[] = {
		{ "version",
			'v',
			0,
			G_OPTION_ARG_NONE,
			&application_handler->version,
			"Show the version of the program.", NULL},
		{ G_OPTION_REMAINING,
			'\0',
			0,
			G_OPTION_ARG_FILENAME_ARRAY,
			&application_handler->file_name,
			"A file containing a matrix for sequence alignment.", NULL},
		{ "help",
			'h',
			0,
			G_OPTION_ARG_NONE,
			&application_handler->help,
			"Show this help description.", NULL},
		{NULL}
	};
	g_option_context_add_main_entries(application_handler->option_context, option_entries, _PROGRAM_NAME_);
	g_option_context_set_help_enabled(application_handler->option_context, TRUE);
	g_option_context_set_ignore_unknown_options(application_handler->option_context, TRUE);
	// Parse options.
	GError *error = NULL;
	gboolean context_result = FALSE;
	context_result = g_option_context_parse(application_handler->option_context,
		&argc,
		&args,
		&error);
	if (context_result) {
		if (application_handler->version) {
			g_printf("%s version %s.\n", _PROGRAM_NAME_, _PROGRAM_VERSION_);
			g_printf("Copyright © %s %s. All rights reserved.\n", _PROGRAM_YEAR_, _PROGRAM_AUTHOR_);
		}
	} else if (error) {
		printf("EE Failed to parser arguments.\n");
	}
	g_option_context_free(application_handler->option_context);
	
	struct cwindow_handler *window_handler = alloc_window_handler(application_handler);
	if (gtk_application_prefers_app_menu(GTK_APPLICATION(application_handler->application))) {
		//gtk_application_set_app_menu(GTK_APPLICATION(application_handler->application), G_MENU_MODEL(window_handler->menu_model));
	}
	gtk_application_add_window(GTK_APPLICATION(application_handler->application), GTK_WINDOW(window_handler->window));
	gtk_widget_show_all(GTK_WIDGET(window_handler->window));
	gtk_window_present(GTK_WINDOW(window_handler->window));
	initialize_lua(window_handler, application_handler);
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(window_handler->notebook), application_handler->tabs_position);
	gtk_widget_set_visible(GTK_WIDGET(window_handler->action_bar), application_handler->show_action_bar);
	//gtk_widget_set_visible(window_handler->menu_bar, application_handler->show_menu_bar);
	gtk_widget_set_visible(GTK_WIDGET(window_handler->search_and_replace_bar), FALSE);
	
	if (application_handler->file_name) {
		gint i = 0;
		while (application_handler->file_name[i]) {
			g_printf("MM Open file \"%s\".\n", application_handler->file_name[i]);
			FILE *file = fopen(application_handler->file_name[i], "r");
			fseek(file, 0, SEEK_END);
			int size = ftell(file);
			char *text = (char *)malloc(sizeof(char) * size + 1);
			memset(text, 0, sizeof(char) * size + 1);
			fseek(file, 0, SEEK_SET);
			fread(text, sizeof(char), size, file);

			create_page(window_handler, application_handler->file_name[i], text);
			fclose(file);
			i++;
		}
	}
	update_editor(window_handler);
	
	gtk_main();
	g_printf("MM Closing Lua state.\n");
	lua_close(application_handler->lua);
	
	return 0;
}

// End of file.
