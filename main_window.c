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
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstring.h>
#include <glib/glist.h>
#include <gio/gio.h>
#include <gdk/gdkkeysyms.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
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

void initialize_lua(struct cwindow_handler *window_handler, struct cpreferences *preferences)
{
	window_handler->application_handler->lua = luaL_newstate();
	lua_State* lua = window_handler->application_handler->lua;
	lua_Number *lua_v = lua_version(lua);
	
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
		lua_pushstring(lua, "window");
		lua_pushlightuserdata(lua, window_handler->window);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "menu_bar");
		lua_pushlightuserdata(lua, window_handler->menu_bar);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "notebook");
		lua_pushlightuserdata(lua, window_handler->notebook);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "status_bar");
		lua_pushlightuserdata(lua, window_handler->status_bar);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "window_box");
		lua_pushlightuserdata(lua, window_handler->box);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "configuration_file");
		lua_pushlightuserdata(lua, preferences->configuration_file);
		lua_settable(lua, -3);
		
		lua_pushstring(lua, "accelerator_group");
		lua_pushlightuserdata(lua, window_handler->accelerator_group);
		lua_settable(lua, -3);
		
		if (preferences->program_path) {
			lua_pushstring(lua, "config_path");
			lua_pushstring(lua, preferences->program_path->str);
			lua_settable(lua, -3);
		}
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
	DIR *dir;
	struct dirent *ent;
	int size;
	if (preferences->extension_path) {
		g_printf("[MESSAGE] Loading extensions.\n");
		dir = opendir(preferences->extension_path->str);
		if (dir) {
			ent = readdir(dir);
			while (ent != NULL) {
				unsigned char *name = ent->d_name;
				if (name[0] == '.') {
					//g_printf("Ignoring hidden item \"%s\".\n", name);
				} else {
					GString *source_full_name = g_string_new(preferences->extension_path->str);
					source_full_name = g_string_append(source_full_name, "/");
					source_full_name = g_string_append(source_full_name, name);
					if (ent->d_type == DT_REG) {
						unsigned char *name_extension = strrchr(name, '.');
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
					g_string_free(source_full_name, NULL);
				}
				ent = readdir(dir);
			}
			closedir(dir);
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
	while (i < gtk_notebook_get_n_pages(window_handler->notebook)) {
		widget = gtk_notebook_get_nth_page(window_handler->notebook, i);
		if (widget) {
			buffer_ref = g_object_get_data(widget, "buf_ref");
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

static void update_page_language(struct cwindow_handler *window_handler, struct cbuffer_ref *buffer_ref)
{
	GtkSourceBuffer *source_buffer = gtk_text_view_get_buffer(buffer_ref->source_view);
	GtkSourceLanguage *language = NULL;
	gboolean result_uncertain;
	gchar *content_type;
	content_type = g_content_type_guess(buffer_ref->file_name->str, NULL, 0, &result_uncertain);
	if (result_uncertain) {
		g_free (content_type);
		content_type = NULL;
	}
	language = gtk_source_language_manager_guess_language(window_handler->preferences->language_manager,
		buffer_ref->file_name->str, content_type);
	if (language) {
		gtk_source_buffer_set_language(source_buffer, language);
	}
	g_free (content_type);
}

static void update_styles(struct cwindow_handler *window_handler)
{
	g_printf("[MESSAGE] Updating text editor schemes.\n");
	gint pages_count = gtk_notebook_get_n_pages(window_handler->notebook);
	gint i = 0;
	
	GtkWidget *widget_page = NULL;
	struct cbuffer_ref *buffer_ref = NULL;
	for (i = 0; i < pages_count; i++) {
		widget_page = gtk_notebook_get_nth_page(window_handler->notebook, i);
		buffer_ref = g_object_get_data(widget_page, "buf_ref");
		if (buffer_ref) {
			if (window_handler->preferences->scheme) {
				gchar *scheme_id = NULL;
				
				if (window_handler->preferences->scheme) {
					scheme_id = gtk_source_style_scheme_get_id(window_handler->preferences->scheme);
				}
				if (scheme_id) {
					g_printf("[MESSAGE] Scheme \"%s\" set at page %i.\n", scheme_id, i);
				} else {
					g_printf("[MESSAGE] Scheme set at page %i.\n", i);
				}
				GtkSourceBuffer *source_buffer = gtk_text_view_get_buffer(buffer_ref->source_view);
				gtk_source_buffer_set_style_scheme(source_buffer, window_handler->preferences->scheme);
				PangoFontDescription *font_description = pango_font_description_from_string(window_handler->preferences->editor_font->str);
				gtk_widget_override_font(buffer_ref->source_view, font_description);
				gtk_source_view_set_show_line_marks(buffer_ref->source_view, window_handler->preferences->show_line_marks);
				gtk_source_view_set_show_line_numbers(buffer_ref->source_view, window_handler->preferences->show_line_numbers);
				gtk_source_view_set_show_right_margin(buffer_ref->source_view, window_handler->preferences->show_right_margin);
				gtk_source_view_set_tab_width(buffer_ref->source_view, window_handler->preferences->tab_width);
				gtk_source_view_set_auto_indent(buffer_ref->source_view, window_handler->preferences->auto_indent);
				gtk_source_buffer_set_highlight_matching_brackets(source_buffer, window_handler->preferences->highlight_matching_brackets);
				gtk_source_view_set_highlight_current_line(buffer_ref->source_view, window_handler->preferences->highlight_current_line);
			}
		}
	}
}

static void update_providers(struct cwindow_handler *window_handler)
{
	gint pages_count = gtk_notebook_get_n_pages(window_handler->notebook);
	gint i = 0;
	gint j = 0;
	
	GtkWidget *widget_page = NULL;
	struct cbuffer_ref *buffer_ref_a = NULL;
	struct cbuffer_ref *buffer_ref_b = NULL;
	for (i = 0; i < pages_count; i++) {
		widget_page = gtk_notebook_get_nth_page(window_handler->notebook, i);
		buffer_ref_a = g_object_get_data(widget_page, "buf_ref");
		GtkSourceBuffer *source_buffer_a = gtk_text_view_get_buffer(buffer_ref_a->source_view);
		GtkSourceCompletion *completion = gtk_source_view_get_completion(buffer_ref_a->source_view);
		
		for (j = 0; j < pages_count; j++) {
			widget_page = gtk_notebook_get_nth_page(window_handler->notebook, j);
			buffer_ref_b = g_object_get_data(widget_page, "buf_ref");
			GtkSourceBuffer *source_buffer_b = gtk_text_view_get_buffer(buffer_ref_b->source_view);
			
			if (completion) {
				GtkSourceCompletionWords *provider_words = buffer_ref_b->provider_words;
				gtk_source_completion_add_provider(completion, provider_words, NULL);
			}
		}
	}
}

static void update_editor(struct cwindow_handler *window_handler)
{
	update_styles(window_handler);
}

// Actions callbacks.
static void menu_item_new_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.new\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	struct cbuffer_ref *buffer_ref = create_page(window_handler, "", FALSE, "", window_handler->preferences);
	
	gtk_label_set_text(buffer_ref->label, "Untitled");
	gint index = gtk_notebook_append_page(GTK_NOTEBOOK(window_handler->notebook), buffer_ref->scrolled_window, buffer_ref->tab);
	gtk_widget_show_all(buffer_ref->tab);
	gtk_widget_show_all(buffer_ref->scrolled_window);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(window_handler->notebook), buffer_ref->scrolled_window, TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(window_handler->notebook), FALSE);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(window_handler->notebook), index);
	
	update_styles(window_handler);
}

static void accel_new(GtkWidget *w, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_new_activate(NULL, user_data);
	}
}

static void menu_item_open_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.open\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	GtkWidget *file_chooser = gtk_file_chooser_dialog_new("Open",
		window_handler->window,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_select_multiple(file_chooser, TRUE);
	gtk_file_chooser_set_show_hidden(file_chooser, TRUE);
	gtk_file_chooser_set_current_folder(file_chooser, window_handler->preferences->last_path->str);
	
	if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
		GSList *filenames = gtk_file_chooser_get_filenames(file_chooser);
		GSList *iterator = filenames;
		while (iterator) {
			gchar *file_name = iterator->data;//gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
			GtkWidget *source_view = NULL;
			if (source_view) {
				g_printf("[MESSAGE] File already open.\n");
				GtkWidget *scrolled_window = gtk_widget_get_parent(source_view);
				if (scrolled_window) {
					gtk_notebook_set_current_page(GTK_NOTEBOOK(window_handler->notebook), 
						gtk_notebook_page_num(GTK_NOTEBOOK(window_handler->notebook), scrolled_window));
				}
			} else {
				unsigned char *last_separator = strrchr(file_name, '/');
				unsigned char *file_only_name = last_separator + 1;
				FILE *file = fopen(file_name, "r");
				fseek(file, 0, SEEK_END);
				int size = ftell(file);
				unsigned char *text = (unsigned char *)malloc(sizeof(unsigned char) * size + 1);
				memset(text, 0, sizeof(unsigned char) * size + 1);
				fseek(file, 0, SEEK_SET);
				fread(text, sizeof(unsigned char), size, file);
			
				struct cbuffer_ref *buffer_ref = create_page(window_handler, file_name, FALSE, text, window_handler->preferences);
				gtk_source_buffer_begin_not_undoable_action(gtk_text_view_get_buffer(buffer_ref->source_view));
				
				gtk_label_set_text(buffer_ref->label, file_only_name);
				gint index = gtk_notebook_append_page(GTK_NOTEBOOK(window_handler->notebook),
					buffer_ref->scrolled_window, buffer_ref->tab);
				gtk_widget_show_all(buffer_ref->tab);
				buffer_ref->file_name = g_string_assign(buffer_ref->file_name, file_name);
				gchar *separator = strrchr(buffer_ref->file_name->str, '/');
				gtk_label_set_text(buffer_ref->label, ++separator);
			
				gtk_widget_show_all(buffer_ref->scrolled_window);
				gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(window_handler->notebook), buffer_ref->scrolled_window, TRUE);
				fclose(file);
			
				gtk_notebook_set_current_page(GTK_NOTEBOOK(window_handler->notebook), index);
				update_page_language(window_handler, buffer_ref);
				update_styles(window_handler);
				update_providers(window_handler);
				//update_completion_providers(window_handler);
				
				gtk_text_buffer_set_modified(gtk_text_view_get_buffer(buffer_ref->source_view), FALSE);
				gtk_source_buffer_end_not_undoable_action(gtk_text_view_get_buffer(buffer_ref->source_view));
				gtk_text_buffer_set_modified(gtk_text_view_get_buffer(buffer_ref->source_view), FALSE);
			}
			iterator = g_slist_next(iterator);
		}
	}
	gtk_widget_destroy(file_chooser);
}

