/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Gotam Gorabh <gautamy672@gmail.com>
 */

#include "mobile-settings-config.h"

#include "mobile-settings-application.h"
#include "mobile-settings-debug-info.h"

#define GMOBILE_USE_UNSTABLE_API
#include <gmobile.h>

/* Copied and adapted from gtk/inspector/general.c */
static void
get_gtk_info (const char **backend,
              const char **renderer)
{
  GdkDisplay *display = gdk_display_get_default ();
  GdkSurface *surface = NULL;
  GskRenderer *gsk_renderer = NULL;

  if (!g_strcmp0 (G_OBJECT_TYPE_NAME (display), "GdkX11Display"))
    *backend = "X11";
  else if (!g_strcmp0 (G_OBJECT_TYPE_NAME (display), "GdkWaylandDisplay"))
    *backend = "Wayland";
  else if (!g_strcmp0 (G_OBJECT_TYPE_NAME (display), "GdkBroadwayDisplay"))
    *backend = "Broadway";
  else if (!g_strcmp0 (G_OBJECT_TYPE_NAME (display), "GdkWin32Display"))
    *backend = "Windows";
  else if (!g_strcmp0 (G_OBJECT_TYPE_NAME (display), "GdkMacosDisplay"))
    *backend = "macOS";
  else
    *backend = G_OBJECT_TYPE_NAME (display);

  surface = gdk_surface_new_toplevel (display);
  gsk_renderer = gsk_renderer_new_for_surface (surface);
  if (!g_strcmp0 (G_OBJECT_TYPE_NAME (gsk_renderer), "GskVulkanRenderer"))
    *renderer = "Vulkan";
  else if (!g_strcmp0 (G_OBJECT_TYPE_NAME (gsk_renderer), "GskGLRenderer"))
    *renderer = "GL";
  else if (!g_strcmp0 (G_OBJECT_TYPE_NAME (gsk_renderer), "GskCairoRenderer"))
    *renderer = "Cairo";
  else
    *renderer = G_OBJECT_TYPE_NAME (gsk_renderer);

  gsk_renderer_unrealize (gsk_renderer);
  g_object_unref (gsk_renderer);
  gdk_surface_destroy (surface);
}

#ifndef G_OS_WIN32
static char *
get_flatpak_info (const char *group,
                  const char *key)
{
  GKeyFile *keyfile = g_key_file_new ();
  g_autofree char *ret = NULL;

  if (g_key_file_load_from_file (keyfile, "/.flatpak-info", 0, NULL))
    ret = g_key_file_get_string (keyfile, group, key, NULL);

  g_key_file_unref (keyfile);

  return ret;
}
#endif

static char *
get_phosh_session_version (void)
{
  g_autoptr(GSubprocess) subprocess = NULL;
  GInputStream *stdout_stream = NULL;
  g_autoptr(GDataInputStream) data_input_stream = NULL;
  g_autofree char *version = NULL;
  g_autoptr(GError) error = NULL;

  subprocess = g_subprocess_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE, &error,
                                 "phosh-session", "--version", NULL);

  if (subprocess == NULL) {
    g_warning ("Failed to read version: %s", error->message);
    return NULL;
  }

  stdout_stream = g_subprocess_get_stdout_pipe (subprocess);
  data_input_stream = g_data_input_stream_new (G_INPUT_STREAM (stdout_stream));
  version = g_data_input_stream_read_line (data_input_stream, NULL, NULL, &error);

  if (version == NULL) {
    g_warning ("Failed to read version: %s", error->message);
    return NULL;
  }

  return g_steal_pointer (&version);
}


static char *
get_os_info (void)
{
  g_autofree char *pretty_name = NULL;
  g_autofree char *os_name = g_get_os_info (G_OS_INFO_KEY_NAME);
  g_autofree char *os_version = g_get_os_info (G_OS_INFO_KEY_VERSION);

  if (os_name && os_version)
    return g_strdup_printf ("%s %s", os_name, os_version);

  pretty_name = g_get_os_info (G_OS_INFO_KEY_PRETTY_NAME);
  if (pretty_name)
    return g_steal_pointer (&pretty_name);

  /* Translators: Not marked as translatable as debug output should stay English */
  return g_strdup ("Unknown");
}


