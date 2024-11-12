/*
 * Copyright (C) 2023-2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-osk-panel"

#include "mobile-settings-config.h"
#include "mobile-settings-enums.h"
#include "ms-completer-info.h"
#include "ms-enum-types.h"
#include "ms-osk-layout-prefs.h"
#include "ms-osk-panel.h"
#include "ms-util.h"

#include <gmobile.h>

#include <glib/gi18n.h>

#define PHOSH_OSK_DBUS_NAME "sm.puri.OSK0"

#define A11Y_SETTINGS                "org.gnome.desktop.a11y.applications"
#define OSK_ENABLED_KEY              "screen-keyboard-enabled"

#define PHOSH_SETTINGS               "sm.puri.phosh"
#define OSK_UNFOLD_DELAY_KEY         "osk-unfold-delay"

#define PHOSH_OSK_SETTINGS           "sm.puri.phosh.osk"
#define WORD_COMPLETION_KEY          "completion-mode"
#define HW_KEYBOARD_KEY              "ignore-hw-keyboards"

#define PHOSH_OSK_COMPLETER_SETTINGS "sm.puri.phosh.osk.Completers"
#define DEFAULT_COMPLETER_KEY        "default"
#define POS_COMPLETER_SUFFIX         ".completer"

#define PHOSH_OSK_TERMINAL_SETTINGS  "sm.puri.phosh.osk.Terminal"
#define SHORTCUTS_KEY                "shortcuts"

#define SQUEEKBOARD_SETTINGS         "sm.puri.Squeekboard"
#define SCALE_WHEN_HORIZONTAL_KEY    "scale-in-horizontal-screen-orientation"
#define SCALE_WHEN_VERTICAL_KEY      "scale-in-vertical-screen-orientation"

/* From phosh osk-stub */
typedef enum {
  PHOSH_OSK_COMPLETION_MODE_NONE   = 0,
  PHOSH_OSK_COMPLETION_MODE_MANUAL = (1 << 0),
  PHOSH_OSK_COMPLETION_MODE_HINT   = (1 << 1),
} CompletionMode;


typedef enum {
  MS_OSK_APP_UNKNOWN = 0,
  MS_OSK_APP_POS = 1,
  MS_OSK_APP_SQUEEKBOARD = 2
} MsOskApp;


struct _MsOskPanel {
  AdwBin            parent;

  GSettings        *a11y_settings;
  GtkWidget        *osk_enable_switch;
  GtkWidget        *osk_layout_prefs;

  GSettings        *phosh_settings;
  GtkWidget        *long_press_combo;

  /* Word completion */
  GSettings        *pos_settings;
  GSettings        *pos_completer_settings;
  GtkWidget        *hw_keyboard_switch;
  GtkWidget        *completion_group;
  AdwSwitchRow     *app_completion_switch;
  AdwSwitchRow     *menu_completion_switch;
  CompletionMode    mode;
  gboolean          updating_flags;
  AdwComboRow      *completer_combo;
  GListStore       *completer_infos;

  /* Terminal layout */
  GSettings        *pos_terminal_settings;
  GtkWidget        *terminal_layout_group;
  GtkWidget        *shortcuts_box;
  GListStore       *shortcuts;
  gboolean          shortcuts_updating;

  GtkWidget        *keyboard_height_prefs;
  GtkWidget        *scale_in_horizontal_orientation;
  GtkWidget        *scale_in_vertical_orientation;
};

G_DEFINE_TYPE (MsOskPanel, ms_osk_panel, ADW_TYPE_BIN)


static void
sync_settings (MsOskPanel *self)
{
  g_auto (GStrv) settings = NULL;
  g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();
  GListModel *model = G_LIST_MODEL (self->shortcuts);

  self->shortcuts_updating = TRUE;

  for (int i = 0; i < g_list_model_get_n_items (model); i++) {
    g_autoptr (GtkStringObject) string = g_list_model_get_item (model, i);

    g_strv_builder_add (builder, gtk_string_object_get_string (string));
  }

  settings = g_strv_builder_end (builder);
  g_settings_set_strv (self->pos_terminal_settings, SHORTCUTS_KEY, (const char *const *)settings);

  self->shortcuts_updating = FALSE;
}


static void
on_drag_begin (GtkDragSource *source, GdkDrag *drag, GtkWidget *shortcut)
{
  GdkPaintable *paintable = gtk_widget_paintable_new (GTK_WIDGET (shortcut));

  gtk_drag_source_set_icon (source, paintable,
                            gtk_widget_get_width (shortcut) / 2,
                            gtk_widget_get_height (shortcut) / 2);
  g_object_unref (paintable);
}


