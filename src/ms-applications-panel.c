/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Suraj Kumar Mahto <suraj.mahto49@gmail.com>
 */

#define G_LOG_DOMAIN "ms-feedback-panel"

#include "mobile-settings-config.h"
#include "mobile-settings-application.h"
#include "mobile-settings-enums.h"
#include "ms-enum-types.h"
#include "ms-feedback-row.h"
#include "ms-applications-panel.h"
#include "ms-util.h"

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>

#define FAVORITES_KEY       "favorites"
#define FAVORITES_SCHEMA_ID "sm.puri.phosh"

struct _MsApplicationsPanel {
  AdwBin     parent;

  /* Favorites */
  GSettings *settings;
  GtkWidget *reset_btn;
};

G_DEFINE_TYPE (MsApplicationsPanel, ms_applications_panel, ADW_TYPE_BIN)


static void
on_reset_btn_clicked (GtkButton *reset_btn, MsApplicationsPanel *self)
{
  g_settings_reset (self->settings, FAVORITES_KEY);
}


static void
ms_applications_panel_finalize (GObject *object)
{
  G_OBJECT_CLASS (ms_applications_panel_parent_class)->finalize (object);
}


static void
ms_applications_panel_class_init (MsApplicationsPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_applications_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-applications-panel.ui");

  gtk_widget_class_bind_template_child (widget_class, MsApplicationsPanel, reset_btn);

  gtk_widget_class_bind_template_callback (widget_class, on_reset_btn_clicked);
}


static void
ms_applications_panel_init (MsApplicationsPanel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new (FAVORITES_SCHEMA_ID);
}


MsApplicationsPanel *
ms_applications_panel_new (void)
{
  return MS_APPLICATIONS_PANEL (g_object_new (MS_TYPE_APPLICATIONS_PANEL, NULL));
}
