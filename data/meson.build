desktop_file = i18n.merge_file(
  input: 'org.sigxcpu.MobileSettings.desktop.in',
  output: 'org.sigxcpu.MobileSettings.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

appstream_file = i18n.merge_file(
  input: 'org.sigxcpu.MobileSettings.metainfo.xml.in',
  output: 'org.sigxcpu.MobileSettings.metainfo.xml',
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

subdir('icons')
