/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-osk-layout-prefs"

#include "mobile-settings-config.h"

#include "ms-osk-add-layout-dialog.h"
#include "ms-osk-layout-prefs.h"
#include "ms-osk-layout-row.h"
#include "ms-osk-layout.h"

#include <json-glib/json-glib.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-xkb-info.h>

#include <gtk/gtk.h>

/**
 * MsOskLayoutPrefs:
 *
 * A preferences prefs managing OSK layouts
 */

#define INPUT_SOURCES_SETTINGS "org.gnome.desktop.input-sources"
#define SOURCES_KEY            "sources"

struct _MsOskLayoutPrefs {
  AdwPreferencesGroup   parent;

  GtkWidget            *layouts_preferences_group;
  GtkWidget            *layouts_list_box;

  /* Currently configured layouts in input-sources */
  GListStore           *source_layouts;
  GSettings            *input_source_settings;
  GVariant             *sources;
  GnomeXkbInfo         *xkbinfo;

  /* All layouts the OSK supports */
  GListStore           *available_layouts;
  GHashTable           *available_layouts_by_id;

  /* Enabled layouts in the input sources that the OSK supports */
  GtkFilterListModel   *usable_layouts;
  GtkCustomFilter      *usable_filter;

  GCancellable         *cancel;
};
G_DEFINE_TYPE (MsOskLayoutPrefs, ms_osk_layout_prefs, ADW_TYPE_PREFERENCES_GROUP)


static void
update_enabled_move_actions (MsOskLayoutPrefs *self)
{
  GtkWidget *child;

  for (child = gtk_widget_get_first_child (GTK_WIDGET (self->layouts_list_box));
       child != NULL;
       child = gtk_widget_get_next_sibling (child)) {
    gint row_idx;
    GtkWidget *sibling;

    if (!MS_IS_OSK_LAYOUT_ROW (child))
      continue;

    row_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (child));
    gtk_widget_action_set_enabled (GTK_WIDGET (child), "row.move-up", row_idx != 0);


    sibling = gtk_widget_get_next_sibling (GTK_WIDGET (child));
    gtk_widget_action_set_enabled (GTK_WIDGET (child), "row.move-down",
                                   MS_IS_OSK_LAYOUT_ROW (sibling));
  }
}


static void
update_remove_actions (MsOskLayoutPrefs *self)
{
  GtkWidget *child;
  gboolean enable;

  enable = g_list_model_get_n_items (G_LIST_MODEL (self->usable_layouts)) > 1;

  for (child = gtk_widget_get_first_child (GTK_WIDGET (self->layouts_list_box));
       child != NULL;
       child = gtk_widget_get_next_sibling (child)) {

    if (!MS_IS_OSK_LAYOUT_ROW (child))
      continue;

    gtk_widget_action_set_enabled (GTK_WIDGET (child), "row.remove", enable);
  }
}


static char *
get_osk_layout_name (MsOskLayoutPrefs *self, const char *type, const char *layout_id)
{
  MsOskLayout *layout = NULL;
  g_autofree char *key = NULL;

  key = g_strdup_printf ("%s:%s", type, layout_id);
  layout = g_hash_table_lookup (self->available_layouts_by_id, key);

  if (!layout)
    return NULL;

  return g_strdup (ms_osk_layout_get_name (layout));
}


static void
on_input_sources_changed (MsOskLayoutPrefs *self, const char *unused, GSettings *settings)
{
  const char *type, *layout_id;
  GVariantIter iter;

  g_debug ("Setting changed, reloading input settings");

  g_clear_pointer (&self->sources, g_variant_unref);
  self->sources = g_settings_get_value (settings, SOURCES_KEY);

  g_list_store_remove_all (self->source_layouts);

  g_variant_iter_init (&iter, self->sources);
  while (g_variant_iter_next (&iter, "(&s&s)", &type, &layout_id)) {
    g_autoptr (MsOskLayout) layout = NULL;
    g_autofree char *name = NULL;

    name = get_osk_layout_name (self, type, layout_id);
    if (!name)
      g_debug ("Failed to get name for %s %s", type, layout_id);

    /* Even without a name we don't drop a layout as the user
     * might want to use those in docked mode */

    layout = ms_osk_layout_new (name, type, layout_id);
    g_list_store_append (self->source_layouts, layout);
  }

  gtk_filter_changed (GTK_FILTER (self->usable_filter), GTK_FILTER_CHANGE_DIFFERENT);
  update_enabled_move_actions (self);
  update_remove_actions (self);
}


