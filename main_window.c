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
#include <sys/stat.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstring.h>
#include <glib/glist.h>
#include <gio/gio.h>
#include <gdk/gdkkeysyms.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <gtksourceview/gtksource.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcestylescheme.h>
#include <gtksourceview/gtksourcestyleschememanager.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/completion-providers/words/gtksourcecompletionwords.h>
#include <gtk/gtkrange.h>
#include <gtk/gtkscrolledwindow.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "main_window.h"
#include "main_window_preferences.h"
#include "module.h"

#define _COLUMN_TEXT_ 0
#define _COLUMN_KEY_ 1
#define _COLUMN_ALIAS_ 2
#define _COLUMN_DESCRIPTION_ 3

static void update_page_language(struct cwindow_handler *window_handler, struct cbuffer_ref *buffer_ref)
{
	GtkSourceBuffer *source_buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)));
	GtkSourceLanguage *language = NULL;
	gboolean result_uncertain;
	gchar *content_type;
	content_type = g_content_type_guess(buffer_ref->file_name->str, NULL, 0, &result_uncertain);
	if (result_uncertain) {
		g_free (content_type);
		content_type = NULL;
	}
	language = NULL;
	if ((buffer_ref->file_name->str) && (content_type)) {
		language = gtk_source_language_manager_guess_language(window_handler->preferences->language_manager,
			buffer_ref->file_name->str, content_type);
	}
	
	if (language) {
		gtk_source_buffer_set_language(source_buffer, language);
	}
	g_free (content_type);
}

static void update_views(struct cwindow_handler *window_handler)
{
	g_printf("[MESSAGE] Updating text editor schemes.\n");
	struct cpreferences *preferences = window_handler->preferences;
	gint pages_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(window_handler->notebook));
	gint i = 0;
	
	GtkSourceDrawSpacesFlags draw_spaces_flag = 0;
	if (preferences->draw_spaces_space) {
		draw_spaces_flag |= GTK_SOURCE_DRAW_SPACES_SPACE;
	}
	if (preferences->draw_spaces_tab) {
		draw_spaces_flag |= GTK_SOURCE_DRAW_SPACES_TAB;
	}
	if (preferences->draw_spaces_nbsp) {
		draw_spaces_flag |= GTK_SOURCE_DRAW_SPACES_NBSP;
	}
	if (preferences->draw_spaces_newline) {
		draw_spaces_flag |= GTK_SOURCE_DRAW_SPACES_NEWLINE;
	}
	if (preferences->draw_spaces_text) {
		draw_spaces_flag |= GTK_SOURCE_DRAW_SPACES_TEXT;
	}
	if (preferences->draw_spaces_leading) {
		draw_spaces_flag |= GTK_SOURCE_DRAW_SPACES_NBSP;
	}
	if (preferences->draw_spaces_trailing) {
		draw_spaces_flag |= GTK_SOURCE_DRAW_SPACES_TRAILING;
	}
	
	GtkWidget *widget_page = NULL;
	struct cbuffer_ref *buffer_ref = NULL;
	for (i = 0; i < pages_count; i++) {
		widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), i);
		buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
		if (buffer_ref) {
			GtkSourceBuffer *source_buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)));
			gtk_source_view_set_background_pattern(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->show_grid);
			gtk_source_view_set_draw_spaces(GTK_SOURCE_VIEW(buffer_ref->source_view), draw_spaces_flag);
			gtk_text_view_set_right_margin(GTK_TEXT_VIEW(buffer_ref->source_view), preferences->show_right_margin);
			gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(buffer_ref->source_view), preferences->wrap_mode);
			gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->right_margin_position);
			gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->show_line_marks);
			gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->show_line_numbers);
			gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->show_right_margin);
			gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->tab_width);
			gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->auto_indent);
			gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(source_buffer), preferences->highlight_matching_brackets);
			gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->highlight_current_line);
			if (window_handler->preferences->scheme) {
				const gchar *scheme_id = NULL;
				
				if (window_handler->preferences->scheme) {
					scheme_id = gtk_source_style_scheme_get_id(window_handler->preferences->scheme);
				}
				if (scheme_id) {
					g_printf("[MESSAGE] Scheme \"%s\" set at page %i.\n", scheme_id, i);
				} else {
					g_printf("[MESSAGE] Scheme set at page %i.\n", i);
				}
				gtk_source_buffer_set_style_scheme(source_buffer, window_handler->preferences->scheme);
				
				GtkStyleContext *style_context = gtk_widget_get_style_context(GTK_WIDGET(buffer_ref->source_view));
				GtkCssProvider *css_provider = gtk_css_provider_new();
				
				GString *css_string = g_string_new("GtkSourceView { font: ");
				css_string = g_string_append(css_string, preferences->editor_font->str);
				css_string = g_string_append(css_string, ";}");
				
				gtk_css_provider_load_from_data(css_provider,
					css_string->str,
					-1,
					NULL);
				g_string_free(css_string, TRUE);
				
				gtk_style_context_add_provider(style_context,
					GTK_STYLE_PROVIDER(css_provider),
					GTK_STYLE_PROVIDER_PRIORITY_USER);
				g_object_unref(G_OBJECT(css_provider));
				gtk_widget_reset_style(GTK_WIDGET(buffer_ref->source_view));
			}
		}
	}
	g_printf("[MESSAGE] Done.\n");
}

static void update_providers(struct cwindow_handler *window_handler)
{
	g_printf("[MESSAGE] Updating providers.\n");
	gint pages_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(window_handler->notebook));
	gint i = 0;
	gint j = 0;
	
	GtkWidget *widget_page = NULL;
	struct cbuffer_ref *buffer_ref_a = NULL;
	struct cbuffer_ref *buffer_ref_b = NULL;
	for (i = 0; i < pages_count; i++) {
		widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), i);
		buffer_ref_a = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
		GtkSourceCompletion *completion = gtk_source_view_get_completion(GTK_SOURCE_VIEW(buffer_ref_a->source_view));
		
		for (j = 0; j < pages_count; j++) {
			widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), j);
			buffer_ref_b = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
			
			if (completion) {
				GtkSourceCompletionWords *provider_words = buffer_ref_b->provider_words;
				gtk_source_completion_add_provider(completion, GTK_SOURCE_COMPLETION_PROVIDER(provider_words), NULL);
			}
		}
	}
	g_printf("[MESSAGE] Done.\n");
}

static void update_editor(struct cwindow_handler *window_handler)
{
	struct cpreferences *preferences = window_handler->preferences;
	g_printf("[MESSAGE] Action bar visible.\n");
	if (preferences->show_action_bar) {
		gtk_widget_show(GTK_WIDGET(window_handler->action_bar));
	} else {
		gtk_widget_hide(GTK_WIDGET(window_handler->action_bar));
	}
	g_printf("[MESSAGE] Decoration.\n");
	if (window_handler->decorated) {
	} else {
		if (preferences->show_menu_bar) {
			gtk_widget_show(GTK_WIDGET(window_handler->menu_bar));
		} else {
			gtk_widget_hide(GTK_WIDGET(window_handler->menu_bar));
		}
	}
	g_printf("[MESSAGE] Tabs position.\n");
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(window_handler->notebook), preferences->tabs_position);
	update_views(window_handler);
	update_providers(window_handler);
	g_printf("[MESSAGE] Done.\n");
}

// Actions callbacks.
static void action_new_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.new\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	create_page(window_handler, "", "");
}

static void accel_new(GtkWidget *w, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_new_activate(NULL, NULL, user_data);
	}
}

static void action_open_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.open\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	GtkWidget *file_chooser = gtk_file_chooser_dialog_new("Open",
		GTK_WINDOW(window_handler->window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		"Cancel",
		GTK_RESPONSE_CANCEL,
		"Open",
		GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(file_chooser), TRUE);
	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(file_chooser), TRUE);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), window_handler->preferences->last_path->str);
	
	g_printf("[MESSAGE] Running dialog window.\n");
	if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
		GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(file_chooser));
		GSList *iterator = filenames;
		while (iterator) {
			gchar *file_name = iterator->data;
			GtkWidget *source_view = NULL;
			if (source_view) {
				g_printf("[MESSAGE] File already open.\n");
				GtkWidget *scrolled_window = gtk_widget_get_parent(source_view);
				if (scrolled_window) {
					gtk_notebook_set_current_page(GTK_NOTEBOOK(window_handler->notebook), 
						gtk_notebook_page_num(GTK_NOTEBOOK(window_handler->notebook), scrolled_window));
				}
			} else {
				FILE *file = fopen(file_name, "r");
				fseek(file, 0, SEEK_END);
				int size = ftell(file);
				char *text = (char *)malloc(sizeof(char) * size + 1);
				memset(text, 0, sizeof(char) * size + 1);
				fseek(file, 0, SEEK_SET);
				fread(text, sizeof(char), size, file);
				
				create_page(window_handler, file_name, text);
				fclose(file);
			}
			iterator = g_slist_next(iterator);
		}
	}
	gtk_widget_destroy(file_chooser);
}

static void accel_open(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_open_activate(NULL, NULL, user_data);
	}
}