char *
mobile_settings_generate_debug_info (void)
{
  GString *string = g_string_new (NULL);
#ifndef G_OS_WIN32
  gboolean flatpak = g_file_test ("/.flatpak-info", G_FILE_TEST_EXISTS);
#endif

  g_string_append_printf (string, "Mobile Settings: %s\n", MOBILE_SETTINGS_VERSION);
  g_string_append (string, "Compiled against:\n");
  g_string_append_printf (string, "- GLib: %d.%d.%d\n", GLIB_MAJOR_VERSION,
                                                        GLIB_MINOR_VERSION,
                                                        GLIB_MICRO_VERSION);
  g_string_append_printf (string, "- GTK: %d.%d.%d\n", GTK_MAJOR_VERSION,
                                                       GTK_MINOR_VERSION,
                                                       GTK_MICRO_VERSION);

  g_string_append_printf (string, "- Libadwaita: %d.%d.%d\n", ADW_MAJOR_VERSION,
                                                              ADW_MINOR_VERSION,
                                                              ADW_MICRO_VERSION);
  g_string_append (string, "\n");

  g_string_append (string, "Running against:\n");
  g_string_append_printf (string, "- GLib: %d.%d.%d\n", glib_major_version,
                                                        glib_minor_version,
                                                        glib_micro_version);
  g_string_append_printf (string, "- GTK: %d.%d.%d\n", gtk_get_major_version (),
                                                       gtk_get_minor_version (),
                                                       gtk_get_micro_version ());

  g_string_append_printf (string, "- Libadwaita: %d.%d.%d\n", adw_get_major_version (),
                                                              adw_get_minor_version (),
                                                              adw_get_micro_version ());
  g_string_append (string, "\n");

  {
    g_autofree char *phosh_session_version = get_phosh_session_version ();
    g_autofree char *os_info = get_os_info ();

    g_string_append (string, "System:\n");
    g_string_append_printf (string, "- Operating System: %s\n", os_info);
    g_string_append_printf (string, "- Phosh-session: %s\n", phosh_session_version);
    g_string_append (string, "\n");
  }

  {
    const char *backend, *renderer;

    get_gtk_info (&backend, &renderer);


    g_string_append (string, "GTK:\n");
    g_string_append_printf (string, "- GDK backend: %s\n", backend);
    g_string_append_printf (string, "- GSK renderer: %s\n", renderer);
    g_string_append (string, "\n");
  }

#ifndef G_OS_WIN32
  if (flatpak) {
    g_autofree char *runtime = get_flatpak_info ("Application", "runtime");
    g_autofree char *runtime_commit = get_flatpak_info ("Instance", "runtime-commit");
    g_autofree char *arch = get_flatpak_info ("Instance", "arch");
    g_autofree char *flatpak_version = get_flatpak_info ("Instance", "flatpak-version");
    g_autofree char *devel = get_flatpak_info ("Instance", "devel");

    g_string_append (string, "Flatpak:\n");
    g_string_append_printf (string, "- Runtime: %s\n", runtime);
    g_string_append_printf (string, "- Runtime commit: %s\n", runtime_commit);
    g_string_append_printf (string, "- Arch: %s\n", arch);
    g_string_append_printf (string, "- Flatpak version: %s\n", flatpak_version);
    g_string_append_printf (string, "- Devel: %s\n", devel ? "yes" : "no");
    g_string_append (string, "\n");
  }
#endif

  {
    const char *desktop = g_getenv ("XDG_CURRENT_DESKTOP");
    const char *session_desktop = g_getenv ("XDG_SESSION_DESKTOP");
    const char *session_type = g_getenv ("XDG_SESSION_TYPE");
    const char *lang = g_getenv ("LANG");
    const char *builder = g_getenv ("INSIDE_GNOME_BUILDER");

    const char * const env_vars[] = { "PHOC_DEBUG", "PHOSH_DEBUG", "GTK_DEBUG", "GTK_THEME",
                                      "ADW_DEBUG_COLOR_SCHEME", "ADW_DEBUG_HIGH_CONTRAST",
                                      "ADW_DISABLE_PORTAL", "WAYLAND_DEBUG", "WAYLAND_DISPLAY",
                                      "WAYLAND_SOCKET", "XDG_RUNTIME_DIR", "WLR_BACKENDS", NULL };

    g_string_append (string, "Environment:\n");
    g_string_append_printf (string, "- Desktop: %s\n", desktop);
    g_string_append_printf (string, "- Session: %s (%s)\n", session_desktop,
                                                            session_type);
    g_string_append_printf (string, "- Language: %s\n", lang);
    g_string_append_printf (string, "- Running inside Builder: %s\n", builder ? "yes" : "no");

    for (int i = 0; env_vars[i] != NULL; i++) {
      const char *env_var = g_getenv (env_vars[i]);
      if (env_var)
        g_string_append_printf (string, "- %s: %s\n", env_vars[i], env_var);
    }
  }
  g_string_append (string, "\n");

  {
    static struct {
      const char *schema;
      const char *key;
    } const schema[] = {
      { "sm.puri.phosh.emergency-calls", "enabled" },
      { "sm.puri.phosh", "automatic-high-contrast" },
      { "sm.puri.phosh.plugins", "lock-screen" },
      { "org.gnome.desktop.a11y.applications", "screen-keyboard-enabled" },
      { "org.gnome.desktop.interface", "gtk-im-module" },
      { "org.gnome.desktop.input-sources", "sources" },

      /* Power related */
      { "org.gnome.settings-daemon.plugins.power", "ambient-enabled" },
      { "org.gnome.settings-daemon.plugins.power", "idle-dim" },
      { "org.gnome.settings-daemon.plugins.power", "sleep-inactive-battery-timeout" },
      { "org.gnome.settings-daemon.plugins.power", "sleep-inactive-battery-type" },
      { "org.gnome.settings-daemon.plugins.power", "sleep-inactive-ac-timeout" },
      { "org.gnome.settings-daemon.plugins.power", "sleep-inactive-ac-type" },

      /* Screen wakeup */
      { "sm.puri.phosh.notifications", "wakeup-screen-categories" },
      { "sm.puri.phosh.notifications", "wakeup-screen-triggers" },
      { "sm.puri.phosh.notifications", "wakeup-screen-urgency" },

      /* Other phosh related */
      { "sm.puri.phosh", "app-filter-mode" },
      { "sm.puri.phosh", "automatic-high-contrast" },
      { "sm.puri.phoc", "auto-maximize" },
    };

    g_string_append (string, "Settings:\n");

    for (int i = 0; i < G_N_ELEMENTS (schema); i++) {
      g_autoptr (GSettings) settings = g_settings_new (schema[i].schema);
      g_autoptr (GVariant) value;
      g_autofree gchar *result;
      value = g_settings_get_value (settings, schema[i].key);
      result = g_variant_print (value, TRUE);
      g_string_append_printf (string, "- %s '%s': %s\n", schema[i].schema, schema[i].key, result);
    }
  }
  g_string_append (string, "\n");

  g_string_append_printf (string, "Wayland Protocols\n");
  {
    MobileSettingsApplication *app = MOBILE_SETTINGS_APPLICATION (g_application_get_default ());
    g_auto (GStrv) wayland_protcols = NULL;

    wayland_protcols = mobile_settings_application_get_wayland_protocols (app);
    for (int i = 0; wayland_protcols[i]; i++) {
      const char *protocol = wayland_protcols[i];
      guint32 version;

      version = mobile_settings_application_get_wayland_protocol_version (app, protocol);
      g_string_append_printf (string, "- %s: %d\n", protocol, version);
    }
  }
  g_string_append (string, "\n");

  g_string_append_printf (string, "Hardware Information:\n");
  {
    g_autoptr (GError) err = NULL;
    g_auto (GStrv) compatibles = gm_device_tree_get_compatibles (NULL, &err);

    if (compatibles && compatibles[0]) {
      g_autofree char *compatible_str = g_strjoinv (" ", compatibles);

      g_string_append_printf (string, "- DT compatibles: %s\n", compatible_str);
    } else {
      g_autofree char *modalias = NULL;

      g_debug ("Couldn't get device tree information: %s", err->message);

      if (g_file_get_contents ("/sys/class/dmi/id/modalias", &modalias, NULL, NULL))
        g_string_append_printf (string, "- DMI modalias: %s\n", modalias);
      else
        g_string_append_printf (string, "Could not read DMI or DT information");
    }
  }

  return g_string_free (string, FALSE);
}
