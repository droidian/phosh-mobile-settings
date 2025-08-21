/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "ms-tweaks-callback-handlers.h"
#include "ms-tweaks-gtk-utils.h"
#include "ms-tweaks-mappings.h"
#include "ms-tweaks-utils.h"


static void
do_set_value (MsTweaksBackend *backend_state, GValue *value_to_set)
{
  const MsTweaksSetting *setting_data = NULL;

  g_assert (MS_IS_TWEAKS_BACKEND (backend_state));
  g_assert (MS_TWEAKS_BACKEND_GET_IFACE (backend_state)->get_setting_data);
  g_assert (MS_TWEAKS_BACKEND_GET_IFACE (backend_state)->set_value);

  setting_data = MS_TWEAKS_BACKEND_GET_IFACE (backend_state)->get_setting_data (backend_state);

  ms_tweaks_mappings_handle_set (value_to_set, setting_data);

  MS_TWEAKS_BACKEND_GET_IFACE (backend_state)->set_value (backend_state, value_to_set);
}

/**
 * ms_tweaks_callback_handlers_type_boolean:
 * @switch_row: Instance of AdwSwitchRow to read the state of. Should be provided by
 *              g_signal_connect (), see explanation for `unused`.
 * @unused: Property spec as provided by the callback from g_signal_connect () when connected to the
 *          "notify::active" signal.
 * @backend_state: Any MsTweaksBackend.
 *
 * Generic handler for MsTweaksBackend types that support MS_TWEAKS_TYPE_BOOLEAN.
 */
void
ms_tweaks_callback_handlers_type_boolean (AdwSwitchRow    *switch_row,
                                          GParamSpec      *unused,
                                          MsTweaksBackend *backend_state)
{
  const gboolean is_active = adw_switch_row_get_active (switch_row);
  GValue is_active_container = G_VALUE_INIT;

  g_value_init (&is_active_container, G_TYPE_BOOLEAN);
  g_value_set_boolean (&is_active_container, is_active);

  do_set_value (backend_state, &is_active_container);
}

/**
 * ms_tweaks_callback_handlers_type_choice:
 * @combo_row: Instance of AdwComboRow to read the state of. Should be provided by
 *             g_signal_connect (), see explanation for `unused`.
 * @unused: Property spec as provided by the callback from g_signal_connect () when connected to the
 *          "notify::selected" signal.
 * @backend_state: Any MsTweaksBackend.
 *
 * Generic handler for MsTweaksBackend types that support MS_TWEAKS_TYPE_CHOICE.
 */
void
ms_tweaks_callback_handlers_type_choice (AdwComboRow     *combo_row,
                                         GParamSpec      *unused,
                                         MsTweaksBackend *backend_state)
{
  GtkStringList *list = GTK_STRING_LIST (adw_combo_row_get_model (combo_row));
  const guint selected = adw_combo_row_get_selected (combo_row);
  const char *string_repr = gtk_string_list_get_string (list, selected);
  const MsTweaksSetting *setting_data = NULL;
  GValue value_container = G_VALUE_INIT;
  char *value = NULL;

  g_assert (MS_IS_TWEAKS_BACKEND (backend_state));
  g_assert (MS_TWEAKS_BACKEND_GET_IFACE (backend_state)->get_setting_data);

  setting_data = MS_TWEAKS_BACKEND_GET_IFACE (backend_state)->get_setting_data (backend_state);
  value = g_hash_table_lookup (setting_data->map, string_repr);

  if (!value) {
    ms_tweaks_critical (setting_data->name,
                        "Couldn't find data in map even when it definitely should have been there");
    return;
  }

  g_value_init (&value_container, G_TYPE_STRING);
  g_value_set_string (&value_container, value);

  do_set_value (backend_state, &value_container);
}


/**
 * ms_tweaks_callback_handlers_type_color:
 * @widget: Instance of GtkColorDialogButton to read the state of. Should be provided by
 *          g_signal_connect (), see explanation for `unused`.
 * @unused: Property spec as provided by the callback from g_signal_connect () when connected to the
 *          "notify::rgba" signal.
 * @backend_state: Any MsTweaksBackend.
 *
 * Generic handler for MsTweaksBackend types that support MS_TWEAKS_TYPE_COLOR.
 */
