/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Suraj Kumar Mahto <suraj.mahto49@gmail.com>
 *          Rudra Pratap Singh <rudrasingh900@gmail.com>
 */

#define G_LOG_DOMAIN "ms-overview-panel"

#include "mobile-settings-config.h"
#include "mobile-settings-application.h"
#include "mobile-settings-enums.h"
#include "ms-enum-types.h"
#include "ms-feedback-row.h"
#include "ms-overview-panel.h"
#include "ms-util.h"

#include <phosh-settings-enums.h>

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>

#define FAVORITES_KEY            "favorites"
#define APP_FILTER_MODE_KEY      "app-filter-mode"
#define PHOSH_SCHEMA_ID          "sm.puri.phosh"
#define FAVORITES_LIST_ICON_SIZE 48

#define BACKGROUND_SCHEMA_ID "org.gnome.desktop.background"
#define BACKGROUND_KEY_PICTURE_OPTIONS "picture-options"

struct _MsOverviewPanel {
  MsPanel                  parent;

  GSettings               *settings;
  GSettings               *background_settings;

  /* Favorites */
  AdwPreferencesGroup     *arrange_favs;
  GtkFlowBox              *fbox;
  GListStore              *apps;
  GtkWidget               *reset_btn;
  /* App filtering */
  AdwSwitchRow            *afm_switch_row;
  /* Wallpaper */
  AdwSwitchRow            *wallpaper_switch;

  AdwToastOverlay         *toast_overlay;
  AdwToast                *toast;
};

G_DEFINE_TYPE (MsOverviewPanel, ms_overview_panel, MS_TYPE_PANEL)


static void
on_reset_favorites_response (MsOverviewPanel *self, char *response)
{
  g_assert (MS_IS_OVERVIEW_PANEL (self));

  if (g_strcmp0 (response, "reset") == 0)
    g_settings_reset (self->settings, FAVORITES_KEY);
}


static void
on_reset_btn_clicked (GtkButton *reset_btn, MsOverviewPanel *self)
{
  AdwDialog *dialog = adw_alert_dialog_new (_("Reset favorites?"), NULL);

  adw_alert_dialog_format_body (ADW_ALERT_DIALOG (dialog),
                                _("Reset list of favorite applications to the default?"));

  adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog),
                                  "cancel",  _("_Cancel"),
                                  "reset", _("_Reset"),
                                  NULL);

  adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog),
                                            "reset",
                                            ADW_RESPONSE_DESTRUCTIVE);

  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "cancel");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");

  g_signal_connect_swapped (dialog, "response", G_CALLBACK (on_reset_favorites_response), self);
  adw_dialog_present (dialog, GTK_WIDGET (self));
}


static void
afm_switch_row_cb (MsOverviewPanel *self)
{
  gboolean switch_state;
  gint flags;

  switch_state = adw_switch_row_get_active (self->afm_switch_row);

  flags = switch_state ? PHOSH_APP_FILTER_MODE_FLAGS_ADAPTIVE : PHOSH_APP_FILTER_MODE_FLAGS_NONE;
  g_settings_set_flags (self->settings, APP_FILTER_MODE_KEY, flags);
}


static void
sync_favorites (int start, int end, MsOverviewPanel *self)
{
  int shift = start < end ? 1 : -1;
  g_auto (GStrv) fav_list = g_settings_get_strv (self->settings, FAVORITES_KEY);
  char *drag_app_id = g_steal_pointer (&fav_list[start]);

  for (int i = start; i != end; i += shift)
    fav_list[i] = fav_list[i + shift];

  fav_list[end] = drag_app_id;

  g_settings_set_strv (self->settings,
                       FAVORITES_KEY,
                       (const char *const *)fav_list);
}


static void
on_drop_flowbox (GtkDropTarget   *target,
                 GValue          *value,
                 gdouble          x,
                 gdouble          y,
                 MsOverviewPanel *self)
{
  GtkWidget *drag_app = g_value_get_object (value);
  GtkFlowBoxChild *child = gtk_flow_box_get_child_at_index (self->fbox, 0);
  g_auto (GStrv) fav_list = g_settings_get_strv (self->settings, FAVORITES_KEY);
  uint max_per_row = gtk_flow_box_get_max_children_per_line (self->fbox);
  uint n_apps = g_list_model_get_n_items (G_LIST_MODEL (self->apps));
  uint n_rows, x_min, y_min;
  GAppInfo *drag_app_info;
  const char *drag_app_id;
  int start = -1;

  /* All rows fully filled, no empty space to drop */
  if (n_apps % max_per_row == 0)
    return;

  n_rows = n_apps / max_per_row + 1;
  x_min = gtk_widget_get_width (GTK_WIDGET (child)) * (n_apps % max_per_row);
  y_min = gtk_widget_get_height (GTK_WIDGET (child)) * (n_rows - 1);

  /* Drop coordinates (x, y) must be greater than minimum acceptable */
  if (x < x_min || y < y_min)
    return;

  drag_app_info = g_object_get_data (G_OBJECT (drag_app), "app-info");
  g_assert (drag_app_info);

  drag_app_id = g_app_info_get_id (drag_app_info);
  g_assert (drag_app_id);

  for (int i = 0; fav_list[i]; i++) {
    if (!g_strcmp0 (drag_app_id, fav_list[i])) {
      start = i;
      break;
    }
  }
  g_assert (start >= 0);

  sync_favorites (start, g_strv_length (fav_list) - 1, self);
}


