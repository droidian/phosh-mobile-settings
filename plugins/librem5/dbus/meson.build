# DBus client interfaces
l5_plugin_dbus_sources = []

l5_plugin_dbus_prefix = 'MsPluginLibrem5DBus'

# org.freedesktop.login1.Manager
l5_plugin_dbus_sources += gnome.gdbus_codegen('login1-manager-dbus',
                                              'org.freedesktop.login1.Manager.xml',
                                              interface_prefix: 'org.freedesktop.login1',
                                              namespace: l5_plugin_dbus_prefix)
