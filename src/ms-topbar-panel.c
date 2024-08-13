/*
 * Copyright (C) 2023-2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Gotam Gorabh <gautamy672@gmail.com>
 */

#include "ms-topbar-panel.h"
#include "ms-plugin-list-box.h"

#include <phosh-settings-enums.h>

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>

#define PHOSH_SCHEMA_ID   "sm.puri.phosh"
#define SHELL_LAYOUT_KEY  "shell-layout"

#define INTERFACE_SCHEMA_ID     "org.gnome.desktop.interface"
#define BATTERY_PERCENTAGE_KEY  "show-battery-percentage"


struct _MsTopbarPanel {
  AdwBin        parent;

  GSettings    *settings;
  GSettings    *interface_settings;

  AdwSwitchRow *battery_percentage_switch;
  AdwSwitchRow *shell_layout_switch;
};

G_DEFINE_TYPE (MsTopbarPanel, ms_topbar_panel, ADW_TYPE_BIN)


static void
shell_layout_switch_row_cb (MsTopbarPanel *self)
{
  gboolean shell_layout_switch_state;
  gint shell_layout_enums;

  shell_layout_switch_state = adw_switch_row_get_active (self->shell_layout_switch);

  shell_layout_enums = shell_layout_switch_state ?
                       PHOSH_SHELL_LAYOUT_DEVICE :
                       PHOSH_SHELL_LAYOUT_NONE;

  g_settings_set_enum (self->settings, SHELL_LAYOUT_KEY, shell_layout_enums);
}


static void
on_shell_layout_setting_changed (MsTopbarPanel *self)
{
  PhoshShellLayout shell_layout;
  gboolean active;

  shell_layout = g_settings_get_enum (self->settings, SHELL_LAYOUT_KEY);

  active = !!(shell_layout & PHOSH_SHELL_LAYOUT_DEVICE);
  adw_switch_row_set_active (self->shell_layout_switch, active);
}


static void
ms_topbar_panel_finalize (GObject *object)
{
  MsTopbarPanel *self = MS_TOPBAR_PANEL (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->interface_settings);

  G_OBJECT_CLASS (ms_topbar_panel_parent_class)->finalize (object);
}


static void
ms_topbar_panel_class_init (MsTopbarPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_topbar_panel_finalize;

  g_type_ensure (MS_TYPE_PLUGIN_LIST_BOX);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-topbar-panel.ui");

  gtk_widget_class_bind_template_child (widget_class, MsTopbarPanel, battery_percentage_switch);
  gtk_widget_class_bind_template_child (widget_class, MsTopbarPanel, shell_layout_switch);

  gtk_widget_class_bind_template_callback (widget_class, shell_layout_switch_row_cb);
}


static void
ms_topbar_panel_init (MsTopbarPanel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new (PHOSH_SCHEMA_ID);

  g_signal_connect_swapped (self->settings, "changed::" SHELL_LAYOUT_KEY,
                            G_CALLBACK (on_shell_layout_setting_changed),
                            self);

  on_shell_layout_setting_changed (self);

  self->interface_settings = g_settings_new (INTERFACE_SCHEMA_ID);

  g_settings_bind (self->interface_settings, BATTERY_PERCENTAGE_KEY,
                   self->battery_percentage_switch, "active", G_SETTINGS_BIND_DEFAULT);
}