static void accel_open(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_open_activate(NULL, user_data);
	}
}

static void menu_item_save_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.save\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	lua_State* lua = window_handler->application_handler->lua;
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
	if (scrolled_window) {
		struct cbuffer_ref *buffer_ref = NULL;
		buffer_ref = g_object_get_data(scrolled_window, "buf_ref");
		if (buffer_ref) {
			if (g_file_test(buffer_ref->file_name->str, G_FILE_TEST_IS_REGULAR)) {
				gchar *file_name = buffer_ref->file_name->str;
				FILE *file = fopen(file_name, "w");
				
				GtkSourceBuffer *source_buffer = gtk_text_view_get_buffer(buffer_ref->source_view);
				GtkTextIter start;
				GtkTextIter end;
				gtk_text_buffer_get_start_iter(source_buffer, &start);
				gtk_text_buffer_get_end_iter(source_buffer, &end);
				gchar *text = gtk_text_buffer_get_text(source_buffer, &start, &end, FALSE);
				
				fwrite(text, sizeof(unsigned char), strlen(text), file);
				g_free(text);
				fclose(file);
				
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
			} else {
				GtkWidget *file_chooser = gtk_file_chooser_dialog_new("Save",
					NULL,
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					NULL);
				gtk_file_chooser_set_show_hidden(file_chooser, TRUE);
				gtk_file_chooser_set_current_folder(file_chooser, window_handler->preferences->last_path->str);
				if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
					gchar *file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
					FILE *file = fopen(file_name, "w");
					
					GtkSourceBuffer *source_buffer = gtk_text_view_get_buffer(buffer_ref->source_view);
					GtkTextIter start;
					GtkTextIter end;
					gtk_text_buffer_get_start_iter(source_buffer, &start);
					gtk_text_buffer_get_end_iter(source_buffer, &end);
					gchar *text = gtk_text_buffer_get_text(source_buffer, &start, &end, FALSE);
					
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
				gtk_label_set_text(buffer_ref->label, ++separator);
			}
			GString *title = g_string_new("Love Text");
			if (buffer_ref->file_name->len > 0) {
				title = g_string_append(title, " - ");
				title = g_string_append(title, buffer_ref->file_name->str);
			}
			gtk_window_set_title(window_handler->window, title->str);
			g_string_free(title, TRUE);
			gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_NORMAL, TRUE);
			gtk_text_buffer_set_modified(gtk_text_view_get_buffer(buffer_ref->source_view), FALSE);
		}
	}
}

static void accel_save(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_save_activate(NULL, user_data);
	}
}

