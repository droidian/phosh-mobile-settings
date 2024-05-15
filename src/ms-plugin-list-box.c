/*
 * Copyright (C) 2024 Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-plugin-list_box"

#include "mobile-settings-config.h"

#include "ms-plugin-list-box.h"
#include "ms-plugin-row.h"

#define PHOSH_PLUGINS_SCHEMA_ID "sm.puri.phosh.plugins"

#define PHOSH_PLUGIN_PREFIX ""
#define PHOSH_PLUGIN_SUFFIX ".plugin"

/**
 * MsPluginList_box:
 *
 * A List box to enable and sort Phosh plugins
 */

enum {
  PROP_0,
  PROP_PLUGIN_TYPE,
  PROP_SETTINGS_KEY,
  PROP_PREFS_EXTENSION_POINT,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsPluginListBox {
  AdwBin              parent;

  GSettings          *settings;
  char               *settings_key;
  GtkWidget          *list_box;
  GListStore         *store;

  MsPluginRow        *selected_row;

  GSimpleActionGroup *action_group;

  char               *plugin_type;
  char               *prefs_extension_point;
};
G_DEFINE_TYPE (MsPluginListBox, ms_plugin_list_box, ADW_TYPE_BIN)



static AdwPreferencesWindow *
load_prefs_window (MsPluginListBox *self, const char *name)
{
  GIOExtensionPoint *ep;
  GIOExtension *ext;
  GType type;

  ep = g_io_extension_point_lookup (self->prefs_extension_point);
  g_return_val_if_fail (ep, NULL);

  ext = g_io_extension_point_get_extension_by_name (ep, name);

  g_debug ("Loading plugin %s", name);
  type = g_io_extension_get_type (ext);
  return g_object_new (type, NULL);
}


static void
open_plugin_prefs_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  MsPluginListBox *self = MS_PLUGIN_LIST_BOX (data);
  AdwPreferencesWindow *prefs;
  GtkWindow *parent;
  g_autoptr (GError) error = NULL;
  g_autoptr (GKeyFile) keyfile = g_key_file_new ();
  g_autofree char *name = NULL;
  const char *filename;

  g_variant_get (parameter, "&s", &filename);
  g_assert (filename);
  g_debug ("Prefs for'%s' activated", filename);

  if (g_key_file_load_from_file (keyfile, filename, G_KEY_FILE_NONE, &error) == FALSE) {
    g_warning ("Failed to load prefs plugin info '%s': %s", filename, error->message);
    return;
  }

  name = g_key_file_get_string (keyfile, "Prefs", "Id", NULL);
  parent = gtk_application_get_active_window (
    GTK_APPLICATION (g_application_get_default ()));
  g_assert (parent);

  prefs = load_prefs_window (self, name);
  g_return_if_fail (prefs);

  gtk_window_set_transient_for (GTK_WINDOW (prefs), parent);
  gtk_window_present (GTK_WINDOW (prefs));
}


static gboolean
is_plugin_name_valid (const char *name)
{
  if (name == NULL)
    return FALSE;

  if (name[0] == '\0')
    return FALSE;

  /* Absolute path is not allowed */
  if (name[0] == '/')
    return FALSE;

  return TRUE;
}


static void
save_plugin_store (MsPluginListBox *self)
{
  g_auto (GStrv) ret = NULL;
  g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();

  guint n_plugins = g_list_model_get_n_items (G_LIST_MODEL (self->store));

  for (guint i = 0; i < n_plugins; i++) {
    MsPluginRow *plugin_row = g_list_model_get_item (G_LIST_MODEL (self->store), i);
    gboolean enabled = ms_plugin_row_get_enabled (plugin_row);
    const char *name = ms_plugin_row_get_name (plugin_row);

    g_debug ("Plugin: %s: %d", name, enabled);

    if (enabled) {
      if (!is_plugin_name_valid (name)) {
        g_warning ("Plugin name '%s' invalid, dropping", name);
        continue;
      }
      g_strv_builder_add (builder, name);
    }
  }
  ret = g_strv_builder_end (builder);
  g_settings_set_strv (self->settings, self->settings_key, (const gchar * const *)ret);
}


static void
update_enabled_move_actions (MsPluginListBox *self)
{
  GtkWidget *child;

  for (child = gtk_widget_get_first_child (GTK_WIDGET (self->list_box));
       child != NULL;
       child = gtk_widget_get_next_sibling (child)) {
    gint row_idx;

    if (!MS_IS_PLUGIN_ROW (child))
      continue;

    row_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (child));
    gtk_widget_action_set_enabled (GTK_WIDGET (child), "row.move-up", row_idx != 0);
    gtk_widget_action_set_enabled (GTK_WIDGET (child), "row.move-down",
                                   gtk_widget_get_next_sibling (GTK_WIDGET (child)) != NULL);
  }
}


