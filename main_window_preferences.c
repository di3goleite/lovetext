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
static gboolean check_button_use_decoration_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->use_decoration = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_show_menu_bar_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->show_menu_bar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_show_action_bar_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->show_action_bar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_use_custom_gtk_theme_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->use_custom_gtk_theme = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
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
	window_preferences_handler->application_handler->tabs_position = gtk_notebook_get_tab_pos(GTK_NOTEBOOK(window_preferences_handler->main_window_notebook));
}

static gboolean entry_custom_gtk_theme_key_press_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	
	if (window_preferences_handler->application_handler->gtk_theme) {
		g_free(window_preferences_handler->application_handler->gtk_theme);
	}
	
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(widget));
	
	if (text) {
		window_preferences_handler->application_handler->gtk_theme = g_strdup(text);
		g_printf("Theme: %s.\n", window_preferences_handler->application_handler->gtk_theme);
	}
	return FALSE;
}

// Page editor signals.
static gboolean check_button_draw_spaces_leading_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->draw_spaces_leading = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_draw_spaces_line_break_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->draw_spaces_newline = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_draw_spaces_nbsp_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->draw_spaces_nbsp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_draw_spaces_space_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->draw_spaces_space = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_draw_spaces_text_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->draw_spaces_text = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_draw_spaces_trailing_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->draw_spaces_trailing = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_draw_spaces_tab_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->draw_spaces_tab = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_highlight_current_line_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->highlight_current_line = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_highlight_matching_brackets_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->highlight_matching_brackets = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_show_grid_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->show_grid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_wrap_lines_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->wrap_lines = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_show_map_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->show_map = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_show_line_numbers_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->show_line_numbers = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static gboolean check_button_show_right_margin_toggled(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->show_right_margin = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
	return FALSE;
}

static void spin_button_right_margin_position_value_changed(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->right_margin_position = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	window_preferences_handler->update_editor(window_preferences_handler->window_handler);
}

