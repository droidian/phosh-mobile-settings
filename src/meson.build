subdir('dbus')

mobile_settings_enum_sources = gnome.mkenums_simple('ms-enum-types',
     sources : files('mobile-settings-enums.h'))

mobile_settings_plugin_sources = [
  'mobile-settings-plugin.h',
  'mobile-settings-plugin.c',
  'ms-plugin-panel.h',
  'ms-plugin-panel.c',
]

mobile_settings_sources = [
  'main.c',
  'mobile-settings-window.h',
  'mobile-settings-window.c',
  'mobile-settings-application.h',
  'mobile-settings-application.c',
  'mobile-settings-debug-info.h',
  'mobile-settings-debug-info.c',
  'ms-applications-panel.c',
  'ms-applications-panel.h',
  'ms-completer-info.c',
  'ms-completer-info.h',
  'ms-compositor-panel.c',
  'ms-compositor-panel.h',
  'ms-convergence-panel.c',
  'ms-convergence-panel.h',
  'ms-features-panel.c',
  'ms-features-panel.h',
  'ms-feedback-row.c',
  'ms-feedback-row.h',
  'ms-feedback-panel.c',
  'ms-feedback-panel.h',
  'ms-head-tracker.c',
  'ms-head-tracker.h',
  'ms-lockscreen-panel.c',
  'ms-lockscreen-panel.h',
  'ms-osk-add-layout-dialog.c',
  'ms-osk-add-layout-dialog.h',
  'ms-osk-layout.c',
  'ms-osk-layout.h',
  'ms-osk-layout-prefs.c',
  'ms-osk-layout-prefs.h',
  'ms-osk-layout-row.c',
  'ms-osk-layout-row.h',
  'ms-osk-panel.c',
  'ms-osk-panel.h',
  'ms-panel-switcher.c',
  'ms-panel-switcher.h',
  'ms-plugin-loader.h',
  'ms-plugin-list-box.c',
  'ms-plugin-list-box.h',
  'ms-plugin-loader.c',
  'ms-plugin-loader.h',
  'ms-plugin-row.c',
  'ms-plugin-row.h',
  'ms-scale-to-fit-row.c',
  'ms-scale-to-fit-row.h',
  'ms-sensor-panel.c',
  'ms-sensor-panel.h',
  'ms-sound-row.c',
  'ms-sound-row.h',
  'ms-topbar-panel.c',
  'ms-topbar-panel.h',
  'ms-toplevel-tracker.c',
  'ms-toplevel-tracker.h',
  'ms-util.c',
  'ms-util.h',
  generated_dbus_sources,
  mobile_settings_enum_sources,
  mobile_settings_plugin_sources,
  wl_proto_headers,
  wl_proto_sources,
]

mobile_settings_deps = [
  adwaita_dep,
  gio_dep,
  gio_unix_dep,
  glib_dep,
  gmobile_dep,
  gmodule_dep,
  gnome_desktop_dep,
  gsound_dep,
  gtk_dep,
  gtk_wayland_dep,
  json_glib_dep,
  phosh_plugins_dep,
  phosh_settings_dep,
  wayland_client_dep,
]

gnome = import('gnome')

mobile_settings_sources += gnome.compile_resources('mobile-settings-resources',
  'mobile-settings.gresource.xml',
  c_name: 'mobile_settings'
)

executable('phosh-mobile-settings', mobile_settings_sources,
  dependencies: mobile_settings_deps,
  install: true,
)


# Helpers for plugins */
ms_plugin_inc = include_directories('.')
ms_plugin_deps = [
  glib_dep,
  adwaita_dep,
]
mobile_settings_plugin_lib = static_library('mobile-settings-plugin',
  mobile_settings_plugin_sources,
  include_directories: [ms_plugin_inc, dbus_inc],
  dependencies: ms_plugin_deps)
mobile_settings_plugin_dep = declare_dependency(
  include_directories: [ms_plugin_inc, dbus_inc],
  link_with: mobile_settings_plugin_lib,
  dependencies: ms_plugin_deps)
