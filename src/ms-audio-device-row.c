/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "ms-audio-device-row"

#include "mobile-settings-config.h"

#include "ms-audio-device.h"
#include "ms-audio-device-row.h"

#include "gvc-mixer-control.h"

#define ADJUSTMENT_MAX_NORMAL PA_VOLUME_NORM

/**
 * MsAudioDeviceRow:
 *
 * A widget intended to be stored in a `GtkListBox` to represent an audio device.
 */

enum {
  PROP_0,
  PROP_AUDIO_DEVICE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsAudioDeviceRow {
  AdwPreferencesRow  parent;

  MsAudioDevice     *audio_device;

  GtkImage          *icon;
  GtkLabel          *description;
  GtkScale          *scale;
  GtkAdjustment     *adjustment;
  gboolean           setting_volume;
};
G_DEFINE_TYPE (MsAudioDeviceRow, ms_audio_device_row, ADW_TYPE_PREFERENCES_ROW)


static void
on_volume_changed (MsAudioDeviceRow *self)
{
  double volume, rounded;
  g_autofree char *name = NULL;
  GvcMixerStream *stream;

  volume = gtk_adjustment_get_value (self->adjustment);
  rounded = round (volume);

  g_debug ("Setting volume %lf (rounded: %lf) for '%s'", volume, rounded,
           ms_audio_device_get_description (self->audio_device));

  stream = ms_audio_device_get_stream (self->audio_device);
  g_return_if_fail (stream);

  if (gvc_mixer_stream_set_volume (stream, (pa_volume_t) rounded) != FALSE)
    gvc_mixer_stream_push_volume (stream);

  gvc_mixer_stream_change_is_muted (stream, (int) rounded == 0);
}


static gboolean
transform_icon_name_to_icon (GBinding     *binding,
                             const GValue *from_value,
                             GValue       *to_value,
                             gpointer      unused)
{
  const char *icon_name = g_value_get_string (from_value);
  GIcon *icon = NULL;

  if (icon_name == NULL)
    icon_name = "audio-speakers-symbolic";

  icon = g_themed_icon_new_with_default_fallbacks (icon_name);

  g_value_take_object (to_value, icon);
  return TRUE;
}


static void
on_stream_volume_changed (MsAudioDeviceRow *self)
{
  GvcMixerStream *stream;

  if (self->setting_volume)
    return;

  stream = ms_audio_device_get_stream (self->audio_device);
  g_return_if_fail (stream);

  self->setting_volume = TRUE;
  g_debug ("Adjusting volume to %d", gvc_mixer_stream_get_volume (stream));
  gtk_adjustment_set_value (self->adjustment, gvc_mixer_stream_get_volume (stream));
  self->setting_volume = FALSE;
}


static void
set_audio_device (MsAudioDeviceRow *self, MsAudioDevice *device)
{
  GvcMixerStream *stream;

  g_set_object (&self->audio_device, device);

  g_object_bind_property (device, "description",
                          self->description, "label",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  g_object_bind_property_full (device, "icon-name",
                               self->icon, "gicon",
                               G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                               transform_icon_name_to_icon,
                               NULL, NULL, NULL);

  stream = ms_audio_device_get_stream (self->audio_device);

  g_signal_connect_object (stream,
                           "notify::volume",
                           G_CALLBACK (on_stream_volume_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_stream_volume_changed (self);
}


static void
ms_audio_device_row_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  MsAudioDeviceRow *self = MS_AUDIO_DEVICE_ROW (object);

  switch (property_id) {
  case PROP_AUDIO_DEVICE:
    set_audio_device (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_audio_device_row_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  MsAudioDeviceRow *self = MS_AUDIO_DEVICE_ROW (object);

  switch (property_id) {
  case PROP_AUDIO_DEVICE:
    g_value_set_object (value, self->audio_device);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_audio_device_row_dispose (GObject *object)
{
  MsAudioDeviceRow *self = MS_AUDIO_DEVICE_ROW (object);

  g_clear_object (&self->audio_device);

  G_OBJECT_CLASS (ms_audio_device_row_parent_class)->dispose (object);
}


static void
ms_audio_device_row_class_init (MsAudioDeviceRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ms_audio_device_row_get_property;
  object_class->set_property = ms_audio_device_row_set_property;
  object_class->dispose = ms_audio_device_row_dispose;

  props[PROP_AUDIO_DEVICE] =
    g_param_spec_object ("audio-device", "", "",
                         MS_TYPE_AUDIO_DEVICE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/"
                                               "ms-audio-device-row.ui");
  gtk_widget_class_bind_template_child (widget_class, MsAudioDeviceRow, description);
  gtk_widget_class_bind_template_child (widget_class, MsAudioDeviceRow, icon);
  gtk_widget_class_bind_template_child (widget_class, MsAudioDeviceRow, scale);
  gtk_widget_class_bind_template_child (widget_class, MsAudioDeviceRow, adjustment);

  gtk_widget_class_bind_template_callback (widget_class, on_volume_changed);
}


static void
ms_audio_device_row_init (MsAudioDeviceRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_adjustment_set_upper (self->adjustment, ADJUSTMENT_MAX_NORMAL);
  gtk_adjustment_set_step_increment (self->adjustment, ADJUSTMENT_MAX_NORMAL / 100.0);
  gtk_adjustment_set_page_increment (self->adjustment, ADJUSTMENT_MAX_NORMAL / 10.0);
}


MsAudioDeviceRow *
ms_audio_device_row_new (MsAudioDevice *audio_device)
{
  return g_object_new (MS_TYPE_AUDIO_DEVICE_ROW,
                       "audio-device", audio_device,
                       NULL);
}

/**
 * ms_audio_device_row_get_audio_device:
 * @self: An audio device row
 *
 * Get the audio device associated with this row
 *
 * Returns:(transfer none): The audio device
 */
MsAudioDevice *
ms_audio_device_row_get_audio_device (MsAudioDeviceRow *self)
{
  g_return_val_if_fail (MS_IS_AUDIO_DEVICE_ROW (self), NULL);

  return self->audio_device;
}