static gboolean
on_drop (GtkDropTarget *drop_target, const GValue  *value, double x, double y, gpointer data)
{
  MsOskPanel *self = MS_OSK_PANEL (data);
  g_autoptr (GtkStringObject) dropped = NULL;
  GtkStringObject *target;
  const char *dropped_accel, *target_accel;
  guint dropped_index, target_index;
  gboolean success;

  g_assert (MS_IS_OSK_PANEL (self));

  if (!G_VALUE_HOLDS (value, GTK_TYPE_STRING_OBJECT)) {
    g_warning ("Dropped unhandled type");
    return FALSE;
  }

  target = g_object_get_data (G_OBJECT (drop_target), "ms-str");
  target_accel = gtk_string_object_get_string (target);
  success = g_list_store_find (self->shortcuts, target, &target_index);
  g_assert (success);

  dropped = g_object_ref (g_value_get_object (value));
  dropped_accel = gtk_string_object_get_string (dropped);
  g_debug ("Dropped %s on %s", dropped_accel, target_accel);

  success = g_list_store_find (self->shortcuts, dropped, &dropped_index);
  g_assert (success);
  g_list_store_remove (self->shortcuts, dropped_index);
  g_list_store_insert (self->shortcuts, target_index, dropped);

  sync_settings (self);

  return TRUE;
}


static GtkWidget *
create_shortcuts_row (gpointer item, gpointer user_data)
{
  MsOskPanel *self = MS_OSK_PANEL (user_data);
  GtkStringObject *string = GTK_STRING_OBJECT (item);
  GtkWidget *label = gtk_shortcut_label_new (gtk_string_object_get_string (string));
  GtkDragSource *drag_source = gtk_drag_source_new ();
  GdkContentProvider *type = gdk_content_provider_new_typed (GTK_TYPE_STRING_OBJECT, string);
  GtkDropTarget *target = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
  GType targets[] = { GTK_TYPE_STRING_OBJECT };

  /* drag */
  gtk_drag_source_set_content (drag_source, type);
  g_signal_connect (drag_source, "drag-begin", G_CALLBACK (on_drag_begin), label);
  gtk_widget_add_controller (label, GTK_EVENT_CONTROLLER (drag_source));

  /* drop */
  gtk_drop_target_set_gtypes (target, targets, G_N_ELEMENTS (targets));
  g_signal_connect (target, "drop", G_CALLBACK (on_drop), self);
  /* No ref as we just use it for string comparison */
  g_object_set_data (G_OBJECT (target), "ms-str", string);
  gtk_widget_add_controller (label, GTK_EVENT_CONTROLLER (target));

  return label;
}


static void
on_terminal_shortcuts_changed (MsOskPanel *self)
{
  g_auto (GStrv) shortcuts = NULL;

  if (self->shortcuts_updating)
    return;

  g_list_store_remove_all (self->shortcuts);

  shortcuts = g_settings_get_strv (self->pos_terminal_settings, SHORTCUTS_KEY);
  for (int i = 0; shortcuts[i] != NULL; i++) {
    g_autoptr (GtkStringObject) shortcut = gtk_string_object_new (shortcuts[i]);

    g_list_store_append (self->shortcuts, shortcut);
  }
}


static void
on_word_completion_key_changed (MsOskPanel *self)
{
  self->mode = g_settings_get_flags (self->pos_settings, WORD_COMPLETION_KEY);
  self->updating_flags = TRUE;

  adw_switch_row_set_active (self->menu_completion_switch,
                             self->mode & PHOSH_OSK_COMPLETION_MODE_MANUAL);
  adw_switch_row_set_active (self->app_completion_switch,
                             self->mode & PHOSH_OSK_COMPLETION_MODE_HINT);
  self->updating_flags = FALSE;
}


static void
on_completion_switch_activate_changed (MsOskPanel *self, GParamSpec *spec, AdwSwitchRow *switch_)
{
  CompletionMode flag;
  gboolean active;

  if (self->updating_flags)
    return;

  if (switch_ == self->app_completion_switch) {
    flag = PHOSH_OSK_COMPLETION_MODE_HINT;
  } else if (switch_ == self->menu_completion_switch) {
    flag = PHOSH_OSK_COMPLETION_MODE_MANUAL;
  } else {
    g_critical ("Unknown completion switch");
    return;
  }

  active = adw_switch_row_get_active (switch_);
  if (active)
    self->mode |= flag;
  else
    self->mode ^= flag;

  g_settings_set_flags (self->pos_settings, WORD_COMPLETION_KEY, self->mode);
}