static void menu_item_save_as_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.save_as\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	lua_State* lua = window_handler->application_handler->lua;
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
	if (scrolled_window) {
		struct cbuffer_ref *buffer_ref = NULL;
		buffer_ref = g_object_get_data(scrolled_window, "buf_ref");
		if (buffer_ref) {
			GtkWidget *file_chooser = gtk_file_chooser_dialog_new("Save as",
				NULL,
				GTK_FILE_CHOOSER_ACTION_SAVE,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				NULL);
			gtk_file_chooser_set_show_hidden(file_chooser, TRUE);
			gtk_file_chooser_set_current_folder(file_chooser, window_handler->preferences->last_path->str);
			if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
				gchar *file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
				FILE *file = fopen(file_name, "w");
				
				GtkSourceBuffer *source_buffer = gtk_text_view_get_buffer(buffer_ref->source_view);
				GtkTextIter start;
				GtkTextIter end;
				gtk_text_buffer_get_start_iter(source_buffer, &start);
				gtk_text_buffer_get_end_iter(source_buffer, &end);
				gchar *text = gtk_text_buffer_get_text(source_buffer, &start, &end, FALSE);
				
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
			
			gchar *separator = strrchr(buffer_ref->file_name->str, '/');
			if (separator) {
				gtk_label_set_text(buffer_ref->label, ++separator);
			}
			GString *title = g_string_new("Love Text");
			if (buffer_ref->file_name->len > 0) {
				title = g_string_append(title, " - ");
				title = g_string_append(title, buffer_ref->file_name->str);
			}
			gtk_window_set_title(window_handler->window, title->str);
			g_string_free(title, TRUE);
			gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_NORMAL, TRUE);
			gtk_text_buffer_set_modified(gtk_text_view_get_buffer(buffer_ref->source_view), FALSE);
		}
	}
}

static void accel_save_as(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_save_as_activate(NULL, user_data);
	}
}

static void menu_item_preferences_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.preferences\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	
	// Preferences window.
	window_handler->window_preferences_handler = alloc_window_preferences_handler(window_handler->preferences);
	if (window_handler->window_preferences_handler) {
		// Circular reference.
		window_handler->window_preferences_handler->window_handler = window_handler;
		window_handler->window_preferences_handler->update_editor = update_editor;
		
	}
	gtk_widget_show_all(window_handler->window_preferences_handler->window);
	gtk_window_set_transient_for(window_handler->window_preferences_handler->window, window_handler->window);
}

static void accel_preferences(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_preferences_activate(NULL, user_data);
	}
}

static void menu_item_close_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.close\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
	
	if (scrolled_window) {
		GtkWidget *source_view = gtk_container_get_focus_child(scrolled_window);
		gtk_notebook_remove_page(GTK_NOTEBOOK(window_handler->notebook),
			gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
		if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(window_handler->notebook)) <= 0) {
			gtk_window_set_title(window_handler->window, "Love Text");
		}
	}
}

static void accel_close(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_close_activate(NULL, user_data);
	}
}

static void menu_item_exit_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.exit\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gtk_widget_destroy(window_handler->window);
}

static void accel_exit(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_exit_activate(NULL, user_data);
	}
}

static void menu_item_undo_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.undo\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(window_handler->notebook);
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(window_handler->notebook, current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(widget_page, "buf_ref");
			if (buffer_ref) {
				gtk_source_buffer_undo(gtk_text_view_get_buffer(buffer_ref->source_view));
				if (gtk_source_buffer_can_undo(gtk_text_view_get_buffer(buffer_ref->source_view))) {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_ACTIVE, TRUE);
				} else {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_NORMAL, TRUE);
				}
			}
		}
	}
}

static void accel_undo(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_undo_activate(NULL, user_data);
	}
}

static void menu_item_redo_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.redo\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(window_handler->notebook);
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(window_handler->notebook, current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(widget_page, "buf_ref");
			if (buffer_ref) {
				gtk_source_buffer_redo(gtk_text_view_get_buffer(buffer_ref->source_view));
				if (gtk_source_buffer_can_undo(gtk_text_view_get_buffer(buffer_ref->source_view))) {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_ACTIVE, TRUE);
				} else {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_NORMAL, TRUE);
				}
			}
		}
	}
}

static void accel_redo(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_redo_activate(NULL, user_data);
	}
}

static void menu_item_copy_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.copy\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(window_handler->notebook);
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(window_handler->notebook, current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(widget_page, "buf_ref");
			if (buffer_ref) {
				if (window_handler->clipboard) {
					gtk_text_buffer_copy_clipboard(gtk_text_view_get_buffer(buffer_ref->source_view), window_handler->clipboard);
				}
			}
		}
	}
}

static void menu_item_paste_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.paste\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(window_handler->notebook);
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(window_handler->notebook, current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(widget_page, "buf_ref");
			if (buffer_ref) {
				if (window_handler->clipboard) {
					gtk_text_buffer_paste_clipboard(gtk_text_view_get_buffer(buffer_ref->source_view), window_handler->clipboard, NULL, TRUE);
				}
			}
		}
	}
}

static void menu_item_cut_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.cut\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(window_handler->notebook);
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(window_handler->notebook, current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(widget_page, "buf_ref");
			if (buffer_ref) {
				if (window_handler->clipboard) {
					gtk_text_buffer_cut_clipboard(gtk_text_view_get_buffer(buffer_ref->source_view), window_handler->clipboard, TRUE);
				}
			}
		}
	}
}

static void accel_cut(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_cut_activate(NULL, user_data);
	}
}

static void menu_item_delete_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.delete\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gint current_page = gtk_notebook_get_current_page(window_handler->notebook);
	if (current_page > -1) {
		GtkWidget *widget_page = gtk_notebook_get_nth_page(window_handler->notebook, current_page);
		struct cbuffer_ref *buffer_ref = NULL;
		if (widget_page) {
			buffer_ref = g_object_get_data(widget_page, "buf_ref");
			if (buffer_ref) {
				if (window_handler->clipboard) {
					if (gtk_text_buffer_delete_selection(gtk_text_view_get_buffer(buffer_ref->source_view), TRUE, TRUE)) {
						g_printf("[MESSAGE] Text deleted.\n");
					}
				}
			}
		}
	}
}

static void accel_delete(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_delete_activate(NULL, user_data);
	}
}

static void menu_item_next_page_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.next_page\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	if (gtk_notebook_get_current_page(window_handler->notebook) == 
	gtk_notebook_get_n_pages(window_handler->notebook) - 1) {
		gtk_notebook_set_current_page(window_handler->notebook, 0);
	} else {
		gtk_notebook_next_page(window_handler->notebook);
	}
}

static void accel_next_page(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		g_printf("[MESSAGE] Activate \"next_page\" accelerator.\n");
		menu_item_next_page_activate(NULL, user_data);
	}
}

static void menu_item_previous_page_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.previous_page\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	if (gtk_notebook_get_current_page(window_handler->notebook) == 0) {
		gtk_notebook_set_current_page(window_handler->notebook,
			gtk_notebook_get_n_pages(window_handler->notebook) - 1);
	} else {
		gtk_notebook_prev_page(window_handler->notebook);
	}
}

static void accel_previous_page(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		g_printf("[MESSAGE] Activate \"previous_page\" accelerator.\n");
		menu_item_previous_page_activate(NULL, user_data);
	}
}

static void menu_item_toggle_menu_bar_activate(GtkWidget *widget, gpointer user_data)
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
		menu_item_toggle_menu_bar_activate(NULL, user_data);
	}
}

