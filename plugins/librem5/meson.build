subdir('dbus')

librem5_plugin_deps = [
  adwaita_dep,
  gio_dep,
  gio_unix_dep,
  glib_dep,
  gtk_dep,
  mobile_settings_plugin_dep,
  cc.find_library('sensors'),
]

librem5_plugin_resources = gnome.compile_resources(
  'ms-plugin-librem5-resources',
  'ms-plugin-librem5.gresources.xml',
  c_name: 'ms_plugin_librem5',
)

librem5_plugin_sources = files(
  'ms-plugin-librem5.c',
  'ms-plugin-librem5-panel.h',
  'ms-plugin-librem5-panel.c',
) + librem5_plugin_resources + l5_plugin_dbus_sources

linrem5_plugin = shared_module(
  'ms-plugin-librem5',
  librem5_plugin_sources,
  dependencies: librem5_plugin_deps,
  c_args : ['-DG_LOG_DOMAIN="ms-plugin-librem5"'],
  include_directories: ms_plugin_inc,
  install: true,
  install_dir: ms_plugins_dir,
)