static void action_save_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.save\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	lua_State* lua = window_handler->application_handler->lua;
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
	if (scrolled_window) {
		struct cbuffer_ref *buffer_ref = NULL;
		buffer_ref = g_object_get_data(G_OBJECT(scrolled_window), "buffer_ref");
		if (buffer_ref) {
			if (g_file_test(buffer_ref->file_name->str, G_FILE_TEST_IS_REGULAR)) {
				gchar *file_name = buffer_ref->file_name->str;
				FILE *file = fopen(file_name, "w");
				
				GtkSourceBuffer *source_buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)));
				GtkTextIter start;
				GtkTextIter end;
				gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(source_buffer), &start);
				gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(source_buffer), &end);
				gchar *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(source_buffer), &start, &end, FALSE);
				
				fwrite(text, sizeof(unsigned char), strlen(text), file);
				g_free(text);
				fclose(file);
				
				lua_getglobal(lua, "editor");
				if (lua_istable(lua, -1)) {
					lua_pushstring(lua, "f_save");
					lua_gettable(lua, -2);
					if (lua_isfunction(lua, -1)) {
						lua_pushlightuserdata(lua, buffer_ref->source_view);
						lua_pushstring(lua, file_name);
						if (lua_pcall(lua, 2, 0, 0) != LUA_OK) {
							g_printf("[ERROR] Fail to call editor[\"f_save\"].\n");
						} else {
							g_printf("[MESSAGE] Function editor[\"f_save\"] called.\n");
						}
					} else {
						lua_pop(lua, 1);
					}
				}
				lua_pop(lua, 1);
			} else {
				GtkWidget *file_chooser = gtk_file_chooser_dialog_new("Save",
					GTK_WINDOW(window_handler->window),
					GTK_FILE_CHOOSER_ACTION_SAVE,
					"Cancel",
					GTK_RESPONSE_CANCEL,
					"Save",
					GTK_RESPONSE_ACCEPT,
					NULL);
				gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(file_chooser), TRUE);
				gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), window_handler->preferences->last_path->str);
				if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
					gchar *file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
					FILE *file = fopen(file_name, "w");
					
					GtkSourceBuffer *source_buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)));
					GtkTextIter start;
					GtkTextIter end;
					gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(source_buffer), &start);
					gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(source_buffer), &end);
					gchar *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(source_buffer), &start, &end, FALSE);
					
					fwrite(text, sizeof(unsigned char), strlen(text), file);
					g_free(text);
					fclose(file);
					buffer_ref->file_name = g_string_assign(buffer_ref->file_name, file_name);
					lua_getglobal(lua, "editor");
					if (lua_istable(lua, -1)) {
						lua_pushstring(lua, "f_save");
						lua_gettable(lua, -2);
						if (lua_isfunction(lua, -1)) {
							//window_handler->id_factory = window_handler->id_factory + 1;
							//lua_pushinteger(lua, window_handler->id_factory);
							lua_pushlightuserdata(lua, buffer_ref->source_view);
							lua_pushstring(lua, file_name);
							if (lua_pcall(lua, 2, 0, 0) != LUA_OK) {
								g_printf("[ERROR] Fail to call editor[\"f_save\"].\n");
							} else {
								g_printf("[MESSAGE] Function editor[\"f_save\"] called.\n");
							}
						} else {
							lua_pop(lua, 1);
						}
					}
					lua_pop(lua, 1);
				}
				gtk_widget_destroy(file_chooser);
				update_page_language(window_handler, buffer_ref);
			}
			gchar *separator = strrchr(buffer_ref->file_name->str, '/');
			if (separator) {
				gtk_label_set_text(GTK_LABEL(buffer_ref->label), ++separator);
			}
			gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window_handler->header_bar), buffer_ref->file_name->str);
			gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_NORMAL, TRUE);
			gtk_text_buffer_set_modified(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), FALSE);
		}
	}
}

static void accel_save(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_save_activate(NULL, NULL, user_data);
	}
}

static void action_save_as_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.save_as\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	lua_State* lua = window_handler->application_handler->lua;
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
	if (scrolled_window) {
		struct cbuffer_ref *buffer_ref = NULL;
		buffer_ref = g_object_get_data(G_OBJECT(scrolled_window), "buffer_ref");
		if (buffer_ref) {
			GtkWidget *file_chooser = gtk_file_chooser_dialog_new("Save as",
				GTK_WINDOW(window_handler->window),
				GTK_FILE_CHOOSER_ACTION_SAVE,
				"Cancel",
				GTK_RESPONSE_CANCEL,
				"Save",
				GTK_RESPONSE_ACCEPT,
				NULL);
			gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(file_chooser), TRUE);
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), window_handler->preferences->last_path->str);
			if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
				gchar *file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
				FILE *file = fopen(file_name, "w");
				
				GtkSourceBuffer *source_buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)));
				GtkTextIter start;
				GtkTextIter end;
				gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(source_buffer), &start);
				gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(source_buffer), &end);
				gchar *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(source_buffer), &start, &end, FALSE);
				
				fwrite(text, sizeof(unsigned char), strlen(text), file);
				g_free(text);
				fclose(file);
				buffer_ref->file_name = g_string_assign(buffer_ref->file_name, file_name);
				
				lua_getglobal(lua, "editor");
				if (lua_istable(lua, -1)) {
					lua_pushstring(lua, "f_close");
					lua_gettable(lua, -2);
					if (lua_isfunction(lua, -1)) {
						lua_pushlightuserdata(lua, buffer_ref->source_view);
						if (lua_pcall(lua, 1, 0, 0) != LUA_OK) {
							g_printf("[ERROR] Fail to call editor[\"f_close\"].\n");
						} else {
							g_printf("[MESSAGE] Function editor[\"f_close\"] called.\n");
						}
					} else {
						lua_pop(lua, 1);
					}
				}
				lua_pop(lua, 1);
				
				lua_getglobal(lua, "editor");
				if (lua_istable(lua, -1)) {
					lua_pushstring(lua, "f_new");
					lua_gettable(lua, -2);
					if (lua_isfunction(lua, -1)) {
						lua_pushlightuserdata(lua, buffer_ref->source_view);
						lua_pushstring(lua, file_name);
						if (lua_pcall(lua, 2, 0, 0) != LUA_OK) {
							g_printf("[ERROR] Fail to call editor[\"f_new\"].\n");
						} else {
							g_printf("[MESSAGE] Function editor[\"f_new\"] called.\n");
						}
					} else {
						lua_pop(lua, 1);
					}
				}
				lua_pop(lua, 1);
				
				lua_getglobal(lua, "editor");
				if (lua_istable(lua, -1)) {
					lua_pushstring(lua, "f_save");
					lua_gettable(lua, -2);
					if (lua_isfunction(lua, -1)) {
						//window_handler->id_factory = window_handler->id_factory + 1;
						//lua_pushinteger(lua, window_handler->id_factory);
						lua_pushlightuserdata(lua, buffer_ref->source_view);
						lua_pushstring(lua, file_name);
						if (lua_pcall(lua, 2, 0, 0) != LUA_OK) {
							g_printf("[ERROR] Fail to call editor[\"f_save\"].\n");
						} else {
							g_printf("[MESSAGE] Function editor[\"f_save\"] called.\n");
						}
					} else {
						lua_pop(lua, 1);
					}
				}
				lua_pop(lua, 1);
			}
			gtk_widget_destroy(file_chooser);
			update_page_language(window_handler, buffer_ref);
			
			gchar *separator = strrchr(buffer_ref->file_name->str, '/');
			if (separator) {
				gtk_label_set_text(GTK_LABEL(buffer_ref->label), ++separator);
			}
			gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window_handler->header_bar), buffer_ref->file_name->str);
			gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_NORMAL, TRUE);
			gtk_text_buffer_set_modified(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), FALSE);
		}
	}
}

static void accel_save_as(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_save_as_activate(NULL, NULL, user_data);
	}
}

static void action_preferences_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.preferences\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	
	// Preferences window.
	window_handler->window_preferences_handler = alloc_window_preferences_handler(window_handler->application_handler, window_handler->preferences);
	gtk_window_set_transient_for(GTK_WINDOW(window_handler->window_preferences_handler->window), GTK_WINDOW(window_handler->window));
	window_handler->window_preferences_handler->main_window_notebook = window_handler->notebook;
	window_handler->window_preferences_handler->window_handler = window_handler;
	window_handler->window_preferences_handler->update_editor = (update_editorf)update_editor;
	gtk_widget_show_all(GTK_WIDGET(window_handler->window_preferences_handler->window));
	gtk_dialog_run(GTK_DIALOG(window_handler->window_preferences_handler->window));
}

static void action_close_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.close\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	lua_State *lua = window_handler->application_handler->lua;
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
	
	if (scrolled_window) {
		GtkWidget *source_view = gtk_container_get_focus_child(GTK_CONTAINER(scrolled_window));
		struct cbuffer_ref *buffer_ref = g_object_get_data(G_OBJECT(source_view), "buffer_ref");
		if (buffer_ref) {
			lua_getglobal(lua, "editor");
			if (lua_istable(lua, -1)) {
				lua_pushstring(lua, "f_close");
				lua_gettable(lua, -2);
				if (lua_isfunction(lua, -1)) {
					lua_pushlightuserdata(lua, buffer_ref->source_view);
					if (lua_pcall(lua, 1, 0, 0) != LUA_OK) {
						g_printf("[ERROR] Fail to call editor[\"f_close\"].\n");
					} else {
						g_printf("[MESSAGE] Function editor[\"f_close\"] called.\n");
					}
				} else {
					lua_pop(lua, 1);
				}
			}
			lua_pop(lua, 1);
		}
		gtk_notebook_remove_page(GTK_NOTEBOOK(window_handler->notebook),
			gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
		if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(window_handler->notebook)) <= 0) {
			gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window_handler->header_bar), NULL);
		}
	}
}

