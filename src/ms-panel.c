/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Rudra Pratap Singh <rudrasingh900@gmail.com>
 */

#include "ms-panel.h"
#include "ms-util.h"

enum {
  PROP_0,
  PROP_KEYWORDS,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

/**
 * MsPanel:
 *
 * Base class for standard panels. Panel implementations need to derive
 * from this class.
 */
typedef struct {
  GtkStringList *keywords;
} MsPanelPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (MsPanel, ms_panel, ADW_TYPE_BIN)


static void
ms_panel_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  MsPanel *self = MS_PANEL (object);

  switch (property_id) {
  case PROP_KEYWORDS:
    ms_panel_set_keywords (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_panel_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  MsPanel *self = MS_PANEL (object);
  MsPanelPrivate *priv = ms_panel_get_instance_private (self);

  switch (property_id) {
  case PROP_KEYWORDS:
    g_value_set_object (value, priv->keywords);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_panel_finalize (GObject *object)
{
  MsPanel *self = MS_PANEL (object);
  MsPanelPrivate *priv = ms_panel_get_instance_private (self);

  g_clear_object (&priv->keywords);

  G_OBJECT_CLASS (ms_panel_parent_class)->finalize (object);
}


static void
ms_panel_class_init (MsPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ms_panel_set_property;
  object_class->get_property = ms_panel_get_property;
  object_class->finalize = ms_panel_finalize;

  /**
   * MsPanel:keywords:
   *
   * The string keyword(s) associated with the panel.
   */
  props[PROP_KEYWORDS] =
    g_param_spec_object ("keywords", "", "",
                         GTK_TYPE_STRING_LIST,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
ms_panel_init (MsPanel *self)
{
}


GtkStringList *
ms_panel_get_keywords (MsPanel *self)
{
  MsPanelPrivate *priv;

  g_return_val_if_fail (MS_IS_PANEL (self), NULL);

  priv = ms_panel_get_instance_private (self);

  return priv->keywords;
}


void
ms_panel_set_keywords (MsPanel *self, GtkStringList *keywords)
{
  MsPanelPrivate *priv;

  g_return_if_fail (MS_IS_PANEL (self));
  g_return_if_fail (GTK_IS_STRING_LIST (keywords));

  priv = ms_panel_get_instance_private (self);

  if (priv->keywords == keywords)
    return;

  g_clear_object (&priv->keywords);
  priv->keywords = ms_get_casefolded_string_list (keywords);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_KEYWORDS]);
}
