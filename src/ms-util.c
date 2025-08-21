/*
 * Copyright (C) 2022 Purism SPC
 *               2024-2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include <ms-util.h>
#include <glib/gi18n.h>

#include <libportal/portal.h>
#include <libportal-gtk4/portal-gtk4.h>

#include <gtk/gtk.h>
#include <adwaita.h>

/* Combining diacritical mark?
 *  Basic range: [0x0300,0x036F]
 *  Supplement:  [0x1DC0,0x1DFF]
 *  For Symbols: [0x20D0,0x20FF]
 *  Half marks:  [0xFE20,0xFE2F]
 */
#define IS_CDM_UCS4(c) (((c) >= 0x0300 && (c) <= 0x036F)  || \
                        ((c) >= 0x1DC0 && (c) <= 0x1DFF)  || \
                        ((c) >= 0x20D0 && (c) <= 0x20FF)  || \
                        ((c) >= 0xFE20 && (c) <= 0xFE2F))

#define IS_SOFT_HYPHEN(c) ((c) == 0x00AD)

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
  for (guint i = 0; i < G_N_ELEMENTS (mappings); i++) {
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


static void
on_set_wallpaper (GObject *source, GAsyncResult *result, gpointer user_data)
{
  g_autoptr (GTask) task = G_TASK (user_data);
  XdpPortal *portal = XDP_PORTAL (source);
  g_autoptr (GError) err = NULL;

  if (!xdp_portal_set_wallpaper_finish (portal, result, &err)) {
    g_task_return_error (task, g_steal_pointer (&err));
    return;
  }

  g_debug ("Updated wallpaper via portal");
  g_task_return_boolean (task, TRUE);
}


static void
on_file_chooser_done (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GTask) task = G_TASK (user_data);
  AdwBin *panel = ADW_BIN (g_task_get_source_object (task));
  GtkWindow *parent = GTK_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (panel), GTK_TYPE_WINDOW));
  GtkFileDialog *filechooser = GTK_FILE_DIALOG (source_object);
  g_autofree char *uri = NULL;
  g_autoptr (XdpParent) xdp_parent = NULL;
  g_autoptr (XdpPortal) portal = NULL;
  g_autoptr (GFile) file = NULL;
  g_autoptr (GError) err = NULL;
  XdpWallpaperFlags flags = XDP_WALLPAPER_FLAG_PREVIEW;
  gboolean lockscreen = !!GPOINTER_TO_INT (g_task_get_task_data (task));

  file = gtk_file_dialog_open_finish (filechooser, res, &err);
  if (!file) {
    g_task_return_error (task, g_steal_pointer (&err));
    return;
  }

  flags |= lockscreen ? XDP_WALLPAPER_FLAG_LOCKSCREEN : XDP_WALLPAPER_FLAG_BACKGROUND;
  uri = g_file_get_uri (file);
  portal = xdp_portal_new ();
  xdp_parent = xdp_parent_new_gtk (parent);
  xdp_portal_set_wallpaper (portal,
                            xdp_parent,
                            uri,
                            flags,
                            NULL,
                            on_set_wallpaper,
                            g_steal_pointer (&task));
}


void
ms_select_wallpaper_async (AdwBin              *panel,
                           GAsyncReadyCallback  callback,
                           gboolean             lockscreen,
                           gpointer             user_data)
{
  GtkFileDialog *filechooser;
  GtkFileFilter *filter;
  GListStore *filters;
  g_autoptr (GFile) pictures_folder = NULL;
  GtkWindow *parent = GTK_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (panel), GTK_TYPE_WINDOW));
  GTask *task;

  task = g_task_new (panel, NULL, callback, user_data);
  g_task_set_task_data (task, GINT_TO_POINTER (lockscreen), NULL);
  g_task_set_source_tag (task, ms_select_wallpaper_async);

  filechooser = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (filechooser, _("Choose Wallpaper"));

  filter = gtk_file_filter_new ();
  gtk_file_filter_add_pixbuf_formats (filter);

  filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
  g_list_store_append (filters, filter);
  gtk_file_dialog_set_filters (filechooser, G_LIST_MODEL (filters));

  pictures_folder = g_file_new_for_path (g_get_user_special_dir (G_USER_DIRECTORY_PICTURES));
  gtk_file_dialog_set_initial_folder (filechooser, pictures_folder);

  gtk_file_dialog_open (filechooser, parent, NULL, on_file_chooser_done, task);
}


gboolean
ms_select_wallpaper_finish (AdwBin *panel, GAsyncResult *result, GError **error)
{
  g_assert (ADW_IS_BIN (panel));
  g_assert (g_async_result_is_tagged (result, ms_select_wallpaper_async));
  g_assert (!error || !*error);

  return g_task_propagate_boolean (G_TASK (result), error);
}


gboolean
ms_picture_mode_to_bool (GValue *out_value, GVariant *in_variant, gpointer user_data)
{
  const char *mode = g_variant_get_string (in_variant, NULL);
  gboolean active;

  active = g_strcmp0 (mode, "none");
  g_value_set_boolean (out_value, active);
  return TRUE;
}


GVariant *
ms_bool_to_picture_mode (const GValue *in_value, const GVariantType *out_type, gpointer data)
{
  gboolean active = g_value_get_boolean (in_value);
  const char *mode;

  mode = active ? "zoom" : "none";

  return g_variant_new_string (mode);
}


/* Copied from gnome-control-center/panels/common/cc-util.c under the GPL
 *
 * Originally written by Aleksander Morgado <aleksander@gnu.org>
 */
char *
ms_normalize_casefold_and_unaccent (const char *str)
{
  g_autofree char *normalized = NULL;
  char *tmp;
  int i = 0, j = 0, ilen;

  if (str == NULL)
    return NULL;

  normalized = g_utf8_normalize (str, -1, G_NORMALIZE_NFKD);
  tmp = g_utf8_casefold (normalized, -1);

  ilen = strlen (tmp);

  while (i < ilen) {
    gunichar unichar;
    char *next_utf8;
    int utf8_len;

    /* Get next character of the word as UCS4 */
    unichar = g_utf8_get_char_validated (&tmp[i], -1);

    /* Invalid UTF-8 character or end of original string. */
    if (unichar == (gunichar) -1 || unichar == (gunichar) -2)
      break;

    /* Find next UTF-8 character */
    next_utf8 = g_utf8_next_char (&tmp[i]);
    utf8_len = next_utf8 - &tmp[i];

    if (IS_CDM_UCS4 (unichar) || IS_SOFT_HYPHEN (unichar)) {
      /* If the given unichar is a combining diacritical mark,
       * just update the original index, not the output one */
      i += utf8_len;
      continue;
    }

    /* If already found a previous combining
     * diacritical mark, indexes are different so
     * need to copy characters. As output and input
     * buffers may overlap, need to use memmove
     * instead of memcpy */
    if (i != j)
      memmove (&tmp[j], &tmp[i], utf8_len);

    /* Update both indexes */
    i += utf8_len;
    j += utf8_len;
  }

  /* Force proper string end */
  tmp[j] = '\0';

  return tmp;
}


GtkStringList *
ms_get_casefolded_string_list (GtkStringList *strlist)
{
  GtkStringList *casefolded_strlist;

  g_assert (GTK_IS_STRING_LIST (strlist));

  casefolded_strlist = gtk_string_list_new (NULL);

  for (uint i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (strlist)); ++i) {
    const char *keyword = gtk_string_list_get_string (strlist, i);

    gtk_string_list_take (casefolded_strlist, ms_normalize_casefold_and_unaccent (keyword));
  }

  return casefolded_strlist;
}