static void menu_item_toggle_status_bar_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.toggle_status_bar\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	if (gtk_widget_get_visible(window_handler->status_bar)) {
		gtk_widget_hide(window_handler->status_bar);
	} else {
		gtk_widget_show(window_handler->status_bar);
	}
	window_handler->preferences->show_status_bar = gtk_widget_get_visible(window_handler->status_bar);
}

static void accel_toggle_status_bar(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_toggle_status_bar_activate(NULL, user_data);
	}
}

static void menu_item_toggle_fullscreen_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.toggle_fullscreen\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	if (window_handler->window_fullscreen) {
		gtk_window_unfullscreen(window_handler->window);
		window_handler->window_fullscreen = FALSE;
	} else {
		gtk_window_fullscreen(window_handler->window);
		window_handler->window_fullscreen = TRUE;
	}
}

static void accel_toggle_fullscreen(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_toggle_fullscreen_activate(NULL, user_data);
	}
}

static void menu_item_search_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.search\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gtk_widget_set_visible(window_handler->search_and_replace_bar, TRUE);
	gtk_widget_set_visible(window_handler->replace_bar, FALSE);
	
	if (gtk_widget_get_visible(window_handler->search_and_replace_bar)) {
		gtk_widget_grab_focus(window_handler->search_entry);
		gint current_page = gtk_notebook_get_current_page(window_handler->notebook);
		if (current_page > -1) {
			GtkWidget *widget_page = gtk_notebook_get_nth_page(window_handler->notebook, current_page);
			struct cbuffer_ref *buffer_ref = NULL;
			buffer_ref = g_object_get_data(widget_page, "buf_ref");
			if (buffer_ref) {
				GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "insert");
				gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_end, cursor);
				buffer_ref->current_search_start = buffer_ref->current_search_end;
			}
		}
	} else {
		gint current_page = gtk_notebook_get_current_page(window_handler->notebook);
		if (current_page > -1) {
			GtkWidget *widget_page = gtk_notebook_get_nth_page(window_handler->notebook, current_page);
			struct cbuffer_ref *buffer_ref = NULL;
			buffer_ref = g_object_get_data(widget_page, "buf_ref");
			if (buffer_ref) {
				gtk_widget_grab_focus(buffer_ref->source_view);
			}
		}
	}
}

static void accel_search(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_search_activate(NULL, user_data);
	}
}

static void menu_item_replace_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.replace\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gtk_widget_set_visible(window_handler->search_and_replace_bar, TRUE);
	gtk_widget_set_visible(window_handler->replace_bar, TRUE);
	
	if (gtk_widget_get_visible(window_handler->search_and_replace_bar)) {
		gtk_widget_grab_focus(window_handler->search_entry);
	} else {
		gint current_page = gtk_notebook_get_current_page(window_handler->notebook);
		if (current_page > -1) {
			GtkWidget *widget_page = gtk_notebook_get_nth_page(window_handler->notebook, current_page);
			struct cbuffer_ref *buffer_ref = NULL;
			buffer_ref = g_object_get_data(widget_page, "buf_ref");
			if (buffer_ref) {
				gtk_widget_grab_focus(buffer_ref->source_view);
			}
		}
	}
}

static void accel_replace(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_replace_activate(NULL, user_data);
	}
}

static void menu_item_refresh_plugins_activate(GtkWidget *widget, gpointer user_data)
{
	g_printf("[MESSAGE] Performing action \"window.refresh_plugins\".\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	lua_State* lua = window_handler->application_handler->lua;
	// Call the initialization function.
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
	if (lua) {
		lua_close(lua);
		lua = NULL;
	}
	lua = luaL_newstate();
	initialize_lua(window_handler, window_handler->preferences);
}

static void accel_refresh_plugins(GtkAccelGroup *accel_group, GObject *acceleratable, guint keyval, GdkModifierType modifier, gpointer user_data)
{
	if (modifier | GDK_RELEASE_MASK) {
		menu_item_refresh_plugins_activate(NULL, user_data);
	}
}

static void menu_item_about_activate(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	g_printf("[MESSAGE] Performing action \"window.about\".\n");
	GtkWidget *window_about = gtk_about_dialog_new();
	
	gtk_about_dialog_set_program_name(window_about, "Love Text");
	gtk_about_dialog_set_version(window_about, "0.8");
	gtk_about_dialog_set_copyright(window_about, "Copyright © 2015 by Felipe Ferreira da Silva");
	gtk_about_dialog_set_comments(window_about, "Love Text is a simple, lightweight and extensible text editor.");
	gtk_about_dialog_set_license_type(window_about, GTK_LICENSE_MIT_X11);
	gtk_about_dialog_set_license(window_about, "The MIT License (MIT)\n\
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
	gtk_about_dialog_set_wrap_license(window_about, TRUE);
	gtk_window_set_icon_name(GTK_WINDOW(window_about), "lovetext");
	gtk_about_dialog_set_logo_icon_name(window_about, "lovetext");
	
	unsigned char *authors[2] = {"Felipe Ferreira da Silva (SILVA, F. F. da) <felipefsdev@gmail.com>", NULL};
	gtk_about_dialog_set_authors(window_about, &authors);
	
	gtk_window_set_transient_for(window_about, window_handler->window);
	gtk_dialog_run(window_about);
	gtk_widget_destroy(window_about);
}

static gboolean entry_search_key_press_event(GtkWidget *widget,  GdkEventKey *event, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	
	if (event->keyval == GDK_KEY_Escape) {
		gtk_widget_set_visible(window_handler->search_and_replace_bar, FALSE);
		gint current_page = gtk_notebook_get_current_page(window_handler->notebook);
		if (current_page > -1) {
			GtkWidget *widget_page = gtk_notebook_get_nth_page(window_handler->notebook, current_page);
			struct cbuffer_ref *buffer_ref = NULL;
			buffer_ref = g_object_get_data(widget_page, "buf_ref");
			if (buffer_ref) {
				gtk_widget_grab_focus(buffer_ref->source_view);
			}
		}
		return TRUE;
	}
	
	gint pages_count = gtk_notebook_get_n_pages(window_handler->notebook);
	gint i = 0;
	GtkWidget *widget_page = NULL;
	struct cbuffer_ref *buffer_ref = NULL;
	for (i = 0; i < pages_count; i++) {
		widget_page = gtk_notebook_get_nth_page(window_handler->notebook, i);
		buffer_ref = g_object_get_data(widget_page, "buf_ref");
		if (buffer_ref) {
			gtk_source_search_settings_set_search_text(buffer_ref->search_settings, gtk_entry_get_text(widget));
		}
	}
	
	if (event->keyval == GDK_KEY_Down) {
		g_printf("[MESSAGE] Going to next search result.\n");
		struct cbuffer_ref *buffer_ref = NULL;
		GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
			gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
		if (scrolled_window) {
			buffer_ref = g_object_get_data(scrolled_window, "buf_ref");
		}
	
		if (buffer_ref) {
			GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "insert");
			gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_start, cursor);
			GtkTextMark *cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "selection_bound");
			gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_end, cursor_end);
			
			gtk_source_search_context_forward(buffer_ref->search_context, &buffer_ref->current_search_end,
				&buffer_ref->current_search_start,
				&buffer_ref->current_search_end);
			int l = gtk_text_iter_get_line(&buffer_ref->current_search_start);
			gtk_text_view_scroll_to_iter(buffer_ref->source_view,
				&buffer_ref->current_search_start,
				0.0,
				TRUE,
				0.5,
				0.5);
			gtk_text_buffer_select_range(gtk_text_view_get_buffer(buffer_ref->source_view),
				&buffer_ref->current_search_start,
				&buffer_ref->current_search_end);
		}
		return TRUE;
	}
	
	if (event->keyval == GDK_KEY_Up) {
		g_printf("[MESSAGE] Going to previous search result.\n");
		struct cbuffer_ref *buffer_ref = NULL;
		GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
			gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
		if (scrolled_window) {
			buffer_ref = g_object_get_data(scrolled_window, "buf_ref");
		}
	
		if (buffer_ref) {
			GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "insert");
			gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_start, cursor);
			GtkTextMark *cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "selection_bound");
			gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_end, cursor_end);
			
			gtk_source_search_context_backward(buffer_ref->search_context, &buffer_ref->current_search_start,
				&buffer_ref->current_search_start,
				&buffer_ref->current_search_end);
			int l = gtk_text_iter_get_line(&buffer_ref->current_search_start);
			gtk_text_view_scroll_to_iter(buffer_ref->source_view,
				&buffer_ref->current_search_start,
				0.0,
				TRUE,
				0.5,
				0.5);
			gtk_text_buffer_select_range(gtk_text_view_get_buffer(buffer_ref->source_view),
				&buffer_ref->current_search_start,
				&buffer_ref->current_search_end);
		}
		return TRUE;
	}
	return FALSE;
}