static void
update_input_sources (MsOskLayoutPrefs *self)
{
  guint n_layouts;
  GVariantBuilder builder;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(ss)"));

  n_layouts = g_list_model_get_n_items (G_LIST_MODEL (self->source_layouts));
  for (guint i = 0; i < n_layouts; i++) {
    MsOskLayout *layout = g_list_model_get_item (G_LIST_MODEL (self->source_layouts), i);

    g_variant_builder_add (&builder, "(ss)",
                           ms_osk_layout_get_type_ (layout),
                           ms_osk_layout_get_id (layout));
  }

  g_settings_set_value (self->input_source_settings, SOURCES_KEY, g_variant_builder_end (&builder));
}


static void
on_layout_selected (MsOskLayoutPrefs *self, MsOskLayout *layout)
{
  g_assert (MS_IS_OSK_LAYOUT_PREFS (self));
  g_assert (MS_IS_OSK_LAYOUT (layout));

  g_signal_handlers_block_by_func (self->input_source_settings,
                                   on_input_sources_changed,
                                   self);

  g_list_store_append (self->source_layouts, layout);
  update_input_sources (self);

  g_signal_handlers_unblock_by_func (self->input_source_settings,
                                     on_input_sources_changed,
                                     self);

  on_input_sources_changed (self, NULL, self->input_source_settings);
}


static gboolean
is_source_layout (MsOskLayoutPrefs *self, MsOskLayout *layout)
{
  const char *type, *layout_id;
  GVariantIter iter;

  g_variant_iter_init (&iter, self->sources);
  while (g_variant_iter_next (&iter, "(&s&s)", &type, &layout_id)) {
    if (g_str_equal (type, ms_osk_layout_get_type_ (layout)) &&
        g_str_equal (layout_id, ms_osk_layout_get_id (layout))) {
      return TRUE;
    }
  }

  return FALSE;
}


static void
on_add_layout_activated (GtkWidget *widget, const char *action_name, GVariant *parameter)
{
  MsOskLayoutPrefs *self = MS_OSK_LAYOUT_PREFS (widget);
  GtkWidget *dialog;
  g_autoptr (GListStore) layouts = g_list_store_new (MS_TYPE_OSK_LAYOUT);
  guint n_items;

  n_items = g_list_model_get_n_items (G_LIST_MODEL (self->available_layouts));
  for (int i = 0; i < n_items; i++) {
    g_autoptr (MsOskLayout) layout = NULL;

    layout = g_list_model_get_item (G_LIST_MODEL (self->available_layouts), i);
    if (is_source_layout (self, layout))
      continue;

    g_list_store_append (layouts, layout);
  }

  dialog = ms_osk_add_layout_dialog_new (G_LIST_MODEL (layouts));
  g_signal_connect_object (dialog,
                           "layout-selected",
                           G_CALLBACK (on_layout_selected),
                           self,
                           G_CONNECT_SWAPPED);

  adw_dialog_present (ADW_DIALOG (dialog), widget);
}


static gboolean
filter_usable (MsOskLayout *layout, MsOskLayoutPrefs *self)
{
  g_autofree char *key = NULL;

  /* Assume all are usable if we couldn't get layout info */
  if (!self->available_layouts)
    return TRUE;

  key = g_strdup_printf ("%s:%s",
                         ms_osk_layout_get_type_ (layout),
                         ms_osk_layout_get_id (layout));
  return g_hash_table_contains (self->available_layouts_by_id, key);
}



