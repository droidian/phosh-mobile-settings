/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-page-builder"

#include "ms-tweaks-page-builder.h"

#include "ms-tweaks-backend-interface.h"
#include "ms-tweaks-callback-handlers.h"
#include "ms-tweaks-mappings.h"
#include "ms-tweaks-utils.h"


enum {
  PROP_0,
  PROP_DATA,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


struct _MsTweaksPreferencesPage {
  AdwPreferencesPage parent_instance;

  const MsTweaksPage *data;
};


static void
set_title_and_subtitle (GtkWidget *widget, const MsTweaksSetting *setting_data)
{
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (widget), setting_data->name);
  adw_action_row_set_subtitle (ADW_ACTION_ROW (widget), setting_data->help);
}


static GtkStringList *
get_keys_from_hashtable (GHashTable *hashtable)
{
  g_autoptr (GPtrArray) hash_table_keys_view = g_hash_table_get_keys_as_ptr_array (hashtable);
  GtkStringList *hash_table_key_list = gtk_string_list_new ((const char **) hash_table_keys_view->pdata);

  return hash_table_key_list;
}


static GtkWidget *
setting_data_to_boolean_widget (const MsTweaksSetting *setting_data,
                                MsTweaksBackend       *backend_state,
                                GValue                *widget_value)
{
  GtkWidget *switch_row = adw_switch_row_new ();

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend_state));

  set_title_and_subtitle (switch_row, setting_data);

  if (widget_value)
    adw_switch_row_set_active (ADW_SWITCH_ROW (switch_row), g_value_get_boolean (widget_value));

  g_signal_connect (switch_row,
                    "notify::active",
                    G_CALLBACK (ms_tweaks_callback_handlers_type_boolean),
                    backend_state);

  return switch_row;
}


static GtkWidget *
setting_data_to_choice_widget (const MsTweaksSetting *setting_data,
                               MsTweaksBackend       *backend_state,
                               GValue                *widget_value)
{
  GtkWidget *combo_row = adw_combo_row_new ();
  GtkStringList *choice_model = NULL;

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend_state));

  if (setting_data->map) {
    choice_model = get_keys_from_hashtable (setting_data->map);
    adw_combo_row_set_model (ADW_COMBO_ROW (combo_row), G_LIST_MODEL (choice_model));

    if (widget_value) {
      gpointer key = NULL, value = NULL;
      gboolean found = FALSE;
      GHashTableIter iter;
      guint at_index = G_MAXUINT;

      /* Iterate over choices to find the index of the one we want to set as default. */
      g_hash_table_iter_init (&iter, setting_data->map);
      while (g_hash_table_iter_next (&iter, &key, &value)) {
        at_index = G_MAXUINT;

        if (g_str_equal (value, g_value_get_string (widget_value)))
          at_index = gtk_string_list_find (choice_model, key);

        found = at_index != G_MAXUINT;

        if (found) {
          adw_combo_row_set_selected (ADW_COMBO_ROW (combo_row), at_index);
          break;
        }
      }
    }
  } else {
    ms_tweaks_warning (setting_data->name,
                       "Choice widget with NULL map — either the datasource failed or the markup is wrong");
    return NULL;
  }

  set_title_and_subtitle (combo_row, setting_data);

  g_signal_connect (combo_row,
                    "notify::selected",
                    G_CALLBACK (ms_tweaks_callback_handlers_type_choice),
                    backend_state);

  return combo_row;
}


static GtkWidget *
setting_data_to_color_widget (const MsTweaksSetting *setting_data,
                              MsTweaksBackend       *backend_state,
                              GValue                *widget_value)
{
  GdkRGBA widget_colour;
  GtkWidget *restrict action_row = adw_action_row_new ();
  GtkColorDialog *color_dialog = gtk_color_dialog_new ();
  const char *colour_from_backend = g_value_get_string (widget_value);
  GtkWidget *restrict color_dialog_button = gtk_color_dialog_button_new (color_dialog);

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend_state));

  set_title_and_subtitle (action_row, setting_data);
  adw_action_row_add_suffix (ADW_ACTION_ROW (action_row), color_dialog_button);

  gdk_rgba_parse (&widget_colour, colour_from_backend);
  gtk_color_dialog_button_set_rgba (GTK_COLOR_DIALOG_BUTTON (color_dialog_button),
                                    &widget_colour);

  /* Set up listener for listening to colours picked by the widget and setting them in the
   * backend. */
  g_signal_connect (color_dialog_button,
                    "notify::rgba",
                    G_CALLBACK (ms_tweaks_callback_handlers_type_color),
                    backend_state);

  return action_row;
}


