/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-plugin-loader"

#include "mobile-settings-config.h"

#include "ms-plugin-loader.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

enum {
  PROP_0,
  PROP_PLUGIN_DIRS,
  PROP_EXTENSION_POINT,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsPluginLoader {
  GObject parent;

  GStrv   plugin_dirs;
  char   *extension_point;
};

G_DEFINE_TYPE (MsPluginLoader, ms_plugin_loader, G_TYPE_OBJECT)

static void
ms_plugin_loader_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MsPluginLoader *self = MS_PLUGIN_LOADER (object);

  switch (property_id) {
  case PROP_PLUGIN_DIRS:
    g_strfreev (self->plugin_dirs);
    self->plugin_dirs = g_value_dup_boxed (value);
    break;
  case PROP_EXTENSION_POINT:
    g_free (self->extension_point);
    self->extension_point = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_plugin_loader_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MsPluginLoader *self = MS_PLUGIN_LOADER (object);

  switch (property_id) {
  case PROP_PLUGIN_DIRS:
    g_value_set_boxed (value, self->plugin_dirs);
    break;
  case PROP_EXTENSION_POINT:
    g_value_set_string (value, self->extension_point);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_plugin_loader_constructed (GObject *object)
{
  MsPluginLoader *self = MS_PLUGIN_LOADER (object);
  GIOExtensionPoint *ep;

  G_OBJECT_CLASS (ms_plugin_loader_parent_class)->constructed (object);

  if (!g_module_supported ()) {
    g_warning ("GModules are not supported on your platform!");
    return;
  }

  ep = g_io_extension_point_register (self->extension_point);
  /* TODO: Make configurable */
  g_io_extension_point_set_required_type (ep, GTK_TYPE_WIDGET);

  for (int i = 0; i < g_strv_length (self->plugin_dirs); i++) {
    g_debug ("Will load plugins from '%s' for '%s'", self->plugin_dirs[i], self->extension_point);
    g_io_modules_scan_all_in_directory (self->plugin_dirs[i]);
  }
}


static void
ms_plugin_loader_dispose (GObject *object)
{
  MsPluginLoader *self = MS_PLUGIN_LOADER (object);

  g_clear_pointer (&self->plugin_dirs, g_strfreev);
  g_clear_pointer (&self->extension_point, g_free);

  G_OBJECT_CLASS (ms_plugin_loader_parent_class)->dispose (object);
}


static void
ms_plugin_loader_class_init (MsPluginLoaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ms_plugin_loader_get_property;
  object_class->set_property = ms_plugin_loader_set_property;
  object_class->constructed = ms_plugin_loader_constructed;
  object_class->dispose = ms_plugin_loader_dispose;

  /**
   * MsPluginLoader:plugin-dirs:
   *
   * The directory to search for plugins
   */
  props[PROP_PLUGIN_DIRS] =
    g_param_spec_boxed ("plugin-dirs", "", "",
                        G_TYPE_STRV,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /**
   * MsPluginLoader:extension-point:
   *
   * The name of the extension point to load plugins for.
   */
  props[PROP_EXTENSION_POINT] =
    g_param_spec_string ("extension-point", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
ms_plugin_loader_init (MsPluginLoader *self)
{
}


MsPluginLoader *
ms_plugin_loader_new (const char * const *plugin_dirs, const char *extension_point)
{
  return g_object_new (MS_TYPE_PLUGIN_LOADER,
                       "plugin-dirs", plugin_dirs,
                       "extension-point", extension_point,
                       NULL);
}


GtkWidget *
ms_plugin_loader_load_plugin (MsPluginLoader *self)
{
  GIOExtensionPoint *ep;
  GIOExtension *extension;
  GType type;
  GList *extensions;

  g_return_val_if_fail (MS_IS_PLUGIN_LOADER (self), NULL);

  ep = g_io_extension_point_lookup (self->extension_point);
  extensions = g_io_extension_point_get_extensions (ep);

  if (extensions == NULL)
    return NULL;

  extension = extensions->data;
  /* We load the highest priority plugin that registered */
  g_debug ("Loading plugin %s", g_io_extension_get_name (extension));
  type = g_io_extension_get_type (extensions->data);
  return g_object_new (type, NULL);
}