static gboolean entry_search_key_release_event(GtkWidget *widget,  GdkEventKey *event, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	
	gint pages_count = gtk_notebook_get_n_pages(window_handler->notebook);
	gint i = 0;
	GtkWidget *widget_page = NULL;
	struct cbuffer_ref *buffer_ref = NULL;
	for (i = 0; i < pages_count; i++) {
		widget_page = gtk_notebook_get_nth_page(window_handler->notebook, i);
		buffer_ref = g_object_get_data(widget_page, "buf_ref");
		if (buffer_ref) {
			gtk_source_search_settings_set_search_text(buffer_ref->search_settings, gtk_entry_get_text(widget));
		}
	}
	return FALSE;
}

static gboolean source_view_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gboolean handled = FALSE;
	
	g_printf("[MESSAGE] Source View key press.\n");
	struct cbuffer_ref *buffer_ref = g_object_get_data(widget, "buf_ref");
	
	if (buffer_ref) {
		gtk_source_search_settings_set_search_text(buffer_ref->search_settings, gtk_entry_get_text(window_handler->search_entry));
	}
	
	if (event->state & GDK_CONTROL_MASK) {
		if (event->keyval == GDK_KEY_Page_Down) {
			menu_item_next_page_activate(NULL, user_data);
			handled = TRUE;
		} else if (event->keyval == GDK_KEY_Page_Up) {
			menu_item_previous_page_activate(NULL, user_data);
			handled = TRUE;
		}
	}
	return FALSE;
}

static gboolean source_view_key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	g_printf("[MESSAGE] Source View key release.\n");
	GtkSourceBuffer *buffer = gtk_text_view_get_buffer(widget);
	struct cbuffer_ref *buffer_ref = g_object_get_data(widget, "buf_ref");
	
	if (buffer) {
		GtkWidget *label = buffer_ref->label;
		if (label) {
			if (buffer_ref->file_name) {
				if (gtk_source_buffer_can_undo(gtk_text_view_get_buffer(buffer_ref->source_view)) &&
				gtk_text_buffer_get_modified(gtk_text_view_get_buffer(buffer_ref->source_view))) {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_ACTIVE, TRUE);
				} else {
					gtk_widget_set_state_flags(buffer_ref->label, GTK_STATE_FLAG_NORMAL, TRUE);
				}
			}
		}
	}
	return FALSE;
}

static gboolean window_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	return FALSE;
}

static gboolean window_key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	return FALSE;
}

