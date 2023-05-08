dbus_inc = include_directories('.')

# DBus client interfaces
generated_dbus_sources = []
dbus_doc_targets = []

dbus_prefix = 'MsDBus'

# iio-sensor-proxy
generated_dbus_sources += gnome.gdbus_codegen('iio-sensor-proxy-dbus',
                                              'net.hadess.SensorProxy.xml',
                                              interface_prefix: 'net.hadess',
                                              namespace: dbus_prefix)