static void
file_widget_open_file_picker (GtkButton                                 *widget,
                              GdkEvent                                  *evnt,
                              MsTweaksPageBuilderOpenFilePickerMetadata *metadata)
{
  GtkFileDialog *file_picker_dialog = gtk_file_dialog_new ();

  gtk_file_dialog_open (file_picker_dialog,
                        NULL,
                        NULL,
                        ms_tweaks_callback_handlers_type_file,
                        metadata);
}


static const char *restrict none_selected_label = "(None selected)";


static void
file_widget_unset (GtkButton                                 *widget,
                   GdkEvent                                  *evnt,
                   MsTweaksPageBuilderOpenFilePickerMetadata *metadata)
{
  MsTweaksBackendInterface *backend = MS_TWEAKS_BACKEND_GET_IFACE (metadata->backend_state);

  g_assert (backend->set_value);

  backend->set_value (metadata->backend_state, NULL);

  gtk_label_set_label (GTK_LABEL (metadata->file_picker_label), none_selected_label);
}


static GtkWidget *
setting_data_to_file_widget (const MsTweaksSetting                     *setting_data,
                             MsTweaksBackend                           *backend_state,
                             const GValue                              *widget_value,
                             MsTweaksPageBuilderOpenFilePickerMetadata *metadata)
{
  GtkWidget *restrict reset_selection_button = gtk_button_new ();
  GtkWidget *restrict file_picker_row = adw_action_row_new ();
  GtkWidget *restrict file_picker_button = gtk_button_new ();
  GtkWidget *restrict file_picker_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *restrict file_picker_icon = gtk_image_new_from_icon_name ("folder-open-symbolic");

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend_state));

  metadata->backend_state = backend_state;
  metadata->file_picker_label = gtk_label_new (none_selected_label);

  set_title_and_subtitle (file_picker_row, setting_data);

  adw_action_row_add_suffix (ADW_ACTION_ROW (file_picker_row), reset_selection_button);
  adw_action_row_add_suffix (ADW_ACTION_ROW (file_picker_row), file_picker_button);

  gtk_box_append (GTK_BOX (file_picker_box), file_picker_icon);
  gtk_button_set_child (GTK_BUTTON (file_picker_button), file_picker_box);
  gtk_button_set_icon_name (GTK_BUTTON (reset_selection_button), "document-revert-symbolic");

  if (widget_value) {
    const char *file_path = g_value_get_string (widget_value);
    g_autofree char *filename = g_path_get_basename (file_path);

    gtk_label_set_text (GTK_LABEL (metadata->file_picker_label), filename);
  }

  g_signal_connect (file_picker_button,
                    "clicked",
                    G_CALLBACK (file_widget_open_file_picker),
                    metadata);
  g_signal_connect (reset_selection_button,
                    "clicked",
                    G_CALLBACK (file_widget_unset),
                    metadata);

  return file_picker_row;
}


static GtkWidget *
setting_data_to_font_widget (const MsTweaksSetting *setting_data,
                             MsTweaksBackend       *backend_state,
                             const GValue          *widget_value)
{
  GtkWidget *restrict action_row = adw_action_row_new ();
  GtkFontDialog *font_dialog = gtk_font_dialog_new ();
  GtkWidget *restrict font_dialog_button = gtk_font_dialog_button_new (font_dialog);

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend_state));

  set_title_and_subtitle (action_row, setting_data);
  adw_action_row_add_suffix (ADW_ACTION_ROW (action_row), font_dialog_button);

  if (widget_value) {
    const char *font_name = g_value_get_string (widget_value);
    PangoFontDescription *font_desc = pango_font_description_from_string (font_name);

    gtk_font_dialog_button_set_font_desc (GTK_FONT_DIALOG_BUTTON (font_dialog_button), font_desc);
    pango_font_description_free (font_desc);
  }

  g_signal_connect (font_dialog_button,
                    "notify::font-desc",
                    G_CALLBACK (ms_tweaks_callback_handlers_type_font),
                    backend_state);

  return action_row;
}


