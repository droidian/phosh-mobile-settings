/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-osk-layout"

#include "mobile-settings-config.h"

#include "ms-osk-layout-row.h"

/**
 * MsOskLayout:
 *
 * A row in the osk layout list box
 */

enum {
  PROP_0,
  PROP_LAYOUT,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


enum {
  MOVE_ROW,
  REMOVE_ROW,
  N_SIGNALS,
};
static guint signals[N_SIGNALS];


struct _MsOskLayoutRow {
  AdwActionRow      parent;

  GtkListBox       *drag_widget;
  gdouble           drag_x;
  gdouble           drag_y;

  MsOskLayout      *layout;
};
G_DEFINE_TYPE (MsOskLayoutRow, ms_osk_layout_row, ADW_TYPE_ACTION_ROW)


static void
on_remove_activated (GtkWidget *widget, const char *action_name, GVariant   *parameter)
{
  MsOskLayoutRow *self = MS_OSK_LAYOUT_ROW (widget);

  g_signal_emit (self, signals[REMOVE_ROW], 0);
}


static void
on_move_up_activated (GtkWidget *widget, const char *action_name, GVariant   *parameter)
{
  MsOskLayoutRow *self = MS_OSK_LAYOUT_ROW (widget);
  GtkListBox *list_box = GTK_LIST_BOX (gtk_widget_get_parent (GTK_WIDGET (self)));
  gint previous_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self)) - 1;
  GtkListBoxRow *previous_row = gtk_list_box_get_row_at_index (list_box, previous_idx);

  if (previous_row == NULL)
    return;

  g_signal_emit (self, signals[MOVE_ROW], 0, previous_row);
}


static void
on_move_down_activated (GtkWidget *widget, const char *action_name, GVariant *parameter)
{
  MsOskLayoutRow *self = MS_OSK_LAYOUT_ROW (widget);
  GtkListBox *list_box = GTK_LIST_BOX (gtk_widget_get_parent (GTK_WIDGET (self)));
  gint next_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self)) + 1;
  GtkListBoxRow *next_row = gtk_list_box_get_row_at_index (list_box, next_idx);

  if (next_row == NULL)
    return;

  g_signal_emit (next_row, signals[MOVE_ROW], 0, self);
}


static GdkContentProvider *
on_drag_prepare (MsOskLayoutRow *self, double x, double y)
{
  self->drag_x = x;
  self->drag_y = y;

  return gdk_content_provider_new_typed (MS_TYPE_OSK_LAYOUT_ROW, self);
}


static void
on_drag_begin (MsOskLayoutRow *self, GdkDrag *drag)
{
  GtkWidget *plugin_row;
  GtkWidget *drag_icon;

  self->drag_widget = GTK_LIST_BOX (gtk_list_box_new ());

  plugin_row = ms_osk_layout_row_new (self->layout);

  gtk_widget_set_size_request (GTK_WIDGET (plugin_row),
                               gtk_widget_get_width (GTK_WIDGET (self)),
                               gtk_widget_get_height (GTK_WIDGET (self)));

  gtk_list_box_append (GTK_LIST_BOX (self->drag_widget), GTK_WIDGET (plugin_row));
  gtk_list_box_drag_highlight_row (self->drag_widget, GTK_LIST_BOX_ROW (plugin_row));

  drag_icon = gtk_drag_icon_get_for_drag (drag);
  gtk_drag_icon_set_child (GTK_DRAG_ICON (drag_icon), GTK_WIDGET (self->drag_widget));
  gdk_drag_set_hotspot (drag, self->drag_x, self->drag_y);
}


static gboolean
on_drop (MsOskLayoutRow *self, const GValue *value, gdouble x, gdouble y)
{
  MsOskLayoutRow *source;

  g_debug ("Dropped osk layout row %p", self);

  if (!G_VALUE_HOLDS (value, MS_TYPE_OSK_LAYOUT_ROW))
    return FALSE;

  source = g_value_get_object (value);

  g_signal_emit (source, signals[MOVE_ROW], 0, self);

  return TRUE;
}


static void
set_layout (MsOskLayoutRow *self, MsOskLayout *layout)
{
  self->layout = g_object_ref (layout);

  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (self), ms_osk_layout_get_name (layout));
}


static void
ms_osk_layout_row_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MsOskLayoutRow *self = MS_OSK_LAYOUT_ROW (object);

  switch (property_id) {
  case PROP_LAYOUT:
    set_layout (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_osk_layout_row_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MsOskLayoutRow *self = MS_OSK_LAYOUT_ROW (object);

  switch (property_id) {
  case PROP_LAYOUT:
    g_value_set_object (value, self->layout);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_osk_layout_row_dispose (GObject *object)
{
  MsOskLayoutRow *self = MS_OSK_LAYOUT_ROW (object);

  g_clear_object (&self->layout);

  G_OBJECT_CLASS (ms_osk_layout_row_parent_class)->dispose (object);
}


static void
ms_osk_layout_row_class_init (MsOskLayoutRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ms_osk_layout_row_get_property;
  object_class->set_property = ms_osk_layout_row_set_property;
  object_class->dispose = ms_osk_layout_row_dispose;

  props[PROP_LAYOUT] =
    g_param_spec_object ("layout", "", "",
                         MS_TYPE_OSK_LAYOUT,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[MOVE_ROW] =
    g_signal_new ("move-row",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, MS_TYPE_OSK_LAYOUT_ROW);

  signals[REMOVE_ROW] =
    g_signal_new ("remove-row",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  gtk_widget_class_install_action (widget_class, "row.move-up", NULL, on_move_up_activated);
  gtk_widget_class_install_action (widget_class, "row.move-down", NULL, on_move_down_activated);
  gtk_widget_class_install_action (widget_class, "row.remove", NULL, on_remove_activated);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-osk-layout-row.ui");
}


static void
ms_osk_layout_row_init (MsOskLayoutRow *self)
{
  GtkDragSource *drag_source;
  GtkDropTarget *drop_target;

  gtk_widget_init_template (GTK_WIDGET (self));

  drag_source = gtk_drag_source_new ();
  gtk_drag_source_set_actions (drag_source, GDK_ACTION_MOVE);
  g_signal_connect_swapped (drag_source, "prepare", G_CALLBACK (on_drag_prepare), self);
  g_signal_connect_swapped (drag_source, "drag-begin", G_CALLBACK (on_drag_begin), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drag_source));

  drop_target = gtk_drop_target_new (MS_TYPE_OSK_LAYOUT_ROW, GDK_ACTION_MOVE);
  g_signal_connect_swapped (drop_target, "drop", G_CALLBACK (on_drop), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drop_target));
}


GtkWidget *
ms_osk_layout_row_new (MsOskLayout *layout)
{
  return g_object_new (MS_TYPE_OSK_LAYOUT_ROW, "layout", layout, NULL);
}


MsOskLayout *
ms_osk_layout_row_get_layout (MsOskLayoutRow *self)
{
  g_assert (MS_IS_OSK_LAYOUT_ROW (self));

  return self->layout;
}