static void
row_move_selected (MsPluginListBox *self, gboolean down)
{
  gint selected_idx, dest_idx;
  MsPluginRow *plugin_row = NULL;

  selected_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self->selected_row));
  dest_idx = down ? selected_idx + 1 : selected_idx - 1;

  plugin_row = g_list_model_get_item (G_LIST_MODEL (self->store), selected_idx);
  g_list_store_remove (self->store, selected_idx);
  g_list_store_insert (self->store, dest_idx, plugin_row);

  update_enabled_move_actions (self);
}


static void
on_row_moved (MsPluginListBox *self, MsPluginRow *dest_row, MsPluginRow *row)
{
  gint source_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (row));
  gint dest_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (dest_row));
  gboolean down;

  self->selected_row = row;

  down = (source_idx - dest_idx) < 0;
  for (int i = 0; i < ABS (source_idx - dest_idx); i++)
    row_move_selected (self, down);

  save_plugin_store (self);
}


static void
on_plugin_activated (MsPluginListBox *self, GParamSpec *pspec, MsPluginRow *row)
{
  save_plugin_store (self);
}


/**
 * sort_plugins_store:
 *
 * Update the plugins store to match the order in phosh
 */
static void
sort_plugins_store (MsPluginListBox *self)
{
  g_auto (GStrv) plugins_order = g_settings_get_strv (self->settings, self->settings_key);

  for (int i = 0; i < g_strv_length (plugins_order); i++) {
    for (int j = 0; j < g_list_model_get_n_items (G_LIST_MODEL (self->store)); j++) {
      MsPluginRow *plugin_row = g_list_model_get_item (G_LIST_MODEL (self->store), j);

      if (g_strcmp0 (plugins_order[i], ms_plugin_row_get_name (plugin_row)) == 0) {
        g_list_store_remove (self->store, j);
        g_list_store_insert (self->store, i, plugin_row);
        break;
      }
    }
  }
}


static void
ms_plugin_list_box_scan_phosh_plugins (MsPluginListBox *self)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (GDir) dir = g_dir_open (MOBILE_SETTINGS_PHOSH_PLUGINS_DIR, 0, &err);
  const char *filename;
  g_auto (GStrv) enabled_plugins = NULL;


  if (dir == NULL) {
    g_warning ("Failed to read phosh plugins from " MOBILE_SETTINGS_PHOSH_PLUGINS_DIR ": %s",
               err->message);
    return;
  }

  enabled_plugins = g_settings_get_strv (self->settings, self->settings_key);
  while ((filename = g_dir_read_name (dir))) {
    GtkWidget *row;
    gboolean enabled;
    g_autofree char *name = NULL;
    g_autofree char *title = NULL;
    g_autofree char *description = NULL;
    g_autofree char *path = NULL;
    g_autofree char *plugin_path = NULL;
    g_autofree char *prefs_path = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GKeyFile) keyfile = g_key_file_new ();
    g_auto (GStrv) types = NULL;

    if (!g_str_has_prefix (filename, PHOSH_PLUGIN_PREFIX) ||
        !g_str_has_suffix (filename, PHOSH_PLUGIN_SUFFIX))
      continue;

    path = g_build_filename (MOBILE_SETTINGS_PHOSH_PLUGINS_DIR, filename, NULL);
    if (g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, &error) == FALSE) {
      g_warning ("Failed to load plugin info '%s': %s", filename, error->message);
      continue;
    }

    name = g_key_file_get_string (keyfile, "Plugin", "Id", NULL);
    if (name == NULL)
      continue;

    plugin_path = g_key_file_get_string (keyfile, "Plugin", "Plugin", NULL);
    if (plugin_path == NULL)
      continue;

    if (g_file_test (plugin_path, G_FILE_TEST_EXISTS) == FALSE) {
      g_warning ("Plugin at %s does not exist", plugin_path);
      continue;
    }

    prefs_path = g_key_file_get_string (keyfile, "Prefs", "Plugin", NULL);

    title = g_key_file_get_locale_string (keyfile, "Plugin", "Name", NULL, NULL);
    description = g_key_file_get_locale_string (keyfile, "Plugin", "Comment", NULL, NULL);
    types = g_key_file_get_string_list (keyfile, "Plugin", "Types", NULL, NULL);

    if (types == NULL)
      g_warning ("Plugin '%s' has no type. Please fix", name);

    if (!g_strv_contains ((const char *const *)types, self->plugin_type))
      continue;

    enabled = g_strv_contains ((const gchar * const*)enabled_plugins, name);
    g_debug ("Found plugin %s, name %s, enabled: %d, prefs: %d", filename, name, enabled,
             !!prefs_path);
    row = g_object_new (MS_TYPE_PLUGIN_ROW,
                        "plugin-name", name,
                        "title", title,
                        "subtitle", description,
                        "enabled", enabled,
                        "has-prefs", !!prefs_path,
                        "filename", path,
                        NULL);
    g_signal_connect_object (row,
                             "notify::enabled",
                             G_CALLBACK (on_plugin_activated),
                             self,
                             G_CONNECT_SWAPPED);

    g_list_store_append (self->store, row);

    g_signal_connect_object (row, "move-row",
                             G_CALLBACK (on_row_moved), self,
                             G_CONNECT_SWAPPED);
  }
  sort_plugins_store (self);
}