static GtkWidget *
setting_data_to_info_widget (const MsTweaksSetting *setting_data,
                             const GValue          *widget_value)
{
  g_assert (setting_data);

  if (widget_value) {
    GtkWidget *action_row = adw_action_row_new ();

    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (action_row), setting_data->name);
    adw_action_row_set_subtitle (ADW_ACTION_ROW (action_row), g_value_get_string (widget_value));

    adw_action_row_set_subtitle_selectable (ADW_ACTION_ROW (action_row), true);
    gtk_widget_add_css_class (action_row, "property");

    return action_row;
  } else {
    ms_tweaks_warning (setting_data->name,
                       "widget_value was NULL in setting_data_to_boolean_widget ()");
    return NULL;
  }
}


static GtkWidget *
setting_data_to_number_widget (const MsTweaksSetting *setting_data,
                               MsTweaksBackend       *backend_state,
                               const GValue          *widget_value)
{
  GtkWidget *spin_row = adw_spin_row_new_with_range (setting_data->min,
                                                     setting_data->max,
                                                     setting_data->step);

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend_state));

  set_title_and_subtitle (spin_row, setting_data);

  if (widget_value)
    adw_spin_row_set_value (ADW_SPIN_ROW (spin_row), g_value_get_double (widget_value));

  g_signal_connect (spin_row,
                    "changed",
                    G_CALLBACK (ms_tweaks_callback_handlers_type_number),
                    backend_state);

  return spin_row;
}


static gboolean
ms_tweaks_preferences_page_initable_init (GInitable     *initable,
                                          GCancellable  *cancellable,
                                          GError       **error)
{
  MsTweaksPreferencesPage *self = MS_TWEAKS_PREFERENCES_PAGE (initable);
  const GList *section_list = ms_tweaks_parser_sort_by_weight (self->data->section_table);
  gboolean page_widget_is_valid = FALSE;

  for (const GList *section_iter = section_list; section_iter; section_iter = section_iter->next) {
    const MsTweaksSection *const section_data = section_iter->data;
    const GList *setting_list = ms_tweaks_parser_sort_by_weight (section_data->setting_table);
    GtkWidget *const restrict section_preference_group = adw_preferences_group_new ();
    gboolean section_widget_is_valid = FALSE;

    adw_preferences_group_set_title (ADW_PREFERENCES_GROUP (section_preference_group),
                                     section_data->name);

    for (const GList *setting_iter = setting_list; setting_iter; setting_iter = setting_iter->next) {
      MsTweaksSetting *const setting_data = setting_iter->data;
      GtkWidget *restrict widget_to_add = NULL;
      gboolean setting_widget_is_valid = TRUE;
      MsTweaksBackend *backend_state = NULL;
      GValue *widget_value = NULL;

      switch (setting_data->backend) {
      case MS_TWEAKS_BACKEND_IDENTIFIER_HARDWAREINFO:
        ms_tweaks_warning (setting_data->name,
                           "The hardwareinfo backend is not supported. Please see the documentation.");
        setting_widget_is_valid = FALSE;
        break;
      case MS_TWEAKS_BACKEND_IDENTIFIER_OSKSDL:
        ms_tweaks_warning (setting_data->name,
                           "The OSKSDL backend is not supported. Please see the documentation.");
        setting_widget_is_valid = FALSE;
        break;
      case MS_TWEAKS_BACKEND_IDENTIFIER_UNKNOWN:
        ms_tweaks_warning (setting_data->name,
                           "Unknown backend type, cannot get value. Is your system up-to-date?");
        break;
      case MS_TWEAKS_BACKEND_IDENTIFIER_CSS:
      case MS_TWEAKS_BACKEND_IDENTIFIER_GSETTINGS:
      case MS_TWEAKS_BACKEND_IDENTIFIER_GTK3SETTINGS:
      case MS_TWEAKS_BACKEND_IDENTIFIER_SYSFS:
      case MS_TWEAKS_BACKEND_IDENTIFIER_XRESOURCES:
      case MS_TWEAKS_BACKEND_IDENTIFIER_SOUNDTHEME:
      case MS_TWEAKS_BACKEND_IDENTIFIER_SYMLINK:
      default:
        ms_tweaks_warning (setting_data->name,
                           "Unimplemented backend type \"%i\"",
                           setting_data->backend);
        break;
      }

      /* Ensure that we actually constructed a tweaks backend. */
      if (!MS_IS_TWEAKS_BACKEND (backend_state)) {
        ms_tweaks_warning (setting_data->name, "Failed to construct backend, ignoring");
        continue;
      }

      /* Get widget value. */
      if (MS_TWEAKS_BACKEND_GET_IFACE (backend_state)->get_value != NULL)
        widget_value = MS_TWEAKS_BACKEND_GET_IFACE (backend_state)->get_value (backend_state);

      /* Handle mappings. */
      if (widget_value)
        ms_tweaks_mappings_handle_get (widget_value, setting_data);

      if (setting_widget_is_valid) {
        switch (setting_data->type) {
        case MS_TWEAKS_TYPE_BOOLEAN:
          widget_to_add = setting_data_to_boolean_widget (setting_data,
                                                          backend_state,
                                                          widget_value);
          break;
        case MS_TWEAKS_TYPE_CHOICE:
          widget_to_add = setting_data_to_choice_widget (setting_data, backend_state, widget_value);
          break;
        case MS_TWEAKS_TYPE_COLOR:
          widget_to_add = setting_data_to_color_widget (setting_data, backend_state, widget_value);
          break;
        case MS_TWEAKS_TYPE_FILE:
          MsTweaksPageBuilderOpenFilePickerMetadata *metadata = g_new (MsTweaksPageBuilderOpenFilePickerMetadata, 1);
          widget_to_add = setting_data_to_file_widget (setting_data,
                                                       backend_state,
                                                       widget_value,
                                                       metadata);
          break;
        case MS_TWEAKS_TYPE_FONT:
          widget_to_add = setting_data_to_font_widget (setting_data, backend_state, widget_value);
          break;
        case MS_TWEAKS_TYPE_INFO:
          widget_to_add = setting_data_to_info_widget (setting_data, widget_value);
          break;
        case MS_TWEAKS_TYPE_NUMBER:
          widget_to_add = setting_data_to_number_widget (setting_data, backend_state, widget_value);
          break;
        case MS_TWEAKS_TYPE_UNKNOWN:
          ms_tweaks_warning (setting_data->name,
                             "Unknown type, cannot create widget. Is your system up-to-date?");
          continue;
        default:
          ms_tweaks_critical (setting_data->name,
                              "Unimplemented setting type \"%i\"",
                              setting_data->type);
        }
      }

      if (!widget_to_add)
        setting_widget_is_valid = FALSE;

      if (setting_widget_is_valid) {
        adw_preferences_group_add (ADW_PREFERENCES_GROUP (section_preference_group), widget_to_add);
        section_widget_is_valid = TRUE;
      } else
        ms_tweaks_warning (setting_data->name, "Failed to construct widget");
    }

    if (section_widget_is_valid) {
      adw_preferences_page_add (ADW_PREFERENCES_PAGE (&self->parent_instance),
                                ADW_PREFERENCES_GROUP (section_preference_group));
      page_widget_is_valid = TRUE;
    } else
      g_warning ("No valid settings in section \"%s\" inside page \"%s\", hiding it",
                 section_data->name,
                 self->data->name);
  }

  return page_widget_is_valid;
}