static void accel_close(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_close_activate(NULL, NULL, user_data);
	}
}

static void action_exit_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.exit\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gtk_widget_destroy(window_handler->window);
}

static void accel_exit(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_exit_activate(NULL, NULL, user_data);
	}
}

static void action_undo_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.undo\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook));
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
			if (buffer_ref) {
				gtk_source_buffer_undo(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(GTK_TEXT_VIEW(buffer_ref->source_view)))));
				if (gtk_source_buffer_can_undo(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view))))) {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_ACTIVE, TRUE);
				} else {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_NORMAL, TRUE);
				}
			}
		}
	}
}

static void action_redo_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.redo\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook));
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
			if (buffer_ref) {
				gtk_source_buffer_redo(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view))));
				if (gtk_source_buffer_can_undo(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view))))) {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_ACTIVE, TRUE);
				} else {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_NORMAL, TRUE);
				}
			}
		}
	}
}

static void action_copy_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.copy\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook));
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
			if (buffer_ref) {
				if (window_handler->clipboard) {
					GtkTextIter ins;
					GtkTextIter bound;
					gboolean selection = FALSE;
					selection = gtk_text_buffer_get_selection_bounds(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
						&ins,
						&bound);
					if (selection) {
						gchar *text = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
							&ins,
							&bound,
							FALSE);
						gtk_clipboard_set_text(window_handler->clipboard, text, -1);
						//gtk_text_buffer_copy_clipboard(gtk_text_view_get_buffer(buffer_ref->source_view), window_handler->clipboard);
						g_free(text);
					}
				}
			}
		}
	}
}

static void action_paste_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.paste\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook));
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
			if (buffer_ref) {
				if (window_handler->clipboard) {
					gtk_text_buffer_paste_clipboard(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), window_handler->clipboard, NULL, TRUE);
				}
			}
		}
	}
}

static void action_cut_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.cut\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook));
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
			if (buffer_ref) {
				if (window_handler->clipboard) {
					gtk_text_buffer_cut_clipboard(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), window_handler->clipboard, TRUE);
				}
			}
		}
	}
}

static void action_delete_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.delete\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook));
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
			if (buffer_ref) {
				if (window_handler->clipboard) {
					if (gtk_text_buffer_delete_selection(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), TRUE, TRUE)) {
						g_printf("[MESSAGE] Text deleted.\n");
					}
				}
			}
		}
	}
}

static void action_next_page_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.next_page\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)) == 
	gtk_notebook_get_n_pages(GTK_NOTEBOOK(window_handler->notebook)) - 1) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(window_handler->notebook), 0);
	} else {
		gtk_notebook_next_page(GTK_NOTEBOOK(window_handler->notebook));
	}
}

static void accel_next_page(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_next_page_activate(NULL, NULL, user_data);
	}
}

static void action_previous_page_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.previous_page\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)) == 0) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(window_handler->notebook),
			gtk_notebook_get_n_pages(GTK_NOTEBOOK(window_handler->notebook)) - 1);
	} else {
		gtk_notebook_prev_page(GTK_NOTEBOOK(window_handler->notebook));
	}
}

static void accel_previous_page(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_previous_page_activate(NULL, NULL, user_data);
	}
}

static void action_toggle_menu_bar_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.toggle_menu_bar\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	if (gtk_widget_get_visible(window_handler->menu_bar)) {
		gtk_widget_hide(window_handler->menu_bar);
	} else {
		gtk_widget_show(window_handler->menu_bar);
	}
	window_handler->preferences->show_menu_bar = gtk_widget_get_visible(window_handler->menu_bar);
}

static void accel_toggle_menu_bar(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_toggle_menu_bar_activate(NULL, NULL, user_data);
	}
}

static void action_toggle_action_bar_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.toggle_action_bar\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	if (gtk_widget_get_visible(window_handler->action_bar)) {
		gtk_widget_set_visible(window_handler->action_bar, FALSE);
		window_handler->preferences->show_action_bar = FALSE;
	} else {
		gtk_widget_set_visible(window_handler->action_bar, TRUE);
		window_handler->preferences->show_action_bar = TRUE;
	}
}

static void action_toggle_fullscreen_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.toggle_fullscreen\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	
	if (window_handler->window_fullscreen) {
		if (window_handler->decorated) {
			g_object_ref(G_OBJECT(window_handler->header_bar));
			gtk_container_remove(GTK_CONTAINER(window_handler->box), window_handler->header_bar);
			gtk_window_set_titlebar(GTK_WINDOW(window_handler->window), window_handler->header_bar);
			gtk_widget_show_all(GTK_WIDGET(window_handler->header_bar));
		}
		gtk_window_unfullscreen(GTK_WINDOW(window_handler->window));
		window_handler->window_fullscreen = FALSE;
	} else {
		if (window_handler->decorated) {
			g_object_ref(G_OBJECT(window_handler->header_bar));
			gtk_container_remove(GTK_CONTAINER(window_handler->window), window_handler->header_bar);
			gtk_widget_unparent(window_handler->header_bar);
			gtk_box_pack_start(GTK_BOX(window_handler->box), window_handler->header_bar, FALSE, TRUE, 0);
		}
		gtk_window_fullscreen(GTK_WINDOW(window_handler->window));
		gtk_widget_show_all(window_handler->header_bar);
		window_handler->window_fullscreen = TRUE;
	}
}

static void accel_toggle_fullscreen(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_toggle_fullscreen_activate(NULL, NULL, user_data);
	}
}

static void action_search_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.search\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gtk_widget_show(GTK_WIDGET(window_handler->action_bar));
	gtk_widget_set_visible(GTK_WIDGET(window_handler->search_and_replace_bar), TRUE);
	gtk_widget_set_visible(GTK_WIDGET(window_handler->replace_bar), FALSE);
	
	if (gtk_widget_get_visible(GTK_WIDGET(window_handler->search_and_replace_bar))) {
		gtk_widget_grab_focus(GTK_WIDGET(window_handler->search_entry));
		gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook));
		if (current_page > -1) {
			GtkWidget *widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), current_page);
			struct cbuffer_ref *buffer_ref = NULL;
			buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
			if (buffer_ref) {
				GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
					"insert");
				gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
					&buffer_ref->current_search_end,
					cursor);
				buffer_ref->current_search_start = buffer_ref->current_search_end;
			}
		}
	} else {
		gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook));
		if (current_page > -1) {
			GtkWidget *widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), current_page);
			struct cbuffer_ref *buffer_ref = NULL;
			buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
			if (buffer_ref) {
				gtk_widget_grab_focus(GTK_WIDGET(buffer_ref->source_view));
			}
		}
	}
}

static void accel_search(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_search_activate(NULL, NULL, user_data);
	}
}

static void action_replace_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.replace\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gtk_widget_show(GTK_WIDGET(window_handler->action_bar));
	gtk_widget_set_visible(GTK_WIDGET(window_handler->search_and_replace_bar), TRUE);
	gtk_widget_set_visible(GTK_WIDGET(window_handler->replace_bar), TRUE);
	
	if (gtk_widget_get_visible(GTK_WIDGET(window_handler->search_and_replace_bar))) {
		gtk_widget_grab_focus(GTK_WIDGET(window_handler->search_entry));
	} else {
		gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook));
		if (current_page > -1) {
			GtkWidget *widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), current_page);
			struct cbuffer_ref *buffer_ref = NULL;
			buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
			if (buffer_ref) {
				gtk_widget_grab_focus(GTK_WIDGET(buffer_ref->source_view));
			}
		}
	}
}

static void accel_replace(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		action_replace_activate(NULL, NULL, user_data);
	}
}

static void action_about_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	g_printf("[MESSAGE] Performing action \"window.about\".\n");
	GtkWidget *window_about = gtk_about_dialog_new();
	GString *version_string = g_string_new(_PROGRAM_VERSION_);
	version_string = g_string_append(version_string, " (build ");
	version_string = g_string_append(version_string, _BUILD_NUMBER_);
	version_string = g_string_append(version_string, ")");
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(window_about), "Love Text");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(window_about), version_string->str);
	g_string_free(version_string, TRUE);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(window_about), "Copyright © 2015 by Felipe Ferreira da Silva");
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(window_about), "Love Text is a simple, lightweight and extensible text editor.");
	gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(window_about), GTK_LICENSE_MIT_X11);
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(window_about), "The MIT License (MIT)\n\
\n\
Copyright (c) 2015 Felipe Ferreira da Silva\n\
\n\
Permission is hereby granted, free of charge, to any person obtaining a copy\
of this software and associated documentation files (the \"Software\"), to deal\
in the Software without restriction, including without limitation the rights\
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\
copies of the Software, and to permit persons to whom the Software is\
furnished to do so, subject to the following conditions:\n\
\n\
The above copyright notice and this permission notice shall be included in\
all copies or substantial portions of the Software.\n\
\n\
THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\
THE SOFTWARE.");
	gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(window_about), TRUE);
	gtk_window_set_icon_name(GTK_WINDOW(window_about), "lovetext");
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(window_about), "lovetext");
	
	const gchar *authors[2] = {"Felipe Ferreira da Silva (SILVA, F. F. da) <felipefsdev@gmail.com>", NULL};
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(window_about), authors);
	
	gtk_window_set_transient_for(GTK_WINDOW(window_about), GTK_WINDOW(window_handler->window));
	gtk_dialog_run(GTK_DIALOG(window_about));
	gtk_widget_destroy(GTK_WIDGET(window_about));
}

