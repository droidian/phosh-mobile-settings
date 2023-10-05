/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "ms-features-panel"

#include "mobile-settings-config.h"
#include "ms-features-panel.h"
#include "ms-util.h"

#include <glib/gi18n.h>

struct _MsFeaturesPanel {
  AdwBin            parent;

  GSettings        *emergency_calls_settings;
  GSettings        *phosh_settings;
  GtkWidget        *emergency_calls_enabled_switch;
  GtkWidget        *manual_suspend_switch;
};

G_DEFINE_TYPE (MsFeaturesPanel, ms_features_panel, ADW_TYPE_BIN)


static void
ms_features_panel_finalize (GObject *object)
{
  MsFeaturesPanel *self = MS_FEATURES_PANEL (object);

  g_clear_object (&self->emergency_calls_settings);
  g_clear_object (&self->phosh_settings);

  G_OBJECT_CLASS (ms_features_panel_parent_class)->finalize (object);
}


static void
ms_features_panel_class_init (MsFeaturesPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_features_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-features-panel.ui");
  gtk_widget_class_bind_template_child (widget_class,
                                        MsFeaturesPanel,
                                        emergency_calls_enabled_switch);
  gtk_widget_class_bind_template_child (widget_class,
                                        MsFeaturesPanel,
                                        manual_suspend_switch);
}


static void
ms_features_panel_init (MsFeaturesPanel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->emergency_calls_settings = g_settings_new ("sm.puri.phosh.emergency-calls");
  g_settings_bind (self->emergency_calls_settings, "enabled",
                   self->emergency_calls_enabled_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);
  self->phosh_settings = g_settings_new ("sm.puri.phosh");
  g_settings_bind (self->phosh_settings, "enable-suspend",
                   self->manual_suspend_switch, "active",
                   G_SETTINGS_BIND_DEFAULT);
}


MsFeaturesPanel *
ms_features_panel_new (void)
{
  return MS_FEATURES_PANEL (g_object_new (MS_TYPE_FEATURES_PANEL, NULL));
}
