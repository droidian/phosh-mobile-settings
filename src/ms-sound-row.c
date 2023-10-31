/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The handling of .local/share/sounds is based on cc-alert-chooser-window.c in
 * GNOME Settings which is:
 * Copyright (C) 2018 Canonical Ltd.
 * Copyright (C) 2023 Marco Melorio
 */

#define G_LOG_DOMAIN "ms-sound-row"

#include "mobile-settings-config.h"

#include "ms-feedback-panel.h"
#include "ms-sound-row.h"

#include <gsound.h>
#include <glib/gi18n.h>

#define SOUND_KEY_SCHEMA "org.gnome.desktop.sound"
#define CUSTOM_SOUND_THEME_NAME "__custom"
#define DIR_MODE 0700

#define STR_IS_NULL_OR_EMPTY(x) ((x) == NULL || (x)[0] == '\0')

/**
 * MsSoundRow:
 *
 * A AdwActionRow that allows to select a sound file
 */

enum {
  PROP_0,
  PROP_FILENAME,
  PROP_EFFECT_NAME,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsSoundRow {
  AdwActionRow          parent;

  char                 *filename;
  GtkWidget            *filename_label;
  char                 *effect_name;

  GtkFileFilter        *sound_filter;
  GSettings            *sound_settings;
};
G_DEFINE_TYPE (MsSoundRow, ms_sound_row, ADW_TYPE_ACTION_ROW)


static char *
ms_sound_row_get_theme_dir (void)
{
  return g_build_filename (g_get_user_data_dir (), "sounds", CUSTOM_SOUND_THEME_NAME, NULL);
}


static void
ms_sound_row_update_dir_mtime (const char *dir_path)
{
  g_autoptr (GFile) dir = NULL;
  g_autoptr (GDateTime) now = NULL;
  g_autoptr (GError) error = NULL;

  now = g_date_time_new_now_utc ();
  dir = g_file_new_for_path (dir_path);
  if (!g_file_set_attribute_uint64 (dir,
                                    G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                    g_date_time_to_unix (now),
                                    G_FILE_QUERY_INFO_NONE,
                                    NULL,
                                    &error)) {
    g_warning ("Failed to update directory modification time for %s: %s",
               dir_path, error->message);
  }
}


/* Update the sound theme if needed */
static void
ms_sound_row_set_custom_sound_theme (MsSoundRow *self)
{
  g_autofree char *dir = NULL;
  g_autofree char *theme_path = NULL;
  g_autofree char *sounds_path = NULL;
  g_autoptr (GKeyFile) theme_file = NULL;
  g_autoptr (GVariant) default_theme = NULL;
  g_autoptr (GError) load_error = NULL;
  g_autoptr (GError) save_error = NULL;

  dir = ms_sound_row_get_theme_dir ();
  theme_path = g_build_filename (dir, "index.theme", NULL);

  default_theme = g_settings_get_default_value (self->sound_settings, "theme-name");

  theme_file = g_key_file_new ();
  if (!g_key_file_load_from_file (theme_file, theme_path, G_KEY_FILE_KEEP_COMMENTS, &load_error)) {
    if (!g_error_matches (load_error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
        g_printerr ("Failed to load theme file %s: %s", theme_path, load_error->message);
  }

  if (g_strcmp0 (g_key_file_get_string (theme_file, "Sound Theme", "Directories", NULL), ".")) {
    g_key_file_set_string (theme_file, "Sound Theme", "Name", _("Custom"));
    if (default_theme != NULL)
      g_key_file_set_string (theme_file, "Sound Theme", "Inherits", g_variant_get_string (default_theme, NULL));
    g_key_file_set_string (theme_file, "Sound Theme", "Directories", ".");

    if (!g_key_file_save_to_file (theme_file, theme_path, &save_error))
      g_warning ("Failed to save theme file %s: %s", theme_path, save_error->message);
  } else {
    g_debug ("Skipping theme write");
  }

  /* Ensure canberra's event-sound-cache will get updated */
  sounds_path = g_build_filename (g_get_user_data_dir (), "sounds", NULL);
  ms_sound_row_update_dir_mtime (sounds_path);

  g_settings_set_string (self->sound_settings, "theme-name", CUSTOM_SOUND_THEME_NAME);
}


static void
ms_sound_row_set_symlink (MsSoundRow *self, const char *target_path)
{
  g_autofree char *dir = NULL;
  g_autofree char *link_filename = NULL;
  g_autofree char *link_name = NULL;
  g_autoptr (GFile) file = NULL;
  g_autoptr (GError) error = NULL;

  dir = ms_sound_row_get_theme_dir ();
  link_filename = g_strdup_printf ("%s.ogg", self->effect_name);
  link_name = g_build_filename (dir, link_filename, NULL);

  file = g_file_new_for_path (link_name);
  if (!g_file_delete (file, NULL, &error)) {
    if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
      g_warning ("Failed to remove existing sound symbolic link %s: %s", link_name, error->message);
  }

  if (target_path) {
    g_mkdir_with_parents (dir, DIR_MODE);
    if (!g_file_make_symbolic_link (file, target_path, NULL, &error))
      g_warning ("Failed to make sound theme symbolic link %s->%s: %s", link_name, target_path, error->message);

    ms_sound_row_set_custom_sound_theme (self);
  }
}


static char *
ms_sound_row_get_target (MsSoundRow *self)
{
  g_autofree char *dir = NULL;
  g_autofree char *path = NULL;
  g_autofree char *effect_filename = NULL;
  g_autoptr (GFile) file = NULL;
  g_autoptr (GFileInfo) info = NULL;
  g_autoptr (GError) error = NULL;
  const char *target;

  dir = ms_sound_row_get_theme_dir ();
  effect_filename = g_strdup_printf ("%s.ogg", self->effect_name);
  path = g_build_filename (dir, effect_filename, NULL);
  file = g_file_new_for_path (path);

  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
                            G_FILE_QUERY_INFO_NONE,
                            NULL,
                            &error);
  if (info == NULL) {
    if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
        g_warning ("Failed to get sound theme symlink %s: %s", path, error->message);
      return NULL;
  }

  target = g_file_info_get_attribute_byte_string (info, G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET);
  if (target == NULL)
    return NULL;

  return g_strdup (target);
}


static gboolean
filename_tranform_to (GBinding *binding, const GValue *from,  GValue *to,  gpointer user_data)
{
  const char *path = g_value_get_string (from);
  char *basename;

  if (path)
    basename = g_path_get_basename (path);
  else
    basename = g_strdup ("");

  g_value_take_string (to, basename);
  return TRUE;
}


static void
clear_filename_activated (GtkWidget *widget,  const char* action_name, GVariant *parameter)
{
  MsSoundRow *self = MS_SOUND_ROW (widget);

  ms_sound_row_set_filename (self, NULL);
}


static void
play_sound_activated  (GtkWidget *widget,  const char* action_name, GVariant *parameter)

{
  MsSoundRow *self = MS_SOUND_ROW (widget);

  g_return_if_fail (!STR_IS_NULL_OR_EMPTY (self->filename));
  gtk_widget_activate_action (widget, "sound-player.play", "s", self->filename);
}


static void
on_file_chooser_response (GtkNativeDialog* dialog, gint response_id, gpointer user_data)
{
  MsSoundRow *self = MS_SOUND_ROW (user_data);
  GtkFileChooser *filechooser = GTK_FILE_CHOOSER (dialog);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    if (response_id == GTK_RESPONSE_ACCEPT) {
      g_autoptr (GFile) file = gtk_file_chooser_get_file (filechooser);
      ms_sound_row_set_filename (self, g_file_get_path (file));
    }
G_GNUC_END_IGNORE_DEPRECATIONS

  g_object_unref (dialog);
}


static void
open_filechooser_activated (GtkWidget *widget,  const char* action_name, GVariant *parameter)
{
  MsSoundRow *self = MS_SOUND_ROW (widget);
  GtkFileChooserNative *filechooser;
  g_autoptr (GFile) current_file = NULL;
  GtkWindow *parent = GTK_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_WINDOW));