static gboolean entry_search_key_press_event(GtkWidget *widget,  GdkEventKey *event, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	struct cpreferences *preferences = window_handler->preferences;
	if (event->keyval == GDK_KEY_Escape) {
		gtk_widget_set_visible(GTK_WIDGET(window_handler->search_and_replace_bar), FALSE);
		if (preferences->show_action_bar == FALSE) {
			gtk_widget_hide(GTK_WIDGET(window_handler->action_bar));
		}
		gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook));
		if (current_page > -1) {
			GtkWidget *widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), current_page);
			struct cbuffer_ref *buffer_ref = NULL;
			buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
			if (buffer_ref) {
				gtk_widget_grab_focus(GTK_WIDGET(buffer_ref->source_view));
			}
		}
		return TRUE;
	}
	
	gint pages_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(window_handler->notebook));
	gint i = 0;
	GtkWidget *widget_page = NULL;
	struct cbuffer_ref *buffer_ref = NULL;
	for (i = 0; i < pages_count; i++) {
		widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), i);
		buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
		if (buffer_ref) {
			gtk_source_search_settings_set_search_text(buffer_ref->search_settings, gtk_entry_get_text(GTK_ENTRY(widget)));
		}
	}
	
	if (buffer_ref) {
		if (event->keyval == GDK_KEY_Down) {
			g_printf("[MESSAGE] Going to next search result.\n");
			GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
				"insert");
			gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
				&buffer_ref->current_search_start, cursor);
			GtkTextMark *cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
				"selection_bound");
			gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
				&buffer_ref->current_search_end,
				cursor_end);
		
			gtk_source_search_context_forward(buffer_ref->search_context, &buffer_ref->current_search_end,
				&buffer_ref->current_search_start,
				&buffer_ref->current_search_end);
			gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(buffer_ref->source_view),
				&buffer_ref->current_search_start,
				0.0,
				TRUE,
				0.5,
				0.5);
			gtk_text_buffer_select_range(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
				&buffer_ref->current_search_start,
				&buffer_ref->current_search_end);
			return TRUE;
		}
		if (event->keyval == GDK_KEY_Up) {
			g_printf("[MESSAGE] Going to previous search result.\n");
			GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), "insert");
			gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
				&buffer_ref->current_search_start,
				cursor);
			GtkTextMark *cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), "selection_bound");
			gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
				&buffer_ref->current_search_end,
				cursor_end);
		
			gtk_source_search_context_backward(buffer_ref->search_context, &buffer_ref->current_search_start,
				&buffer_ref->current_search_start,
				&buffer_ref->current_search_end);
			gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(buffer_ref->source_view),
				&buffer_ref->current_search_start,
				0.0,
				TRUE,
				0.5,
				0.5);
			gtk_text_buffer_select_range(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
				&buffer_ref->current_search_start,
				&buffer_ref->current_search_end);
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean entry_search_key_release_event(GtkWidget *widget,  GdkEventKey *event, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	
	gint pages_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(window_handler->notebook));
	gint i = 0;
	GtkWidget *widget_page = NULL;
	struct cbuffer_ref *buffer_ref = NULL;
	for (i = 0; i < pages_count; i++) {
		widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), i);
		buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
		if (buffer_ref) {
			gtk_source_search_settings_set_search_text(buffer_ref->search_settings, gtk_entry_get_text(GTK_ENTRY(widget)));
		}
	}
	return FALSE;
}

static gboolean source_view_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	
	g_printf("[MESSAGE] Source View key press.\n");
	struct cbuffer_ref *buffer_ref = g_object_get_data(G_OBJECT(widget), "buffer_ref");
	
	if (buffer_ref) {
		gtk_source_search_settings_set_search_text(buffer_ref->search_settings, gtk_entry_get_text(GTK_ENTRY(window_handler->search_entry)));
	}
	return FALSE;
}

static gboolean source_view_key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	g_printf("[MESSAGE] Source View key release.\n");
	GtkSourceBuffer *buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)));
	struct cbuffer_ref *buffer_ref = g_object_get_data(G_OBJECT(widget), "buffer_ref");
	
	if (buffer) {
		GtkWidget *label = buffer_ref->label;
		if (label) {
			if (buffer_ref->file_name) {
				if (gtk_source_buffer_can_undo(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)))) &&
				gtk_text_buffer_get_modified(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)))) {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_ACTIVE, TRUE);
				} else {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_NORMAL, TRUE);
				}
			}
		}
	}
	return FALSE;
}

static void button_close_tab_clicked(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	struct cbuffer_ref *buffer_ref = g_object_get_data(G_OBJECT(widget), "buffer_ref");
	lua_State* lua = window_handler->application_handler->lua;
	
	if (buffer_ref->scrolled_window) {
		gtk_notebook_remove_page(GTK_NOTEBOOK(window_handler->notebook),
			gtk_notebook_page_num(GTK_NOTEBOOK(window_handler->notebook), buffer_ref->scrolled_window));
		lua_getglobal(lua, "editor");
		if (lua_istable(lua, -1)) {
			lua_pushstring(lua, "f_close");
			lua_gettable(lua, -2);
			if (lua_isfunction(lua, -1)) {
				lua_pushlightuserdata(lua, buffer_ref->source_view);
				if (lua_pcall(lua, 1, 0, 0) != LUA_OK) {
					g_printf("[ERROR] Fail to call editor[\"f_close\"].\n");
				} else {
					g_printf("[MESSAGE] Function editor[\"f_close\"] called.\n");
				}
			} else {
				lua_pop(lua, 1);
			}
		}
		lua_pop(lua, 1);
		if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(window_handler->notebook)) <= 0) {
			gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window_handler->header_bar), NULL);
		}
	}
}