static void
on_drop (GtkDropTarget   *target,
         GValue          *value,
         gdouble          x,
         gdouble          y,
         MsOverviewPanel *self)
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
on_enter (GtkDropTarget *target, gdouble x, gdouble y, MsOverviewPanel *self)
{
  GtkWidget *app = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (target));

  gtk_button_set_has_frame (GTK_BUTTON (app), TRUE);

  return GDK_ACTION_COPY;
}


static void
on_leave (GtkDropTarget *target, MsOverviewPanel *self)
{
  GtkWidget *app = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (target));

  gtk_button_set_has_frame (GTK_BUTTON (app), FALSE);
}


static void
add_drop_target (GtkWidget *app, MsOverviewPanel *self)
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
  MsOverviewPanel *self = MS_OVERVIEW_PANEL (user_data);
  GIcon *icon = g_app_info_get_icon (app_info);
  GtkWidget *img = gtk_image_new_from_gicon (icon);

  gtk_image_set_pixel_size (GTK_IMAGE (img), FAVORITES_LIST_ICON_SIZE);

  gtk_button_set_child (GTK_BUTTON (app), img);
  gtk_button_set_has_frame (GTK_BUTTON (app), FALSE);

  add_drag_source (app);
  add_drop_target (app, self);

  g_object_set_data_full (G_OBJECT (app), "app-info", app_info, g_object_unref);

  return app;
}


static void
on_favorites_changed (MsOverviewPanel *self)
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
on_afm_setting_changed (MsOverviewPanel *self)
{
  PhoshAppFilterModeFlags filter_mode;
  gboolean active;

  filter_mode = g_settings_get_flags (self->settings, APP_FILTER_MODE_KEY);

  active = !!(filter_mode & PHOSH_APP_FILTER_MODE_FLAGS_ADAPTIVE);
  adw_switch_row_set_active (self->afm_switch_row, active);
}


static void
on_select_wallpaper_ready (GObject *source, GAsyncResult *result, gpointer user_data)
{
  MsOverviewPanel *self = MS_OVERVIEW_PANEL (source);
  g_autoptr (GError) err = NULL;
  gboolean success;

  success = ms_select_wallpaper_finish (ADW_BIN (source), result, &err);
  if (!success  && !g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
    AdwToast *toast;

    toast = adw_toast_new (_("Failed to set wallpaper for overview"));
    adw_toast_overlay_add_toast (self->toast_overlay, toast);
    return;
  }
}


static void
on_select_wallpaper_clicked (MsOverviewPanel *self)
{
  ms_select_wallpaper_async (ADW_BIN (self), on_select_wallpaper_ready, FALSE, NULL);
}


static void
ms_overview_panel_finalize (GObject *object)
{
  MsOverviewPanel *self = MS_OVERVIEW_PANEL (object);

  g_clear_object (&self->apps);
  g_clear_object (&self->settings);
  g_clear_object (&self->background_settings);

  G_OBJECT_CLASS (ms_overview_panel_parent_class)->finalize (object);
}


static void
ms_overview_panel_class_init (MsOverviewPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_overview_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-overview-panel.ui");

  gtk_widget_class_bind_template_child (widget_class, MsOverviewPanel, afm_switch_row);
  gtk_widget_class_bind_template_child (widget_class, MsOverviewPanel, arrange_favs);
  gtk_widget_class_bind_template_child (widget_class, MsOverviewPanel, fbox);
  gtk_widget_class_bind_template_child (widget_class, MsOverviewPanel, reset_btn);
  gtk_widget_class_bind_template_child (widget_class, MsOverviewPanel, toast_overlay);
  gtk_widget_class_bind_template_child (widget_class, MsOverviewPanel, wallpaper_switch);

  gtk_widget_class_bind_template_callback (widget_class, afm_switch_row_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_reset_btn_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_select_wallpaper_clicked);
}


static void
ms_overview_panel_init (MsOverviewPanel *self)
{
  GtkDropTarget *target = gtk_drop_target_new (GTK_TYPE_WIDGET, GDK_ACTION_COPY);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->apps = g_list_store_new (G_TYPE_APP_INFO);
  self->settings = g_settings_new (PHOSH_SCHEMA_ID);

  g_signal_connect_swapped (self->settings, "changed::" FAVORITES_KEY,
                            G_CALLBACK (on_favorites_changed),
                            self);

  gtk_flow_box_bind_model (self->fbox,
                           G_LIST_MODEL (self->apps),
                           create_fav_app,
                           self,
                           NULL);
  g_signal_connect (target, "drop", G_CALLBACK (on_drop_flowbox), self);
  gtk_widget_add_controller (GTK_WIDGET (self->fbox), GTK_EVENT_CONTROLLER (target));

  on_favorites_changed (self);

  g_signal_connect_swapped (self->settings, "changed::" APP_FILTER_MODE_KEY,
                            G_CALLBACK (on_afm_setting_changed),
                            self);

  on_afm_setting_changed (self);

  self->background_settings = g_settings_new (BACKGROUND_SCHEMA_ID);
  g_settings_bind_with_mapping (self->background_settings,
                                BACKGROUND_KEY_PICTURE_OPTIONS,
                                self->wallpaper_switch,
                                "active",
                                G_SETTINGS_BIND_DEFAULT,
                                ms_picture_mode_to_bool,
                                ms_bool_to_picture_mode,
                                NULL, NULL);
}


MsOverviewPanel *
ms_overview_panel_new (void)
{
  return MS_OVERVIEW_PANEL (g_object_new (MS_TYPE_OVERVIEW_PANEL, NULL));
}
