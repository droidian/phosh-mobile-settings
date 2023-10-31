/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-feedback-row"

#include "mobile-settings-config.h"
#include "mobile-settings-enums.h"
#include "ms-enum-types.h"
#include "ms-util.h"
#include "ms-feedback-row.h"


enum {
  PROP_0,
  PROP_FEEDBACK_PROFILE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsFeedbackRow {
  AdwComboRow       parent;

  MsFeedbackProfile profile;
};
G_DEFINE_TYPE (MsFeedbackRow, ms_feedback_row, ADW_TYPE_COMBO_ROW)


static char *
item_feedback_profile_name (AdwEnumListItem   *item,
                            gpointer user_data G_GNUC_UNUSED)
{
  return ms_feedback_profile_to_label (adw_enum_list_item_get_value (item));
}


static void
ms_feedback_row_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  MsFeedbackRow *self = MS_FEEDBACK_ROW (object);

  switch (property_id) {
  case PROP_FEEDBACK_PROFILE:
    self->profile = g_value_get_enum (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_feedback_row_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MsFeedbackRow *self = MS_FEEDBACK_ROW (object);

  switch (property_id) {
  case PROP_FEEDBACK_PROFILE:
    g_value_set_enum (value, self->profile);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_feedback_row_class_init (MsFeedbackRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ms_feedback_row_get_property;
  object_class->set_property = ms_feedback_row_set_property;

  props[PROP_FEEDBACK_PROFILE] =
    g_param_spec_enum ("feedback-profile", "", "",
                       MS_TYPE_FEEDBACK_PROFILE,
                       MS_FEEDBACK_PROFILE_FULL,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  g_type_ensure (MS_TYPE_FEEDBACK_PROFILE);
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-feedback-row.ui");
  gtk_widget_class_bind_template_callback (widget_class, item_feedback_profile_name);
}


static void
ms_feedback_row_init (MsFeedbackRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


MsFeedbackRow *
ms_feedback_row_new (void)
{
  return MS_FEEDBACK_ROW (g_object_new (MS_TYPE_FEEDBACK_ROW, NULL));
}
