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
#include "main_window_preferences.h"
#include "module.h"

/*
static void cell_renderer_toggle_toggled(GtkCellRendererToggle *cell_renderer, gchar *path, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_handler *)user_data;
	gchar *path_string = gtk_tree_path_to_string(path);
	
	GtkTreeModel *tree_model = gtk_tree_view_get_model(window_preferences_handler->tree_view);
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_from_string(tree_model, &iter, path_string)) {
		gtk_cell_renderer_toggle_set_active(cell_renderer, TRUE);
	}
	g_free(path_string);
}*/

static void combo_box_text_changed(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	int index = gtk_combo_box_get_active(widget);
	
	if (index > -1) {
		gtk_notebook_set_tab_pos(window_preferences_handler->main_window_notebook, index);
	}
	window_preferences_handler->preferences->tabs_position = gtk_notebook_get_tab_pos(window_preferences_handler->main_window_notebook);
}

static void check_button_use_custom_gtk_theme(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->use_custom_gtk_theme = gtk_toggle_button_get_active(widget);
	
	/*
	// Run-time update.
	GtkSettings *settings = gtk_settings_get_default();
	
	if ((window_preferences_handler->preferences->use_custom_gtk_theme) && (window_preferences_handler->preferences->gtk_theme)) {
		g_printf("[MESSAGE] Setting custom theme.\n");
		g_object_set(settings, "gtk-theme-name", window_preferences_handler->preferences->gtk_theme, NULL);
	}
	*/
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static void check_button_show_line_numbers_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->show_line_numbers = gtk_toggle_button_get_active(widget);
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static void check_button_show_right_margin_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->show_right_margin = gtk_toggle_button_get_active(widget);
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static void check_button_highlight_matching_brackets_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->highlight_matching_brackets = gtk_toggle_button_get_active(widget);
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static void check_button_highlight_current_line_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->highlight_current_line = gtk_toggle_button_get_active(widget);
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static void check_button_show_menu_bar_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->show_menu_bar = gtk_toggle_button_get_active(widget);
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static void check_button_show_tool_bar_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->show_tool_bar = gtk_toggle_button_get_active(widget);
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static void check_button_show_status_bar_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->show_status_bar = gtk_toggle_button_get_active(widget);
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static void button_font_font_set(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->editor_font = g_string_assign(window_preferences_handler->preferences->editor_font,
		gtk_font_button_get_font_name(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static void tree_view_schemes_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	gchar *path_string = gtk_tree_path_to_string(path);
	
	GtkTreeModel *tree_model = gtk_tree_view_get_model(tree_view);
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_from_string(tree_model, &iter, path_string)) {
		gchar *id = NULL;
		gtk_tree_model_get(tree_model, &iter, _COLUMN_ID_, &id, -1);
		if (id) {
			GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(window_preferences_handler->preferences->style_scheme_manager, id);
			if (scheme) {
				window_preferences_handler->preferences->scheme = scheme;
				gtk_tree_model_get(tree_model, &iter, _COLUMN_ID_, &id, -1);
				
				GtkTreeIter child_iter;
				GtkTreeModel *child_model = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(tree_model));
				gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(tree_model), &child_iter, &iter);
				gtk_tree_store_set(GTK_TREE_STORE(child_model), &child_iter, _COLUMN_SCHEME_TOGGLE_, TRUE, -1);
			}
		}
	}
	g_free(path_string);
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static gboolean entry_custom_gtk_theme_key_press_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	
	if (window_preferences_handler->preferences->gtk_theme) {
		g_free(window_preferences_handler->preferences->gtk_theme);
	}
	
	gchar *text = gtk_entry_get_text(widget);
	
	if (text) {
		window_preferences_handler->preferences->gtk_theme = g_strdup(text);
		g_printf("Theme: %s.\n", window_preferences_handler->preferences->gtk_theme);
	}
	return FALSE;
}

static void window_preferences_destroy(GtkWidget *widget, gpointer user_pointer)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_pointer;
	gtk_widget_hide(widget);
}

