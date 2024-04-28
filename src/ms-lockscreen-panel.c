/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-lockscreen-panel"

#include "mobile-settings-config.h"
#include "ms-lockscreen-panel.h"
#include "ms-plugin-row.h"

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>

#include <phosh-plugin.h>

/* Verbatim from phosh */
#define LOCKSCREEN_SCHEMA_ID "sm.puri.phosh.lockscreen"
#define LOCKSCREEN_KEY_SCALE_TO_FIT "shuffle-keypad"


struct _MsLockscreenPanel {
  AdwBin     parent;

  GSettings *settings;
  GtkWidget *shuffle_switch;
};

G_DEFINE_TYPE (MsLockscreenPanel, ms_lockscreen_panel, ADW_TYPE_BIN)


static void
ms_lockscreen_panel_finalize (GObject *object)
{
  MsLockscreenPanel *self = MS_LOCKSCREEN_PANEL (object);

  g_clear_object (&self->settings);

  G_OBJECT_CLASS (ms_lockscreen_panel_parent_class)->finalize (object);
}


static void
ms_lockscreen_panel_class_init (MsLockscreenPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_lockscreen_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-lockscreen-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, MsLockscreenPanel, shuffle_switch);
}


static void
ms_lockscreen_panel_init (MsLockscreenPanel *self)
{
  /* Scan prefs plugins */
  g_io_modules_scan_all_in_directory (MOBILE_SETTINGS_PHOSH_PREFS_DIR);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new (LOCKSCREEN_SCHEMA_ID);
  g_settings_bind (self->settings,
                   LOCKSCREEN_KEY_SCALE_TO_FIT,
                   self->shuffle_switch,
                   "active",
                   G_SETTINGS_BIND_DEFAULT);
}


MsLockscreenPanel *
ms_lockscreen_panel_new (void)
{
  return MS_LOCKSCREEN_PANEL (g_object_new (MS_TYPE_LOCKSCREEN_PANEL, NULL));
}