struct cbuffer_ref *create_page(struct cwindow_handler *window_handler, const gchar *file_name, const gchar *text)
{
	g_printf("[MESSAGE] Creating new page.\n");
	struct cpreferences *preferences = window_handler->preferences;
	lua_State* lua = window_handler->application_handler->lua;
	struct cbuffer_ref *buffer_ref = (struct cbuffer_ref *)malloc(sizeof(struct cbuffer_ref));
	buffer_ref->file_name = g_string_new(file_name);
	buffer_ref->modified = FALSE;
	buffer_ref->source_view = gtk_source_view_new();
	gtk_widget_grab_focus(buffer_ref->source_view);
	gtk_source_view_set_background_pattern(GTK_SOURCE_VIEW(buffer_ref->source_view), GTK_SOURCE_BACKGROUND_PATTERN_TYPE_GRID);
	gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->show_line_marks);
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->show_line_numbers);
	gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->show_right_margin);
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->tab_width);
	gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->auto_indent);
	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view))),
		preferences->highlight_matching_brackets);
	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(buffer_ref->source_view), preferences->highlight_current_line);
	
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), text, -1);
	
	GtkStyleContext *style_context = gtk_widget_get_style_context(buffer_ref->source_view);
	GtkCssProvider *css_provider = gtk_css_provider_new();
	
	GString *css_string = g_string_new("GtkSourceView { font: ");
	css_string = g_string_append(css_string, preferences->editor_font->str);
	css_string = g_string_append(css_string, ";}");
	
	gtk_css_provider_load_from_data(css_provider,
		css_string->str,
		-1,
		NULL);
	g_string_free(css_string, TRUE);
	
	gtk_style_context_add_provider(style_context,
		GTK_STYLE_PROVIDER(css_provider),
		GTK_STYLE_PROVIDER_PRIORITY_USER);
	g_object_unref(css_provider);
	gtk_widget_reset_style(GTK_WIDGET(buffer_ref->source_view));
	
	GtkSourceCompletion *completion = gtk_source_view_get_completion(GTK_SOURCE_VIEW(buffer_ref->source_view));
	if (completion) {
		gchar *separator = strrchr(buffer_ref->file_name->str, '/');
		if (separator) {
			separator++;
		} else {
			separator = buffer_ref->file_name->str;
		}
		GtkSourceCompletionWords *provider_words = gtk_source_completion_words_new(separator, NULL);
		buffer_ref->provider_words = provider_words;
		g_object_set(provider_words, "interactive-delay", 0, NULL);
		gtk_source_completion_words_register(provider_words, gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)));
		gtk_source_completion_add_provider(completion, GTK_SOURCE_COMPLETION_PROVIDER(provider_words), NULL);
	}
	
	buffer_ref->search_context = gtk_source_search_context_new(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view))), NULL);
	buffer_ref->search_settings = gtk_source_search_context_get_settings(buffer_ref->search_context);
	
	buffer_ref->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(buffer_ref->scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	GtkAdjustment *hadjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(buffer_ref->scrolled_window));
	GtkAdjustment *vadjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(buffer_ref->scrolled_window));
	gtk_adjustment_set_page_increment(hadjustment, 0.1);
	gtk_adjustment_set_step_increment(hadjustment, 0.1);
	gtk_adjustment_set_page_increment(vadjustment, 0.1);
	gtk_adjustment_set_step_increment(vadjustment, 0.1);
	gtk_container_add(GTK_CONTAINER(buffer_ref->scrolled_window), GTK_WIDGET(buffer_ref->source_view));
	
	g_signal_connect(buffer_ref->source_view, "key-press-event", G_CALLBACK(source_view_key_press_event), window_handler);
	g_signal_connect(buffer_ref->source_view, "key-release-event", G_CALLBACK(source_view_key_release_event), window_handler);
	
	g_object_set_data(G_OBJECT(buffer_ref->source_view), "buffer_ref", buffer_ref);
	g_object_set_data(G_OBJECT(buffer_ref->scrolled_window), "buffer_ref", buffer_ref);
	
	gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), &buffer_ref->current_search_end);
	gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), &buffer_ref->current_search_start);
	
	// Tab.
	GtkWidget *tab = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	GtkWidget *label = gtk_label_new("Untitled");
	
	gchar *separator = strrchr(buffer_ref->file_name->str, '/');
	if (separator) {
		gtk_label_set_text(GTK_LABEL(label), ++separator);
	}
	GtkWidget *button_close = gtk_button_new_from_icon_name("gtk-close", GTK_ICON_SIZE_BUTTON);
	g_object_set_data(G_OBJECT(button_close), "buffer_ref", buffer_ref);
	g_signal_connect(button_close, "clicked", G_CALLBACK(button_close_tab_clicked), window_handler);
	
	gtk_box_pack_start(GTK_BOX(tab), GTK_WIDGET(label), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tab), GTK_WIDGET(button_close), FALSE, TRUE, 0);
	
	buffer_ref->tab = tab;
	buffer_ref->label = label;
	
	gtk_widget_set_can_focus(GTK_WIDGET(tab), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(label), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(button_close), FALSE);
	gtk_button_set_focus_on_click(GTK_BUTTON(button_close), FALSE);
	
	style_context = gtk_widget_get_style_context(buffer_ref->label);
	css_provider = gtk_css_provider_new();
	
	css_string = g_string_new("GtkLabel:active { color: ");
	css_string = g_string_append(css_string, "red");
	css_string = g_string_append(css_string, ";}");
	
	gtk_css_provider_load_from_data(css_provider,
		css_string->str,
		-1,
		NULL);
	g_string_free(css_string, TRUE);
	
	gtk_style_context_add_provider(style_context,
		GTK_STYLE_PROVIDER(css_provider),
		GTK_STYLE_PROVIDER_PRIORITY_USER);
	g_object_unref(css_provider);
	gtk_widget_reset_style(GTK_WIDGET(buffer_ref->label));
	
	gint index = gtk_notebook_append_page(GTK_NOTEBOOK(window_handler->notebook),
		GTK_WIDGET(buffer_ref->scrolled_window), GTK_WIDGET(buffer_ref->tab));
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(window_handler->notebook), GTK_WIDGET(buffer_ref->scrolled_window), TRUE);
	gtk_widget_show_all(GTK_WIDGET(buffer_ref->tab));
	gtk_widget_show_all(GTK_WIDGET(buffer_ref->scrolled_window));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(window_handler->notebook), index);
	
	gtk_source_buffer_begin_not_undoable_action(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view))));
	gtk_text_buffer_set_modified(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), FALSE);
	gtk_source_buffer_end_not_undoable_action(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view))));
	gtk_text_buffer_set_modified(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)), FALSE);
	
	GtkTextIter start_iter;
	gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
		&start_iter);
	gtk_text_buffer_place_cursor(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
		&start_iter);
	
	gtk_widget_grab_focus(buffer_ref->source_view);
	g_printf("[MESSAGE] Sending widget to Lua state.\n");
	lua_getglobal(lua, "editor");
	if (lua_istable(lua, -1)) {
		lua_pushstring(lua, "f_new");
		lua_gettable(lua, -2);
		if (lua_isfunction(lua, -1)) {
			lua_pushlightuserdata(lua, buffer_ref->source_view);
			lua_pushstring(lua, file_name);
			if (lua_pcall(lua, 2, 0, 0) != LUA_OK) {
				g_printf("[ERROR] Fail to call editor[\"f_new\"].\n");
			} else {
				g_printf("[MESSAGE] Function editor[\"f_new\"] called.\n");
			}
		} else {
			lua_pop(lua, 1);
		}
	}
	lua_pop(lua, 1);
	
	update_page_language(window_handler, buffer_ref);
	update_views(window_handler);
	update_providers(window_handler);
	
	g_printf("[MESSAGE] New page created.\n");
	return buffer_ref;
}

static int lua_create_page(lua_State *lua)
{
	struct cwindow_handler *window_handler = (void *)lua_topointer(lua, 1);
	const char *file_name = lua_tostring(lua, 2);
	const char *text = lua_tostring(lua, 3);
	create_page(window_handler, file_name, text);
	return 0;
}
    
static void button_previous_clicked(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	struct cbuffer_ref *buffer_ref = NULL;
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
	if (scrolled_window) {
		buffer_ref = g_object_get_data(G_OBJECT(scrolled_window), "buffer_ref");
	}
	
	if (buffer_ref) {
		GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			"insert");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			&buffer_ref->current_search_start, cursor);
		GtkTextMark *cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			"selection_bound");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			&buffer_ref->current_search_end, cursor_end);
		
		gtk_source_search_context_backward(buffer_ref->search_context, &buffer_ref->current_search_start,
			&buffer_ref->current_search_start,
			&buffer_ref->current_search_end);
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(buffer_ref->source_view),
			&buffer_ref->current_search_start,
			0.0,
			TRUE,
			0.5,
			0.5);
		gtk_text_buffer_select_range(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			&buffer_ref->current_search_start,
			&buffer_ref->current_search_end);
	}
}

static void button_next_clicked(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	g_printf("[MESSAGE] Going to next search result.\n");
	struct cbuffer_ref *buffer_ref = NULL;
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
	if (scrolled_window) {
		buffer_ref = g_object_get_data(G_OBJECT(scrolled_window), "buffer_ref");
	}

	if (buffer_ref) {
		GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			"insert");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			&buffer_ref->current_search_start, cursor);
		GtkTextMark *cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			"selection_bound");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			&buffer_ref->current_search_end, cursor_end);
		
		gtk_source_search_context_forward(buffer_ref->search_context, &buffer_ref->current_search_end,
			&buffer_ref->current_search_start,
			&buffer_ref->current_search_end);
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(buffer_ref->source_view),
			&buffer_ref->current_search_start,
			0.0,
			TRUE,
			0.5,
			0.5);
		gtk_text_buffer_select_range(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			&buffer_ref->current_search_start,
			&buffer_ref->current_search_end);
	}
}

static void button_replace_clicked(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	g_printf("[MESSAGE] Going to previous search result.\n");
	struct cbuffer_ref *buffer_ref = NULL;
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
	if (scrolled_window) {
		buffer_ref = g_object_get_data(G_OBJECT(scrolled_window), "buffer_ref");
	}

	if (buffer_ref) {
		GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			"insert");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			&buffer_ref->current_search_start, cursor);
		GtkTextMark *cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			"selection_bound");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			&buffer_ref->current_search_end, cursor_end);
		
		gtk_source_search_context_replace(buffer_ref->search_context,
			&buffer_ref->current_search_start,
			&buffer_ref->current_search_end,
			gtk_entry_get_text(GTK_ENTRY(window_handler->replace_with_entry)),
			-1,
			NULL);
		
		cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			"insert");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			&buffer_ref->current_search_start, cursor);
		cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			"selection_bound");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			&buffer_ref->current_search_end, cursor_end);
		
		gtk_source_search_context_forward(buffer_ref->search_context, &buffer_ref->current_search_end,
			&buffer_ref->current_search_start,
			&buffer_ref->current_search_end);
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(buffer_ref->source_view),
			&buffer_ref->current_search_start,
			0.0,
			TRUE,
			0.5,
			0.5);
		gtk_text_buffer_select_range(gtk_text_view_get_buffer(GTK_TEXT_VIEW(buffer_ref->source_view)),
			&buffer_ref->current_search_start,
			&buffer_ref->current_search_end);
	}
}

static void button_replace_all_clicked(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	struct cbuffer_ref *buffer_ref = NULL;
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
	if (scrolled_window) {
		buffer_ref = g_object_get_data(G_OBJECT(scrolled_window), "buffer_ref");
	}
	
	if (buffer_ref) {
		gtk_source_search_context_replace_all(buffer_ref->search_context,
			gtk_entry_get_text(GTK_ENTRY(window_handler->replace_with_entry)),
			-1,
			NULL);
	}
}

