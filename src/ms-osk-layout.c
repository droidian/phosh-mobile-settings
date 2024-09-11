/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-osk-layout"

#include "mobile-settings-config.h"

#include "ms-osk-layout.h"

#include <gtk/gtk.h>

/**
 * MsOskLayout:
 *
 * Information about an OSK layout
 */

enum {
  PROP_0,
  PROP_NAME,
  PROP_TYPE,
  PROP_ID,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsOskLayout {
  GObject               parent;

  char                 *name;
  char                 *type;
  char                 *id;
};

G_DEFINE_TYPE (MsOskLayout, ms_osk_layout, G_TYPE_OBJECT)


static void
ms_osk_layout_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  MsOskLayout *self = MS_OSK_LAYOUT (object);

  switch (property_id) {
  case PROP_NAME:
    self->name = g_value_dup_string (value);
    break;
  case PROP_TYPE:
    self->type = g_value_dup_string (value);
    break;
  case PROP_ID:
    self->id = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_osk_layout_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MsOskLayout *self = MS_OSK_LAYOUT (object);

  switch (property_id) {
  case PROP_NAME:
    g_value_set_string (value, self->name);
    break;
  case PROP_TYPE:
    g_value_set_string (value, self->type);
    break;
  case PROP_ID:
    g_value_set_string (value, self->id);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_osk_layout_finalize (GObject *object)
{
  MsOskLayout *self = MS_OSK_LAYOUT (object);

  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->type, g_free);
  g_clear_pointer (&self->id, g_free);

  G_OBJECT_CLASS (ms_osk_layout_parent_class)->finalize (object);
}


static void
ms_osk_layout_class_init (MsOskLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ms_osk_layout_get_property;
  object_class->set_property = ms_osk_layout_set_property;
  object_class->finalize = ms_osk_layout_finalize;

  /**
   * PosOskLayout:name:
   *
   * The human readable (translated) name, e.g. `German` or
   * `Malayalam`.
   */
  props[PROP_NAME] =
    g_param_spec_string ("name", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  /**
   * PosOskLayout:type:
   *
   * The type (`xkb` or `ibus`)
   */
  props[PROP_TYPE] =
    g_param_spec_string ("type", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  /**
   * PosOskLayout:id:
   *
   * THe id identifiying the layout e.g. `de` for `xkb` or
   * `varname:ml` for `ibus`
   */
  props[PROP_ID] =
    g_param_spec_string ("id", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
ms_osk_layout_init (MsOskLayout *self)
{
}


MsOskLayout *
ms_osk_layout_new (const char *name, const char *type, const char *id)
{
  return g_object_new (MS_TYPE_OSK_LAYOUT,
                       "name", name,
                       "type", type,
                       "id", id,
                       NULL);
}


const char *
ms_osk_layout_get_id (MsOskLayout *self)
{
  g_assert (MS_IS_OSK_LAYOUT (self));

  return self->id;
}


const char *
ms_osk_layout_get_type_ (MsOskLayout *self)
{
  g_assert (MS_IS_OSK_LAYOUT (self));

  return self->type;
}


const char *
ms_osk_layout_get_name (MsOskLayout *self)
{
  g_assert (MS_IS_OSK_LAYOUT (self));

  return self->name;
}
