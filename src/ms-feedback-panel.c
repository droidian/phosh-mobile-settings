/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-feedback-panel"

#include "mobile-settings-config.h"
#include "mobile-settings-enums.h"
#include "ms-enum-types.h"
#include "ms-feedback-row.h"
#include "ms-feedback-panel.h"
#include "ms-util.h"

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>

/* Verbatim from feedbackd */
#define FEEDBACKD_SCHEMA_ID "org.sigxcpu.feedbackd"
#define FEEDBACKD_KEY_PROFILE "profile"
#define FEEDBACKD_THEME_VAR "FEEDBACK_THEME"
#define APP_SCHEMA FEEDBACKD_SCHEMA_ID ".application"
#define APP_PREFIX "/org/sigxcpu/feedbackd/application/"

enum {
  PROP_0,
  PROP_FEEDBACK_PROFILE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct {
  char      *munged_app_id;
  GAppInfo  *app_info;
  GSettings *settings;
} MsFbdApplication;

struct _MsFeedbackPanel {
  AdwBin            parent;

  GtkListBox       *app_listbox;
  GHashTable       *known_applications;

  GSettings        *settings;
  MsFeedbackProfile profile;
};

G_DEFINE_TYPE (MsFeedbackPanel, ms_feedback_panel, ADW_TYPE_BIN)


static void
app_destroy (gpointer data)
{
  MsFbdApplication *app = data;

  g_clear_object (&app->settings);
  g_clear_object (&app->app_info);
  g_clear_pointer (&app->munged_app_id, g_free);
}


static char *
item_feedback_profile_name (AdwEnumListItem   *item,
                            gpointer user_data G_GNUC_UNUSED)
{
  return ms_feedback_profile_to_name (adw_enum_list_item_get_value (item));
}


static gboolean
settings_name_to_profile (GValue *value, GVariant *variant, gpointer user_data)
{
  const gchar *name;

  name = g_variant_get_string (variant, NULL);

  g_value_set_enum (value, ms_feedback_profile_from_name (name));

  return TRUE;
}


static GVariant *
settings_profile_to_name (const GValue       *value,
                          const GVariantType *expected_type,
                          gpointer            user_data)
{
  MsFeedbackProfile profile;

  profile = g_value_get_enum (value);

  return g_variant_new_take_string (ms_feedback_profile_to_name (profile));
}


static void
add_application_row (MsFeedbackPanel *self, MsFbdApplication *app)
{
  GtkWidget *w;
  MsFeedbackRow *row;
  g_autoptr (GIcon) icon = NULL;
  const gchar *app_name;

  app_name = g_app_info_get_name (app->app_info);
  if (STR_IS_NULL_OR_EMPTY (app_name))
    return;

  icon = g_app_info_get_icon (app->app_info);
  if (icon == NULL)
    icon = g_themed_icon_new ("application-x-executable");
  else
    g_object_ref (icon);

  row = ms_feedback_row_new ();

  /* TODO: we can move most of this into MsMobileSettingsRow */
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row),
                                 g_markup_escape_text (app_name, -1));
  g_object_set_data_full (G_OBJECT (row), "app", app, app_destroy);
  g_settings_bind_with_mapping (app->settings, FEEDBACKD_KEY_PROFILE,
                                row, "feedback-profile",
                                G_SETTINGS_BIND_DEFAULT,
                                settings_name_to_profile,
                                settings_profile_to_name,
                                NULL, NULL);

  gtk_list_box_append (self->app_listbox, GTK_WIDGET (row));

  w = gtk_image_new_from_gicon (icon);
  gtk_style_context_add_class (gtk_widget_get_style_context (w), "lowres-icon");
  gtk_image_set_icon_size (GTK_IMAGE (w), GTK_ICON_SIZE_LARGE);
  adw_action_row_add_prefix (ADW_ACTION_ROW (row), w);

  g_hash_table_add (self->known_applications, g_strdup (app->munged_app_id));
}


