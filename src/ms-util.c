/*
 * Copyright (C) 2022 Purism SPC
 *               2024 THe Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include <ms-util.h>
#include <glib/gi18n.h>

/**
 * ms_munge_app_id:
 * @app_id: the app_id
 *
 * Munges an app_id according to the rules used by
 * gnome-shell, feedbackd and phoc for gsettings:
 *
 * Returns: The munged_app id
 */
char *
ms_munge_app_id (const char *app_id)
{
  char *id = g_strdup (app_id);
  int i;

  if (g_str_has_suffix (id, ".desktop")) {
    char *c = g_strrstr (id, ".desktop");
    if (c)
      *c = '\0';
  }

  g_strcanon (id,
              "0123456789"
              "abcdefghijklmnopqrstuvwxyz"
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "-",
              '-');
  for (i = 0; id[i] != '\0'; i++)
    id[i] = g_ascii_tolower (id[i]);

  return id;
}


/**
 * ms_get_desktop_app_info_for_app_id:
 * @app_id: the app_id
 *
 * Looks up an app info object for specified application ID.
 * Tries a bunch of transformations in order to maximize compatibility
 * with X11 and non-GTK applications that may not report the exact same
 * string as their app-id and in their desktop file.
 *
 * This is based on what phosh does.
 *
 * Returns: (transfer full): GDesktopAppInfo for requested app_id
 */
GDesktopAppInfo *
ms_get_desktop_app_info_for_app_id (const char *app_id)
{
  g_autofree char *desktop_id = NULL;
  g_autofree char *lowercase = NULL;
  GDesktopAppInfo *app_info = NULL;
  char *last_component;
  static char *mappings[][2] = {
    { "org.gnome.ControlCenter", "gnome-control-center" },
    { "gnome-usage", "org.gnome.Usage" },
  };

  g_assert (app_id);

  /* fix up applications with known broken app-id */
  for (int i = 0; i < G_N_ELEMENTS (mappings); i++) {
    if (strcmp (app_id, mappings[i][0]) == 0) {
      app_id = mappings[i][1];
      break;
    }
  }

  desktop_id = g_strdup_printf ("%s.desktop", app_id);
  g_return_val_if_fail (desktop_id, NULL);
  app_info = g_desktop_app_info_new (desktop_id);

  if (app_info)
    return app_info;

  /* try to handle the case where app-id is rev-DNS, but desktop file is not */
  last_component = strrchr(app_id, '.');
  if (last_component) {
    g_free (desktop_id);
    desktop_id = g_strdup_printf ("%s.desktop", last_component + 1);
    g_return_val_if_fail (desktop_id, NULL);
    app_info = g_desktop_app_info_new (desktop_id);
    if (app_info)
      return app_info;
  }

  /* X11 WM_CLASS is often capitalized, so try in lowercase as well */
  lowercase = g_utf8_strdown (last_component ?: app_id, -1);
  g_free (desktop_id);
  desktop_id = g_strdup_printf ("%s.desktop", lowercase);
  g_return_val_if_fail (desktop_id, NULL);
  app_info = g_desktop_app_info_new (desktop_id);

  if (!app_info)
    g_message ("Could not find application for app-id '%s'", app_id);

  return app_info;
}


MsFeedbackProfile
ms_feedback_profile_from_setting (const char *name)
{
  if (g_strcmp0 (name, "full") == 0) {
    return MS_FEEDBACK_PROFILE_FULL;
  } else if (g_strcmp0 (name, "quiet") == 0) {
    return MS_FEEDBACK_PROFILE_QUIET;
  } else if (g_strcmp0 (name, "silent") == 0) {
    return MS_FEEDBACK_PROFILE_SILENT;
  }

  g_return_val_if_reached (MS_FEEDBACK_PROFILE_FULL);
}


char *
ms_feedback_profile_to_setting (MsFeedbackProfile profile)
{
  switch (profile) {
  case MS_FEEDBACK_PROFILE_FULL:
    return g_strdup ("full");
  case MS_FEEDBACK_PROFILE_QUIET:
    return g_strdup ("quiet");
  case MS_FEEDBACK_PROFILE_SILENT:
    return g_strdup ("silent");
  default:
    g_return_val_if_reached (NULL);
  }
}


char *
ms_feedback_profile_to_label (MsFeedbackProfile profile)
{
  switch (profile) {
  case MS_FEEDBACK_PROFILE_FULL:
    /* Translators: "Full" represents the feedback profile with maximum haptic, led and sounds */
    return g_strdup (_("Full"));
  case MS_FEEDBACK_PROFILE_QUIET:
   /* Translators: "Quiet" represents a feedback profile with haptic and LED */
    return g_strdup (_("Quiet"));
  case MS_FEEDBACK_PROFILE_SILENT:
    /* Translators: "Silent" represents a feedback profile with LED only */
    return g_strdup (_("Silent"));
  default:
    g_return_val_if_reached (NULL);
  }
}

/**
 * ms_schema_bind_property:
 * @id: The schema id
 * @key: The name of the key to bind to
 * @object: The object's property that should be bound
 * @property: The property that gets updated on schema changes
 * @flags: The flags
 *
 * Bind an `object`'s `property` to a `key` in the schema with the
 * given `id` if the schema and `key` exist. The lifetime of the binding
 * is bound to `object`.
 *
 * Returns: `TRUE` if the binding was created, otherwise `FALSE`.
 */
gboolean
ms_schema_bind_property (const char         *id,
                         const char         *key,
                         GObject            *object,
                         const char         *property,
                         GSettingsBindFlags  flags)
{
  GSettingsSchemaSource *source = g_settings_schema_source_get_default ();
  g_autoptr (GSettingsSchema) schema = NULL;
  g_autoptr (GSettings) settings = NULL;

  g_return_val_if_fail (source, FALSE);

  schema = g_settings_schema_source_lookup (source, id, TRUE);
  if (!schema)
    return FALSE;

  if (!g_settings_schema_has_key (schema, key))
    return FALSE;

  settings = g_settings_new (id);
  g_settings_bind (settings, key, object, property, flags);
  return TRUE;
}
