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
  PROP_ENABLED,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsPluginRow {
  AdwActionRow parent;

  char        *name;
  GtkWidget   *toggle;
  gboolean     enabled;
};
G_DEFINE_TYPE (MsPluginRow, ms_plugin_row, ADW_TYPE_ACTION_ROW)


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
  case PROP_ENABLED:
    self->enabled = g_value_get_boolean (value);
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
  case PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_plugin_row_class_init (MsPluginRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ms_plugin_row_get_property;
  object_class->set_property = ms_plugin_row_set_property;

  props[PROP_PLUGIN_NAME] =
    g_param_spec_string ("plugin-name", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled", "", "",
                          FALSE,
                          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sigxcpu/MobileSettings/ui/ms-plugin-row.ui");
  gtk_widget_class_bind_template_child (widget_class, MsPluginRow, toggle);
}


static void
ms_plugin_row_init (MsPluginRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property (self,
                          "enabled",
                          self->toggle,
                          "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
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