struct cwindow_preferences_handler *alloc_window_preferences_handler(struct cpreferences *preferences)
{
	g_printf("[MESSAGE] Creating preferences window.\n");
	struct cwindow_preferences_handler *window_preferences_handler = malloc(sizeof(struct cwindow_preferences_handler));
	window_preferences_handler->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window_preferences_handler->window), "Love Text - Preferences");
	gtk_widget_set_size_request(GTK_WINDOW(window_preferences_handler->window), 480, 400);
	gtk_container_set_border_width(window_preferences_handler->window, 8);
	g_signal_connect(window_preferences_handler->window, "destroy", G_CALLBACK(window_preferences_destroy), window_preferences_handler);
	//window_preferences_handler->box = gtk_vbox_new(FALSE, 2);
	
	window_preferences_handler->notebook = gtk_notebook_new();
	window_preferences_handler->box = gtk_vbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(window_preferences_handler->box), window_preferences_handler->notebook, TRUE, TRUE, 0);
	
	GtkWidget *box_page = NULL;
	GtkWidget *box_tmp = NULL;
	GtkWidget *widget = NULL;
	GtkWidget *box_group = NULL;
	GtkWidget *scrolled_window = NULL;
	
	// Create general page.
	box_page = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(box_page, 8);
	gtk_notebook_append_page(GTK_NOTEBOOK(window_preferences_handler->notebook), box_page, gtk_label_new("General"));
	
	widget = gtk_check_button_new_with_label("Show menu bar.");
	gtk_toggle_button_set_active(widget, preferences->show_menu_bar);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_show_menu_bar_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	widget = gtk_check_button_new_with_label("Show tool bar.");
	gtk_widget_set_margin_top(widget, 8);
	gtk_toggle_button_set_active(widget, preferences->show_tool_bar);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_show_tool_bar_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	widget = gtk_check_button_new_with_label("Show status bar.");
	gtk_widget_set_margin_top(widget, 8);
	gtk_toggle_button_set_active(widget, preferences->show_status_bar);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_show_status_bar_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	box_group = gtk_vbox_new(FALSE, 2);
	widget = gtk_check_button_new_with_label("Use custom gtk theme:");
	gtk_widget_set_margin_top(widget, 8);
	gtk_toggle_button_set_active(widget, preferences->use_custom_gtk_theme);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_use_custom_gtk_theme), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_group), widget, TRUE, FALSE, 0);
	widget = gtk_entry_new();
	gtk_entry_set_text(widget, preferences->gtk_theme);
	g_signal_connect(widget, "key-press-event", G_CALLBACK(entry_custom_gtk_theme_key_press_event), window_preferences_handler);
	g_signal_connect(widget, "key-release-event", G_CALLBACK(entry_custom_gtk_theme_key_press_event), window_preferences_handler);
	gtk_widget_set_tooltip_markup(widget, "Select a gtk2 theme by informing the <b>directory</b> where the theme is located.");
	gtk_box_pack_start(GTK_BOX(box_group), widget, TRUE, FALSE, 0);
	
	widget = gtk_label_new("Tabs position:");
	gtk_widget_set_margin_top(widget, 8);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	widget = gtk_combo_box_text_new();
	window_preferences_handler->combo_box_tab_pos = widget;
	gtk_widget_set_margin_top(widget, 2);
	gtk_combo_box_text_append(widget, "left", "Left");
	gtk_combo_box_text_append(widget, "right", "Right");
	gtk_combo_box_text_append(widget, "top", "Top");
	gtk_combo_box_text_append(widget, "bottom", "Bottom");
	g_signal_connect(widget, "changed", G_CALLBACK(combo_box_text_changed), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(box_page), box_group, FALSE, FALSE, 0);
	
	gtk_widget_show_all(box_page);
	
	// Create editor page.
	box_page = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(box_page, 8);
	gtk_notebook_append_page(GTK_NOTEBOOK(window_preferences_handler->notebook), box_page, gtk_label_new("Editor"));
	
	widget = gtk_check_button_new_with_label("Show line numbers.");
	gtk_toggle_button_set_active(widget, preferences->show_line_numbers);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_show_line_numbers_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	widget = gtk_check_button_new_with_label("Show right margin.");
	gtk_widget_set_margin_top(widget, 8);
	gtk_toggle_button_set_active(widget, preferences->show_right_margin);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_show_right_margin_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	widget = gtk_check_button_new_with_label("Highlight current line.");
	gtk_widget_set_margin_top(widget, 8);
	gtk_toggle_button_set_active(widget, preferences->highlight_current_line);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_highlight_current_line_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	widget = gtk_check_button_new_with_label("Highlight matching brackets.");
	gtk_widget_set_margin_top(widget, 8);
	gtk_toggle_button_set_active(widget, preferences->highlight_matching_brackets);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_highlight_matching_brackets_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	widget = gtk_label_new("Font:");
	gtk_widget_set_margin_top(widget, 8);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, TRUE, 0);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	
	widget = gtk_font_button_new();
	gtk_font_button_set_font_name(widget, preferences->editor_font->str);
	g_signal_connect(widget, "font-set", G_CALLBACK(button_font_font_set), window_preferences_handler);
	gtk_widget_set_margin_top(widget, 2);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	gtk_widget_show_all(box_page);
	
	widget = gtk_label_new("Color scheme:");
	gtk_widget_set_margin_top(widget, 8);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, TRUE, 0);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	
	widget = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(widget, FALSE);
	
	window_preferences_handler->tree_model = GTK_TREE_MODEL(gtk_tree_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING));
	window_preferences_handler->tree_model_sorted = gtk_tree_model_sort_new_with_model(window_preferences_handler->tree_model);
	
	window_preferences_handler->tree_view = gtk_tree_view_new_with_model(window_preferences_handler->tree_model_sorted);
	g_signal_connect(window_preferences_handler->tree_view, "row-activated", G_CALLBACK(tree_view_schemes_row_activated), window_preferences_handler);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(window_preferences_handler->tree_view), FALSE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(window_preferences_handler->tree_view), FALSE);
	
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell_renderer;
	
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "Commands");
	gtk_tree_view_append_column(GTK_TREE_VIEW(window_preferences_handler->tree_view), column);
	
	cell_renderer = gtk_cell_renderer_toggle_new();
	window_preferences_handler->cell_renderer_toggle = cell_renderer;
	gtk_tree_view_column_pack_start(column, cell_renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "active", _COLUMN_SCHEME_TOGGLE_);
	gtk_cell_renderer_toggle_set_radio(cell_renderer, TRUE);
	//g_signal_connect(cell_renderer, "toggled", G_CALLBACK(cell_renderer_toggle_toggled), window_preferences_handler);
	
	gtk_cell_renderer_toggle_set_activatable(cell_renderer, TRUE);
	
	cell_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell_renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", _COLUMN_SCHEME_);
	
	
	gchar **idp = preferences->scheme_ids;
	gchar *id = *idp;
	while (id != NULL) {
		GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(preferences->style_scheme_manager, id);
		const gchar *name = gtk_source_style_scheme_get_name(scheme);
		
		GtkTreeIter iter;
		gtk_tree_store_append(GTK_TREE_STORE(window_preferences_handler->tree_model), &iter, NULL);
		gtk_tree_store_set(GTK_TREE_STORE(window_preferences_handler->tree_model), &iter, _COLUMN_ID_, id, _COLUMN_SCHEME_, name, -1);
		idp++;
		id = *idp;
	}
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(window_preferences_handler->tree_model_sorted), _COLUMN_SCHEME_, GTK_SORT_ASCENDING);
	
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(scrolled_window, GTK_SHADOW_IN);
	gtk_widget_set_size_request(scrolled_window, -1, 128);
	gtk_scrolled_window_set_policy(scrolled_window, GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_container_add(scrolled_window, window_preferences_handler->tree_view);
	gtk_box_pack_start(GTK_BOX(box_page), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show_all(box_page);
	
	// Create plugins page.
	box_page = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(box_page, 8);
	gtk_notebook_append_page(GTK_NOTEBOOK(window_preferences_handler->notebook), box_page, gtk_label_new("Plugins"));
	gtk_widget_show_all(box_page);
	
	gtk_container_add(GTK_CONTAINER(window_preferences_handler->window), window_preferences_handler->box);
	
	window_preferences_handler->preferences = preferences;
	
	g_printf("[MESSAGE] Preferences window created.\n");
	
	return window_preferences_handler;
}

// End of file.