static void
process_app_info (MsFeedbackPanel *self, GAppInfo *app_info)
{
  MsFbdApplication *app;
  const char *app_id;
  g_autofree char *munged_id = NULL;
  g_autofree char *path = NULL;

  app_id = g_app_info_get_id (app_info);
  if (STR_IS_NULL_OR_EMPTY (app_id))
    return;

  munged_id = ms_munge_app_id (app_id);

  app = g_slice_new (MsFbdApplication);

  path = g_strconcat (APP_PREFIX, munged_id, "/", NULL);
  g_debug ("Monitoring settings path: %s", path);
  app->settings = g_settings_new_with_path (APP_SCHEMA, path);
  app->app_info = g_object_ref (app_info);
  app->munged_app_id = g_steal_pointer (&munged_id);

  if (g_hash_table_contains (self->known_applications, app->munged_app_id))
    return;

  g_debug ("Processing queued application %s", app->munged_app_id);

  add_application_row (self, app);
}



static void
load_apps (MsFeedbackPanel *self)
{
  GList *iter, *apps;

  apps = g_app_info_get_all ();

  for (iter = apps; iter; iter = iter->next) {
    GDesktopAppInfo *app;

    app = G_DESKTOP_APP_INFO (iter->data);
    if (g_desktop_app_info_get_boolean (app, "X-Phosh-UsesFeedback")) {
      g_debug ("App '%s' uses libfeedback", g_app_info_get_id (G_APP_INFO (app)));
      process_app_info (self, G_APP_INFO (app));
    }
  }
  g_list_free_full (apps, g_object_unref);
}


static void
ms_feedback_panel_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MsFeedbackPanel *self = MS_FEEDBACK_PANEL (object);

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
ms_feedback_panel_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MsFeedbackPanel *self = MS_FEEDBACK_PANEL (object);

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
ms_feedback_panel_constructed (GObject *object)
{
  MsFeedbackPanel *self = MS_FEEDBACK_PANEL (object);

  G_OBJECT_CLASS (ms_feedback_panel_parent_class)->constructed (object);

  load_apps (self);

  self->settings = g_settings_new (FEEDBACKD_SCHEMA_ID);
  g_settings_bind_with_mapping (self->settings, FEEDBACKD_KEY_PROFILE,
                                self, "feedback-profile",
                                G_SETTINGS_BIND_DEFAULT,
                                settings_name_to_profile,
                                settings_profile_to_name,
                                NULL, NULL);
}


static void
ms_feedback_panel_dispose (GObject *object)
{
  MsFeedbackPanel *self = MS_FEEDBACK_PANEL (object);

  g_clear_object (&self->settings);
  g_clear_pointer (&self->known_applications, g_hash_table_unref);

  G_OBJECT_CLASS (ms_feedback_panel_parent_class)->dispose (object);
}


static void
ms_feedback_panel_class_init (MsFeedbackPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ms_feedback_panel_get_property;
  object_class->set_property = ms_feedback_panel_set_property;
  object_class->constructed = ms_feedback_panel_constructed;
  object_class->dispose = ms_feedback_panel_dispose;

  props[PROP_FEEDBACK_PROFILE] =
    g_param_spec_enum ("feedback-profile", "", "",
                       MS_TYPE_FEEDBACK_PROFILE,
                       MS_FEEDBACK_PROFILE_FULL,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  g_type_ensure (MS_TYPE_FEEDBACK_PROFILE);
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sigxcpu/MobileSettings/ui/ms-feedback-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, MsFeedbackPanel, app_listbox);
  gtk_widget_class_bind_template_callback (widget_class, item_feedback_profile_name);
}


static void
ms_feedback_panel_init (MsFeedbackPanel *self)
{
  self->known_applications = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                    NULL, g_free);

  gtk_widget_init_template (GTK_WIDGET (self));
}


MsFeedbackPanel *
ms_feedback_panel_new (void)
{
  return MS_FEEDBACK_PANEL (g_object_new (MS_TYPE_FEEDBACK_PANEL, NULL));
}