static void button_close_tab_clicked(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	struct cbuffer_ref *buffer_ref = g_object_get_data(widget, "buf_ref");
	lua_State* lua = window_handler->application_handler->lua;
	
	if (buffer_ref->scrolled_window) {
		GtkWidget *source_view = gtk_container_get_focus_child(buffer_ref->scrolled_window);
		gtk_notebook_remove_page(GTK_NOTEBOOK(window_handler->notebook),
			gtk_notebook_page_num(window_handler->notebook, buffer_ref->scrolled_window));
		//gtk_widget_destroy(source_view);
		//gtk_widget_destroy(scrolled_window);
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
}

struct cbuffer_ref *create_page(struct cwindow_handler *window_handler, gchar *file_name, gboolean modified, gchar *text, struct cpreferences *preferences)
{
	lua_State* lua = window_handler->application_handler->lua;
	struct cbuffer_ref *buffer_ref = (struct cbuffer_ref *)malloc(sizeof(struct cbuffer_ref));
	buffer_ref->file_name = g_string_new(file_name);
	buffer_ref->modified = modified;
	buffer_ref->source_view = gtk_source_view_new();
	gtk_widget_grab_focus(buffer_ref->source_view);
	gtk_source_view_set_show_line_marks(buffer_ref->source_view, preferences->show_line_marks);
	gtk_source_view_set_show_line_numbers(buffer_ref->source_view, preferences->show_line_numbers);
	gtk_source_view_set_show_right_margin(buffer_ref->source_view, preferences->show_right_margin);
	gtk_source_view_set_tab_width(buffer_ref->source_view, preferences->tab_width);
	gtk_source_view_set_auto_indent(buffer_ref->source_view, preferences->auto_indent);
	gtk_source_buffer_set_highlight_matching_brackets(gtk_text_view_get_buffer(buffer_ref->source_view), preferences->highlight_matching_brackets);
	gtk_source_view_set_highlight_current_line(buffer_ref->source_view, preferences->highlight_current_line);
	
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(buffer_ref->source_view), text, -1);
	PangoFontDescription *font_description = pango_font_description_from_string(preferences->editor_font->str);
	gtk_widget_modify_font(buffer_ref->source_view, font_description);
	
	GtkSourceCompletion *completion = gtk_source_view_get_completion(buffer_ref->source_view);
	if (completion) {
		gchar *separator = strrchr(buffer_ref->file_name->str, '/');
		if (separator) {
			separator++;
		} else {
			separator = buffer_ref->file_name;
		}
		GtkSourceCompletionWords *provider_words = gtk_source_completion_words_new(separator, NULL);
		buffer_ref->provider_words = provider_words;
		g_object_set(provider_words, "interactive-delay", 0, NULL);
		gtk_source_completion_words_register(provider_words, gtk_text_view_get_buffer(buffer_ref->source_view));
		gtk_source_completion_add_provider(completion, provider_words, NULL);
	}
	
	buffer_ref->search_context = gtk_source_search_context_new(gtk_text_view_get_buffer(buffer_ref->source_view), NULL);
	buffer_ref->search_settings = gtk_source_search_context_get_settings(buffer_ref->search_context);
	
	buffer_ref->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(buffer_ref->scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	GtkAdjustment *hadjustment = gtk_scrolled_window_get_hadjustment(buffer_ref->scrolled_window);
	GtkAdjustment *vadjustment = gtk_scrolled_window_get_vadjustment(buffer_ref->scrolled_window);
	gtk_adjustment_set_page_increment(hadjustment, 0.1);
	gtk_adjustment_set_step_increment(hadjustment, 0.1);
	gtk_adjustment_set_page_increment(vadjustment, 0.1);
	gtk_adjustment_set_step_increment(vadjustment, 0.1);
	gtk_container_add(buffer_ref->scrolled_window, buffer_ref->source_view);
	
	//g_signal_connect_after(buffer_ref->source_view, "draw", G_CALLBACK(source_view_draw), window_handler);
	g_signal_connect(buffer_ref->source_view, "key-press-event", G_CALLBACK(source_view_key_press_event), window_handler);
	g_signal_connect(buffer_ref->source_view, "key-release-event", G_CALLBACK(source_view_key_release_event), window_handler);
	
	g_object_set_data(buffer_ref->source_view, "buf_ref", buffer_ref);
	g_object_set_data(buffer_ref->scrolled_window, "buf_ref", buffer_ref);
	
	gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_end);
	gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_start);
	
	GtkWidget *tab = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	GtkWidget *label = gtk_label_new("Untitled");
	
	gchar *separator = strrchr(buffer_ref->file_name->str, '/');
	if (separator) {
		gtk_label_set_text(label, ++separator);
	}
	GtkWidget *button_close = gtk_button_new_from_icon_name("gtk-close", GTK_ICON_SIZE_BUTTON);
	g_object_set_data(button_close, "buf_ref", buffer_ref);
	g_signal_connect(button_close, "clicked", G_CALLBACK(button_close_tab_clicked), window_handler);
	
	gtk_box_pack_start(tab, label, TRUE, TRUE, 0);
	gtk_box_pack_start(tab, button_close, FALSE, TRUE, 0);
	
	buffer_ref->tab = tab;
	buffer_ref->label = label;
	
	gtk_widget_set_can_focus(tab, FALSE);
	gtk_widget_set_can_focus(label, FALSE);
	gtk_widget_set_can_focus(button_close, FALSE);
	gtk_button_set_focus_on_click(button_close, FALSE);
	
	GdkRGBA rgba;
	rgba.red = 1.0;
	rgba.green = 0.0;
	rgba.blue = 0.0;
	rgba.alpha = 1.0;
	gtk_widget_override_color(label, GTK_STATE_FLAG_ACTIVE, &rgba);
	
	lua_getglobal(lua, "editor");
	if (lua_istable(lua, -1)) {
		lua_pushstring(lua, "f_new");
		lua_gettable(lua, -2);
		if (lua_isfunction(lua, -1)) {
			//window_handler->id_factory = window_handler->id_factory + 1;
			//lua_pushinteger(lua, window_handler->id_factory);
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
	return buffer_ref;
}

static void button_previous_clicked(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	struct cbuffer_ref *buffer_ref = NULL;
	GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window_handler->notebook),
		gtk_notebook_get_current_page(GTK_NOTEBOOK(window_handler->notebook)));
	if (scrolled_window) {
		buffer_ref = g_object_get_data(scrolled_window, "buf_ref");
	}
	
	if (buffer_ref) {
		GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "insert");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_start, cursor);
		GtkTextMark *cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "selection_bound");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_end, cursor_end);
		
		gtk_source_search_context_backward(buffer_ref->search_context, &buffer_ref->current_search_start,
			&buffer_ref->current_search_start,
			&buffer_ref->current_search_end);
		int l = gtk_text_iter_get_line(&buffer_ref->current_search_start);
		gtk_text_view_scroll_to_iter(buffer_ref->source_view,
			&buffer_ref->current_search_start,
			0.0,
			TRUE,
			0.5,
			0.5);
		gtk_text_buffer_select_range(gtk_text_view_get_buffer(buffer_ref->source_view),
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
		buffer_ref = g_object_get_data(scrolled_window, "buf_ref");
	}

	if (buffer_ref) {
		GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "insert");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_start, cursor);
		GtkTextMark *cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "selection_bound");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_end, cursor_end);
		
		gtk_source_search_context_forward(buffer_ref->search_context, &buffer_ref->current_search_end,
			&buffer_ref->current_search_start,
			&buffer_ref->current_search_end);
		int l = gtk_text_iter_get_line(&buffer_ref->current_search_start);
		gtk_text_view_scroll_to_iter(buffer_ref->source_view,
			&buffer_ref->current_search_start,
			0.0,
			TRUE,
			0.5,
			0.5);
		gtk_text_buffer_select_range(gtk_text_view_get_buffer(buffer_ref->source_view),
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
		buffer_ref = g_object_get_data(scrolled_window, "buf_ref");
	}

	if (buffer_ref) {
		GtkTextMark *cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "insert");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_start, cursor);
		GtkTextMark *cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "selection_bound");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_end, cursor_end);
		
		gtk_source_search_context_replace(buffer_ref->search_context,
			&buffer_ref->current_search_start,
			&buffer_ref->current_search_end,
			gtk_entry_get_text(window_handler->replace_with_entry),
			-1,
			NULL);
		
		cursor = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "insert");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_start, cursor);
		cursor_end = gtk_text_buffer_get_mark(gtk_text_view_get_buffer(buffer_ref->source_view), "selection_bound");
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(buffer_ref->source_view), &buffer_ref->current_search_end, cursor_end);
		
		gtk_source_search_context_forward(buffer_ref->search_context, &buffer_ref->current_search_end,
			&buffer_ref->current_search_start,
			&buffer_ref->current_search_end);
		int l = gtk_text_iter_get_line(&buffer_ref->current_search_start);
		gtk_text_view_scroll_to_iter(buffer_ref->source_view,
			&buffer_ref->current_search_start,
			0.0,
			TRUE,
			0.5,
			0.5);
		gtk_text_buffer_select_range(gtk_text_view_get_buffer(buffer_ref->source_view),
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
		buffer_ref = g_object_get_data(scrolled_window, "buf_ref");
	}
	
	if (buffer_ref) {
		gtk_source_search_context_replace_all(buffer_ref->search_context,
			gtk_entry_get_text(window_handler->replace_with_entry),
			-1,
			NULL);
	}
}

