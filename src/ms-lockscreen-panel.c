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

/* Verbatim from lockscreen */
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
};

G_DEFINE_TYPE (MsLockscreenPanel, ms_lockscreen_panel, ADW_TYPE_BIN)


static GStrv
plugin_append (const char *const *plugins, const char *plugin)
{
  g_autoptr (GPtrArray) array = g_ptr_array_new_with_free_func (g_free);

  for (int i = 0; i < g_strv_length ((GStrv)plugins); i++)
    g_ptr_array_add (array, g_strdup (plugins[i]));

  g_ptr_array_add (array, g_strdup (plugin));
  g_ptr_array_add (array, NULL);

  return (GStrv) g_ptr_array_steal (array, NULL);
}


static GStrv
plugin_remove (const char *const *plugins, const char *plugin)
{
  g_autoptr (GPtrArray) array = g_ptr_array_new_with_free_func (g_free);

  for (int i = 0; i < g_strv_length ((GStrv)plugins); i++) {
    if (g_strcmp0 (plugins[i], plugin) == 0)
      continue;
    g_ptr_array_add (array, g_strdup (plugins[i]));
  }
  g_ptr_array_add (array, NULL);

  return (GStrv) g_ptr_array_steal (array, NULL);
}


static void
on_plugin_activated (MsLockscreenPanel *self, GParamSpec *pspec, MsPluginRow *row)
{
  gboolean enabled = ms_plugin_row_get_enabled (row);
  const char *name = ms_plugin_row_get_name (row);

  g_auto (GStrv) enabled_plugins = NULL;
  g_auto (GStrv) e = NULL;

  g_debug ("Plugin: %s: %d", name, enabled);
  enabled_plugins = g_settings_get_strv (self->plugins_settings, LOCKSCREEN_PLUGINS_KEY);

  if (enabled)
    e = plugin_append ((const char * const *) enabled_plugins, name);
  else
    e = plugin_remove ((const char * const *) enabled_plugins, name);

  g_settings_set_strv (self->plugins_settings, LOCKSCREEN_PLUGINS_KEY, (const char * const *)e);
}


static GtkWidget *
create_plugins_row (gpointer object, gpointer user_data)
{
  return GTK_WIDGET (object);
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

    title = g_key_file_get_locale_string (keyfile, "Plugin", "Name", NULL, NULL);
    description = g_key_file_get_locale_string (keyfile, "Plugin", "Comment", NULL, NULL);

    enabled = g_strv_contains ((const gchar * const*)enabled_plugins, name);
    g_debug ("Found plugin %s, name %s, enabled: %d", filename, name, enabled);
    row = g_object_new (MS_TYPE_PLUGIN_ROW,
                        "plugin-name", name,
                        "title", title,
                        "subtitle", description,
                        "enabled", enabled,
                        NULL);
    g_signal_connect_object (row,
                             "notify::enabled",
                             G_CALLBACK (on_plugin_activated),
                             self,
                             G_CONNECT_SWAPPED);

    g_list_store_append (self->plugins_store, row);
  }
}

static void
ms_lockscreen_panel_finalize (GObject *object)
{
  MsLockscreenPanel *self = MS_LOCKSCREEN_PANEL (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->plugins_settings);
  g_clear_object (&self->plugins_store);

  G_OBJECT_CLASS (ms_lockscreen_panel_parent_class)->finalize (object);
}


static void
ms_lockscreen_panel_class_init (MsLockscreenPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_lockscreen_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sigxcpu/MobileSettings/ui/ms-lockscreen-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, MsLockscreenPanel, shuffle_switch);
  gtk_widget_class_bind_template_child (widget_class, MsLockscreenPanel, plugins_listbox);
}


static void
ms_lockscreen_panel_init (MsLockscreenPanel *self)
{
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
}


MsLockscreenPanel *
ms_lockscreen_panel_new (void)
{
  return MS_LOCKSCREEN_PANEL (g_object_new (MS_TYPE_LOCKSCREEN_PANEL, NULL));
}
