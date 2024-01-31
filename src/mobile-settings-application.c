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
#include "mobile-settings-debug-info.h"

#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include "protocols/wlr-output-management-unstable-v1-client-protocol.h"

#include <gdk/wayland/gdkwayland.h>
#include <wayland-client.h>
#include <glib/gi18n.h>

#define PHOC_LAYER_SHELL_EFFECTS_PROTOCOL_NAME "zphoc_layer_shell_effects_v1"
#define PHOSH_PRIVATE_PROTOCOL_NAME "phosh_private"

enum {
  PROP_0,
  PROP_TOPLEVEL_TRACKER,
  PROP_HEAD_TRACKER,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


struct _MobileSettingsApplication {
  AdwApplication  parent_instance;

  MsPluginLoader *device_plugin_loader;
  GtkWidget      *device_panel;

  struct wl_display  *wl_display;
  struct wl_registry *wl_registry;
  struct zwlr_foreign_toplevel_manager_v1 *foreign_toplevel_manager;
  struct zwlr_output_manager_v1 *output_manager;
  MsToplevelTracker  *toplevel_tracker;
  MsHeadTracker     *head_tracker;

  GHashTable        *wayland_protocols;
};

G_DEFINE_TYPE (MobileSettingsApplication, mobile_settings_application, ADW_TYPE_APPLICATION)

static const GOptionEntry entries[] = {
  {
    "list", 'l',
    G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
    NULL, "Lists the available panels in phosh-mobile-settings", NULL
  },
  {
    G_OPTION_REMAINING, '\0',
    G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME_ARRAY,
    NULL, "Panel to display", "[PANEL]"
  },
  G_OPTION_ENTRY_NULL
};


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

