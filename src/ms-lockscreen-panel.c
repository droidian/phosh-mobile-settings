/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-lockscreen-panel"

#include "mobile-settings-config.h"
#include "ms-lockscreen-panel.h"
#include "ms-plugin-row.h"

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>

#include <phosh-plugin.h>

/* Verbatim from phosh */
#define LOCKSCREEN_SCHEMA_ID "sm.puri.phosh.lockscreen"
#define LOCKSCREEN_KEY_SCALE_TO_FIT "shuffle-keypad"

#define LOCKSCREEN_PLUGINS_SCHEMA_ID "sm.puri.phosh.plugins"
#define LOCKSCREEN_PLUGINS_KEY "lock-screen"

#define PLUGIN_PREFIX ""
#define PLUGIN_SUFFIX ".plugin"

struct _MsLockscreenPanel {
  AdwBin      parent;

  GSettings  *settings;
  GtkWidget  *shuffle_switch;

  GSettings  *plugins_settings;
  GtkListBox *plugins_listbox;
  GListStore *plugins_store;

  MsPluginRow *selected_row;

  GSimpleActionGroup *action_group;
};

G_DEFINE_TYPE (MsLockscreenPanel, ms_lockscreen_panel, ADW_TYPE_BIN)


static AdwPreferencesWindow *
load_prefs_window (const char *name)
{
  GIOExtensionPoint *ep;
  GIOExtension *ext;
  GType type;

  ep = g_io_extension_point_lookup (PHOSH_PLUGIN_EXTENSION_POINT_LOCKSCREEN_WIDGET_PREFS);
  g_return_val_if_fail (ep, NULL);

  ext = g_io_extension_point_get_extension_by_name (ep, name);

  g_debug ("Loading plugin %s", name);
  type = g_io_extension_get_type (ext);
  return g_object_new (type, NULL);
}


static void
open_plugin_prefs_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  AdwPreferencesWindow *prefs;
  GtkWindow*parent;
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

  prefs = load_prefs_window (name);
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
save_plugin_store (MsLockscreenPanel *self)
{
  g_auto (GStrv) ret = NULL;
  g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();

  guint n_plugins = g_list_model_get_n_items (G_LIST_MODEL (self->plugins_store));

  for (guint i = 0; i < n_plugins; i++) {
    MsPluginRow *plugin_row = g_list_model_get_item (G_LIST_MODEL (self->plugins_store), i);
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
  g_settings_set_strv (self->plugins_settings, LOCKSCREEN_PLUGINS_KEY, (const gchar * const *)ret);
}


static void
lockscreen_panel_update_enabled_move_actions (MsLockscreenPanel *self)
{
  GtkWidget *child;

  for (child = gtk_widget_get_first_child (GTK_WIDGET (self->plugins_listbox));
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
lockscreen_panel_move_selected (MsLockscreenPanel *self,
                                gboolean           down)
{
  gint selected_idx, dest_idx;
  MsPluginRow *plugin_row = NULL;

  selected_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self->selected_row));
  dest_idx = down ? selected_idx + 1 : selected_idx - 1;

  plugin_row = g_list_model_get_item (G_LIST_MODEL (self->plugins_store), selected_idx);
  g_list_store_remove (self->plugins_store, selected_idx);
  g_list_store_insert (self->plugins_store, dest_idx, plugin_row);

  lockscreen_panel_update_enabled_move_actions (self);
}


static void
on_row_moved (MsLockscreenPanel  *self,
              MsPluginRow        *dest_row,
              MsPluginRow        *row)
{
  gint source_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (row));
  gint dest_idx = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (dest_row));
  gboolean down;

  self->selected_row = row;

  down = (source_idx - dest_idx) < 0;
  for (int i = 0; i < ABS (source_idx - dest_idx); i++)
    lockscreen_panel_move_selected (self, down);

  save_plugin_store (self);
}


static void
on_plugin_activated (MsLockscreenPanel *self, GParamSpec *pspec, MsPluginRow *row)
{
  save_plugin_store (self);
}


