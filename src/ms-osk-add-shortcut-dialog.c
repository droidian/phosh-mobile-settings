/*
 * Copyright (C) 2025 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Gotam Gorabh <gautamy672@gmail.com>
 */

#define G_LOG_DOMAIN "ms-osk-add-shortcut-dialog"

#include "mobile-settings-config.h"

#include "ms-osk-add-shortcut-dialog.h"

#define PHOSH_OSK_TERMINAL_SETTINGS  "sm.puri.phosh.osk.Terminal"
#define SHORTCUTS_KEY                "shortcuts"

/**
 * MsOskAddShortcutDialog:
 *
 * Dialog to add an OSK shortcut
 */

static const char * const shortcut_modifiers_names[] = {
  "ctrl",
  "alt",
  "shift",
  "super",
  NULL
};

typedef enum {
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12,
  KEY_TAB,
  KEY_DELETE,
  KEY_LAST
}ShortcutKeys;

static const char * const shortcut_keys_names[] = {
  "Up",
  "Down",
  "Left",
  "Right",
  "F1",
  "F2",
  "F3",
  "F4",
  "F5",
  "F6",
  "F7",
  "F8",
  "F9",
  "F10",
  "F11",
  "F12",
  "Tab",
  "Delete",
  NULL
};

struct _MsOskAddShortcutDialog {
  AdwDialog                parent;

  GSettings               *pos_terminal_settings;

  AdwToastOverlay         *toast_overlay;
  GtkWidget               *add_button;

  GtkCheckButton          *shortcut_modifiers[G_N_ELEMENTS(shortcut_modifiers_names)];

  AdwEntryRow             *shortcut_key_entry;
  GtkFlowBox              *key_flowbox;
  GtkFlowBox              *preview_flowbox;
};
G_DEFINE_TYPE (MsOskAddShortcutDialog, ms_osk_add_shortcut_dialog, ADW_TYPE_DIALOG)


static void
is_valid_shortcut (MsOskAddShortcutDialog *self)
{
  GtkFlowBoxChild *flow_child = gtk_flow_box_get_child_at_index (self->preview_flowbox, 0);
  GtkWidget *shortcut_label = gtk_widget_get_first_child (GTK_WIDGET (flow_child));
  GtkWidget *label_child = gtk_widget_get_first_child (shortcut_label);

  /* If a shortcut is valid then GtkShortcutLabel has one or more GtkLabel */
  if (!label_child) {
    GtkWidget *invalid_label = gtk_label_new ("Invalid Shortcut");
    gtk_widget_add_css_class (invalid_label, "error");

    for (int i = 0; shortcut_modifiers_names[i]; i++)
      gtk_check_button_set_active (self->shortcut_modifiers[i], FALSE);

    gtk_flow_box_remove_all (self->preview_flowbox);
    gtk_flow_box_append (self->preview_flowbox, invalid_label);
    gtk_editable_set_text (GTK_EDITABLE (self->shortcut_key_entry), "");
    gtk_widget_set_sensitive (GTK_WIDGET (self->add_button), FALSE);
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (self->add_button), TRUE);
  }
}


static char *
extract_modifiers (const char *shortcut)
{
  GString *current = g_string_new ("");
  const char * const modifiers[] = { "<ctrl>", "<alt>", "<shift>", "<super>", NULL };

  for (int i = 0; modifiers[i]; i++) {
    if (strstr (shortcut, modifiers[i]))
      g_string_append (current, modifiers[i]);
  }

  return g_string_free_and_steal (current);
}


static GStrv
shortcut_append (const char *const *shortcuts, const char *shortcut)
{
  g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();

  if (g_strv_contains (shortcuts, shortcut))
    return g_strdupv ((GStrv)shortcuts);

  for (int i = 0; shortcuts[i]; i++)
    g_strv_builder_add (builder, shortcuts[i]);

  g_strv_builder_add (builder, shortcut);

  return g_strv_builder_end (builder);
}


static const char *
get_current_preview_shortcut (MsOskAddShortcutDialog *self)
{
  GtkFlowBoxChild *child = gtk_flow_box_get_child_at_index (self->preview_flowbox, 0);
  GtkWidget *shortcut_label;

  if (!child)
    return "";

  shortcut_label = gtk_widget_get_first_child (GTK_WIDGET (child));

  /* when users continue to choose shortcut without clearing preview */
  if (GTK_IS_LABEL (shortcut_label)) {
    gtk_flow_box_remove_all (self->preview_flowbox);
    return "";
  }
  return gtk_shortcut_label_get_accelerator (GTK_SHORTCUT_LABEL (shortcut_label));
}


static void
on_add_clicked (MsOskAddShortcutDialog *self)
{
  const char *curr_shortcut = get_current_preview_shortcut (self);
  g_auto (GStrv) pos_terminal_shortcuts = NULL;
  g_auto (GStrv) shortcut = NULL;

  pos_terminal_shortcuts = g_settings_get_strv (self->pos_terminal_settings, SHORTCUTS_KEY);
  shortcut = shortcut_append ((const char * const *) pos_terminal_shortcuts, curr_shortcut);

  g_settings_set_strv (self->pos_terminal_settings, SHORTCUTS_KEY, (const char * const *)shortcut);
  adw_dialog_close (ADW_DIALOG (self));
}