static void
ms_plugin_list_box_set_settings_key (MsPluginListBox *self, const char *key)
{
  self->settings_key = g_strdup (key);
  ms_plugin_list_box_scan_phosh_plugins (self);
}


static void
ms_plugin_list_box_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MsPluginListBox *self = MS_PLUGIN_LIST_BOX (object);

  switch (property_id) {
  case PROP_PLUGIN_TYPE:
    self->plugin_type = g_value_dup_string (value);
    break;
  case PROP_SETTINGS_KEY:
    ms_plugin_list_box_set_settings_key (self, g_value_get_string (value));
    break;
  case PROP_PREFS_EXTENSION_POINT:
    self->prefs_extension_point = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_plugin_list_box_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MsPluginListBox *self = MS_PLUGIN_LIST_BOX (object);

  switch (property_id) {
  case PROP_PLUGIN_TYPE:
    g_value_set_string (value, self->plugin_type);
    break;
  case PROP_SETTINGS_KEY:
    g_value_set_string (value, self->settings_key);
    break;
  case PROP_PREFS_EXTENSION_POINT:
    g_value_set_string (value, self->prefs_extension_point);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_plugin_list_box_finalize (GObject *object)
{
  MsPluginListBox *self = MS_PLUGIN_LIST_BOX (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->store);
  g_clear_object (&self->action_group);

  g_clear_pointer (&self->prefs_extension_point, g_free);
  g_clear_pointer (&self->settings_key, g_free);
  g_clear_pointer (&self->plugin_type, g_free);

  G_OBJECT_CLASS (ms_plugin_list_box_parent_class)->finalize (object);
}


static void
ms_plugin_list_box_class_init (MsPluginListBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ms_plugin_list_box_get_property;
  object_class->set_property = ms_plugin_list_box_set_property;
  object_class->finalize = ms_plugin_list_box_finalize;

  /**
   * MsPluginListBox:plugin-type:
   *
   * The type of plugins in this store
   */
  props[PROP_PLUGIN_TYPE] =
    g_param_spec_string ("plugin-type", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * MsPluginListBox:settings-key:
   *
   * The settings key in Phosh's schema that lists the enabled plugins
   */
  props[PROP_SETTINGS_KEY] =
    g_param_spec_string ("settings-key", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * MsPluginListBox:prefs-extension-point:
   *
   * The extension point for plugin preferences
   */
  props[PROP_PREFS_EXTENSION_POINT] =
    g_param_spec_string ("prefs-extension-point", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static GtkWidget *
create_row (gpointer object, gpointer user_data)
{
  return GTK_WIDGET (object);
}


static GActionEntry entries[] =
{
  { .name = "open-plugin-prefs", .parameter_type = "s", .activate = open_plugin_prefs_activated },
};


static void
ms_plugin_list_box_init (MsPluginListBox *self)
{
  /* Scan prefs plugins */
  g_io_modules_scan_all_in_directory (MOBILE_SETTINGS_PHOSH_PREFS_DIR);

  self->list_box = gtk_list_box_new ();
  adw_bin_set_child (ADW_BIN (self), self->list_box);
  gtk_widget_add_css_class (self->list_box, "boxed-list");

  self->settings = g_settings_new (PHOSH_PLUGINS_SCHEMA_ID);
  self->store = g_list_store_new (MS_TYPE_PLUGIN_ROW);

  gtk_list_box_bind_model (GTK_LIST_BOX (self->list_box),
                           G_LIST_MODEL (self->store),
                           create_row,
                           self, NULL);

  self->action_group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (self->action_group),
                                   entries,
                                   G_N_ELEMENTS (entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "plugin-list-box",
                                  G_ACTION_GROUP (self->action_group));

}


MsPluginListBox *
ms_plugin_list_box_new (void)
{
  return g_object_new (MS_TYPE_PLUGIN_LIST_BOX, NULL);
}