static MsOskApp
is_osk_app (void)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GDBusProxy) proxy = NULL;
  g_autoptr (GVariant) ret = NULL;
  g_autofree char *proc_path = NULL;
  g_autofree char *exe = NULL;
  guint32 pid;
  const char *forced_osk;

  forced_osk = g_getenv ("MS_FORCE_OSK");
  if (forced_osk) {
    if (g_str_equal (forced_osk, "pos"))
      return MS_OSK_APP_POS;
    else if (g_str_equal (forced_osk, "squeekboard"))
      return MS_OSK_APP_SQUEEKBOARD;
    else
      return MS_OSK_APP_UNKNOWN;
  }

  proxy =  g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                          G_DBUS_PROXY_FLAGS_NONE,
                                          NULL,
                                          "org.freedesktop.DBus",
                                          "/org/freesktop/DBus",
                                          "org.freedesktop.DBus",
                                          NULL,
                                          &error);
  if (proxy == NULL) {
    g_warning ("Failed to query dbus: %s", error->message);
    return MS_OSK_APP_UNKNOWN;
  }

  ret = g_dbus_proxy_call_sync (proxy,
                                "GetConnectionUnixProcessID",
                                g_variant_new ("(s)", PHOSH_OSK_DBUS_NAME),
                                G_DBUS_CALL_FLAGS_NONE,
                                1000,
                                NULL,
                                &error);
  if (proxy == NULL || ret == NULL) {
    g_debug ("Failed to query osk pid: %s", error->message);
    return MS_OSK_APP_UNKNOWN;
  }

  g_variant_get (ret, "(u)", &pid);
  proc_path = g_strdup_printf ("/proc/%d/exe", pid);

  exe = g_file_read_link (proc_path, &error);
  if (exe == NULL) {
    g_warning ("Failed to query osk exe: %s", error->message);
    return MS_OSK_APP_UNKNOWN;
  }

  if (g_str_has_suffix (exe, "/phosh-osk-stub"))
    return MS_OSK_APP_POS;
  if (g_str_has_suffix (exe, "/squeekboard"))
    return MS_OSK_APP_SQUEEKBOARD;

  return MS_OSK_APP_UNKNOWN;
}


static void
ms_osk_panel_finalize (GObject *object)
{
  MsOskPanel *self = MS_OSK_PANEL (object);

  g_clear_object (&self->completer_infos);
  g_clear_object (&self->shortcuts);
  g_clear_object (&self->a11y_settings);
  g_clear_object (&self->phosh_settings);
  g_clear_object (&self->pos_settings);
  g_clear_object (&self->pos_completer_settings);
  g_clear_object (&self->pos_terminal_settings);

  G_OBJECT_CLASS (ms_osk_panel_parent_class)->finalize (object);
}


static void
ms_osk_panel_class_init (MsOskPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_osk_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-osk-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, hw_keyboard_switch);
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, osk_enable_switch);
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, osk_layout_prefs);

  /* OSK handling */
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, long_press_combo);

  /* Completion group */
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, completer_combo);
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, completion_group);
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, app_completion_switch);
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, menu_completion_switch);
  gtk_widget_class_bind_template_callback (widget_class, on_completion_switch_activate_changed);

  /* Terminal layout group */
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, terminal_layout_group);
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, shortcuts_box);

  /* Squeekboard panel-size */
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, keyboard_height_prefs);
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, scale_in_horizontal_orientation);
  gtk_widget_class_bind_template_child (widget_class, MsOskPanel, scale_in_vertical_orientation);
}


static gboolean
long_press_delay_get_mapping (GValue *value, GVariant *variant, gpointer user_data)
{
  float delay = g_variant_get_double (variant);
  guint selection = 0;

  if (delay >= 1.5)
    selection = 2;
  else if (delay >= 1.0)
    selection = 1;

  g_value_set_uint (value, selection);
  return TRUE;
}


static GVariant *
long_press_delay_set_mapping (const GValue *value, const GVariantType *expected_type, gpointer user_data)
{
  guint selection = g_value_get_uint (value);
  double delay;

  switch (selection) {
  case 0:
    delay = 0.5;
    break;
  case 2:
    delay = 2.0;
    break;
  case 1:
  case GTK_INVALID_LIST_POSITION:
  default:
    delay = 1.0;
    break;
  }

  return g_variant_new_double (delay);
}


static gboolean
transform_to_subtitle (GBinding *binding, const GValue *from, GValue *to, gpointer user_data)
{
  MsCompleterInfo *info;
  const char *description, *comment;
  GString *subtitle = g_string_new ("");

  info = g_value_get_object (from);
  g_assert (MS_IS_COMPLETER_INFO (info));

  description = ms_completer_info_get_description (info);
  if (description) {
    g_string_append (subtitle, description);
    g_string_append (subtitle, ". ");
  }
  comment = ms_completer_info_get_comment (info);
  if (comment)
    g_string_append (subtitle, comment);

  g_value_take_string (to, g_string_free_and_steal (subtitle));

  return TRUE;
}

