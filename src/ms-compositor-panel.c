/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-compositor-panel"

#include "mobile-settings-config.h"

#include "mobile-settings-application.h"
#include "ms-compositor-panel.h"
#include "ms-scale-to-fit-row.h"

/* Verbatim from compositor */
#define COMPOSITOR_SCHEMA_ID "sm.puri.phoc"
#define COMPOSITOR_KEY_SCALE_TO_FIT "scale-to-fit"


struct _MsCompositorPanel {
  AdwBin     parent;

  GSettings *settings;
  GtkWidget *scale_to_fit_switch;

  GtkListBox *running_apps_listbox;
  GListStore *running_apps_rows;
  MsToplevelTracker *tracker;
};

G_DEFINE_TYPE (MsCompositorPanel, ms_compositor_panel, ADW_TYPE_BIN)


static void
add_application_row (MsCompositorPanel *self, const char *app_id)
{
  MsScaleToFitRow *row;

  row = ms_scale_to_fit_row_new (app_id);
  g_list_store_append (self->running_apps_rows, row);
}


static void
remove_application_row (MsCompositorPanel *self, const char *app_id)
{
  for (guint pos = 0; pos < g_list_model_get_n_items (G_LIST_MODEL (self->running_apps_rows)); pos++) {
    MsScaleToFitRow *row = g_list_model_get_item (G_LIST_MODEL (self->running_apps_rows), pos);

    if (g_strcmp0 (ms_scale_to_fit_row_get_app_id (row), app_id) == 0) {
      g_list_store_remove (self->running_apps_rows, pos);
      break;
    }
  }
}


static void
on_app_id_added (MsCompositorPanel *self, const char *app_id)
{
  g_debug ("Adding app-id: %s", app_id);
  add_application_row (self, app_id);
}


static void
on_app_id_removed (MsCompositorPanel *self, const char *app_id)
{
  g_debug ("Removing app-id: %s", app_id);
  remove_application_row (self, app_id);
}


static GtkWidget *
create_scale_to_fit_row (gpointer object, gpointer user_data)
{
  return GTK_WIDGET (object);
}

static void
on_toplevel_tracker_changed (MsCompositorPanel *self, GParamSpec *spec, MobileSettingsApplication *app)
{
  MsToplevelTracker *tracker = mobile_settings_application_get_toplevel_tracker (app);
  g_auto (GStrv) app_ids = NULL;

  if (tracker == NULL)
    return;

  self->tracker = g_object_ref (tracker);
  g_object_connect (self->tracker,
                    "swapped_object_signal::app-id-added",
                    G_CALLBACK (on_app_id_added),
                    self,
                    "swapped_object_signal::app-id-removed",
                    G_CALLBACK (on_app_id_removed),
                    self,
                    NULL);
  app_ids = ms_toplevel_tracker_get_app_ids (self->tracker);

  for (int i = 0; i < g_strv_length(app_ids); i++) {
    g_debug ("app-id: %s", app_ids[i]);
    add_application_row (self, app_ids[i]);
  }
}


static void
ms_compositor_panel_finalize (GObject *object)
{
  MsCompositorPanel *self = MS_COMPOSITOR_PANEL (object);

  g_clear_object (&self->running_apps_rows);
  g_clear_object (&self->tracker);
  g_clear_object (&self->settings);

  G_OBJECT_CLASS (ms_compositor_panel_parent_class)->finalize (object);
}


static void
ms_compositor_panel_class_init (MsCompositorPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_compositor_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-compositor-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, MsCompositorPanel, scale_to_fit_switch);
  gtk_widget_class_bind_template_child (widget_class, MsCompositorPanel, running_apps_listbox);
}


static void
ms_compositor_panel_init (MsCompositorPanel *self)
{
  MobileSettingsApplication *app;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->running_apps_rows = g_list_store_new (MS_TYPE_SCALE_TO_FIT_ROW);
  gtk_list_box_bind_model (self->running_apps_listbox,
                           G_LIST_MODEL (self->running_apps_rows),
                           create_scale_to_fit_row,
                           NULL, NULL);

  self->settings = g_settings_new (COMPOSITOR_SCHEMA_ID);
  g_settings_bind (self->settings,
                   COMPOSITOR_KEY_SCALE_TO_FIT,
                   self->scale_to_fit_switch,
                   "active",
                   G_SETTINGS_BIND_DEFAULT);

  app = MOBILE_SETTINGS_APPLICATION (g_application_get_default ());
  g_signal_connect_swapped (app, "notify::toplevel-tracker",
                            G_CALLBACK (on_toplevel_tracker_changed), self);
  on_toplevel_tracker_changed(self, NULL,
                              MOBILE_SETTINGS_APPLICATION (g_application_get_default ()));
}


MsCompositorPanel *
ms_compositor_panel_new (void)
{
  return MS_COMPOSITOR_PANEL (g_object_new (MS_TYPE_COMPOSITOR_PANEL, NULL));
}
