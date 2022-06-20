/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "ms-plugin-panel.h"

enum {
  PROP_0,
  PROP_TITLE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

/**
 * MsPluginPanel:
 *
 * Base class for panel plugins. Panel implementations from loadable modules
 * need to drive from this class.
 */
typedef struct _MsPluginPanelPrivate {
  char *title;
} MsPluginPanelPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (MsPluginPanel, ms_plugin_panel, ADW_TYPE_BIN)


static void
ms_plugin_panel_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  MsPluginPanel *self = MS_PLUGIN_PANEL (object);
  MsPluginPanelPrivate *priv = ms_plugin_panel_get_instance_private (self);

  switch (property_id) {
  case PROP_TITLE:
    g_free (priv->title);
    priv->title = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_plugin_panel_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MsPluginPanel *self = MS_PLUGIN_PANEL (object);
  MsPluginPanelPrivate *priv = ms_plugin_panel_get_instance_private (self);

  switch (property_id) {
  case PROP_TITLE:
    g_value_set_string (value, priv->title);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_plugin_panel_finalize (GObject *object)
{
  MsPluginPanel *self = MS_PLUGIN_PANEL (object);
  MsPluginPanelPrivate *priv = ms_plugin_panel_get_instance_private (self);

  g_clear_pointer (&priv->title, g_free);

  G_OBJECT_CLASS (ms_plugin_panel_parent_class)->finalize (object);
}


static void
ms_plugin_panel_class_init (MsPluginPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ms_plugin_panel_get_property;
  object_class->set_property = ms_plugin_panel_set_property;
  object_class->finalize = ms_plugin_panel_finalize;

  props[PROP_TITLE] =
    g_param_spec_string ("title", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
ms_plugin_panel_init (MsPluginPanel *self)
{
}


MsPluginPanel *
ms_plugin_panel_new (const char *title)
{
  return MS_PLUGIN_PANEL (g_object_new (MS_TYPE_PLUGIN_PANEL,
                                        "title", title,
                                        NULL));
}


const char *
ms_plugin_panel_get_title (MsPluginPanel *self)
{
  MsPluginPanelPrivate *priv;

  g_return_val_if_fail (MS_IS_PLUGIN_PANEL (self), NULL);
  priv = ms_plugin_panel_get_instance_private (self);

  return priv->title;
}