/**
 * ms_osk_panel_parse_pos_completers:
 * @self: The osk panel
 *
 * Parse the completer information provided by p-o-s via the file systems.
 */
static void
ms_osk_panel_parse_pos_completers (MsOskPanel *self)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (GDir) dir = NULL;
  const char *filename;

  dir = g_dir_open (MOBILE_SETTINGS_OSK_COMPLETERS_DIR, 0, &err);
  if (!dir) {
    g_warning ("Failed to load completer info from " MOBILE_SETTINGS_OSK_COMPLETERS_DIR ": %s",
               err->message);
  }

  while (dir && (filename = g_dir_read_name (dir))) {
    g_autofree char *path = NULL;
    g_autofree char *id = NULL;
    g_autofree char *name = NULL;
    g_autofree char *description = NULL;
    g_autofree char *comment = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GKeyFile) keyfile = g_key_file_new ();
    g_autoptr (MsCompleterInfo) info = NULL;

    if (!g_str_has_suffix (filename, POS_COMPLETER_SUFFIX))
      continue;

    path = g_build_filename (MOBILE_SETTINGS_OSK_COMPLETERS_DIR, filename, NULL);
    if (g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, &error) == FALSE) {
      g_warning ("Failed to load completer info '%s': %s", filename, error->message);
      continue;
    }

    id = g_key_file_get_string (keyfile, "Completer", "Id", NULL);
    if (id == NULL)
      continue;

    name = g_key_file_get_locale_string (keyfile, "Completer", "Name", NULL, NULL);
    if (name == NULL)
      continue;

    description = g_key_file_get_locale_string (keyfile, "Completer", "Description", NULL, NULL);
    if (description == NULL)
      continue;

    comment = g_key_file_get_locale_string (keyfile, "Completer", "Comment", NULL, NULL);
    if (comment == NULL)
      continue;

    g_debug ("Found completer %s, id %s, name: %s", filename, id, name);
    info = g_object_new (MS_TYPE_COMPLETER_INFO,
                         "id", id,
                         "name", name,
                         "description", description,
                         "comment", comment,
                         NULL);
    g_list_store_append (self->completer_infos, info);
  }
}


static void
on_completer_selected_item_changed (MsOskPanel *self)
{
  MsCompleterInfo *info;

  info = adw_combo_row_get_selected_item (self->completer_combo);
  g_settings_set_string (self->pos_completer_settings,
                         DEFAULT_COMPLETER_KEY,
                         ms_completer_info_get_id (info));
}


static void
ms_osk_panel_init_pos_completer (MsOskPanel *self)
{
  char *enabled_completer = NULL;
  gboolean found = FALSE;

  ms_osk_panel_parse_pos_completers (self);
  adw_combo_row_set_model (self->completer_combo, G_LIST_MODEL (self->completer_infos));

  enabled_completer = g_settings_get_string (self->pos_completer_settings, DEFAULT_COMPLETER_KEY);

  for (guint i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->completer_infos)); i++) {
    g_autoptr (MsCompleterInfo) info = NULL;

    info = g_list_model_get_item (G_LIST_MODEL (self->completer_infos), i);
    if (g_str_equal (ms_completer_info_get_id (info), enabled_completer)) {
      g_warning ("Current completer is %s", enabled_completer);
      adw_combo_row_set_selected (self->completer_combo, i);
      found = TRUE;
      break;
    }
  }

  if (!found) {
    g_autoptr (MsCompleterInfo) info = NULL;
    const char *name;
    const char *description = NULL;

    if (gm_str_is_null_or_empty (enabled_completer)) {
      /* Translators: The default completer */
      name = _("Default");
      description = _("The default completer selected by the OSK");
    } else {
      name = enabled_completer;
      description = _("No information available for this completer");
      g_warning_once ("Enabled completer %s unknown - please fix", enabled_completer);
    }

    info = g_object_new (MS_TYPE_COMPLETER_INFO,
                         "id", enabled_completer,
                         "name", name,
                         "description", description,
                         NULL);

    g_list_store_insert (self->completer_infos, 0, info);
    adw_combo_row_set_selected (self->completer_combo, 0);
  }

  g_object_bind_property_full (self->completer_combo, "selected-item",
                               self->completer_combo, "subtitle",
                               G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                               transform_to_subtitle,
                               NULL, NULL, NULL);
  /* All set up, now listen for changes */
  g_signal_connect_swapped (self->completer_combo, "notify::selected-item",
                            G_CALLBACK (on_completer_selected_item_changed),
                            self);
}