static void button_font_font_set(GtkWidget *widget, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	window_preferences_handler->application_handler->editor_font = g_string_assign(window_preferences_handler->application_handler->editor_font,
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
			GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(window_preferences_handler->application_handler->style_scheme_manager, id);
			if (scheme) {
				window_preferences_handler->application_handler->scheme = scheme;
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

static void list_box_preferences_row_activated(GtkListBox *list_box, GtkListBoxRow *row, gpointer user_data)
{
	struct cwindow_preferences_handler *window_preferences_handler = (struct cwindow_preferences_handler *)user_data;
	g_printf("MM Change page.\n");
	gint index = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row));
	
	if (index == 0) {
		gtk_stack_set_visible_child_name(GTK_STACK(window_preferences_handler->stack), "general");
	}
	if (index == 1) {
		gtk_stack_set_visible_child_name(GTK_STACK(window_preferences_handler->stack), "editor");
	}
	if (index == 2) {
		gtk_stack_set_visible_child_name(GTK_STACK(window_preferences_handler->stack), "plugins");
	}
	if (index == 3) {
		gtk_stack_set_visible_child_name(GTK_STACK(window_preferences_handler->stack), "keybindings");
	}
	
}

struct cwindow_preferences_handler *alloc_window_preferences_handler(struct capplication_handler *application_handler)
{
	g_printf("[MESSAGE] Creating preferences window.\n");
	struct cwindow_preferences_handler *window_preferences_handler = malloc(sizeof(struct cwindow_preferences_handler));
	window_preferences_handler->application_handler = application_handler;
	window_preferences_handler->window = gtk_dialog_new_with_buttons("Preferences",
		NULL,
		GTK_DIALOG_USE_HEADER_BAR | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
		"Close",
		GTK_RESPONSE_CANCEL,
		NULL);
	
	// Get content area.
	GtkWidget *box_content = gtk_dialog_get_content_area(GTK_DIALOG(window_preferences_handler->window));
	//GtkWidget *box_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(GTK_WIDGET(box_content), 400, 400);
	window_preferences_handler->page = box_content;
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
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 160, -1);
	
	// List box project.
	GtkWidget *list_box = gtk_list_box_new();
	g_signal_connect(list_box, "row-activated", G_CALLBACK(list_box_preferences_row_activated), window_preferences_handler);
	gtk_widget_set_size_request(GTK_WIDGET(list_box), 160, -1);
	gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(list_box), TRUE);
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_SINGLE);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(list_box));
	
	GtkWidget *image_icon = NULL;
	GtkWidget *label = NULL;
	GtkWidget *box = NULL;
	
	label = gtk_label_new("General");
	image_icon = gtk_image_new_from_icon_name("view-grid-symbolic", GTK_ICON_SIZE_DND);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>General</b>");
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(label), GTK_ALIGN_START);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_container_set_border_width(GTK_CONTAINER(box), 8);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(image_icon), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(label), TRUE, TRUE, 0);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), GTK_WIDGET(box), -1);
	GtkListBoxRow *list_box_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(list_box), 0);
	gtk_list_box_select_row(GTK_LIST_BOX(list_box), list_box_row);
	
	label = gtk_label_new("Editor");
	image_icon = gtk_image_new_from_icon_name("accessories-text-editor-symbolic", GTK_ICON_SIZE_DND);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Editor</b>");
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(label), GTK_ALIGN_START);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_container_set_border_width(GTK_CONTAINER(box), 8);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(image_icon), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(label), TRUE, TRUE, 0);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), GTK_WIDGET(box), -1);
	
	label = gtk_label_new("Plugins");
	image_icon = gtk_image_new_from_icon_name("view-list-symbolic", GTK_ICON_SIZE_DND);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Plugins</b>");
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(label), GTK_ALIGN_START);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_container_set_border_width(GTK_CONTAINER(box), 8);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(image_icon), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(label), TRUE, TRUE, 0);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), GTK_WIDGET(box), -1);
	
	label = gtk_label_new("Keybindings");
	image_icon = gtk_image_new_from_icon_name("input-keyboard-symbolic", GTK_ICON_SIZE_DND);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Keybindings</b>");
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(label), GTK_ALIGN_START);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_container_set_border_width(GTK_CONTAINER(box), 8);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(image_icon), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(label), TRUE, TRUE, 0);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), GTK_WIDGET(box), -1);
	
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
	
	// Pages.
	GtkWidget *box_page = NULL;
	GtkWidget *widget = NULL;
	label = NULL;
	
	// Page general.
	g_printf("[MESSAGE] Create page \"general\".\n");
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER,
		GTK_POLICY_AUTOMATIC);
	gtk_stack_add_named(GTK_STACK(window_preferences_handler->stack),
		GTK_WIDGET(scrolled_window),
		"general");
	box_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(box_page));
	
	// Option decoration.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->use_decoration);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_use_decoration_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Decoration");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Decoration</b>\nUse client-side decoration instead of server-side decoration.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option show menu bar.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->show_menu_bar);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_show_menu_bar_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Show menu bar");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Show menu bar</b>\nShow menu bar when using server-side decoration.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option show action bar.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->show_action_bar);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_show_action_bar_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Show action bar");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Show action bar</b>\nAlways keep the action bar visible.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option use custom gtk theme.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->use_custom_gtk_theme);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_use_custom_gtk_theme_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Use custom gtk theme");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Use custom gtk theme</b>\nUse custom gtk theme.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	widget = gtk_entry_new();
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 2);
	gtk_entry_set_text(GTK_ENTRY(widget), application_handler->gtk_theme);
	g_signal_connect(widget, "key-press-event", G_CALLBACK(entry_custom_gtk_theme_key_press_event), window_preferences_handler);
	g_signal_connect(widget, "key-release-event", G_CALLBACK(entry_custom_gtk_theme_key_press_event), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, TRUE, 0);
	
	// Option tabs position.
	label = gtk_label_new("Tabs position");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Tabs position</b>");
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
	gtk_widget_set_margin_top(GTK_WIDGET(label), 8);
	gtk_box_pack_start(GTK_BOX(box_page), label, FALSE, FALSE, 0);
	
	widget = gtk_combo_box_text_new();
	window_preferences_handler->combo_box_tab_pos = widget;
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 2);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), "left", "Left");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), "right", "Right");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), "top", "Top");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), "bottom", "Bottom");
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), application_handler->tabs_position);
	g_signal_connect(widget, "changed", G_CALLBACK(combo_box_tab_position_changed), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page), widget, FALSE, FALSE, 0);
	
	// Page editor.
	g_printf("[MESSAGE] Create page \"editor\".\n");
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER,
		GTK_POLICY_AUTOMATIC);
	gtk_stack_add_named(GTK_STACK(window_preferences_handler->stack),
		GTK_WIDGET(scrolled_window),
		"editor");
	box_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(box_page));
	
	// Option show line numbers.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->show_line_numbers);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_show_line_numbers_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Show line numbers");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Show line numbers</b>\nDescription.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option highlight current line.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->highlight_current_line);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_highlight_current_line_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Highlight current line");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Highlight current line</b>\nDescription.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option highlight matching brackets.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->highlight_matching_brackets);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_highlight_matching_brackets_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Highlight matching brackets");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Highlight matching brackets</b>\nDescription.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option show grid.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->show_grid);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_show_grid_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Show grid");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Show grid</b>\nDescription.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option wrap lines.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->wrap_lines);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_wrap_lines_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Wrap lines");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Wrap lines</b>\nDescription.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option show right margin.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->show_right_margin);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_show_right_margin_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Show right margin");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Show righ margin</b>\nDescription.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option show overview map.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->show_map);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_show_map_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Show overview map");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Show overview map</b>\nDescription.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option draw spaces.
	label = gtk_label_new("Rules for drawing spaces");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Rules for drawing spaces</b>");
	gtk_widget_set_margin_top(GTK_WIDGET(label), 8);
	gtk_box_pack_start(GTK_BOX(box_page), label, FALSE, TRUE, 0);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
	
	// Option draw space character.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->draw_spaces_space);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_draw_spaces_space_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Draw space character.");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "Draw space character.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option draw tab character.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->draw_spaces_tab);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_draw_spaces_tab_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Draw tab character.");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "Draw tab character.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option draw line break character.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->draw_spaces_newline);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_draw_spaces_line_break_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Draw line break character.");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "Draw line break character.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option draw non-breaking whitespace character.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->draw_spaces_nbsp);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_draw_spaces_nbsp_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Draw non-breaking whitespace character.");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "Draw non-breaking whitespace character.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option draw leading whitespace character.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->draw_spaces_leading);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_draw_spaces_leading_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Draw leading whitespace character.");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "Draw leading whitespace character.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option draw whitespace inside text.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->draw_spaces_text);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_draw_spaces_text_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Draw whitespace inside text.");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "Draw whitespace inside text.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option draw trailing whitespace text.
	widget = gtk_check_button_new();
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), application_handler->draw_spaces_trailing);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_draw_spaces_trailing_toggled), window_preferences_handler);
	gtk_box_pack_start(GTK_BOX(box_page),
		GTK_WIDGET(widget),
		FALSE,
		TRUE,
		0);
	label = gtk_label_new("Draw trailing whitespace text.");
	gtk_widget_set_margin_start(GTK_WIDGET(label), 2);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "Draw trailing whitespace text.");
	gtk_container_add(GTK_CONTAINER(widget), label);
	
	// Option font.
	label = gtk_label_new("Font");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Font</b>");
	gtk_widget_set_margin_top(GTK_WIDGET(label), 8);
	gtk_box_pack_start(GTK_BOX(box_page), label, FALSE, TRUE, 0);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
	
	widget = gtk_font_button_new();
	gtk_font_button_set_font_name(GTK_FONT_BUTTON(widget), application_handler->editor_font->str);
	g_signal_connect(widget, "font-set", G_CALLBACK(button_font_font_set), window_preferences_handler);
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 2);
	gtk_box_pack_start(GTK_BOX(box_page), GTK_WIDGET(widget), FALSE, FALSE, 0);
	
	// Option right margin position.
	label = gtk_label_new("Right margin position");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Right margin position</b>");
	gtk_widget_set_margin_top(GTK_WIDGET(label), 8);
	gtk_box_pack_start(GTK_BOX(box_page), label, FALSE, TRUE, 0);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
	
	widget = gtk_spin_button_new_with_range(1, 800, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), application_handler->right_margin_position);
	g_signal_connect(widget, "value-changed", G_CALLBACK(spin_button_right_margin_position_value_changed), window_preferences_handler);
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 2);
	gtk_box_pack_start(GTK_BOX(box_page), GTK_WIDGET(widget), FALSE, TRUE, 0);
	
	// Option color scheme.
	label = gtk_label_new("Color scheme");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Color scheme</b>");
	gtk_widget_set_margin_top(GTK_WIDGET(label), 8);
	gtk_box_pack_start(GTK_BOX(box_page), label, FALSE, TRUE, 0);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
	
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
	
	gchar **idp = (gchar **)application_handler->scheme_ids;
	gchar *id = *idp;
	while (id != NULL) {
		GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(application_handler->style_scheme_manager, id);
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
	gtk_stack_add_named(GTK_STACK(window_preferences_handler->stack),
		GTK_WIDGET(scrolled_window),
		"plugins");
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
	
	// Page keybindings.
	g_printf("[MESSAGE] Create page \"keybindings\".\n");
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_NEVER,
		GTK_POLICY_AUTOMATIC);
	gtk_stack_add_named(GTK_STACK(window_preferences_handler->stack),
		GTK_WIDGET(scrolled_window),
		"keybindings");
	box_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(box_page));
	
	label = gtk_label_new("Search keybindings:");
	gtk_box_pack_start(GTK_BOX(box_page), GTK_WIDGET(label), FALSE, TRUE, 0);
	
	widget = gtk_search_entry_new();
	gtk_widget_set_margin_top(GTK_WIDGET(widget), 2);
	gtk_box_pack_start(GTK_BOX(box_page), GTK_WIDGET(widget), FALSE, TRUE, 0);
	
	gtk_widget_show_all(GTK_WIDGET(box_page));
	g_printf("[MESSAGE] Preferences window created.\n");
	return window_preferences_handler;
}

// End of file.
