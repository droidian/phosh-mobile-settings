/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-scale_to_fit-row"

#include "mobile-settings-config.h"
#include "mobile-settings-enums.h"
#include "ms-enum-types.h"
#include "ms-util.h"
#include "ms-scale-to-fit-row.h"


#define APP_KEY_SCALE_TO_FIT "scale-to-fit"
#define APP_SCHEMA "sm.puri.phoc.application"
#define APP_PREFIX "/sm/puri/phoc/application/"


enum {
  PROP_0,
  PROP_APP_ID,
  PROP_SCALE_TO_FIT,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsScaleToFitRow {
  AdwActionRow parent;

  char        *app_id;
  GSettings   *settings;
  gboolean     scale_to_fit;
  GtkWidget   *scale_to_fit_switch;
};
G_DEFINE_TYPE (MsScaleToFitRow, ms_scale_to_fit_row, ADW_TYPE_ACTION_ROW)

static void
ms_scale_to_fit_row_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  MsScaleToFitRow *self = MS_SCALE_TO_FIT_ROW (object);

  switch (property_id) {
  case PROP_SCALE_TO_FIT:
    self->scale_to_fit = g_value_get_boolean (value);
    break;
  case PROP_APP_ID:
    g_free (self->app_id);
    self->app_id = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_scale_to_fit_row_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  MsScaleToFitRow *self = MS_SCALE_TO_FIT_ROW (object);

  switch (property_id) {
  case PROP_SCALE_TO_FIT:
    g_value_set_boolean (value, self->scale_to_fit);
    break;
  case PROP_APP_ID:
    g_value_set_string (value, self->app_id);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_scale_to_fit_row_constructed (GObject *object)
{
  MsScaleToFitRow *self = MS_SCALE_TO_FIT_ROW (object);
  g_autofree char *path = NULL;
  g_autofree char *munged_id = NULL;
  g_autoptr (GDesktopAppInfo) app_info = NULL;
  g_autoptr (GIcon) icon = NULL;
  GtkWidget *w;
  const char *title = NULL;

  G_OBJECT_CLASS (ms_scale_to_fit_row_parent_class)->constructed (object);

  munged_id = ms_munge_app_id (self->app_id);
  path = g_strconcat (APP_PREFIX, munged_id, "/", NULL);
  g_debug ("Monitoring settings path: %s", path);
  self->settings = g_settings_new_with_path (APP_SCHEMA, path);

  g_settings_bind (self->settings, APP_KEY_SCALE_TO_FIT,
                   self, "scale-to-fit",
                   G_SETTINGS_BIND_DEFAULT);

  app_info = ms_get_desktop_app_info_for_app_id (self->app_id);

  if (app_info == NULL)
    icon = g_themed_icon_new ("application-x-executable");

  if (icon == NULL)
    icon = g_app_info_get_icon (G_APP_INFO (app_info));

  if (icon == NULL)
    icon = g_themed_icon_new ("application-x-executable");
  else
    g_object_ref (icon);

  w = gtk_image_new_from_gicon (icon);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gtk_style_context_add_class (gtk_widget_get_style_context (w), "lowres-icon");
G_GNUC_END_IGNORE_DEPRECATIONS
  gtk_image_set_icon_size (GTK_IMAGE (w), GTK_ICON_SIZE_LARGE);
  adw_action_row_add_prefix (ADW_ACTION_ROW (self), w);

  if (app_info)
    title = g_app_info_get_name (G_APP_INFO (app_info));

  if (title == NULL)
    title = self->app_id;

  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (self), title);
}


static void
ms_scale_to_fit_row_finalize (GObject *object)
{
  MsScaleToFitRow *self = MS_SCALE_TO_FIT_ROW (object);

  g_clear_pointer (&self->app_id, g_free);
  g_clear_object (&self->settings);

  G_OBJECT_CLASS (ms_scale_to_fit_row_parent_class)->finalize (object);
}


static void
ms_scale_to_fit_row_class_init (MsScaleToFitRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = ms_scale_to_fit_row_constructed;
  object_class->finalize = ms_scale_to_fit_row_finalize;
  object_class->get_property = ms_scale_to_fit_row_get_property;
  object_class->set_property = ms_scale_to_fit_row_set_property;

  props[PROP_SCALE_TO_FIT] =
    g_param_spec_boolean ("scale-to-fit", "", "",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_APP_ID] =
    g_param_spec_string ("app-id", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sigxcpu/MobileSettings/ui/ms-scale-to-fit-row.ui");
  gtk_widget_class_bind_template_child (widget_class, MsScaleToFitRow, scale_to_fit_switch);
}


static void
ms_scale_to_fit_row_init (MsScaleToFitRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property (self,
                          "scale-to-fit",
                          self->scale_to_fit_switch,
                          "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}


MsScaleToFitRow *
ms_scale_to_fit_row_new (const char *app_id)
{
  return MS_SCALE_TO_FIT_ROW (g_object_new (MS_TYPE_SCALE_TO_FIT_ROW,
                                            "app-id", app_id,
                                            NULL));
}

const char *
ms_scale_to_fit_row_get_app_id (MsScaleToFitRow *self)
{
  g_return_val_if_fail (MS_IS_SCALE_TO_FIT_ROW (self), NULL);

  return self->app_id;
}