static GtkWidget *
create_plugins_row (gpointer object, gpointer user_data)
{
  return GTK_WIDGET (object);
}


 /* update the plugins_store to match the order on the lock-screen */
static void
sort_plugins_store (MsLockscreenPanel *self)
{
  g_auto (GStrv) plugins_order = g_settings_get_strv (self->plugins_settings, LOCKSCREEN_PLUGINS_KEY);

  for (int i = 0; i < g_strv_length (plugins_order); i++) {
    for (int j = 0; j < g_list_model_get_n_items (G_LIST_MODEL (self->plugins_store)); j++) {
      MsPluginRow *plugin_row = g_list_model_get_item (G_LIST_MODEL (self->plugins_store), j);

      if (g_strcmp0 (plugins_order[i], ms_plugin_row_get_name (plugin_row)) == 0) {
        g_list_store_remove (self->plugins_store, j);
        g_list_store_insert (self->plugins_store, i, plugin_row);
        break;
      }
    }
  }
}


static void
ms_lockscreen_panel_scan_phosh_lockscreen_plugins (MsLockscreenPanel *self)
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

  enabled_plugins = g_settings_get_strv (self->plugins_settings, LOCKSCREEN_PLUGINS_KEY);
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

    if (!g_str_has_prefix (filename, PLUGIN_PREFIX) || !g_str_has_suffix (filename, PLUGIN_SUFFIX))
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

    enabled = g_strv_contains ((const gchar * const*)enabled_plugins, name);
    g_debug ("Found plugin %s, name %s, enabled: %d, prefs: %d", filename, name, enabled, !!prefs_path);
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

    g_list_store_append (self->plugins_store, row);

    g_signal_connect_object (row, "move-row",
                             G_CALLBACK (on_row_moved), self,
                             G_CONNECT_SWAPPED);
  }
  sort_plugins_store (self);
}

static void
ms_lockscreen_panel_finalize (GObject *object)
{
  MsLockscreenPanel *self = MS_LOCKSCREEN_PANEL (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->plugins_settings);
  g_clear_object (&self->plugins_store);
  g_clear_object (&self->action_group);

  G_OBJECT_CLASS (ms_lockscreen_panel_parent_class)->finalize (object);
}


static void
ms_lockscreen_panel_class_init (MsLockscreenPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_lockscreen_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-lockscreen-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, MsLockscreenPanel, shuffle_switch);
  gtk_widget_class_bind_template_child (widget_class, MsLockscreenPanel, plugins_listbox);
}


static GActionEntry entries[] =
{
  { .name = "open-plugin-prefs", .parameter_type = "s", .activate = open_plugin_prefs_activated },
};

static void
ms_lockscreen_panel_init (MsLockscreenPanel *self)
{
  /* Scan prefs plugins */
  g_io_modules_scan_all_in_directory (MOBILE_SETTINGS_PHOSH_PREFS_DIR);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new (LOCKSCREEN_SCHEMA_ID);
  g_settings_bind (self->settings,
                   LOCKSCREEN_KEY_SCALE_TO_FIT,
                   self->shuffle_switch,
                   "active",
                   G_SETTINGS_BIND_DEFAULT);

  self->plugins_settings = g_settings_new (LOCKSCREEN_PLUGINS_SCHEMA_ID);
  self->plugins_store = g_list_store_new (MS_TYPE_PLUGIN_ROW);
  gtk_list_box_bind_model (self->plugins_listbox,
                           G_LIST_MODEL (self->plugins_store),
                           create_plugins_row,
                           self, NULL);
  ms_lockscreen_panel_scan_phosh_lockscreen_plugins (self);

  self->action_group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (self->action_group),
                                   entries,
                                   G_N_ELEMENTS (entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "lockscreen-panel",
                                  G_ACTION_GROUP (self->action_group));

}


MsLockscreenPanel *
ms_lockscreen_panel_new (void)
{
  return MS_LOCKSCREEN_PANEL (g_object_new (MS_TYPE_LOCKSCREEN_PANEL, NULL));
}
