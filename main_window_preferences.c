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
#include "main_window_preferences.h"
#include "module.h"

// Page general signals.
static gboolean switch_use_decoration_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->use_decoration = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_show_menu_bar_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->show_menu_bar = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_show_action_bar_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->show_action_bar = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_use_custom_gtk_theme_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->use_custom_gtk_theme = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static void combo_box_tab_position_changed(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	
	if (index > -1) {
		gtk_notebook_set_tab_pos(GTK_NOTEBOOK(window_preferences_handler->main_window_notebook), index);
	}
	window_preferences_handler->preferences->tabs_position = gtk_notebook_get_tab_pos(GTK_NOTEBOOK(window_preferences_handler->main_window_notebook));
}

static gboolean entry_custom_gtk_theme_key_press_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	
	if (window_preferences_handler->preferences->gtk_theme) {
		g_free(window_preferences_handler->preferences->gtk_theme);
	}
	
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(widget));
	
	if (text) {
		window_preferences_handler->preferences->gtk_theme = g_strdup(text);
		g_printf("Theme: %s.\n", window_preferences_handler->preferences->gtk_theme);
	}
	return FALSE;
}

// Page editor signals.
static gboolean switch_draw_spaces_leading_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->draw_spaces_leading = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_draw_spaces_line_break_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->draw_spaces_newline = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_draw_spaces_nbsp_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->draw_spaces_nbsp = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_draw_spaces_space_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->draw_spaces_space = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_draw_spaces_text_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->draw_spaces_text = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_draw_spaces_trailing_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->draw_spaces_trailing = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_draw_spaces_tab_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->draw_spaces_tab = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_highlight_current_line_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->highlight_current_line = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_highlight_matching_brackets_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->highlight_matching_brackets = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_show_grid_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->show_grid = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_show_map_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->show_map = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_show_line_numbers_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->show_line_numbers = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean switch_show_right_margin_state_set(GtkWidget *widget, gboolean state, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->show_right_margin = state;
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static void spin_button_right_margin_position_value_changed(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->right_margin_position = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static void button_font_font_set(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->preferences->editor_font = g_string_assign(window_preferences_handler->preferences->editor_font,
		gtk_font_button_get_font_name(GTK_FONT_BUTTON(widget)));
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

struct cwindow_preferences_handler *alloc_window_preferences_handler(struct capplication_handler *application_handler, struct cpreferences *preferences)
{
	g_printf("[MESSAGE] Creating preferences window.\n");
	struct cwindow_preferences_handler *window_preferences_handler = malloc(sizeof(struct cwindow_preferences_handler));
	window_preferences_handler->application_handler = application_handler;
	window_preferences_handler->preferences = preferences;
	window_preferences_handler->window = gtk_dialog_new_with_buttons("Preferences",
		NULL,
		GTK_DIALOG_USE_HEADER_BAR,
		"Close",
		GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(GTK_WIDGET(window_preferences_handler->window), 480, 480);
	
	// Get content area.
	GtkWidget *box_content = gtk_dialog_get_content_area(GTK_DIALOG(window_preferences_handler->window));
	gtk_orientable_set_orientation(GTK_ORIENTABLE(box_content), GTK_ORIENTATION_HORIZONTAL);
	gtk_container_set_border_width(GTK_CONTAINER(box_content), 8);
	gtk_box_set_spacing(GTK_BOX(box_content), 8);
	
	// Stack sidebar.
	g_printf("[MESSAGE] Create stack sidebar.\n");
	GtkWidget *scrolled_window = NULL;
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(box_content),
		GTK_WIDGET(scrolled_window),
		FALSE,
		TRUE,
		0);
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 120, -1);
	
	window_preferences_handler->stack_sidebar = gtk_stack_sidebar_new();
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(window_preferences_handler->stack_sidebar));
	
	// Stack.
	g_printf("[MESSAGE] Create stack.\n");
	window_preferences_handler->stack = gtk_stack_new();
	gtk_stack_set_transition_type(GTK_STACK(window_preferences_handler->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN);
	gtk_stack_set_transition_duration(GTK_STACK(window_preferences_handler->stack), 500);
	gtk_box_pack_end(GTK_BOX(box_content),
		GTK_WIDGET(window_preferences_handler->stack),
		TRUE,
		TRUE,
		0);
	gtk_stack_sidebar_set_stack(GTK_STACK_SIDEBAR(window_preferences_handler->stack_sidebar),
		GTK_STACK(window_preferences_handler->stack));
	
	// Pages.
	GtkWidget *box_page = NULL;
	GtkWidget *box_widget = NULL;
	GtkWidget *widget = NULL;
	
	// Page general.
	g_printf("[MESSAGE] Create page \"general\".\n");
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER,
		GTK_POLICY_AUTOMATIC);
	gtk_stack_add_titled(GTK_STACK(window_preferences_handler->stack),
		GTK_WIDGET(scrolled_window),
		"general",
		"General");
	box_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(box_page));
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->use_decoration);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_use_decoration_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Decoration");
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Decoration</b>\nUse client-side decoration instead of server-side decoration.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option show menu bar.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(box_widget, 8);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->show_menu_bar);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_show_menu_bar_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Show menu bar");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Show menu bar</b>\nShow menu bar when using server-side decoration.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option show action bar.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(GTK_WIDGET(box_widget), 8);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->show_action_bar);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_show_action_bar_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Show action bar");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Show action bar</b>\nAlways keep the action bar visible.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option use custom gtk theme.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(GTK_WIDGET(box_widget), 8);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->use_custom_gtk_theme);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_use_custom_gtk_theme_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Use custom gtk theme");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Use custom gtk theme</b>\nUse custom gtk theme.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	widget = gtk_entry_new();
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 2);
	gtk_entry_set_text(GTK_ENTRY(widget), preferences->gtk_theme);
	g_signal_connect(widget, "key-press-event", G_CALLBACK(entry_custom_gtk_theme_key_press_event), window_preferences_handler);
	g_signal_connect(widget, "key-release-event", G_CALLBACK(entry_custom_gtk_theme_key_press_event), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, TRUE, 0);
	
	// Option tabs position.
	widget = gtk_label_new("Tabs position");
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Tabs position</b>");
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 8);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	widget = gtk_combo_box_text_new();
	window_preferences_handler->combo_box_tab_pos = widget;
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 2);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), "left", "Left");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), "right", "Right");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), "top", "Top");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), "bottom", "Bottom");
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), preferences->tabs_position);
	g_signal_connect(widget, "changed", G_CALLBACK(combo_box_tab_position_changed), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	// Page editor.
	g_printf("[MESSAGE] Create page \"editor\".\n");
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER,
		GTK_POLICY_AUTOMATIC);
	gtk_stack_add_titled(GTK_STACK(window_preferences_handler->stack),
		GTK_WIDGET(scrolled_window),
		"editor",
		"Editor");
	box_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(box_page));
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->show_line_numbers);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_show_line_numbers_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Show line numbers");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Show line numbers</b>\nDescription.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(GTK_WIDGET(box_widget), 8);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->highlight_current_line);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_highlight_current_line_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Highlight current line");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Highlight current line</b>\nDescription.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(box_widget, 8);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->highlight_matching_brackets);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_highlight_matching_brackets_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Highlight matching brackets");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Highlight matching brackets</b>\nDescription.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(GTK_WIDGET(box_widget), 8);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->show_grid);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_show_grid_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Show grid");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Show grid</b>\nDescription.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(GTK_WIDGET(box_widget), 8);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->show_right_margin);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_show_right_margin_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Show right margin");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Show righ margin</b>\nDescription.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(box_widget, 8);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->show_map);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_show_map_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Show overview map");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Show overview map</b>\nDescription.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option draw spaces.
	widget = gtk_label_new("Rules for drawing spaces");
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Rules for drawing spaces</b>");
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 8);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, TRUE, 0);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(box_widget, 2);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->draw_spaces_space);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_draw_spaces_space_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Draw space character.");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "Draw space character.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(box_widget, 2);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->draw_spaces_tab);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_draw_spaces_tab_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Draw tab character.");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "Draw tab character.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(box_widget, 2);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->draw_spaces_newline);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_draw_spaces_line_break_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Draw line break character.");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "Draw line break character.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(box_widget, 2);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->draw_spaces_nbsp);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_draw_spaces_nbsp_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Draw non-breaking whitespace character.");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "Draw non-breaking whitespace character.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(box_widget, 2);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->draw_spaces_leading);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_draw_spaces_leading_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Draw leading whitespace character.");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "Draw leading whitespace character.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(box_widget, 2);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->draw_spaces_text);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_draw_spaces_text_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Draw whitespace inside text.");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "Draw whitespace inside text.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	// Option decoration.
	box_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_top(box_widget, 2);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(box_widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_switch_new();
	gtk_switch_set_state(GTK_SWITCH(widget), preferences->draw_spaces_trailing);
	g_signal_connect(widget, "state-set", G_CALLBACK(switch_draw_spaces_trailing_state_set), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	widget = gtk_label_new("Draw trailing whitespace text.");
	gtk_widget_set_margin_start(GTK_WIDGET(widget), 2);
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "Draw trailing whitespace text.");
	gtk_box_pack_start(GTK_BOX(box_widget),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	
	
	// Option font.
	widget = gtk_label_new("Font");
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Font</b>");
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 8);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, TRUE, 0);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	
	widget = gtk_font_button_new();
	gtk_font_button_set_font_name(GTK_FONT_BUTTON(widget), preferences->editor_font->str);
	g_signal_connect(widget, "font-set", G_CALLBACK(button_font_font_set), window_preferences_handler);
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 2);
	gtk_box_pack_start(GTK_BOX(box_page), GTK_WIDGET(widget), FALSE, FALSE, 0);
	
	// Option right margin position.
	widget = gtk_label_new("Right margin position");
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Right margin position</b>");
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 8);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, TRUE, 0);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	
	widget = gtk_spin_button_new_with_range(1, 800, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), preferences->right_margin_position);
	g_signal_connect(widget, "value-changed", G_CALLBACK(spin_button_right_margin_position_value_changed), window_preferences_handler);
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 2);
	gtk_box_pack_start(GTK_BOX(box_page), GTK_WIDGET(widget), FALSE, FALSE, 0);
	
	gtk_widget_show_all(box_page);
	
	// Option color scheme.
	widget = gtk_label_new("Color scheme");
	gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Color scheme</b>");
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 8);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, TRUE, 0);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	
	widget = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(widget), FALSE);
	
	window_preferences_handler->tree_model = GTK_TREE_MODEL(gtk_tree_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING));
	window_preferences_handler->tree_model_sorted = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(window_preferences_handler->tree_model));
	
	window_preferences_handler->tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(window_preferences_handler->tree_model_sorted));
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
	gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(cell_renderer), TRUE);
	//g_signal_connect(cell_renderer, "toggled", G_CALLBACK(cell_renderer_toggle_toggled), window_preferences_handler);
	
	gtk_cell_renderer_toggle_set_activatable(GTK_CELL_RENDERER_TOGGLE(cell_renderer), TRUE);
	
	cell_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell_renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", _COLUMN_SCHEME_);
	
	gchar **idp = (gchar **)preferences->scheme_ids;
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
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
	gtk_widget_set_size_request(scrolled_window, -1, 128);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(scrolled_window), window_preferences_handler->tree_view);
	gtk_box_pack_start(GTK_BOX(box_page), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show_all(box_page);
	
	
	// Page plugins.
	g_printf("[MESSAGE] Create page \"plugins\".\n");
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER,
		GTK_POLICY_AUTOMATIC);
	gtk_stack_add_titled(GTK_STACK(window_preferences_handler->stack),
		GTK_WIDGET(scrolled_window),
		"plugins",
		"Plugins");
	box_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(box_page));
	
	lua_State *lua = window_preferences_handler->application_handler->lua;
	if (lua) {
		g_printf("[MESSAGE] Creating plugins page.\n");
		lua_getglobal(lua, "editor");
		if (lua_istable(lua, -1)) {
			lua_pushstring(lua, "f_plugins_page");
			lua_gettable(lua, -2);
			if (lua_isfunction(lua, -1)) {
				lua_pushlightuserdata(lua, box_page);
				if (lua_pcall(lua, 1, 0, 0) != LUA_OK) {
					g_printf("[ERROR] Fail to call editor[\"f_plugins_page\"].\n");
				} else {
					g_printf("[MESSAGE] Function editor[\"f_plugins_page\"] called.\n");
				}
			} else {
				g_printf("[ERROR] Failed to get function editor[\"f_plugins_page\"].\n");
				lua_pop(lua, 1);
			}
		}
		lua_pop(lua, 1);
	}
	
	g_printf("[MESSAGE] Preferences window created.\n");
	return window_preferences_handler;
}

// End of file.
