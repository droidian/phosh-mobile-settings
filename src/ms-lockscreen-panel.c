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

#define SCREENSAVER_SCHEMA_ID "org.gnome.desktop.screensaver"
#define SCREENSAVER_KEY_LOCK_DELAY "lock-delay"

struct _MsLockscreenPanel {
  AdwBin     parent;

  GSettings *settings;
  GSettings *screensaver_settings;
  GtkWidget *shuffle_switch;
  GtkWidget *lock_delay_adjustment;
};

G_DEFINE_TYPE (MsLockscreenPanel, ms_lockscreen_panel, ADW_TYPE_BIN)


static gboolean
uint32_to_double_get_mapping (GValue *out_value, GVariant *in_variant, gpointer user_data)
{
  guint32 uint32_value = g_variant_get_uint32 (in_variant);

  g_value_set_double (out_value, (double) uint32_value);
  return TRUE;
}


static GVariant *
double_to_uint32_set_mapping (const GValue *in_value, const GVariantType *out_type, gpointer data)
{
  double dbl_value = g_value_get_double (in_value);
  guint32 int32_value = (guint32) CLAMP (dbl_value, 0.0, (double) G_MAXUINT32);

  return g_variant_new_uint32 (int32_value);
}


static void
ms_lockscreen_panel_finalize (GObject *object)
{
  MsLockscreenPanel *self = MS_LOCKSCREEN_PANEL (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->screensaver_settings);

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
  gtk_widget_class_bind_template_child (widget_class, MsLockscreenPanel, lock_delay_adjustment);
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

  self->screensaver_settings = g_settings_new (SCREENSAVER_SCHEMA_ID);
  g_settings_bind_with_mapping (self->screensaver_settings,
                                SCREENSAVER_KEY_LOCK_DELAY,
                                self->lock_delay_adjustment,
                                "value",
                                G_SETTINGS_BIND_DEFAULT,
                                uint32_to_double_get_mapping,
                                double_to_uint32_set_mapping,
                                NULL, NULL);
}


MsLockscreenPanel *
ms_lockscreen_panel_new (void)
{
  return MS_LOCKSCREEN_PANEL (g_object_new (MS_TYPE_LOCKSCREEN_PANEL, NULL));
}
