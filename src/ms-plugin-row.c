/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-plugin-row"

#include "mobile-settings-config.h"

#include "ms-plugin-row.h"

#include "gtk/gtk.h"

enum {
  PROP_0,
  PROP_PLUGIN_NAME,
  PROP_SUBTITLE,
  PROP_ENABLED,
  PROP_FILENAME,
  PROP_HAS_PREFS,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


enum {
  SIGNAL_MOVE_ROW,
  SIGNAL_LAST
};
static guint signals[SIGNAL_LAST];


struct _MsPluginRow {
  AdwActionRow parent;

  GtkSwitch   *toggle;
  GtkWidget   *prefs;

  char        *name;
  char        *subtitle;
  char        *filename;
  gboolean     enabled;
  gboolean     has_prefs;

  GtkListBox  *drag_widget;
  gdouble      drag_x;
  gdouble      drag_y;

  GSimpleActionGroup *action_group;
};
G_DEFINE_TYPE (MsPluginRow, ms_plugin_row, ADW_TYPE_ACTION_ROW)


static void
on_open_prefs_activated (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       data)
{
  MsPluginRow *self = MS_PLUGIN_ROW (data);

  gtk_widget_activate_action (GTK_WIDGET (self),
                              "plugin-list-box.open-plugin-prefs",
                              "s", self->filename);
}


static void
ms_plugin_row_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  MsPluginRow *self = MS_PLUGIN_ROW (object);

  switch (property_id) {
  case PROP_PLUGIN_NAME:
    self->name = g_value_dup_string (value);
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (self), self->name);
    break;
  case PROP_SUBTITLE:
    self->subtitle = g_value_dup_string (value);
    adw_action_row_set_subtitle (ADW_ACTION_ROW (self), self->subtitle);
    break;
  case PROP_FILENAME:
    self->filename = g_value_dup_string (value);
    break;
  case PROP_ENABLED:
    self->enabled = g_value_get_boolean (value);
    break;
  case PROP_HAS_PREFS:
    self->has_prefs = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_plugin_row_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MsPluginRow *self = MS_PLUGIN_ROW (object);

  switch (property_id) {
  case PROP_PLUGIN_NAME:
    g_value_set_string (value, ms_plugin_row_get_name (self));
    break;
  case PROP_SUBTITLE:
    g_value_set_string (value, self->subtitle);
    break;
  case PROP_FILENAME:
    g_value_set_string (value, self->filename);
    break;
  case PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  case PROP_HAS_PREFS:
    g_value_set_boolean (value, self->has_prefs);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
update_move_actions_after_row_moved_up (MsPluginRow *self)
{
  GtkListBox *list_box = GTK_LIST_BOX (gtk_widget_get_parent (GTK_WIDGET (self)));
  gint previous_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self)) - 1;
  GtkListBoxRow *previous_row = gtk_list_box_get_row_at_index (list_box, previous_idx);

  if (gtk_list_box_get_row_at_index (list_box, previous_idx - 1) == NULL) {
    gtk_widget_action_set_enabled (GTK_WIDGET (self), "row.move-up", FALSE);
  }

  gtk_widget_action_set_enabled (GTK_WIDGET (previous_row), "row.move-up", TRUE);
  gtk_widget_action_set_enabled (GTK_WIDGET (previous_row), "row.move-down",
                                 gtk_widget_get_next_sibling (GTK_WIDGET (self)) != NULL);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "row.move-down", TRUE);
}


static void
update_move_actions_after_row_moved_down (MsPluginRow *self)
{
  GtkListBox *list_box = GTK_LIST_BOX (gtk_widget_get_parent (GTK_WIDGET (self)));
  gint next_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self)) + 1;
  GtkListBoxRow *next_row = gtk_list_box_get_row_at_index (list_box, next_idx);

  if (gtk_widget_get_next_sibling (GTK_WIDGET (next_row)) == NULL) {
    gtk_widget_action_set_enabled (GTK_WIDGET (self), "row.move-down", FALSE);
  }

  gtk_widget_action_set_enabled (GTK_WIDGET (next_row), "row.move-up", next_idx-1 != 0);
  gtk_widget_action_set_enabled (GTK_WIDGET (next_row), "row.move-down", TRUE);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "row.move-up", TRUE);
}


static void
on_move_up_activated (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  MsPluginRow *self = MS_PLUGIN_ROW (user_data);
  GtkListBox *list_box = GTK_LIST_BOX (gtk_widget_get_parent (GTK_WIDGET (self)));
  gint previous_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self)) - 1;
  GtkListBoxRow *previous_row = gtk_list_box_get_row_at_index (list_box, previous_idx);

  if (previous_row == NULL)
    return;

  update_move_actions_after_row_moved_up (self);

  g_signal_emit (self, signals[SIGNAL_MOVE_ROW], 0, previous_row);
}