static void
update_shortcut (MsOskAddShortcutDialog *self, const char *update)
{
  const char *current = get_current_preview_shortcut (self);
  g_autofree char *modifiers = NULL, *new = NULL;
  GtkWidget *shortcut_label;

  modifiers = extract_modifiers (current);
  new = g_strconcat (modifiers, update, NULL);

  shortcut_label = gtk_shortcut_label_new (new);
  gtk_flow_box_remove_all (self->preview_flowbox);
  gtk_flow_box_append (self->preview_flowbox, shortcut_label);
  is_valid_shortcut (self);
}


static void
on_modifiers_toggled (MsOskAddShortcutDialog *self)
{
  g_autofree char *current_modifers = extract_modifiers (get_current_preview_shortcut (self));

  for (int i = 0; shortcut_modifiers_names[i]; i++) {
    GtkWidget *check_button_child = gtk_check_button_get_child (self->shortcut_modifiers[i]);
    const char *mod_label = gtk_shortcut_label_get_accelerator (GTK_SHORTCUT_LABEL (check_button_child));
    gboolean active = gtk_check_button_get_active (self->shortcut_modifiers[i]);
    const char *contain = g_strrstr (current_modifers, mod_label);
    GtkWidget *shortcut_label;

    if (active && !contain) {
      update_shortcut (self, mod_label);
    } else if (!active && contain) {
      g_autofree char *accltr = NULL;
      g_auto (GStrv) parts = g_strsplit (current_modifers, mod_label, -1);

      accltr = g_strjoinv ("", parts);
      shortcut_label = gtk_shortcut_label_new (accltr);
      gtk_flow_box_remove_all (self->preview_flowbox);
      gtk_flow_box_append (self->preview_flowbox, shortcut_label);
      is_valid_shortcut (self);
    }
  }
}


static void
on_shortcut_key_apply (MsOskAddShortcutDialog *self)
{
  const char *key = gtk_editable_get_text (GTK_EDITABLE (self->shortcut_key_entry));

  update_shortcut (self, key);
}


static void
on_key_activated (MsOskAddShortcutDialog *self, GtkFlowBoxChild *child, GtkFlowBox *box)
{
  GtkWidget *shortcut_label_child = gtk_widget_get_first_child (GTK_WIDGET (child));
  const char *box_key = gtk_shortcut_label_get_accelerator (GTK_SHORTCUT_LABEL (shortcut_label_child));

  update_shortcut (self, box_key);
}


static void
on_preview_clear_clicked (MsOskAddShortcutDialog *self)
{
  for (int i = 0; shortcut_modifiers_names[i]; i++)
    gtk_check_button_set_active (self->shortcut_modifiers[i], FALSE);

  gtk_flow_box_remove_all (self->preview_flowbox);
  gtk_editable_set_text (GTK_EDITABLE (self->shortcut_key_entry), "");
  gtk_widget_set_sensitive (GTK_WIDGET (self->add_button), FALSE);
}


static void
ms_osk_add_shortcut_dialog_dispose (GObject *object)
{
  MsOskAddShortcutDialog *self = MS_OSK_ADD_SHORTCUT_DIALOG (object);

  g_clear_object (&self->pos_terminal_settings);

  G_OBJECT_CLASS (ms_osk_add_shortcut_dialog_parent_class)->dispose (object);
}


static void
ms_osk_add_shortcut_dialog_class_init (MsOskAddShortcutDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = ms_osk_add_shortcut_dialog_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/"
                                               "ui/ms-osk-add-shortcut-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, MsOskAddShortcutDialog, toast_overlay);
  gtk_widget_class_bind_template_child (widget_class, MsOskAddShortcutDialog, add_button);

  for (int i = 0; shortcut_modifiers_names[i]; i++) {
    g_autofree char *widget_name = g_strdup_printf ("%s_modifier", shortcut_modifiers_names[i]);
    gtk_widget_class_bind_template_child_full (widget_class,
                                               widget_name,
                                               FALSE,
                                               G_STRUCT_OFFSET (MsOskAddShortcutDialog,
                                                                shortcut_modifiers[i]));
  }

  gtk_widget_class_bind_template_child (widget_class, MsOskAddShortcutDialog, shortcut_key_entry);
  gtk_widget_class_bind_template_child (widget_class, MsOskAddShortcutDialog, key_flowbox);
  gtk_widget_class_bind_template_child (widget_class, MsOskAddShortcutDialog, preview_flowbox);

  gtk_widget_class_bind_template_callback (widget_class, on_add_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_modifiers_toggled);
  gtk_widget_class_bind_template_callback (widget_class, on_shortcut_key_apply);
  gtk_widget_class_bind_template_callback (widget_class, on_key_activated);
  gtk_widget_class_bind_template_callback (widget_class, on_preview_clear_clicked);
}


static void
ms_osk_add_shortcut_dialog_init (MsOskAddShortcutDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->pos_terminal_settings = g_settings_new (PHOSH_OSK_TERMINAL_SETTINGS);

  for (int i = 0; i < KEY_LAST; i++) {
    GtkWidget *key_shortcut_label = gtk_shortcut_label_new (shortcut_keys_names[i]);
    gtk_widget_set_halign (key_shortcut_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (key_shortcut_label, GTK_ALIGN_CENTER);
    gtk_flow_box_append (self->key_flowbox, key_shortcut_label);
  }
}


GtkWidget *
ms_osk_add_shortcut_dialog_new (void)
{
  return g_object_new (MS_TYPE_OSK_ADD_SHORTCUT_DIALOG, NULL);
}