static void
on_row_moved (MsOskLayoutPrefs *self, MsOskLayoutRow *dest_row, MsOskLayoutRow *row)
{
  guint idx, dest_idx;
  MsOskLayout *layout, *dest_layout;
  gboolean success;

  layout = ms_osk_layout_row_get_layout (row);
  g_assert (layout);
  success = g_list_store_find (self->source_layouts, layout, &idx);
  g_assert (success);

  dest_layout = ms_osk_layout_row_get_layout (dest_row);
  g_assert (dest_layout);
  success = g_list_store_find (self->source_layouts, dest_layout, &dest_idx);
  g_assert (success);

  g_signal_handlers_block_by_func (self->input_source_settings,
                                   on_input_sources_changed,
                                   self);
  /* The row holds a ref on layout so it won't get destroyed on
   * removal */
  g_list_store_remove (self->source_layouts, idx);
  g_list_store_insert (self->source_layouts, dest_idx, layout);

  update_input_sources (self);

  g_signal_handlers_unblock_by_func (self->input_source_settings,
                                     on_input_sources_changed,
                                     self);

  on_input_sources_changed (self, NULL, self->input_source_settings);
}


static void
on_row_removed (MsOskLayoutPrefs *self, MsOskLayoutRow *dest_row, MsOskLayoutRow *row)
{
  guint idx;
  MsOskLayout *layout;
  gboolean success;

  layout = ms_osk_layout_row_get_layout (row);
  g_assert (layout);
  success = g_list_store_find (self->source_layouts, layout, &idx);
  g_assert (success);

  g_signal_handlers_block_by_func (self->input_source_settings,
                                   on_input_sources_changed,
                                   self);
  /* The row holds a ref on layout so it won't get destroyed on
   * removal */
  g_list_store_remove (self->source_layouts, idx);

  update_input_sources (self);

  g_signal_handlers_unblock_by_func (self->input_source_settings,
                                     on_input_sources_changed,
                                     self);

  on_input_sources_changed (self, NULL, self->input_source_settings);
}