static void window_destroy(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	struct cpreferences *preferences = window_handler->preferences;
	g_printf("[MESSAGE] Closing main window.\n");
	
	gchar *scheme_id = gtk_source_style_scheme_get_id(preferences->scheme);
	g_key_file_set_string(preferences->configuration_file,
		"editor",
		"scheme_id",
		scheme_id);
	
	g_key_file_set_string(preferences->configuration_file,
		"editor",
		"font",
		preferences->editor_font->str);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"general",
		"start_new_page",
		preferences->start_new_page);
	
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
		"show_status_bar",
		preferences->show_status_bar);
	
	g_key_file_set_string(preferences->configuration_file,
		"general",
		"gtk_theme",
		preferences->gtk_theme);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"general",
		"use_custom_gtk_theme",
		preferences->use_custom_gtk_theme);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"show_line_numbers",
		preferences->show_line_numbers);
	
	g_key_file_set_boolean(preferences->configuration_file,
		"editor",
		"show_right_margin",
		preferences->show_right_margin);
	
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
	GString *title = g_string_new("Love Text");
	
	if (scrolled_window) {
		struct cbuffer_ref *buffer_ref = NULL;
		buffer_ref = g_object_get_data(scrolled_window, "buf_ref");
		
		window_handler->preferences->last_path = g_string_assign(window_handler->preferences->last_path, buffer_ref->file_name->str);
		
		char *i = strrchr(window_handler->preferences->last_path->str, '/');
		if (i) {
			window_handler->preferences->last_path = g_string_set_size(window_handler->preferences->last_path, i - window_handler->preferences->last_path->str);
		}
		//g_printf("PATH %i TO \"%s\"\n\n\n", page_num, window_handler->preferences->last_path->str);
		
		if (buffer_ref->file_name->len > 0) {
			title = g_string_append(title, " - ");
			title = g_string_append(title, buffer_ref->file_name->str);
		}
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
	gtk_window_set_title(window_handler->window, title->str);
	g_string_free(title, TRUE);
}

static void window_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint type, guint time, gpointer user_data)
{
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
	gchar *sel_data = gtk_selection_data_get_data(selection_data);
	
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
	
	unsigned char *last_separator = strrchr(file_name, '/');
	unsigned char *file_only_name = last_separator + 1;
	FILE *file = fopen(file_name, "r");
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	unsigned char *text = (unsigned char *)malloc(sizeof(unsigned char) * size + 1);
	memset(text, 0, sizeof(unsigned char) * size + 1);
	fseek(file, 0, SEEK_SET);
	fread(text, sizeof(unsigned char), size, file);
	
	struct cbuffer_ref *buffer_ref = create_page(window_handler, file_name, FALSE, text, window_handler->preferences);
	gtk_source_buffer_begin_not_undoable_action(gtk_text_view_get_buffer(buffer_ref->source_view));
	
	gint index = gtk_notebook_append_page(GTK_NOTEBOOK(window_handler->notebook),
		buffer_ref->scrolled_window, buffer_ref->tab);
	gtk_widget_show_all(buffer_ref->tab);
	
	gtk_widget_show_all(buffer_ref->scrolled_window);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(window_handler->notebook), buffer_ref->scrolled_window, TRUE);
	fclose(file);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(window_handler->notebook), index);
	update_page_language(window_handler, buffer_ref);
	update_styles(window_handler);
	update_providers(window_handler);
	//update_completion_providers(window_handler);
	
	gtk_text_buffer_set_modified(gtk_text_view_get_buffer(buffer_ref->source_view), FALSE);
	gtk_source_buffer_end_not_undoable_action(gtk_text_view_get_buffer(buffer_ref->source_view));
	gtk_text_buffer_set_modified(gtk_text_view_get_buffer(buffer_ref->source_view), FALSE);
	
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
	struct cwindow_handler *window_handler = (struct cwindow_handler *)user_data;
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

static GtkWidget *create_menu_bar(struct cwindow_handler *window_handler)
{
	// Accelerator group.
	GtkAccelGroup *accelerator_group = gtk_accel_group_new();
	window_handler->accelerator_group = accelerator_group;
	gtk_window_add_accel_group(window_handler->window, accelerator_group);
	
	// Accelerators.
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
	
	// Accelerators.
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_M,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_toggle_menu_bar), window_handler, NULL));
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_F11,
		0,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_toggle_fullscreen), window_handler, NULL));
	
	// Accelerators.
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_F,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_search), window_handler, NULL));
	gtk_accel_group_connect(accelerator_group,
		GDK_KEY_H,
		GDK_CONTROL_MASK,
		GTK_ACCEL_VISIBLE,
		g_cclosure_new(G_CALLBACK(accel_replace), window_handler, NULL));
	
	// Menu bar.
	GtkWidget *menu_bar = gtk_menu_bar_new();
	GtkWidget *menu = NULL;
	GtkWidget *menu_item = NULL;
	GtkWidget *menu_label = NULL;
	
	menu = gtk_menu_new();
	menu_item = gtk_menu_item_new_with_label("Main");
	gtk_menu_item_set_submenu(menu_item, menu);
	gtk_menu_shell_append(menu_bar, menu_item);
	gtk_menu_set_accel_group(menu, accelerator_group);
	
	menu_item = gtk_menu_item_new_with_label("New");
	//gtk_widget_add_accelerator(menu_item, NULL, accelerator_group, 
	//	GDK_KEY_N, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_N, GDK_CONTROL_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_new_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Open");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_O, GDK_CONTROL_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_open_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Save");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_S, GDK_CONTROL_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_save_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Save As");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_S, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_save_as_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Preferences");
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_preferences_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Close");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_W, GDK_CONTROL_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_close_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Exit");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_F4, GDK_MOD1_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_exit_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	// Menu Edit.
	menu = gtk_menu_new();
	menu_item = gtk_menu_item_new_with_label("Edit");
	gtk_menu_item_set_submenu(menu_item, menu);
	gtk_menu_shell_append(menu_bar, menu_item);
	gtk_menu_set_accel_group(menu, accelerator_group);
	
	menu_item = gtk_menu_item_new_with_label("Undo");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_Z, GDK_CONTROL_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_undo_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Redo");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_Z, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_redo_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Paste");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_V, GDK_CONTROL_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_paste_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Cut");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_X, GDK_CONTROL_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_cut_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Delete");
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_delete_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	// Menu View.
	menu = gtk_menu_new();
	menu_item = gtk_menu_item_new_with_label("View");
	gtk_menu_item_set_submenu(menu_item, menu);
	gtk_menu_shell_append(menu_bar, menu_item);
	gtk_menu_set_accel_group(menu, accelerator_group);
	
	menu_item = gtk_menu_item_new_with_label("Next Page");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_Page_Down, GDK_CONTROL_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_next_page_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Previous Page");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_Page_Up, GDK_CONTROL_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_previous_page_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Toggle Menu Bar");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_M, GDK_CONTROL_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_toggle_menu_bar_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Toggle Status Bar");
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_toggle_status_bar_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	// Menu Tools.
	menu = gtk_menu_new();
	menu_item = gtk_menu_item_new_with_label("Tools");
	gtk_menu_item_set_submenu(menu_item, menu);
	gtk_menu_shell_append(menu_bar, menu_item);
	gtk_menu_set_accel_group(menu, accelerator_group);
	
	menu_item = gtk_menu_item_new_with_label("Search");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_F, GDK_CONTROL_MASK);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_search_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Search Next");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_F3, 0);
	//g_signal_connect(G_OBJECT(menu_item), "activate",
	//	G_CALLBACK(menu_item_search_next_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Search Previous");
	menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
	gtk_accel_label_set_accel(GTK_ACCEL_LABEL(menu_label), GDK_KEY_F3, GDK_SHIFT_MASK);
	//g_signal_connect(G_OBJECT(menu_item), "activate",
	//	G_CALLBACK(menu_item_search_previous_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	menu_item = gtk_menu_item_new_with_label("Refresh Plugins");
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_refresh_plugins_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	
	// Menu Help.
	menu = gtk_menu_new();
	menu_item = gtk_menu_item_new_with_label("Help");
	gtk_menu_item_set_submenu(menu_item, menu);
	gtk_menu_shell_append(menu_bar, menu_item);
	gtk_menu_set_accel_group(menu, accelerator_group);
	
	menu_item = gtk_menu_item_new_with_label("About");
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(menu_item_about_activate), window_handler);
	gtk_menu_shell_append(menu, menu_item);
	return menu_bar;
}

