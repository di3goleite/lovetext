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
#include "main_window.h"
#include "main_window_preferences.h"
#include "module.h"

#define _PROGRAM_NAME_ "LoveText"
#define _PROGRAM_YEAR_ "2015"
#define _PROGRAM_VERSION_ "0.8"
#define _PROGRAM_AUTHOR_ "Felipe Ferreira da Silva"

static gint main_application_command_line_handler(gpointer user_data)
{
	struct capplication_handler *application_handler = (struct capplication_handler *)user_data;
	GApplicationCommandLine *command_line = application_handler->command_line;
	GApplication *application = application_handler->application;
	GOptionContext *option_context = application_handler->option_context;
	GError *error;
	gint i;
	gint argc;
	gchar **args;
	gchar **argv;
	args = g_application_command_line_get_arguments(command_line, &argc);
	argv = g_new (gchar*, argc + 1);
	for (i = 0; i <= argc; i++) {
		argv[i] = args[i];
	}
	gboolean context_result = FALSE;
	error = NULL;
	context_result = g_option_context_parse_strv(option_context, &args, &error);
	if (context_result) {
		if (application_handler->help) {
			// gchar *text;
			// text = g_option_context_get_help(option_context, FALSE, g_option_context_get_main_group(option_context));
			// g_print("%s",  text);
			// g_free (text);
		}
		if (application_handler->version) {
			g_printf("%s version %s.\n", _PROGRAM_NAME_, _PROGRAM_VERSION_);
			g_printf("Copyright © %s %s. All rights reserved.\n", _PROGRAM_YEAR_, _PROGRAM_AUTHOR_);
		}
		
		if (application_handler->file_name_input) {
			
		}
	}
	if (error) {
		printf("[ERROR] Failed to parser arguments.\n");
	}
	g_option_context_free(option_context);
	g_object_unref(command_line);
	return G_SOURCE_REMOVE;
}

static gint main_application_command_line(GApplication *application, GApplicationCommandLine *command_line, gpointer user_data)
{
	struct capplication_handler *application_handler = (struct capplication_handler *)user_data;
	g_object_set_data_full(G_OBJECT(command_line), "application", application, (GDestroyNotify)g_application_release);
	g_object_ref(command_line);
	application_handler->command_line = command_line;
	g_idle_add(main_application_command_line_handler, application_handler);
	
	g_application_activate(application);
	
	return 0;
}

static gint main_application_handle_local_options(GApplication *application, GVariantDict *options, gpointer user_data)
{
	//g_printf("[MESSAGE] Handling local options.\n");
	return -1;
}

static void main_application_activate(GApplication *application, gpointer user_data)
{
	g_printf("[MESSAGE] Program activated.\n");
	struct capplication_handler *application_handler = (struct capplication_handler *)user_data;
	struct cpreferences *preferences = alloc_preferences();
	
	GtkSettings *settings = gtk_settings_get_default();
	
	if ((preferences->use_custom_gtk_theme) && (preferences->gtk_theme)) {
		g_printf("[MESSAGE] Setting custom theme.\n");
		g_object_set(settings, "gtk-theme-name", preferences->gtk_theme, NULL);
	} else {
		g_printf("[MESSAGE] Setting no custom theme.\n");
	}
	
	struct cwindow_handler *window_handler = alloc_window_handler(preferences);
	window_handler->application_handler = application_handler;
	gtk_application_add_window(application_handler->application, window_handler->window);
	gtk_window_present(window_handler->window);
	gtk_widget_show_all(window_handler->window);
	initialize_lua(window_handler, preferences);
	gtk_widget_set_visible(window_handler->status_bar, preferences->show_status_bar);
	gtk_widget_set_visible(window_handler->menu_bar, preferences->show_menu_bar);
	gtk_widget_set_visible(window_handler->search_and_replace_bar, FALSE);
}

static void main_application_startup(GApplication *application, gpointer user_data)
{
	struct capplication_handler *application_handler = (struct capplication_handler *)user_data;
	g_application_hold(application);
	//GNotification *notification = g_notification_new(_PROGRAM_NAME_ " starting.");
	//g_notification_set_body(notification, "The program is executing.");
	//g_application_send_notification(application, NULL, notification);
	// Handle option context.
	GOptionEntry option_entries[] = {
		{ "version",
			'v',
			0,
			G_OPTION_ARG_NONE,
			&application_handler->version,
			"\n\t\t\t\tShow the version of the program.", NULL},
		{ "file",
			'i',
			0,
			G_OPTION_ARG_STRING,
			&application_handler->file_name_input,
			"\n\t\t\t\tSet the input file.", NULL},
		{NULL}
	};
	
	g_application_add_main_option_entries(application, option_entries);
	application_handler->option_context = g_option_context_new(_PROGRAM_NAME_);
	g_option_context_set_help_enabled(application_handler->option_context, FALSE);
	g_option_context_set_summary(application_handler->option_context, _PROGRAM_NAME_ " description.");
	g_option_context_add_main_entries(application_handler->option_context, option_entries, _PROGRAM_NAME_);
}

static void main_application_shutdown(GApplication *application, gpointer user_data)
{
	//GNotification *notification = g_notification_new(_PROGRAM_NAME_ " terminating.");
	//g_notification_set_body(notification, "The program has no more tasks.");
	//g_application_send_notification(application, NULL, notification);
}

int main(int argc, char *args[])
{
	GtkApplication *application = gtk_application_new(_PROGRAM_NAME_".instance", G_APPLICATION_FLAGS_NONE | G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_NON_UNIQUE);
	struct capplication_handler *application_handler = alloc_application_handler();
	application_handler->application = application;
	application_handler->GUID = g_dbus_generate_guid();
	g_signal_connect(application, "activate", G_CALLBACK(main_application_activate), application_handler);
	g_signal_connect(application, "startup", G_CALLBACK(main_application_startup), application_handler);
	g_signal_connect(application, "shutdown", G_CALLBACK(main_application_shutdown), application_handler);
	g_signal_connect(application, "command-line", G_CALLBACK(main_application_command_line), application_handler);
	g_signal_connect(application, "handle-local-options", G_CALLBACK(main_application_handle_local_options), application_handler);
	
	//gtk_main();
	g_application_run(application, argc, args);
	g_printf("[MESSAGE] Terminating application.\n");
	g_object_unref(application);
	g_printf("[MESSAGE] Closing Lua state.\n");
	lua_close(application_handler->lua);
	g_printf("[MESSAGE] Done.\n");
	
	return 0;
}

// End of file.