static GtkWidget *
create_layout_row (gpointer item, gpointer user_data)
{
  MsOskLayoutPrefs *self = MS_OSK_LAYOUT_PREFS (user_data);
  MsOskLayout *layout = MS_OSK_LAYOUT (item);
  GtkWidget *row;

  row = ms_osk_layout_row_new (layout);
  g_signal_connect_object (row, "move-row", G_CALLBACK (on_row_moved), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (row, "remove-row",
                           G_CALLBACK (on_row_removed),
                           self,
                           G_CONNECT_SWAPPED);

  return row;
}


static void
on_load_osk_layouts_from_stream_ready (JsonParser *parser, GAsyncResult *res, gpointer user_data)
{
  gboolean success;
  g_autoptr (GError) err = NULL;
  MsOskLayoutPrefs *self = NULL;
  JsonNode *node;
  JsonObject *obj;
  JsonArray *layouts;

  success = json_parser_load_from_stream_finish  (parser, res, &err);
  if (!success) {
    g_warning ("Failed to load layouts: %s", err->message);
    return;
  }

  self = MS_OSK_LAYOUT_PREFS (user_data);
  g_assert (MS_IS_OSK_LAYOUT_PREFS (self));

  node = json_parser_get_root (parser);

  obj = json_node_get_object (node);
  if (!obj) {
    g_warning ("Failed to get root object");
    return;
  }

  layouts = json_object_get_array_member (obj, "layouts");
  if (!obj) {
    g_warning ("Failed to get array of layouts");
    return;
  }

  for (int i = 0; i < json_array_get_length (layouts); i++) {
    g_autoptr (MsOskLayout) layout = NULL;
    JsonObject *layout_obj;
    const char *name, *type, *layout_id, *display_name;
    g_autofree char *key = NULL, *layout_name = NULL;

    layout_obj = json_array_get_object_element (layouts, i);
    g_assert (layout_obj);

    name = json_object_get_string_member (layout_obj, "name");
    type = json_object_get_string_member (layout_obj, "type");
    layout_id = json_object_get_string_member (layout_obj, "layout-id");

    if (!name || !type || !layout_id) {
      g_warning ("Skipping layout %d", i);
      continue;
    }

    /* Get a nicer (translated) display name for xkb layouts */
    if (g_str_equal (type, "xkb")) {
      if (gnome_xkb_info_get_layout_info (self->xkbinfo, layout_id, &display_name,
                                          NULL, NULL, NULL)) {
        layout_name = g_strdup (display_name);
      }
    }

    if (!layout_name)
      layout_name = g_strdup (name);

    g_debug ("Adding layout %s", layout_name);
    layout = ms_osk_layout_new (layout_name, type, layout_id);
    g_list_store_append (self->available_layouts, layout);

    key = g_strdup_printf ("%s:%s", type, layout_id);
    g_hash_table_insert (self->available_layouts_by_id,
                         g_steal_pointer (&key),
                         g_steal_pointer (&layout));
  }

  /* Reload the layouts as we got more layout information */
  on_input_sources_changed (self, SOURCES_KEY, self->input_source_settings);
}


static void
ms_osk_layout_prefs_dispose (GObject *object)
{
  MsOskLayoutPrefs *self = MS_OSK_LAYOUT_PREFS (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_object (&self->usable_filter);
  g_clear_object (&self->usable_layouts);

  g_clear_pointer (&self->available_layouts_by_id, g_hash_table_destroy);
  g_clear_object (&self->available_layouts);

  g_clear_object (&self->xkbinfo);
  g_clear_pointer (&self->sources, g_variant_unref);
  g_clear_object (&self->input_source_settings);
  g_clear_object (&self->source_layouts);

  G_OBJECT_CLASS (ms_osk_layout_prefs_parent_class)->dispose (object);
}


static void
ms_osk_layout_prefs_class_init (MsOskLayoutPrefsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = ms_osk_layout_prefs_dispose;


  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/"
                                               "ms-osk-layout-prefs.ui");

  gtk_widget_class_install_action (widget_class, "osk-prefs.add-layout", NULL,
                                   on_add_layout_activated);

  gtk_widget_class_bind_template_child (widget_class, MsOskLayoutPrefs, layouts_list_box);
}


static void
ms_osk_layout_prefs_init (MsOskLayoutPrefs *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->cancel = g_cancellable_new ();
  self->xkbinfo = gnome_xkb_info_new ();

  self->available_layouts = g_list_store_new (MS_TYPE_OSK_LAYOUT);
  self->available_layouts_by_id = g_hash_table_new_full (g_str_hash,
                                                         g_str_equal,
                                                         g_free,
                                                         g_object_unref);

  self->source_layouts = g_list_store_new (MS_TYPE_OSK_LAYOUT);
  self->usable_filter = gtk_custom_filter_new ((GtkCustomFilterFunc)filter_usable, self, NULL);
  g_object_ref (self->source_layouts);
  g_object_ref (self->usable_filter);
  self->usable_layouts = gtk_filter_list_model_new (G_LIST_MODEL (self->source_layouts),
                                                    GTK_FILTER (self->usable_filter));
  gtk_list_box_bind_model (GTK_LIST_BOX (self->layouts_list_box),
                           G_LIST_MODEL (self->usable_layouts),
                           create_layout_row,
                           self,
                           NULL);

  self->input_source_settings = g_settings_new (INPUT_SOURCES_SETTINGS);
  g_signal_connect_swapped (self->input_source_settings, "changed::" SOURCES_KEY,
                            G_CALLBACK (on_input_sources_changed),
                            self);
  on_input_sources_changed (self, SOURCES_KEY, self->input_source_settings);
}


MsOskLayoutPrefs *
ms_osk_layout_prefs_new (void)
{
  return g_object_new (MS_TYPE_OSK_LAYOUT_PREFS, NULL);
}

/**
 * ms_osk_layout_prefs_load_osk_layouts:
 * @self: The OSK layout prefs
 *
 * Load the layouts the currently running OSK can handle.
 */
void
ms_osk_layout_prefs_load_osk_layouts (MsOskLayoutPrefs *self)
{
  const char *layouts_path;
  g_autoptr (GFile) layouts_file = NULL;
  g_autoptr (JsonParser) parser = json_parser_new_immutable ();
  g_autoptr (GFileInputStream) stream = NULL;
  g_autoptr (GError) err = NULL;

  layouts_path = getenv ("MS_OSK_LAYOUTS") ?: MOBILE_SETTINGS_OSK_LAYOUTS;
  layouts_file = g_file_new_for_path (layouts_path);

  stream = g_file_read (layouts_file, self->cancel, &err);
  if (!stream) {
    g_warning ("Can't load keyboard layouts: %s", err->message);
    return;
  }

  json_parser_load_from_stream_async (parser,
                                      G_INPUT_STREAM (stream),
                                      self->cancel,
                                      (GAsyncReadyCallback)on_load_osk_layouts_from_stream_ready,
                                      self);
}