static void
on_move_down_activated (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
  MsPluginRow *self = MS_PLUGIN_ROW (user_data);
  GtkListBox *list_box = GTK_LIST_BOX (gtk_widget_get_parent (GTK_WIDGET (self)));
  gint next_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self)) + 1;
  GtkListBoxRow *next_row = gtk_list_box_get_row_at_index (list_box, next_idx);

  if (next_row == NULL)
    return;

  update_move_actions_after_row_moved_down (self);

  g_signal_emit (next_row, signals[SIGNAL_MOVE_ROW], 0, self);
}


static GdkContentProvider *
on_drag_prepare (MsPluginRow *self, double x, double y)
{
  self->drag_x = x;
  self->drag_y = y;

  return gdk_content_provider_new_typed (MS_TYPE_PLUGIN_ROW, self);
}


static void
on_drag_begin (MsPluginRow *self, GdkDrag *drag)
{
  MsPluginRow *plugin_row;
  GtkWidget *drag_icon;

  self->drag_widget = GTK_LIST_BOX (gtk_list_box_new ());

  plugin_row = g_object_new (MS_TYPE_PLUGIN_ROW,
                             "plugin-name", self->name,
                             "subtitle", self->subtitle,
                             "enabled", self->enabled,
                             "filename", self->filename,
                             "has-prefs", self->has_prefs,
                             NULL);

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
on_drop (MsPluginRow *self, const GValue *value, gdouble x, gdouble y)
{
  MsPluginRow *source;

  g_debug ("Droped");

  if (!G_VALUE_HOLDS (value, MS_TYPE_PLUGIN_ROW))
    return FALSE;

  source = g_value_get_object (value);

  g_signal_emit (source, signals[SIGNAL_MOVE_ROW], 0, self);

  return TRUE;
}


static void
ms_plugin_row_finalize (GObject *object)
{
  MsPluginRow *self = MS_PLUGIN_ROW (object);

  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->filename, g_free);
  g_clear_object (&self->action_group);

  G_OBJECT_CLASS (ms_plugin_row_parent_class)->finalize (object);
}


static void
ms_plugin_row_class_init (MsPluginRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ms_plugin_row_get_property;
  object_class->set_property = ms_plugin_row_set_property;
  object_class->finalize = ms_plugin_row_finalize;

  props[PROP_PLUGIN_NAME] =
    g_param_spec_string ("plugin-name", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_SUBTITLE] =
    g_param_spec_string ("subtitle", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /**
   * MsPluginRow:filename:
   *
   * Name of the file with information about the plugin
   */
  props[PROP_FILENAME] =
    g_param_spec_string ("filename", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled", "", "",
                          FALSE,
                          G_PARAM_READWRITE);
  /**
   * MsPluginRow:has-prefs:
   *
   * Whether the plugin has a preferences dialog
   */
  props[PROP_HAS_PREFS] =
    g_param_spec_boolean ("has-prefs", "", "",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[SIGNAL_MOVE_ROW] =
    g_signal_new ("move-row",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, MS_TYPE_PLUGIN_ROW);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-plugin-row.ui");
  gtk_widget_class_bind_template_child (widget_class, MsPluginRow, toggle);
  gtk_widget_class_bind_template_child (widget_class, MsPluginRow, prefs);
}


static GActionEntry entries[] =
{
  { .name = "open-prefs", .activate = on_open_prefs_activated },
  { .name = "move-up", .activate = on_move_up_activated },
  { .name = "move-down", .activate = on_move_down_activated },
};


static void
ms_plugin_row_init (MsPluginRow *self)
{
  GAction *action;
  GtkDragSource *drag_source;
  GtkDropTarget *drop_target;

  gtk_widget_init_template (GTK_WIDGET (self));

  drag_source = gtk_drag_source_new ();
  gtk_drag_source_set_actions (drag_source, GDK_ACTION_MOVE);
  g_signal_connect_swapped (drag_source, "prepare", G_CALLBACK (on_drag_prepare), self);
  g_signal_connect_swapped (drag_source, "drag-begin", G_CALLBACK (on_drag_begin), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drag_source));

  drop_target = gtk_drop_target_new (MS_TYPE_PLUGIN_ROW, GDK_ACTION_MOVE);
  g_signal_connect_swapped (drop_target, "drop", G_CALLBACK (on_drop), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drop_target));

  g_object_bind_property (self,
                          "enabled",
                          self->toggle,
                          "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  self->action_group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (self->action_group),
                                   entries,
                                   G_N_ELEMENTS (entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "row",
                                  G_ACTION_GROUP (self->action_group));

  action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "open-prefs");
  g_object_bind_property (self, "has-prefs", action, "enabled", G_BINDING_SYNC_CREATE);
}


const char *
ms_plugin_row_get_name (MsPluginRow *self)
{
  g_return_val_if_fail (MS_IS_PLUGIN_ROW (self), NULL);

  return self->name;
}


gboolean
ms_plugin_row_get_enabled (MsPluginRow *self)
{
  g_return_val_if_fail (MS_IS_PLUGIN_ROW (self), FALSE);

  return self->enabled;
}