static void window_destroy(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	struct cpreferences *preferences = window_handler->preferences;
	g_printf("[MESSAGE] Closing main window.\n");
	
	const gchar *scheme_id = gtk_source_style_scheme_get_id(preferences->scheme);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"general",
		"start_new_page",
		preferences->start_new_page);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"general",
		"use_decoration",
		preferences->use_decoration);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"general",
		"use_custom_gtk_theme",
		preferences->use_custom_gtk_theme);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"general",
		"show_menu_bar",
		preferences->show_menu_bar);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"general",
		"show_tool_bar",
		preferences->show_tool_bar);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"general",
		"show_action_bar",
		preferences->show_action_bar);
	
	g_key_file_set_string(preferences->configuration_file,
		"general",
		"gtk_theme",
		preferences->gtk_theme);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"general",
		"use_custom_gtk_theme",
		preferences->use_custom_gtk_theme);
	
	g_key_file_set_integer(preferences->configuration_file,
		"general",
		"tabs_position",
		preferences->tabs_position);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"show_line_numbers",
		preferences->show_line_numbers);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"show_map",
		preferences->show_map);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"highlight_current_line",
		preferences->highlight_current_line);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"highlight_matching_brackets",
		preferences->highlight_matching_brackets);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"show_line_marks",
		preferences->show_line_marks);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"auto_indent",
		preferences->auto_indent);
	
	g_key_file_set_integer(preferences->configuration_file,
		"editor",
		"tab_width",
		preferences->tab_width);
	
	g_key_file_set_string(preferences->configuration_file,
		"editor",
		"scheme_id",
		scheme_id);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"show_right_margin",
		preferences->show_right_margin);
	
	g_key_file_set_integer(preferences->configuration_file,
		"editor",
		"right_margin_position",
		preferences->right_margin_position);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"show_grid",
		preferences->show_grid);
	
	g_key_file_set_string(preferences->configuration_file,
		"editor",
		"font",
		preferences->editor_font->str);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"draw_spaces_space",
		preferences->draw_spaces_space);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"draw_spaces_leading",
		preferences->draw_spaces_leading);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"draw_spaces_newline",
		preferences->draw_spaces_newline);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"draw_spaces_tab",
		preferences->draw_spaces_tab);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"draw_spaces_trailing",
		preferences->draw_spaces_trailing);
	
	if (preferences->configuration_file_path) {
		if (g_key_file_save_to_file(preferences->configuration_file, preferences->configuration_file_path->str, NULL)) {
			g_printf("[MESSAGE] Configuration saved.\n");
		}
	} else {
		g_printf("[ERROR] Configuration not saved. Lack configuration file path.\n");
	}
	
	lua_State* lua = window_handler->application_handler->lua;
	lua_getglobal(lua, "editor");
	if (lua_istable(lua, -1)) {
		lua_pushstring(lua, "f_terminate");
		lua_gettable(lua, -2);
		if (lua_isfunction(lua, -1)) {
			if (lua_pcall(lua, 0, 0, 0) != LUA_OK) {
				g_printf("[ERROR] Fail to call editor[\"f_terminate\"].\n");
			} else {
				g_printf("[MESSAGE] Function editor[\"f_terminate\"] called.\n");
			}
		} else {
			lua_pop(lua, 1);
		}
	} else {
		lua_pop(lua, 1);
	}
}

static void notebook_switch_page(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	lua_State* lua = window_handler->application_handler->lua;
	g_printf("[MESSAGE] Page switched. Calling Lua function editor[\"f_page_switch\"].\n");
	
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		page_num);
	
	if (scrolled_window) {
		struct cbuffer_ref *buffer_ref = NULL;
		buffer_ref = g_object_get_data(G_OBJECT(scrolled_window), "buffer_ref");
		if (buffer_ref) {
			window_handler->preferences->last_path = g_string_assign(window_handler->preferences->last_path, buffer_ref->file_name->str);
		
			char *i = strrchr(window_handler->preferences->last_path->str, '/');
			if (i) {
				window_handler->preferences->last_path = g_string_set_size(window_handler->preferences->last_path, i - window_handler->preferences->last_path->str);
			}
			//g_printf("PATH %i TO \"%s\"\n\n\n", page_num, window_handler->preferences->last_path->str);
		
			gtk_header_bar_set_subtitle(GTK_HEADER_BAR(window_handler->header_bar), buffer_ref->file_name->str);
			lua_getglobal(lua, "editor");
			if (lua_istable(lua, -1)) {
				lua_pushstring(lua, "f_page_switch");
				lua_gettable(lua, -2);
				if (lua_isfunction(lua, -1)) {
					g_printf("[MESSAGE] Passing widget as Lua user-data.\n");
					lua_pushlightuserdata(lua, (void *)buffer_ref->source_view);
					if (lua_pcall(lua, 1, 0, 0) != LUA_OK) {
						g_printf("[ERROR] Fail to call editor[\"f_page_switch\"].\n");
					} else {
						g_printf("[MESSAGE] Function editor[\"f_page_switch\"] called.\n");
					}
				} else {
					lua_pop(lua, 1);
				}
			}
			lua_pop(lua, 1);
		}
	}
}

static void window_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint type, guint time, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	const gchar *sel_data = (const gchar *)gtk_selection_data_get_data(selection_data);
	
	GString* file_string = g_string_new(sel_data);
	gchar *file_name = file_string->str;
	gint length = file_string->len;
	
	while ((file_name[length - 1] == ' ') || (file_name[length - 1] == '\n') || (file_name[length - 1] == '\r')) {
		file_name[length - 1] = '\0';
		length = length - 1;
	}
	
	file_name = file_name + 7;
	g_printf("\nData received (%i): %s.\n", strlen(file_name), file_name);
	gtk_drag_finish(context, TRUE, FALSE, time);
	
	g_printf("[MESSAGE] Get separator.\n");
	char *last_separator = strrchr(file_name, '/');
	char *file_only_name = last_separator + 1;
	g_printf("[MESSAGE] Loading file \"%s\".\n", file_only_name);
	FILE *file = fopen(file_name, "r");
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	char *text = (char *)malloc(sizeof(char) * size + 1);
	memset(text, 0, sizeof(char) * size + 1);
	fseek(file, 0, SEEK_SET);
	fread(text, sizeof(char), size, file);
	
	g_printf("[MESSAGE] Updating text editor schemes.\n");
	gint pages_count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(window_handler->notebook));
	gint i = 0;
	
	gboolean already_open = FALSE;
	GtkWidget *widget_page = NULL;
	struct cbuffer_ref *buffer_ref = NULL;
	for (i = 0; i < pages_count; i++) {
		widget_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), i);
		buffer_ref = g_object_get_data(G_OBJECT(widget_page), "buffer_ref");
		if (buffer_ref) {
			if (strcmp(buffer_ref->file_name->str, file_name) == 0) {
				already_open = TRUE;
			}
		}
	}
	
	if (already_open == FALSE) {
		create_page(window_handler, file_name, text);
	}
	fclose(file);
	window_handler->drag_and_drop = FALSE;
}

static void window_drag_leave(GtkWidget *widget, GdkDragContext *context, guint time, gpointer *user_data) {
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	window_handler->drag_and_drop = FALSE;
	g_printf("Drag-leave.\n");
}

static gboolean window_drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	
	if (window_handler->drag_and_drop) {
		return TRUE;
	}
	g_printf("Drag-motion.\n");
	
	window_handler->drag_and_drop = TRUE;
	return TRUE;
}

static gboolean window_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data)
{
	GdkAtom target_type;
	gboolean result = FALSE;
	GList *list_of_targets = gdk_drag_context_list_targets(context);
	if (list_of_targets) {
		target_type = GDK_POINTER_TO_ATOM(g_list_nth_data(list_of_targets, 0));
		gtk_drag_get_data(widget, context, target_type, time);
		result = TRUE;
		g_printf("Drag-drop resulting TRUE.\n");
	} else {
		result = FALSE;
		g_printf("Drag-drop resulting FALSE.\n");
	}
	return result;
}

