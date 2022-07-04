/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "mobile-settings-config.h"

#include "mobile-settings-application.h"
#include "mobile-settings-window.h"
#include "mobile-settings-plugin.h"
#include "ms-plugin-loader.h"
#include "ms-toplevel-tracker.h"
#include "ms-head-tracker.h"

#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include "protocols/wlr-output-management-unstable-v1-client-protocol.h"

#include <gdk/wayland/gdkwayland.h>
#include <glib/gi18n.h>

enum {
  PROP_0,
  PROP_TOPLEVEL_TRACKER,
  PROP_HEAD_TRACKER,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


struct _MobileSettingsApplication {
  GtkApplication  parent_instance;

  MsPluginLoader *device_plugin_loader;
  GtkWidget      *device_panel;

  struct wl_display  *wl_display;
  struct wl_registry *wl_registry;
  struct zwlr_foreign_toplevel_manager_v1 *foreign_toplevel_manager;
  struct zwlr_output_manager_v1 *output_manager;
  MsToplevelTracker  *toplevel_tracker;
  MsHeadTracker     *head_tracker;
};

G_DEFINE_TYPE (MobileSettingsApplication, mobile_settings_application, ADW_TYPE_APPLICATION)


static void
mobile_settings_application_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  MobileSettingsApplication *self = MOBILE_SETTINGS_APPLICATION (object);

  switch (property_id) {
  case PROP_TOPLEVEL_TRACKER:
    g_value_set_object (value, self->toplevel_tracker);
    break;
  case PROP_HEAD_TRACKER:
    g_value_set_object (value, self->head_tracker);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}



static void
registry_handle_global (void               *data,
                        struct wl_registry *registry,
                        uint32_t            name,
                        const char         *interface,
                        uint32_t            version)
{
  MobileSettingsApplication *self = MOBILE_SETTINGS_APPLICATION (data);

  if (strcmp (interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0) {
    self->foreign_toplevel_manager =
      wl_registry_bind (registry, name, &zwlr_foreign_toplevel_manager_v1_interface, 1);
  } else if (strcmp (interface, zwlr_output_manager_v1_interface.name) == 0) {
    self->output_manager =
      wl_registry_bind (registry, name, &zwlr_output_manager_v1_interface, 2);
  }

  if (self->foreign_toplevel_manager && self->output_manager &&
      self->toplevel_tracker == NULL) {
    g_debug ("Found all wayland protocols. Creating listeners.");

    self->toplevel_tracker = ms_toplevel_tracker_new (self->foreign_toplevel_manager);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TOPLEVEL_TRACKER]);

    self->head_tracker = ms_head_tracker_new (self->output_manager);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HEAD_TRACKER]);
  }
}


static void
registry_handle_global_remove (void               *data,
                               struct wl_registry *registry,
                               uint32_t            name)
{
  g_warning ("Global %d removed but not handled", name);
}


static const struct wl_registry_listener registry_listener = {
  registry_handle_global,
  registry_handle_global_remove
};


MobileSettingsApplication *
mobile_settings_application_new (gchar *application_id,
                                 GApplicationFlags flags)
{
  return g_object_new (MOBILE_SETTINGS_TYPE_APPLICATION,
                       "application-id", application_id,
                       "flags", flags,
                       NULL);
}

static void
mobile_settings_application_activate (GApplication *app)
{
  GtkWindow *window;
  GdkDisplay *gdk_display;
  MobileSettingsApplication *self = MOBILE_SETTINGS_APPLICATION (app);

  g_assert (GTK_IS_APPLICATION (app));

  window = gtk_application_get_active_window (GTK_APPLICATION (app));
  if (window == NULL)
    window = g_object_new (MOBILE_SETTINGS_TYPE_WINDOW,
                           "application", app,
                           NULL);

  if (self->wl_display == NULL) {
    gdk_display = gdk_display_get_default ();
    self->wl_display = gdk_wayland_display_get_wl_display (gdk_display);
    if (self->wl_display != NULL) {
      self->wl_registry = wl_display_get_registry (self->wl_display);
      wl_registry_add_listener (self->wl_registry, &registry_listener, self);
    } else {
      g_critical ("Failed to get display: %m\n");
    }
  }

  gtk_window_present (window);
}


static void
mobile_settings_application_class_init (MobileSettingsApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->get_property = mobile_settings_application_get_property;

  app_class->activate = mobile_settings_application_activate;

  props[PROP_TOPLEVEL_TRACKER] =
    g_param_spec_object ("toplevel-tracker", "", "",
                         MS_TYPE_TOPLEVEL_TRACKER,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_HEAD_TRACKER] =
    g_param_spec_object ("head-tracker", "", "",
                         MS_TYPE_HEAD_TRACKER,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}

static void
mobile_settings_application_show_about (GSimpleAction *action,
                                        GVariant      *parameter,
                                        gpointer       user_data)
{
  MobileSettingsApplication *self = MOBILE_SETTINGS_APPLICATION (user_data);
  GtkWindow *window = NULL;
  const gchar *authors[] = {"Guido Günther", NULL};
  const gchar *artists[] = {"Sam Hewitt ", NULL};

  g_return_if_fail (MOBILE_SETTINGS_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  gtk_show_about_dialog (window,
                         "artists", artists,
                         "authors", authors,
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "logo-icon-name", "org.sigxcpu.MobileSettings",
                         "program-name", "Mobile Settings",
                         "title", _("About Mobile Settings"),
                         "translator-credits", _("translator-credits"),
                         "version", MOBILE_SETTINGS_VERSION,
                         "website", "https://gitlab.gnome.org/guidog/phosh-mobile-settings",
                         NULL);
}


static void
mobile_settings_application_init (MobileSettingsApplication *self)
{
  const char *plugin_dirs[] = { MOBILE_SETTINGS_PLUGINS_DIR, NULL };
  g_autoptr (GSimpleAction) about_action = NULL;
  g_autoptr (GSimpleAction) quit_action = NULL;

  quit_action = g_simple_action_new ("quit", NULL);
  g_signal_connect_swapped (quit_action, "activate", G_CALLBACK (g_application_quit), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (quit_action));

  about_action = g_simple_action_new ("about", NULL);
  g_signal_connect (about_action, "activate", G_CALLBACK (mobile_settings_application_show_about), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (about_action));

  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "app.quit",
                                         (const char *[]) {
    "<primary>q",
    NULL,
  });

  self->device_plugin_loader = ms_plugin_loader_new (plugin_dirs, MS_EXTENSION_POINT_DEVICE_PANEL);
}


GtkWidget *
mobile_settings_application_get_device_panel (MobileSettingsApplication *self)
{
  if (self->device_panel)
    return self->device_panel;

  self->device_panel = ms_plugin_loader_load_plugin (self->device_plugin_loader);
  return self->device_panel;
}


MsToplevelTracker *
mobile_settings_application_get_toplevel_tracker (MobileSettingsApplication *self)
{
  g_assert (MOBILE_SETTINGS_APPLICATION (self));

  return self->toplevel_tracker;
}


MsHeadTracker *
mobile_settings_application_get_head_tracker (MobileSettingsApplication *self)
{
  g_assert (MOBILE_SETTINGS_APPLICATION (self));

  return self->head_tracker;
}
