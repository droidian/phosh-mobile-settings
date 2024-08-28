/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-osk-add-layout-dialog"

#include "mobile-settings-config.h"

#include "ms-osk-add-layout-dialog.h"
#include "ms-osk-layout.h"

/**
 * MsOskAddLayoutDialog:
 *
 * Dialog to add an OSK layout
 */

enum {
  PROP_0,
  PROP_LAYOUTS,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


enum {
  LAYOUT_SELECTED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


struct _MsOskAddLayoutDialog {
  AdwDialog           parent;

  GtkWidget          *add_button;
  GtkListBox         *layouts_listbox;

  GtkFilterListModel *model;
};
G_DEFINE_TYPE (MsOskAddLayoutDialog, ms_osk_add_layout_dialog, ADW_TYPE_DIALOG)


static void
on_search_changed (MsOskAddLayoutDialog *self, GtkSearchEntry *entry)
{
  const char *text;
  GtkStringFilter *filter;

  text = gtk_editable_get_text (GTK_EDITABLE (entry));
  filter = GTK_STRING_FILTER (gtk_filter_list_model_get_filter (self->model));
  gtk_string_filter_set_search (filter, text);
}


static void
on_row_selected (MsOskAddLayoutDialog *self)
{
  gtk_widget_set_sensitive (self->add_button, TRUE);
}


static void
on_add_clicked (MsOskAddLayoutDialog *self)
{
  GtkListBoxRow *row = gtk_list_box_get_selected_row (self->layouts_listbox);
  MsOskLayout *layout;
  int index;

  index = gtk_list_box_row_get_index (row);
  layout = g_list_model_get_item (G_LIST_MODEL (self->model), index);

  g_signal_emit (self, signals[LAYOUT_SELECTED], 0, layout);

  adw_dialog_close (ADW_DIALOG (self));
}


static GtkWidget *
create_layout_row (gpointer item, gpointer user_data)
{
  MsOskLayout *layout = MS_OSK_LAYOUT (item);
  GtkWidget *row;

  row = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), ms_osk_layout_get_name (layout));

  return row;
}


static void
set_layouts (MsOskAddLayoutDialog *self, GListModel *layouts)
{
  GtkSorter *sorter;
  GtkStringFilter *filter;
  GtkSortListModel *sorted;
  GtkExpression *expression;

  expression = gtk_property_expression_new (MS_TYPE_OSK_LAYOUT, NULL, "name");
  sorter = GTK_SORTER (gtk_string_sorter_new (expression));

  expression = gtk_property_expression_new (MS_TYPE_OSK_LAYOUT, NULL, "name");
  filter = gtk_string_filter_new (expression);
  gtk_string_filter_set_ignore_case (filter, TRUE);
  gtk_string_filter_set_match_mode (filter, GTK_STRING_FILTER_MATCH_MODE_PREFIX);

  /* gtk_sort_list_model takes ownership */
  g_object_ref (layouts);
  sorted = gtk_sort_list_model_new (layouts, sorter);
  self->model = gtk_filter_list_model_new (G_LIST_MODEL (sorted),
                                           GTK_FILTER (filter));

  gtk_list_box_bind_model (GTK_LIST_BOX (self->layouts_listbox),
                           G_LIST_MODEL (self->model),
                           create_layout_row,
                           self,
                           NULL);
}


static void
ms_osk_add_layout_dialog_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  MsOskAddLayoutDialog *self = MS_OSK_ADD_LAYOUT_DIALOG (object);

  switch (property_id) {
  case PROP_LAYOUTS:
    set_layouts (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_osk_add_layout_dialog_dispose (GObject *object)
{
  MsOskAddLayoutDialog *self = MS_OSK_ADD_LAYOUT_DIALOG (object);

  g_clear_object (&self->model);

  G_OBJECT_CLASS (ms_osk_add_layout_dialog_parent_class)->dispose (object);
}


static void
ms_osk_add_layout_dialog_class_init (MsOskAddLayoutDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = ms_osk_add_layout_dialog_set_property;
  object_class->dispose = ms_osk_add_layout_dialog_dispose;

  props[PROP_LAYOUTS] =
    g_param_spec_object ("layouts", "", "",
                         G_TYPE_LIST_MODEL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[LAYOUT_SELECTED] =
    g_signal_new ("layout-selected",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, MS_TYPE_OSK_LAYOUT);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/"
                                               "ui/ms-osk-add-layout-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, MsOskAddLayoutDialog, layouts_listbox);
  gtk_widget_class_bind_template_child (widget_class, MsOskAddLayoutDialog, add_button);

  gtk_widget_class_bind_template_callback (widget_class, on_add_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_row_selected);
  gtk_widget_class_bind_template_callback (widget_class, on_search_changed);
}


static void
ms_osk_add_layout_dialog_init (MsOskAddLayoutDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
ms_osk_add_layout_dialog_new (GListModel *layouts)
{
  return g_object_new (MS_TYPE_OSK_ADD_LAYOUT_DIALOG,
                       "layouts", layouts,
                       NULL);
}