  g_hash_table_insert (self->wayland_protocols, g_strdup (interface), GUINT_TO_POINTER(version));
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


static GtkWindow *
get_active_window (MobileSettingsApplication *self)
{
  GtkWindow *window = gtk_application_get_active_window (GTK_APPLICATION (self));

  if (window == NULL)
    window = g_object_new (MOBILE_SETTINGS_TYPE_WINDOW, "application", self, NULL);

  return window;
}


static void
list_available_panels (GApplication *self)
{
  MobileSettingsWindow *window;
  g_autoptr (GListModel) list = NULL;
  g_autoptr (GtkStackPage) page = NULL;
  const char *name;

  // Since we're in the local instance, just get us a window
  adw_init ();

  window = g_object_new (MOBILE_SETTINGS_TYPE_WINDOW, NULL);
  list = G_LIST_MODEL (mobile_settings_window_get_stack_pages (window));

  g_print ("Available panels:\n");

  for (uint i = 0; i < g_list_model_get_n_items (list); ++i) {
    page = g_list_model_get_item (list, i);
    name = gtk_stack_page_get_name (page);

    g_print ("- %s\n", name);
  }
}


static void
set_panel_activated (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  MobileSettingsApplication *self = MOBILE_SETTINGS_APPLICATION (user_data);
  MobileSettingsWindow *window;
  MsPanelSwitcher *panel_switcher;
  gchar *panel;

  g_variant_get (parameter, "(&s)", &panel);

  g_debug ("'set-panel' '%s'", panel);

  window = MOBILE_SETTINGS_WINDOW (get_active_window (self));
  panel_switcher = mobile_settings_window_get_panel_switcher (window);

  if (!ms_panel_switcher_set_active_panel_name (panel_switcher, panel))
    g_warning ("Error: panel `%s` not available, launching with default options.", panel);
}


MobileSettingsApplication *
mobile_settings_application_new (gchar *application_id)
{
  return g_object_new (MOBILE_SETTINGS_TYPE_APPLICATION,
                       "application-id", application_id,
                       "flags", G_APPLICATION_DEFAULT_FLAGS,
                       NULL);
}


static int
mobile_settings_application_handle_local_options (GApplication *self,
                                                  GVariantDict *options)
{
  g_autofree GStrv panels = NULL;

  if (g_variant_dict_contains (options, "list")) {
    list_available_panels (self);

    return 0;
  } else if (g_variant_dict_lookup (options, G_OPTION_REMAINING, "^a&ay", &panels)) {
    const char *panel;

    g_return_val_if_fail (panels && panels[0], EXIT_FAILURE);
    panel = panels[0];

    g_application_register (G_APPLICATION (self), NULL, NULL);
    g_action_group_activate_action (G_ACTION_GROUP (self), "set-panel", g_variant_new ("(s)", panel));
  }

  return G_APPLICATION_CLASS (mobile_settings_application_parent_class)->handle_local_options (self,
                                                                                               options);
}


static void
mobile_settings_application_activate (GApplication *app)
{
  GtkWindow *window;
  GdkDisplay *gdk_display;
  MobileSettingsApplication *self = MOBILE_SETTINGS_APPLICATION (app);

  g_assert (GTK_IS_APPLICATION (app));

  window = get_active_window (self);

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


static const GActionEntry actions[] = {
  { "set-panel", set_panel_activated, "(s)", NULL, NULL, { 0 } },
};


static void
mobile_settings_application_startup (GApplication *app)
{
  MobileSettingsApplication *self = MOBILE_SETTINGS_APPLICATION (app);

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   actions,
                                   G_N_ELEMENTS (actions),
                                   self);

  G_APPLICATION_CLASS (mobile_settings_application_parent_class)->startup (app);
}


static void
mobile_settings_application_finalize (GObject *object)
{
  MobileSettingsApplication *self = MOBILE_SETTINGS_APPLICATION (object);

  g_clear_object (&self->device_plugin_loader);
  g_clear_pointer (&self->wayland_protocols, g_hash_table_destroy);

  G_OBJECT_CLASS (mobile_settings_application_parent_class)->finalize (object);
}


static void
mobile_settings_application_class_init (MobileSettingsApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->finalize = mobile_settings_application_finalize;
  object_class->get_property = mobile_settings_application_get_property;

  app_class->activate = mobile_settings_application_activate;
  app_class->startup = mobile_settings_application_startup;
  app_class->handle_local_options = mobile_settings_application_handle_local_options;

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
  const gchar *developers[] = {"Guido Günther", NULL};
  const gchar *artists[] = {"Sam Hewitt ", NULL};

  g_return_if_fail (MOBILE_SETTINGS_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  adw_show_about_window (window,
                         "application-name", _("Mobile Settings"),
                         "application-icon", MOBILE_SETTINGS_APP_ID,
                         "version", MOBILE_SETTINGS_VERSION,
                         "copyright", "Copyright (C) 2022 Guido Günther",
                         "website", "https://gitlab.gnome.org/World/Phosh/phosh-mobile-settings",
                         "issue-url", "https://gitlab.gnome.org/World/Phosh/phosh-mobile-settings/-/issues/new",
                         "debug-info", mobile_settings_generate_debug_info (),
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "developers", developers,
                         "artists", artists,
                         "translator-credits", _("translator-credits"),
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

  g_application_set_option_context_parameter_string (G_APPLICATION (self),
                                                     _("- Manage your mobile settings"));
  g_application_add_main_option_entries (G_APPLICATION (self), entries);

  self->device_plugin_loader = ms_plugin_loader_new (plugin_dirs, MS_EXTENSION_POINT_DEVICE_PANEL);
  self->wayland_protocols = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   g_free,
                                                   NULL);
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


GStrv
mobile_settings_application_get_wayland_protocols (MobileSettingsApplication *self)
{
  g_autoptr (GList) keys = NULL;
  g_autoptr (GStrvBuilder) protocols = g_strv_builder_new ();

  g_assert (MOBILE_SETTINGS_APPLICATION (self));

  keys = g_hash_table_get_keys (self->wayland_protocols);
  if (keys == NULL)
    return NULL;

  for (GList *l = keys; l; l = l->next)
    g_strv_builder_add (protocols, l->data);

  return g_strv_builder_end (protocols);
}


guint32
mobile_settings_application_get_wayland_protocol_version (MobileSettingsApplication *self,
                                                          const char                *protocol)
{
  gpointer *version;

  g_assert (MOBILE_SETTINGS_APPLICATION (self));

  version = g_hash_table_lookup (self->wayland_protocols, protocol);

  return GPOINTER_TO_UINT (version);
}