void
ms_tweaks_callback_handlers_type_color (GtkColorDialogButton *widget,
                                        GParamSpec           *unused,
                                        MsTweaksBackend      *backend_state)
{
  const GdkRGBA *picked_colour = gtk_color_dialog_button_get_rgba (widget);
  GValue picked_colour_container = G_VALUE_INIT;

  g_value_init (&picked_colour_container, G_TYPE_STRING);
  g_value_set_string (&picked_colour_container,
                      ms_tweaks_util_gdkrgba_to_rgb_hex_string (picked_colour));

  do_set_value (backend_state, &picked_colour_container);
}


void
ms_tweaks_callback_handlers_type_file (GObject *source_object, GAsyncResult *result, gpointer data)
{
  MsTweaksPageBuilderOpenFilePickerMetadata *metadata = (MsTweaksPageBuilderOpenFilePickerMetadata *) data;
  g_autoptr (GtkFileDialog) file_picker_dialog = NULL;
  const MsTweaksSetting *setting_data = NULL;
  GValue path_to_picked_file = G_VALUE_INIT;
  g_autofree char *picked_file_name = NULL;
  g_autoptr (GFile) picked_file = NULL;
  g_autoptr (GError) error = NULL;

  g_assert (metadata->backend_state);
  g_assert (MS_TWEAKS_BACKEND_GET_IFACE (metadata->backend_state)->get_setting_data);

  file_picker_dialog = GTK_FILE_DIALOG (source_object);
  picked_file = gtk_file_dialog_open_finish (file_picker_dialog, result, &error);
  setting_data = MS_TWEAKS_BACKEND_GET_IFACE (metadata->backend_state)->get_setting_data (metadata->backend_state);

  if (error != NULL) {
    ms_tweaks_warning (setting_data->name, "Something went wrong when picking a file: %s",
                       error->message);
    return;
  }

  g_value_init (&path_to_picked_file, G_TYPE_STRING);
  g_value_set_string (&path_to_picked_file, g_file_get_path (picked_file));

  do_set_value (metadata->backend_state, &path_to_picked_file);

  picked_file_name = g_path_get_basename (g_value_get_string (&path_to_picked_file));
  gtk_label_set_text (GTK_LABEL (metadata->file_picker_label), picked_file_name);
}

/**
 * ms_tweaks_callback_handlers_type_font:
 * @widget: Instance of GtkFontDialogButton to read the state of. Should be provided by
 *          g_signal_connect (), see explanation for `unused`.
 * @unused: Property spec as provided by the callback from g_signal_connect () when connected to the
 *          "notify::font-desc" signal.
 * @backend_state: Any MsTweaksBackend.
 *
 * Generic handler for MsTweaksBackend types that support MS_TWEAKS_TYPE_FONT.
 */
void
ms_tweaks_callback_handlers_type_font (GtkFontDialogButton *widget,
                                       GParamSpec          *unused,
                                       MsTweaksBackend     *backend_state)
{
  PangoFontDescription *font_desc = gtk_font_dialog_button_get_font_desc (widget);
  GValue font_container = G_VALUE_INIT;

  g_value_init (&font_container, G_TYPE_STRING);
  g_value_set_string (&font_container, pango_font_description_to_string (font_desc));

  do_set_value (backend_state, &font_container);
}


void
ms_tweaks_callback_handlers_type_number (AdwSpinRow *spin_row, MsTweaksBackend *backend_state)
{
  const double spin_row_value = adw_spin_row_get_value (spin_row);
  GValue spin_row_value_container = G_VALUE_INIT;
  g_autofree char *spin_row_value_string = NULL;

  g_assert (MS_IS_TWEAKS_BACKEND (backend_state));

  spin_row_value_string = g_strdup_printf ("%f", spin_row_value);

  g_value_init (&spin_row_value_container, G_TYPE_STRING);
  g_value_set_string (&spin_row_value_container, spin_row_value_string);

  do_set_value (backend_state, &spin_row_value_container);
}