void initialize_lua(struct cwindow_handler *window_handler, struct cpreferences *preferences)
{
	g_printf("[MESSAGE] Initializing Lua.\n");
	window_handler->application_handler->lua = luaL_newstate();
	lua_State* lua = window_handler->application_handler->lua;
	const lua_Number *lua_v = lua_version(lua);
	
	if (lua_v) {
		g_printf("[MESSAGE] Lua version is %s.\n", LUA_VERSION);
	}
	
	luaL_openlibs(lua);
	if (lua) {
		g_printf("[MESSAGE] Lua state created.\n");
	} else {
		g_printf("[ERROR] Failed to create lua state.\n");
		return;
	}
	
	lua_newtable(lua);
	lua_setglobal(lua, "editor");
	
	lua_getglobal(lua, "editor");
	if (lua_istable(lua, -1)) {
		lua_pushstring(lua, "window_handler");
		lua_pushlightuserdata(lua, window_handler);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "window");
		lua_pushlightuserdata(lua, window_handler->window);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "menu_model");
		lua_pushlightuserdata(lua, window_handler->menu_model);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "menu_bar");
		lua_pushlightuserdata(lua, window_handler->menu_bar);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "header_bar");
		lua_pushlightuserdata(lua, window_handler->header_bar);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "notebook");
		lua_pushlightuserdata(lua, window_handler->notebook);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "action_bar");
		lua_pushlightuserdata(lua, window_handler->action_bar);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "window_box");
		lua_pushlightuserdata(lua, window_handler->box);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "configuration_directory");
		if ( preferences->program_path) {
			lua_pushstring(lua, preferences->program_path->str);
		} else {
			lua_pushstring(lua, "");
		}
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "accelerator_group");
		lua_pushlightuserdata(lua, window_handler->accelerator_group);
		lua_settable(lua, -3);
		
		if (preferences->program_path) {
			lua_pushstring(lua, "config_path");
			lua_pushstring(lua, preferences->program_path->str);
			lua_settable(lua, -3);
		}
		
		lua_pushstring(lua, "create_page");
		lua_pushcfunction(lua, lua_create_page);
		lua_settable(lua, -3);
		
	}
	lua_pop(lua, 1);
	
	g_printf("[MESSAGE] Loading base extension.\n");
	// Load base script.
	GString *base_full_name = NULL;
	if (preferences->program_path) {
		base_full_name = g_string_new(preferences->program_path->str);
		base_full_name = g_string_append(base_full_name, "/base.lua");
	}
	if (base_full_name) {
		luaL_loadfile(lua, base_full_name->str);
		if (lua_pcall(lua, 0, 0, 0) != LUA_OK) {
			g_printf("[ERROR] Fail to load lua script \"%s\".\n", base_full_name->str);
		} else {
			g_printf("[MESSAGE] Lua script \"%s\" loaded.\n", base_full_name->str);
		}
		g_string_free(base_full_name, TRUE);
		base_full_name = NULL;
	}
	
	// Call the initialization function.
	lua_getglobal(lua, "editor");
	if (lua_istable(lua, -1)) {
		lua_pushstring(lua, "f_initialize");
		lua_gettable(lua, -2);
		if (lua_isfunction(lua, -1)) {
			lua_pushlightuserdata(lua, window_handler->window);
			if (lua_pcall(lua, 1, 0, 0) != LUA_OK) {
				g_printf("[ERROR] Fail to call editor[\"f_initialize\"].\n");
			} else {
				g_printf("[MESSAGE] Function editor[\"f_initialize\"] called.\n");
			}
		} else {
			lua_pop(lua, 1);
		}
	} else {
		lua_pop(lua, 1);
	}
	
	// Load extension scripts.
	if (preferences->extension_path) {
		g_printf("[MESSAGE] Loading extensions.\n");
		GDir *dir = g_dir_open(preferences->extension_path->str,
			0,
			NULL);
		if (dir) {
			//ent = readdir(dir);
			const gchar *ent = g_dir_read_name(dir);
			while (ent != NULL) {
				g_printf("-> Directory \"%s\".\n", ent);
				GString *source_full_name = g_string_new(preferences->extension_path->str);
				source_full_name = g_string_append(source_full_name, "/");
				source_full_name = g_string_append(source_full_name, ent);
				if (g_file_test(source_full_name->str, G_FILE_TEST_IS_REGULAR)) {
					char *name_extension = strrchr(ent, '.');
					if (name_extension != NULL ) {
						if (strcmp(name_extension, ".lua") == 0) {
							luaL_loadfile(lua, source_full_name->str);
							if (lua_pcall(lua, 0, 0, 0) != LUA_OK) {
								g_printf("[ERROR] Fail to load lua script \"%s\".\n", source_full_name->str);
							} else {
								g_printf("[MESSAGE] Lua script \"%s\" loaded.\n", source_full_name->str);
							}
						}
					}
				}
				g_string_free(source_full_name, TRUE);
				ent = g_dir_read_name(dir);
			}
			g_dir_close(dir);
		}
	} else {
		g_printf("[ERROR] Ignoring extensions. Extensions path not set.\n");
	}
	
	// Call the plugins initialization functions
	lua_getglobal(lua, "editor");
	if (lua_istable(lua, -1)) {
		lua_pushstring(lua, "f_initialize_plugins");
		lua_gettable(lua, -2);
		if (lua_isfunction(lua, -1)) {
			lua_pushlightuserdata(lua, window_handler->window);
			if (lua_pcall(lua, 1, 0, 0) != LUA_OK) {
				g_printf("[ERROR] Fail to call editor[\"f_initialize_plugins\"].\n");
			} else {
				g_printf("[MESSAGE] Function editor[\"f_initialize_plugins\"] called.\n");
			}
		} else {
			lua_pop(lua, 1);
		}
	} else {
		lua_pop(lua, 1);
	}
	
	// Send pages to Lua side.
	int i = 0;
	struct cbuffer_ref *buffer_ref = NULL;
	GtkWidget *widget = NULL;
	while (i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(window_handler->notebook))) {
		widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook), i);
		if (widget) {
			buffer_ref = g_object_get_data(G_OBJECT(widget), "buffer_ref");
			if (buffer_ref) {
				lua_getglobal(lua, "editor");
				if (lua_istable(lua, -1)) {
					lua_pushstring(lua, "f_new");
					lua_gettable(lua, -2);
					if (lua_isfunction(lua, -1)) {
						lua_pushlightuserdata(lua, buffer_ref->source_view);
						lua_pushstring(lua, buffer_ref->file_name->str);
						if (lua_pcall(lua, 2, 0, 0) != LUA_OK) {
							g_printf("[ERROR] Fail to call editor[\"f_new\"].\n");
						} else {
							g_printf("[MESSAGE] Function editor[\"f_new\"] called.\n");
						}
					} else {
						lua_pop(lua, 1);
					}
				}
				lua_pop(lua, 1);
			}
		}
		i = i + 1;
	}
	g_printf("[MESSAGE] Lua initialized.\n");
}

static GMenu *create_model(struct cwindow_handler *window_handler)
{
	GMenu *menu_model = g_menu_new();
	GSimpleActionGroup *action_group = NULL;
	
	// Main action group.
	action_group = g_simple_action_group_new();
	gtk_widget_insert_action_group(GTK_WIDGET(window_handler->window), "main", G_ACTION_GROUP(action_group));
	
	GSimpleAction *action = NULL;
	action = g_simple_action_new("new", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_new_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("open", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_open_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("save", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_save_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("save_as", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_save_as_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("about", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_about_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("preferences", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_preferences_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("exit", NULL);
	//g_signal_connect(action, "activate", G_CALLBACK(action_exit_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	// Edit action group.
	action_group = g_simple_action_group_new();
	gtk_widget_insert_action_group(GTK_WIDGET(window_handler->window), "edit", G_ACTION_GROUP(action_group));
	
	action = g_simple_action_new("undo", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_undo_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("redo", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_redo_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
		
	action = g_simple_action_new("copy", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_copy_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("paste", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_paste_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("cut", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_cut_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("delete", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_delete_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	// View action group.
	action_group = g_simple_action_group_new();
	gtk_widget_insert_action_group(GTK_WIDGET(window_handler->window), "view", G_ACTION_GROUP(action_group));
	
	action = g_simple_action_new("toggle_fullscreen", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_toggle_fullscreen_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("toggle_menu_bar", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_toggle_menu_bar_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	action = g_simple_action_new("toggle_action_bar", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(action_toggle_action_bar_activate), window_handler);
	g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
	
	GMenu *menu_model_sub = NULL;
	
	// Menu main.
	menu_model_sub = g_menu_new();
	g_menu_append_submenu(G_MENU(menu_model), "Main", G_MENU_MODEL(menu_model_sub));
	g_menu_append(menu_model_sub, "New", "main.new");
	g_menu_append(menu_model_sub, "Open", "main.open");
	g_menu_append(menu_model_sub, "Save", "main.save");
	g_menu_append(menu_model_sub, "Save as", "main.save_as");
	g_menu_append(menu_model_sub, "Preferences", "main.preferences");
	g_menu_append(menu_model_sub, "About", "main.about");
	g_menu_append(menu_model_sub, "Exit", "main.exit");
	
	// Menu edit.
	menu_model_sub = g_menu_new();
	g_menu_append_submenu(G_MENU(menu_model), "Edit", G_MENU_MODEL(menu_model_sub));
	g_menu_append(menu_model_sub, "Undo", "edit.undo");
	g_menu_append(menu_model_sub, "Redo", "edit.redo");
	g_menu_append(menu_model_sub, "Copy", "edit.copy");
	g_menu_append(menu_model_sub, "Paste", "edit.paste");
	g_menu_append(menu_model_sub, "Cut", "edit.cut");
	g_menu_append(menu_model_sub, "Delete", "edit.delete");
	
	// Menu view.
	menu_model_sub = g_menu_new();
	g_menu_append_submenu(G_MENU(menu_model), "View", G_MENU_MODEL(menu_model_sub));
	g_menu_append(menu_model_sub, "Toggle action bar", "view.toggle_action_bar");
	g_menu_append(menu_model_sub, "Toggle menu bar", "view.toggle_menu_bar");
	g_menu_append(menu_model_sub, "Toggle fullscreen", "view.toggle_fullscreen");
	
	GtkAccelGroup *accelerator_group = gtk_accel_group_new();
	window_handler->accelerator_group = accelerator_group;
	gtk_window_add_accel_group(GTK_WINDOW(window_handler->window), accelerator_group);
	
	// Main accelerators.
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_N,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_new), window_handler, NULL));
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_O,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_open), window_handler, NULL));
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_S,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_save), window_handler, NULL));
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_S,
		GDK_CONTROL_MASK | GDK_SHIFT_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_save_as), window_handler, NULL));
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_W,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_close), window_handler, NULL));
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_F4,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_exit), window_handler, NULL));
	
	// Edit accelerators.
	
	// View accelerators.
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_F11,
		0,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_toggle_fullscreen), window_handler, NULL));
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_M,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_toggle_menu_bar), window_handler, NULL));
	
	// View accelerators.
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_M,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_toggle_menu_bar), window_handler, NULL));
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_Page_Down,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_next_page), window_handler, NULL));
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_Page_Up,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_previous_page), window_handler, NULL));
	
	// Tools.
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_F,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_search), window_handler, NULL));
	
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_R,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_replace), window_handler, NULL));
	
	return menu_model;
}

