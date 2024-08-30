desktop_file = i18n.merge_file(
  input: 'mobi.phosh.MobileSettings.desktop.in',
  output: 'mobi.phosh.MobileSettings.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

appstream_file = i18n.merge_file(
  input: 'mobi.phosh.MobileSettings.metainfo.xml.in',
  output: 'mobi.phosh.MobileSettings.metainfo.xml',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo')
)

appstream_util = find_program('appstreamcli', required: false)
if appstream_util.found()
  test('Validate appstream file', appstream_util,
    args: ['validate', '--no-net', appstream_file]
  )
endif

dbus_service_conf = configuration_data()
dbus_service_conf.set('bindir', bindir)
dbus_service_conf.set('appid', app_id)

dbus_service = '@0@.service'.format(app_id)
configure_file(
          input : dbus_service + '.in',
         output : dbus_service,
        install : true,
    install_dir : datadir / 'dbus-1' / 'services',
  configuration : dbus_service_conf
)

schemas = ['mobi.phosh.MobileSettings.gschema.xml']
compiled = gnome.compile_schemas(
  build_by_default: true
)

compile_schemas = find_program('glib-compile-schemas', required: false)
if compile_schemas.found()
  test('Validate schema file', compile_schemas,
       args: ['--strict', '--dry-run', meson.current_source_dir()])
endif

install_data(
  schemas,
  install_dir: 'share/glib-2.0/schemas'
)

subdir('icons')