struct cwindow_handler *alloc_window_handler(struct cpreferences *preferences)
{
	g_printf("[MESSAGE] Creating main window.\n");
	struct cwindow_handler *window_handler = (struct cwindow_handler *)malloc(sizeof(struct cwindow_handler));
	window_handler->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	g_signal_connect(window_handler->window, "key-press-event", G_CALLBACK(window_key_press_event), window_handler);
	g_signal_connect(window_handler->window, "key-release-event", G_CALLBACK(window_key_release_event), window_handler);
	
	window_handler->drag_and_drop = FALSE;
	gtk_window_set_icon_name(GTK_WINDOW(window_handler->window), "lovetext");
	gtk_window_set_title(GTK_WINDOW(window_handler->window), "Love Text");
	gtk_widget_set_size_request(GTK_WINDOW(window_handler->window), 480, 380);
	g_signal_connect(window_handler->window, "destroy", G_CALLBACK(window_destroy), window_handler);
	
	GtkWidget *menu_bar = create_menu_bar(window_handler);
	window_handler->menu_bar = menu_bar;
	
	window_handler->clipboard = NULL;
	GdkAtom clipboard_atom = gdk_atom_intern("CLIPBOARD", TRUE);
	if (clipboard_atom) {
		window_handler->clipboard = gtk_clipboard_get(clipboard_atom);
	}
	
	window_handler->window_fullscreen = FALSE;
	window_handler->status_bar = gtk_statusbar_new();
	window_handler->notebook = gtk_notebook_new();
	g_signal_connect(window_handler->notebook, "switch-page", G_CALLBACK(notebook_switch_page), window_handler);
	
	gtk_notebook_set_scrollable(window_handler->notebook, TRUE);
	//gtk_notebook_set_show_border(window_handler->notebook, FALSE);
	window_handler->box = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(window_handler->box), menu_bar, FALSE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(window_handler->box), window_handler->notebook, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(window_handler->box), window_handler->status_bar, FALSE, TRUE, 0);
	
	// Search bar.
	GtkWidget *search_and_replace_bar = gtk_vbox_new(FALSE, 8);
	window_handler->search_and_replace_bar = search_and_replace_bar;
	
	GtkWidget *search_bar = gtk_hbox_new(FALSE, 8);
	window_handler->search_bar = search_bar;
	window_handler->search_entry = gtk_entry_new();
	g_signal_connect(window_handler->search_entry, "key-press-event", G_CALLBACK(entry_search_key_press_event), window_handler);
	g_signal_connect(window_handler->search_entry, "key-release-event", G_CALLBACK(entry_search_key_release_event), window_handler);
	
	GtkWidget *label = gtk_label_new("Search:");
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	
	gtk_box_pack_start(GTK_BOX(search_bar), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(search_bar), window_handler->search_entry, TRUE, TRUE, 0);
	
	GtkWidget *button = gtk_button_new_with_label("Previous");
	gtk_widget_set_size_request(button, 96, -1);
	g_signal_connect(button, "clicked", G_CALLBACK(button_previous_clicked), window_handler);
	gtk_box_pack_start(GTK_BOX(search_bar), button, FALSE, FALSE, 0);
	button = gtk_button_new_with_label("Next");
	gtk_widget_set_size_request(button, 96, -1);
	g_signal_connect(button, "clicked", G_CALLBACK(button_next_clicked), window_handler);
	gtk_box_pack_start(GTK_BOX(search_bar), button, FALSE, FALSE, 0);
	
	// Replace bar
	GtkWidget *replace_bar = gtk_hbox_new(FALSE, 8);
	window_handler->replace_bar = replace_bar;
	window_handler->replace_with_entry = gtk_entry_new();
	//g_signal_connect(window_handler->search_entry, "key-press-event", G_CALLBACK(entry_search_key_press_event), window_handler);
	//g_signal_connect(window_handler->search_entry, "key-release-event", G_CALLBACK(entry_search_key_release_event), window_handler);
	
	label = gtk_label_new("Replace for:");
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	
	gtk_box_pack_start(GTK_BOX(replace_bar), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(replace_bar), window_handler->replace_with_entry, TRUE, TRUE, 0);
	
	button = gtk_button_new_with_label("Replace");
	gtk_widget_set_size_request(button, 96, -1);
	g_signal_connect(button, "clicked", G_CALLBACK(button_replace_clicked), window_handler);
	gtk_box_pack_start(GTK_BOX(replace_bar), button, FALSE, FALSE, 0);
	button = gtk_button_new_with_label("Replace All");
	gtk_widget_set_size_request(button, 96, -1);
	g_signal_connect(button, "clicked", G_CALLBACK(button_replace_all_clicked), window_handler);
	gtk_box_pack_start(GTK_BOX(replace_bar), button, FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(window_handler->search_and_replace_bar), search_bar, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(window_handler->search_and_replace_bar), replace_bar, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(window_handler->box), search_and_replace_bar, FALSE, TRUE, 0);
	
	gtk_container_add(GTK_CONTAINER(window_handler->window), window_handler->box);
	
	window_handler->id_factory = 0;
	
	window_handler->preferences = preferences;
	
	gtk_drag_dest_set(window_handler->notebook, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
	gtk_drag_dest_add_text_targets(window_handler->notebook);
	gtk_drag_dest_add_uri_targets(window_handler->notebook);
	g_signal_connect(window_handler->notebook, "drag-drop", G_CALLBACK(window_drag_drop), window_handler);
	g_signal_connect(window_handler->notebook, "drag-leave", G_CALLBACK(window_drag_leave), window_handler);
	g_signal_connect(window_handler->notebook, "drag-motion", G_CALLBACK(window_drag_motion), window_handler);
	g_signal_connect(window_handler->notebook, "drag-data-received", G_CALLBACK(window_drag_data_received), window_handler);
	
	g_printf("[MESSAGE] Main window created.\n");
	if (preferences->start_new_page) {
		//action_new_activate(NULL, NULL, window_handler);
	}
	return window_handler;
}

// End of file.