struct cwindow_handler *alloc_window_handler(struct capplication_handler *application_handler, struct cpreferences *preferences)
{
	g_printf("[MESSAGE] Creating main window.\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)malloc(sizeof(struct cwindow_handler));
	window_handler->application_handler = application_handler;
	window_handler->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	window_handler->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	window_handler->revealer = NULL;
	
	window_handler->drag_and_drop = FALSE;
	gtk_window_set_icon_name(GTK_WINDOW(window_handler->window), "lovetext");
	gtk_window_set_title(GTK_WINDOW(window_handler->window), "Love Text");
	gtk_widget_set_size_request(GTK_WIDGET(window_handler->window), 480, 380);
	g_signal_connect(window_handler->window, "destroy", G_CALLBACK(window_destroy), window_handler);
	
	// Header bar.
	window_handler->header_bar = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(window_handler->header_bar), "Love Text");
	gtk_header_bar_set_decoration_layout(GTK_HEADER_BAR(window_handler->header_bar),
		"close,maximize,minimize:menu");
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(window_handler->header_bar), TRUE);
	if (preferences->use_decoration) {
		gtk_window_set_titlebar(GTK_WINDOW(window_handler->window), GTK_WIDGET(window_handler->header_bar));
	}
	
	// Main button.
	window_handler->main_button = gtk_menu_button_new();
	gtk_header_bar_pack_start(GTK_HEADER_BAR(window_handler->header_bar), GTK_WIDGET(window_handler->main_button));
	GList *first_iterator = gtk_container_get_children(GTK_CONTAINER(window_handler->main_button));
	GtkWidget *child = first_iterator->data;
	if (child) {
		gtk_container_remove(GTK_CONTAINER(window_handler->main_button), child);
	}
	
	GIcon *icon = g_themed_icon_new("view-sidebar-symbolic");
	child = gtk_image_new_from_gicon(icon, GTK_ICON_SIZE_BUTTON);
	g_object_unref(icon);
	gtk_container_add(GTK_CONTAINER(window_handler->main_button), GTK_WIDGET(child));
	
	gtk_widget_set_halign(GTK_WIDGET(window_handler->main_button), GTK_ALIGN_FILL);
	
	// Main menu model.
	GMenu *menu_model = create_model(window_handler);
	
	window_handler->menu_model = menu_model;
	if (preferences->use_decoration == FALSE) {
		gtk_application_set_menubar(GTK_APPLICATION(application_handler->application), G_MENU_MODEL(window_handler->menu_model));
	}
	
	// Main popover.
	window_handler->main_popover = gtk_popover_new_from_model(GTK_WIDGET(window_handler->main_button), G_MENU_MODEL(menu_model));
	gtk_popover_set_modal(GTK_POPOVER(window_handler->main_popover), TRUE);
	gtk_popover_set_relative_to(GTK_POPOVER(window_handler->main_popover), GTK_WIDGET(window_handler->main_button));
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(window_handler->main_button), window_handler->main_popover);
	
	// Menu bar.
	g_printf("[MESSAGE] Creating menu bar.\n");
	GtkWidget *menu_bar = gtk_menu_bar_new_from_model(G_MENU_MODEL(menu_model));
	window_handler->menu_bar = menu_bar;
	if (preferences->use_decoration == FALSE) {
		gtk_box_pack_start(GTK_BOX(window_handler->box), window_handler->menu_bar, FALSE, TRUE, 0);
	}
	
	// Clipboard.
	window_handler->clipboard = NULL;
	GdkAtom clipboard_atom = gdk_atom_intern("CLIPBOARD", TRUE);
	if (clipboard_atom) {
		window_handler->clipboard = gtk_clipboard_get(clipboard_atom);
	}
	
	window_handler->window_fullscreen = FALSE;
	window_handler->action_bar = gtk_action_bar_new();
	
	// Notebook.
	window_handler->notebook = gtk_notebook_new();
	g_signal_connect(window_handler->notebook, "switch-page", G_CALLBACK(notebook_switch_page), window_handler);
	
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(window_handler->notebook), TRUE);
	
	gtk_box_pack_end(GTK_BOX(window_handler->box), window_handler->action_bar, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(window_handler->box), window_handler->notebook, TRUE, TRUE, 0);
	
	// Search bar.
	GtkWidget *search_and_replace_bar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	window_handler->search_and_replace_bar = search_and_replace_bar;
	
	GtkWidget *search_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	window_handler->search_bar = search_bar;
	window_handler->search_entry = gtk_entry_new();
	g_signal_connect(window_handler->search_entry, "key-press-event", G_CALLBACK(entry_search_key_press_event), window_handler);
	g_signal_connect(window_handler->search_entry, "key-release-event", G_CALLBACK(entry_search_key_release_event), window_handler);
	
	GtkWidget *label = gtk_label_new("Search:");
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
	
	gtk_box_pack_start(GTK_BOX(search_bar), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(search_bar), window_handler->search_entry, TRUE, TRUE, 0);
	
	GtkWidget *button = gtk_button_new_with_label("Previous");
	gtk_widget_set_size_request(GTK_WIDGET(button), 96, -1);
	g_signal_connect(GTK_WIDGET(button), "clicked", G_CALLBACK(button_previous_clicked), window_handler);
	gtk_box_pack_start(GTK_BOX(search_bar), button, FALSE, FALSE, 0);
	button = gtk_button_new_with_label("Next");
	gtk_widget_set_size_request(GTK_WIDGET(button), 96, -1);
	g_signal_connect(button, "clicked", G_CALLBACK(button_next_clicked), window_handler);
	gtk_box_pack_start(GTK_BOX(search_bar), button, FALSE, FALSE, 0);
	
	// Replace bar
	GtkWidget *replace_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	window_handler->replace_bar = replace_bar;
	window_handler->replace_with_entry = gtk_entry_new();
	//g_signal_connect(window_handler->search_entry, "key-press-event", G_CALLBACK(entry_search_key_press_event), window_handler);
	//g_signal_connect(window_handler->search_entry, "key-release-event", G_CALLBACK(entry_search_key_release_event), window_handler);
	
	label = gtk_label_new("Replace for:");
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
	
	gtk_box_pack_start(GTK_BOX(replace_bar), GTK_WIDGET(label), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(replace_bar), GTK_WIDGET(window_handler->replace_with_entry), TRUE, TRUE, 0);
	
	button = gtk_button_new_with_label("Replace");
	gtk_widget_set_size_request(GTK_WIDGET(button), 96, -1);
	g_signal_connect(button, "clicked", G_CALLBACK(button_replace_clicked), window_handler);
	gtk_box_pack_start(GTK_BOX(replace_bar), GTK_WIDGET(button), FALSE, FALSE, 0);
	button = gtk_button_new_with_label("Replace All");
	gtk_widget_set_size_request(GTK_WIDGET(button), 96, -1);
	g_signal_connect(button, "clicked", G_CALLBACK(button_replace_all_clicked), window_handler);
	gtk_box_pack_start(GTK_BOX(replace_bar), GTK_WIDGET(button), FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(window_handler->search_and_replace_bar), GTK_WIDGET(search_bar), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(window_handler->search_and_replace_bar), GTK_WIDGET(replace_bar), FALSE, TRUE, 0);
	gtk_action_bar_pack_start(GTK_ACTION_BAR(window_handler->action_bar), search_and_replace_bar);//, TRUE, TRUE, 0);
	
	gtk_container_add(GTK_CONTAINER(window_handler->window), window_handler->box);
	
	window_handler->id_factory = 0;
	
	window_handler->preferences = preferences;
	
	window_handler->decorated = window_handler->preferences->use_decoration;
	
	gtk_drag_dest_set(window_handler->notebook, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
	gtk_drag_dest_add_text_targets(window_handler->notebook);
	gtk_drag_dest_add_uri_targets(window_handler->notebook);
	g_signal_connect(window_handler->notebook, "drag-drop", G_CALLBACK(window_drag_drop), window_handler);
	g_signal_connect(window_handler->notebook, "drag-leave", G_CALLBACK(window_drag_leave), window_handler);
	g_signal_connect(window_handler->notebook, "drag-motion", G_CALLBACK(window_drag_motion), window_handler);
	g_signal_connect(window_handler->notebook, "drag-data-received", G_CALLBACK(window_drag_data_received), window_handler);
	
	g_printf("[MESSAGE] Main window created.\n");
	update_editor(window_handler);
	return window_handler;
}

// End of file.