  g_assert (MS_IS_SOUND_ROW (self));
  gtk_widget_activate_action (GTK_WIDGET (self), "sound-player.stop", NULL, NULL);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  filechooser = gtk_file_chooser_native_new (_("Choose sound file"),
                                             parent,
                                             GTK_FILE_CHOOSER_ACTION_OPEN,
                                             _("_Open"),
                                             _("_Cancel"));
 gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (filechooser), self->sound_filter);
G_GNUC_END_IGNORE_DEPRECATIONS

  if (!STR_IS_NULL_OR_EMPTY (self->filename)) {
    current_file = g_file_new_for_path (self->filename);
    if (current_file) {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      gtk_file_chooser_set_file (GTK_FILE_CHOOSER (filechooser), current_file, NULL);
G_GNUC_END_IGNORE_DEPRECATIONS
    }
  }

  g_signal_connect (filechooser, "response", G_CALLBACK (on_file_chooser_response), self);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (filechooser));
}


static void
ms_sound_row_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  MsSoundRow *self = MS_SOUND_ROW (object);

  switch (property_id) {
  case PROP_FILENAME:
    ms_sound_row_set_filename (self, g_value_get_string (value));
    break;
  case PROP_EFFECT_NAME:
    self->effect_name = g_value_dup_string (value);
    ms_sound_row_set_filename (self, ms_sound_row_get_target (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_sound_row_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  MsSoundRow *self = MS_SOUND_ROW (object);

  switch (property_id) {
  case PROP_FILENAME:
    g_value_set_string (value, self->filename);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_sound_row_finalize (GObject *object)
{
  MsSoundRow *self = MS_SOUND_ROW(object);

  g_clear_object (&self->sound_settings);

  g_clear_pointer (&self->filename, g_free);
  g_clear_pointer (&self->effect_name, g_free);

  G_OBJECT_CLASS (ms_sound_row_parent_class)->finalize (object);
}


static void
ms_sound_row_class_init (MsSoundRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ms_sound_row_get_property;
  object_class->set_property = ms_sound_row_set_property;
  object_class->finalize = ms_sound_row_finalize;

  props[PROP_FILENAME] =
    g_param_spec_string ("filename", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_EFFECT_NAME] =
    g_param_spec_string ("effect-name", "", "",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-sound-row.ui");
  gtk_widget_class_bind_template_child (widget_class, MsSoundRow, filename_label);
  gtk_widget_class_bind_template_child (widget_class, MsSoundRow, sound_filter);

  gtk_widget_class_install_action (widget_class, "sound-row.open-filechooser", NULL,
                                   open_filechooser_activated);
  gtk_widget_class_install_action (widget_class, "sound-row.clear-filename", NULL,
                                   clear_filename_activated);
  gtk_widget_class_install_action (widget_class, "sound-row.play-sound", NULL,
                                   play_sound_activated);
}


static void
ms_sound_row_init (MsSoundRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "sound-row.clear-filename", FALSE);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "sound-row.play-sound", FALSE);

  g_object_bind_property_full (self, "filename",
                               self->filename_label, "label",
                               G_BINDING_DEFAULT,
                               filename_tranform_to,
                               NULL,
                               NULL,
                               NULL);

  self->sound_settings = g_settings_new (SOUND_KEY_SCHEMA);
}


MsSoundRow *
ms_sound_row_new (void)
{
  return MS_SOUND_ROW (g_object_new (MS_TYPE_SOUND_ROW, NULL));
}


void
ms_sound_row_set_filename (MsSoundRow *self, const char *filename)
{
  g_return_if_fail (MS_IS_SOUND_ROW (self));

  if (g_strcmp0 (self->filename, filename) == 0)
      return;

  g_free (self->filename);
  self->filename = g_strdup (filename);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "sound-row.clear-filename",
                                 !STR_IS_NULL_OR_EMPTY (self->filename));
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "sound-row.play-sound",
                                 !STR_IS_NULL_OR_EMPTY (self->filename));
  gtk_widget_activate_action (GTK_WIDGET (self), "sound-player.stop", NULL, NULL);

  ms_sound_row_set_symlink (self, self->filename);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FILENAME]);
}