static gboolean
completer_combo_sensitive_mapping (GValue *value, GVariant *variant, gpointer user_data)
{
  const char *const *flags = g_variant_get_strv (variant, NULL);

  g_value_set_boolean (value, !gm_strv_is_null_or_empty (flags));

  return TRUE;
}


static void
ms_osk_panel_init_pos (MsOskPanel *self)
{
  gtk_widget_set_visible (self->hw_keyboard_switch, TRUE);
  self->pos_settings = g_settings_new (PHOSH_OSK_SETTINGS);
  g_settings_bind (self->pos_settings, HW_KEYBOARD_KEY,
                   self->hw_keyboard_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);

  gtk_widget_set_visible (self->completion_group, TRUE);
  self->mode = g_settings_get_flags (self->pos_settings, WORD_COMPLETION_KEY);
  g_signal_connect_swapped (self->pos_settings, "changed::" WORD_COMPLETION_KEY,
                            G_CALLBACK (on_word_completion_key_changed),
                            self);
  on_word_completion_key_changed (self);
  g_settings_bind_with_mapping (self->pos_settings, WORD_COMPLETION_KEY,
                                self->completer_combo, "sensitive",
                                G_SETTINGS_BIND_GET,
                                completer_combo_sensitive_mapping,
                                NULL, NULL, NULL);

  gtk_widget_set_visible (self->terminal_layout_group, TRUE);
  self->shortcuts = g_list_store_new (GTK_TYPE_STRING_OBJECT);
  gtk_flow_box_bind_model (GTK_FLOW_BOX (self->shortcuts_box),
                           G_LIST_MODEL (self->shortcuts),
                           create_shortcuts_row,
                           self,
                           NULL);

  self->pos_terminal_settings = g_settings_new (PHOSH_OSK_TERMINAL_SETTINGS);
  g_signal_connect_swapped (self->pos_terminal_settings, "changed::" SHORTCUTS_KEY,
                            G_CALLBACK (on_terminal_shortcuts_changed),
                            self);
  on_terminal_shortcuts_changed (self);

  gtk_widget_set_visible (self->osk_layout_prefs, TRUE);
  ms_osk_layout_prefs_load_osk_layouts (MS_OSK_LAYOUT_PREFS (self->osk_layout_prefs));

  self->pos_completer_settings = g_settings_new (PHOSH_OSK_COMPLETER_SETTINGS);
  ms_osk_panel_init_pos_completer (self);
}


static void
ms_osk_panel_init (MsOskPanel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->completer_infos = g_list_store_new (MS_TYPE_COMPLETER_INFO);
  self->a11y_settings = g_settings_new (A11Y_SETTINGS);
  g_settings_bind (self->a11y_settings, OSK_ENABLED_KEY,
                   self->osk_enable_switch, "active", G_SETTINGS_BIND_DEFAULT);

  self->phosh_settings = g_settings_new (PHOSH_SETTINGS);
  g_settings_bind_with_mapping (self->phosh_settings, OSK_UNFOLD_DELAY_KEY,
                                self->long_press_combo, "selected",
                                G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY,
                                long_press_delay_get_mapping,
                                long_press_delay_set_mapping,
                                self,
                                NULL);

  if (is_osk_app () == MS_OSK_APP_POS) {
    ms_osk_panel_init_pos (self);
  } else if (is_osk_app () == MS_OSK_APP_SQUEEKBOARD) {
    gboolean found_key_horizontal;
    gboolean found_key_vertical;

    found_key_horizontal = ms_schema_bind_property (SQUEEKBOARD_SETTINGS, SCALE_WHEN_HORIZONTAL_KEY,
                                                    G_OBJECT (self->scale_in_horizontal_orientation),
                                                    "value", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_visible (self->scale_in_horizontal_orientation, found_key_horizontal);

    found_key_vertical = ms_schema_bind_property (SQUEEKBOARD_SETTINGS, SCALE_WHEN_VERTICAL_KEY,
                                                  G_OBJECT (self->scale_in_vertical_orientation),
                                                  "value", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_visible (self->scale_in_vertical_orientation, found_key_vertical);

    gtk_widget_set_visible (self->keyboard_height_prefs,
                            found_key_horizontal | found_key_vertical);
  }
}


MsOskPanel *
ms_osk_panel_new (void)
{
  return MS_OSK_PANEL (g_object_new (MS_TYPE_OSK_PANEL, NULL));
}