MsTweaksPreferencesPage *
ms_tweaks_preferences_page_new (const MsTweaksPage *data)
{
  return g_initable_new (MS_TYPE_TWEAKS_PREFERENCES_PAGE, NULL, NULL, "data", data, NULL);
}


static void
ms_tweaks_preferences_page_initable_iface_init (GInitableIface *iface)
{
  iface->init = ms_tweaks_preferences_page_initable_init;
}


G_DEFINE_TYPE_WITH_CODE (MsTweaksPreferencesPage,
                         ms_tweaks_preferences_page,
                         ADW_TYPE_PREFERENCES_PAGE,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                ms_tweaks_preferences_page_initable_iface_init))


static void
ms_tweaks_preferences_page_init (MsTweaksPreferencesPage *self)
{

}


static void
ms_tweaks_preferences_page_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  MsTweaksPreferencesPage *self = MS_TWEAKS_PREFERENCES_PAGE (object);

  switch (property_id) {
  case PROP_DATA:
    self->data = g_value_get_boxed (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_tweaks_preferences_page_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  MsTweaksPreferencesPage *self = MS_TWEAKS_PREFERENCES_PAGE (object);

  switch (property_id) {
  case PROP_DATA:
    g_value_set_boxed (value, self->data);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_tweaks_preferences_page_class_init (MsTweaksPreferencesPageClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = ms_tweaks_preferences_page_set_property;
  gobject_class->get_property = ms_tweaks_preferences_page_get_property;

  props[PROP_DATA] = g_param_spec_boxed ("data", NULL, NULL, MS_TYPE_TWEAKS_PAGE, G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, G_N_ELEMENTS (props), props);
}
