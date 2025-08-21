/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * Author: Evangelos Ribeiro Tzaras <devrtz@fortysixandtwo.eu>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "ms-cb-message-row"

#include "mobile-settings-config.h"

#include "ms-cb-message-row.h"

#include <glib/gi18n.h>

/**
 * MsCbMessageRow:
 *
 * A widget intended to be stored in a `GtkListBox`
 * to represent a cell broadcast message.
 */

enum {
  PROP_0,
  PROP_MESSAGE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsCbMessageRow {
  AdwExpanderRow parent;

  LcbMessage    *msg;

  GtkImage      *icon;
  GtkLabel      *description;
};
G_DEFINE_TYPE (MsCbMessageRow, ms_cb_message_row, ADW_TYPE_EXPANDER_ROW)


static gboolean
transform_text (GBinding     *binding,
                const GValue *from_value,
                GValue       *to_value,
                gpointer      unused)
{
  const char *text = g_value_get_string (from_value);
  g_autofree char* stripped = g_strdup (text);

  g_strstrip (stripped);

  g_value_set_string (to_value, stripped);
  return TRUE;
}


static gboolean
transform_timestamp_to_subtitle (GBinding     *binding,
                                 const GValue *from_value,
                                 GValue       *to_value,
                                 gpointer      unused)
{
  g_autoptr (GDateTime) timestamp = NULL;
  gint64 ts = g_value_get_int64 (from_value);

  timestamp = g_date_time_new_from_unix_utc (ts);

  g_value_take_string (to_value, g_date_time_format (timestamp, "%c"));

  return TRUE;
}


static gboolean
transform_severity_to_icon (GBinding     *binding,
                            const GValue *from_value,
                            GValue       *to_value,
                            gpointer      unused)
{
  LcbSeverityLevel severity = g_value_get_flags (from_value);
  const char *icon_name;
  GIcon *icon = NULL;

  switch (severity) {
  case LCB_SEVERITY_LEVEL_UNKNOWN:
  case LCB_SEVERITY_LEVEL_PRESIDENTIAL:
  case LCB_SEVERITY_LEVEL_EXTREME:
  case LCB_SEVERITY_LEVEL_SEVERE:
  case LCB_SEVERITY_LEVEL_PUBLIC_SAFETY:
  case LCB_SEVERITY_LEVEL_AMBER:
  case LCB_SEVERITY_LEVEL_TEST:

  default:
    icon_name = "dialog-warning-symbolic";
  }

  icon = g_themed_icon_new_with_default_fallbacks (icon_name);

  g_value_take_object (to_value, icon);
  return TRUE;
}


static gboolean
transform_severity_subject_to_title (GBinding     *binding,
                                     const GValue *from_value,
                                     GValue       *to_value,
                                     gpointer      unused)
{
  const char *subject = g_value_get_string (from_value);

  if (!subject)
    subject = _("Message of unknown severity");

  g_value_set_string (to_value, subject);
  return TRUE;
}


static void
set_message (MsCbMessageRow *self, LcbMessage *msg)
{
  g_debug ("%p", msg);

  g_object_bind_property_full (msg, "text",
                               self->description, "label",
                               G_BINDING_SYNC_CREATE,
                               transform_text,
                               NULL, NULL, NULL);

  g_object_bind_property_full (msg, "timestamp",
                               self, "subtitle",
                               G_BINDING_SYNC_CREATE,
                               transform_timestamp_to_subtitle,
                               NULL, NULL, NULL);

  g_object_bind_property_full (msg, "severity",
                               self->icon, "gicon",
                               G_BINDING_SYNC_CREATE,
                               transform_severity_to_icon,
                               NULL, NULL, NULL);

  g_object_bind_property_full (msg, "severity-subject",
                               self, "title",
                               G_BINDING_SYNC_CREATE,
                               transform_severity_subject_to_title,
                               NULL, NULL, NULL);
}


static void
ms_cb_message_row_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MsCbMessageRow *self = MS_CB_MESSAGE_ROW (object);

  switch (property_id) {
  case PROP_MESSAGE:
    set_message (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_cb_message_row_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MsCbMessageRow *self = MS_CB_MESSAGE_ROW (object);

  switch (property_id) {
  case PROP_MESSAGE:
    g_value_set_object (value, self->msg);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_cb_message_row_dispose (GObject *object)
{
  MsCbMessageRow *self = MS_CB_MESSAGE_ROW (object);

  g_clear_object (&self->msg);

  G_OBJECT_CLASS (ms_cb_message_row_parent_class)->dispose (object);
}


static void
ms_cb_message_row_class_init (MsCbMessageRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ms_cb_message_row_get_property;
  object_class->set_property = ms_cb_message_row_set_property;
  object_class->dispose = ms_cb_message_row_dispose;

  props[PROP_MESSAGE] =
    g_param_spec_object ("message", "", "",
                         LCB_TYPE_MESSAGE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/"
                                               "ms-cb-message-row.ui");
  gtk_widget_class_bind_template_child (widget_class, MsCbMessageRow, description);
  gtk_widget_class_bind_template_child (widget_class, MsCbMessageRow, icon);
}


static void
ms_cb_message_row_init (MsCbMessageRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

}


MsCbMessageRow *
ms_cb_message_row_new (LcbMessage *message)
{
  return g_object_new (MS_TYPE_CB_MESSAGE_ROW,
                       "message", message,
                       NULL);
}

/**
 * ms_cb_message_row_get_message:
 * @self: An cellbroadcast message row
 *
 * Get the cellbroadcast message associated with this row
 *
 * Returns:(transfer none): The message
 */
LcbMessage *
ms_cb_message_row_get_message (MsCbMessageRow *self)
{
  g_return_val_if_fail (MS_IS_CB_MESSAGE_ROW (self), NULL);

  return self->msg;
}
