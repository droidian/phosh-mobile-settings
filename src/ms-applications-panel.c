/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Suraj Kumar Mahto <suraj.mahto49@gmail.com>
 *          Rudra Pratap Singh <rudrasingh900@gmail.com>
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

#define FAVORITES_KEY            "favorites"
#define FAVORITES_SCHEMA_ID      "sm.puri.phosh"
#define FAVORITES_LIST_ICON_SIZE 48

struct _MsApplicationsPanel {
  AdwBin      parent;

  GSettings  *settings;

  /* Favorites */
  GtkFlowBox *fbox;
  GListStore *apps;
  GtkWidget  *reset_btn;
};

G_DEFINE_TYPE (MsApplicationsPanel, ms_applications_panel, ADW_TYPE_BIN)


static void
on_reset_btn_clicked (GtkButton *reset_btn, MsApplicationsPanel *self)
{
  g_settings_reset (self->settings, FAVORITES_KEY);
}


static void
sync_favorites (int start, int end, MsApplicationsPanel *self)
{
  int shift = start < end ? 1 : -1;
  g_auto (GStrv) fav_list = g_settings_get_strv (self->settings, FAVORITES_KEY);
  char *drag_app_id = g_steal_pointer (&fav_list[start]);

  for (int i = start; i != end; i += shift)
    fav_list[i] = fav_list[i + shift];

  fav_list[end] = drag_app_id;

  g_settings_set_strv (self->settings,
                       FAVORITES_KEY,
                       (const gchar *const *)fav_list);
}


static void
on_drop (GtkDropTarget       *target,
         GValue              *value,
         gdouble              x,
         gdouble              y,
         MsApplicationsPanel *self)
{
  int start = -1, end = -1;
  const char *drag_app_id, *drop_app_id;
  GAppInfo *drag_app_info, *drop_app_info;
  GtkWidget *drag_app = g_value_get_object (value);
  GtkWidget *drop_app = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (target));
  g_auto (GStrv) fav_list = g_settings_get_strv (self->settings, FAVORITES_KEY);

  if (drag_app == drop_app)
    return;

  drag_app_info = g_object_get_data (G_OBJECT (drag_app), "app-info");
  drop_app_info = g_object_get_data (G_OBJECT (drop_app), "app-info");
  g_assert (drag_app_info && drop_app_info);

  drag_app_id = g_app_info_get_id (drag_app_info);
  drop_app_id = g_app_info_get_id (drop_app_info);
  g_assert (drag_app_id && drop_app_id);

  for (int i = 0; fav_list[i]; ++i) {
    if (!g_strcmp0 (drag_app_id, fav_list[i]))
      start = i;
    if (!g_strcmp0 (drop_app_id, fav_list[i]))
      end = i;
  }
  g_assert (start >= 0 && end >= 0);

  sync_favorites (start, end, self);
}


static GdkDragAction
on_enter (GtkDropTarget *target, gdouble x, gdouble y, MsApplicationsPanel *self)
{
  GtkWidget *app = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (target));

  gtk_button_set_has_frame (GTK_BUTTON (app), TRUE);

  return GDK_ACTION_COPY;
}


static void
on_leave (GtkDropTarget *target, MsApplicationsPanel *self)
{
  GtkWidget *app = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (target));

  gtk_button_set_has_frame (GTK_BUTTON (app), FALSE);
}


static void
add_drop_target (GtkWidget *app, MsApplicationsPanel *self)
{
  GtkDropTarget *target = gtk_drop_target_new (GTK_TYPE_WIDGET, GDK_ACTION_COPY);

  g_object_connect (target,
                    "signal::enter", G_CALLBACK (on_enter), self,
                    "signal::leave", G_CALLBACK (on_leave), self,
                    "signal::drop", G_CALLBACK (on_drop), self,
                    NULL);

  gtk_widget_add_controller (app, GTK_EVENT_CONTROLLER (target)); 
}


static void
on_drag_begin (GtkDragSource *source, GdkDrag *drag, gpointer user_data)
{
  GtkWidget *app = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (source));
  g_autoptr (GdkPaintable) paintable = gtk_widget_paintable_new (app);

  gtk_drag_source_set_icon (source,
                            paintable,
                            gtk_widget_get_width (app) / 2,
                            gtk_widget_get_height (app) / 2);
}


static void
add_drag_source (GtkWidget *app)
{
  GtkDragSource *source = gtk_drag_source_new ();

  gtk_drag_source_set_content (source, gdk_content_provider_new_typed (GTK_TYPE_WIDGET, app));

  g_signal_connect (source, "drag-begin", G_CALLBACK (on_drag_begin), NULL);

  gtk_widget_add_controller (app, GTK_EVENT_CONTROLLER (source));
}


static GtkWidget *
create_fav_app (gpointer item, gpointer user_data)
{
  GtkWidget *app = gtk_button_new ();
  GAppInfo *app_info = G_APP_INFO (item);
  MsApplicationsPanel *self = MS_APPLICATIONS_PANEL (user_data);
  GIcon *icon = g_app_info_get_icon (app_info);
  GtkWidget *img = gtk_image_new_from_gicon (icon);

  gtk_image_set_pixel_size (GTK_IMAGE (img), FAVORITES_LIST_ICON_SIZE);

  gtk_button_set_child (GTK_BUTTON (app), img);
  gtk_button_set_has_frame (GTK_BUTTON (app), FALSE);

  add_drag_source (app);
  add_drop_target (app, self);

  g_object_set_data_full (G_OBJECT (app), "app-info", app_info, g_object_unref);
  g_object_ref (app_info);

  return app;
}


static void
on_favorites_changed (MsApplicationsPanel *self)
{
  g_autoptr (GDesktopAppInfo) app_info = NULL;
  g_auto (GStrv) fav_list = g_settings_get_strv (self->settings, FAVORITES_KEY);

  g_list_store_remove_all (self->apps);

  for (int i = 0; fav_list[i]; ++i) {
    app_info = g_desktop_app_info_new (fav_list[i]);

    if (app_info)
      g_list_store_append (self->apps, app_info);
  }
}


static void
ms_applications_panel_finalize (GObject *object)
{
  MsApplicationsPanel *self = MS_APPLICATIONS_PANEL (object);

  g_clear_pointer (&self->apps, g_object_unref);
  g_clear_pointer (&self->settings, g_object_unref);

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

  gtk_widget_class_bind_template_child (widget_class, MsApplicationsPanel, fbox);
  gtk_widget_class_bind_template_child (widget_class, MsApplicationsPanel, reset_btn);

  gtk_widget_class_bind_template_callback (widget_class, on_reset_btn_clicked);
}


static void
ms_applications_panel_init (MsApplicationsPanel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->apps = g_list_store_new (G_TYPE_APP_INFO);
  self->settings = g_settings_new (FAVORITES_SCHEMA_ID);

  g_signal_connect_swapped (self->settings, "changed::" FAVORITES_KEY,
                            G_CALLBACK (on_favorites_changed),
                            self);

  gtk_flow_box_bind_model (self->fbox,
                           G_LIST_MODEL (self->apps),
                           create_fav_app,
                           self,
                           NULL);

  on_favorites_changed (self);
}


MsApplicationsPanel *
ms_applications_panel_new (void)
{
  return MS_APPLICATIONS_PANEL (g_object_new (MS_TYPE_APPLICATIONS_PANEL, NULL));
}
