/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-completer-info"

#include "mobile-settings-config.h"

#include "ms-completer-info.h"

/**
 * MsCompleterInfo:
 *
 * Info about a OSK text completion engine
 */

enum {
  PROP_0,
  PROP_ID,
  PROP_NAME,
  PROP_DESCRIPTION,
  PROP_COMMENT,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsCompleterInfo {
  GObject  parent;

  char    *id;
  char    *name;
  char    *description;
  char    *comment;
};
G_DEFINE_TYPE (MsCompleterInfo, ms_completer_info, G_TYPE_OBJECT)


static void
ms_completer_info_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MsCompleterInfo *self = MS_COMPLETER_INFO (object);

  switch (property_id) {
  case PROP_ID:
    self->id = g_value_dup_string (value);
    break;
  case PROP_NAME:
    self->name = g_value_dup_string (value);
    break;
  case PROP_DESCRIPTION:
    self->description = g_value_dup_string (value);
    break;
  case PROP_COMMENT:
    self->comment = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_completer_info_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MsCompleterInfo *self = MS_COMPLETER_INFO (object);

  switch (property_id) {
  case PROP_ID:
    g_value_set_string (value, self->id);
    break;
  case PROP_NAME:
    g_value_set_string (value, self->name);
    break;
  case PROP_DESCRIPTION:
    g_value_set_string (value, self->description);
    break;
  case PROP_COMMENT:
    g_value_set_string (value, self->comment);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_completer_info_finalize (GObject *object)
{
  MsCompleterInfo *self = MS_COMPLETER_INFO (object);

  g_clear_pointer (&self->id, g_free);
  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->description, g_free);
  g_clear_pointer (&self->comment, g_free);

  G_OBJECT_CLASS (ms_completer_info_parent_class)->finalize (object);
}


static void
ms_completer_info_class_init (MsCompleterInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ms_completer_info_get_property;
  object_class->set_property = ms_completer_info_set_property;
  object_class->finalize = ms_completer_info_finalize;

  props[PROP_ID] =
    g_param_spec_string ("id", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  props[PROP_NAME] =
    g_param_spec_string ("name", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  props[PROP_DESCRIPTION] =
    g_param_spec_string ("description", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  props[PROP_COMMENT] =
    g_param_spec_string ("comment", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
ms_completer_info_init (MsCompleterInfo *self)
{
}


MsCompleterInfo *
ms_completer_info_new (void)
{
  return g_object_new (MS_TYPE_COMPLETER_INFO, NULL);
}


const char *
ms_completer_info_get_id (MsCompleterInfo *self)
{
  g_return_val_if_fail (MS_IS_COMPLETER_INFO (self), NULL);

  return self->id;
}

const char *
ms_completer_info_get_name (MsCompleterInfo *self)
{
  g_return_val_if_fail (MS_IS_COMPLETER_INFO (self), NULL);

  return self->name;
}


const char *
ms_completer_info_get_description (MsCompleterInfo *self)
{
  g_return_val_if_fail (MS_IS_COMPLETER_INFO (self), NULL);

  return self->description;
}


const char *
ms_completer_info_get_comment (MsCompleterInfo *self)
{
  g_return_val_if_fail (MS_IS_COMPLETER_INFO (self), NULL);

  return self->comment;
}
