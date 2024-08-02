project('phosh-mobile-settings', 'c',
          version: '0.40.0',
    meson_version: '>= 1.0.0',
  default_options: [ 'warning_level=2',
                     'werror=false',
                     'c_std=gnu11',
                   ],
)

app_id = 'mobi.phosh.MobileSettings'

prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
datadir = prefix / get_option('datadir')
libdir = prefix / get_option('libdir')
ms_plugins_dir = prefix / libdir / 'phosh-mobile-settings' / 'plugins'

glib_ver = '2.74'
glib_ver_str = 'GLIB_VERSION_@0@'.format(glib_ver.replace('.','_'))
glib_ver_cmp = '>=@0@'.format(glib_ver)

gtk_ver = '4.12.5'
gtk_ver_arr = gtk_ver.split('.')
gtk_ver_str = 'GDK_VERSION_@0@_@1@'.format(gtk_ver_arr[0], gtk_ver_arr[1])
gtk_ver_cmp = '>=@0@'.format(gtk_ver)

adw_ver = '1.5'
adw_ver_str = 'ADW_VERSION_@0@'.format(adw_ver.replace('.','_'))
adw_ver_cmp = '>=@0@'.format(adw_ver)

glib_dep = dependency('glib-2.0', version: glib_ver_cmp)
gio_dep = dependency('gio-2.0', version: glib_ver_cmp)
gio_unix_dep =  dependency('gio-unix-2.0', version: glib_ver_cmp)
gmodule_dep = dependency('gmodule-2.0', version: glib_ver_cmp)
gmobile_dep = dependency('gmobile',
  version: '>= 0.2.0',
  fallback: ['gmobile', 'gmobile_dep'],
  default_options:
    [ 'examples=false',
      'introspection=false',
      'gtk_doc=false',
      'tests=false'
    ])
gsound_dep = dependency('gsound')
gtk_dep = dependency('gtk4', version: gtk_ver_cmp)
gtk_wayland_dep = dependency('gtk4-wayland', version: gtk_ver_cmp)
phosh_plugins_dep = dependency('phosh-plugins', version: '>= 0.23.0')
phosh_settings_dep = dependency('phosh-settings-schemas', required: false)
wayland_client_dep = dependency('wayland-client', version: '>=1.14')
wayland_protos_dep = dependency('wayland-protocols', version: '>=1.12')

adwaita_dep = dependency(
  'libadwaita-1',
  version: adw_ver_cmp,
  fallback: ['libadwaita', 'adwaita_dep'],
  default_options: [
    'examples=false',
    'introspection=disabled',
    'tests=false',
  ],
)

i18n = import('i18n')

gnome = import('gnome')

cc = meson.get_compiler('c')
global_c_args = []
test_c_args = [
  '-Wcast-align',
  '-Wdate-time',
  '-Wdeclaration-after-statement',
  ['-Werror=format-security', '-Werror=format=2'],
  '-Wendif-labels',
  '-Werror=incompatible-pointer-types',
  '-Werror=missing-declarations',
  '-Werror=overflow',
  '-Werror=return-type',
  '-Werror=shift-count-overflow',
  '-Werror=shift-overflow=2',
  '-Werror=implicit-fallthrough=3',
  '-Wfloat-equal',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Winit-self',
  '-Wmaybe-uninitialized',
  '-Wmissing-field-initializers',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wnested-externs',
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-strict-aliasing',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wshadow',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wtype-limits',
  '-Wundef',
  '-Wunused-function',
]
if get_option('buildtype') != 'plain'
  test_c_args += '-fstack-protector-strong'
endif

foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    global_c_args += arg
  endif
endforeach
add_project_arguments(
  [global_c_args,
   '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_ver_str),
   '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_ver_str),
   '-DGDK_VERSION_MIN_REQUIRED=@0@'.format(gtk_ver_str),
   '-DGDK_VERSION_MAX_ALLOWED=@0@'.format(gtk_ver_str),
   '-DADW_VERSION_MIN_REQUIRED=@0@'.format(adw_ver_str),
   '-DADW_VERSION_MAX_ALLOWED=@0@'.format(adw_ver_str),
  ],
  language: 'c',
)

run_data = configuration_data()
run_data.set('ABS_BUILDDIR', meson.current_build_dir())
configure_file(
  input: 'run.in',
  output: 'run',
  configuration: run_data)

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'phosh-mobile-settings')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config_h.set_quoted('MOBILE_SETTINGS_APP_ID', app_id)
config_h.set_quoted('MOBILE_SETTINGS_VERSION', meson.project_version())
config_h.set_quoted('MOBILE_SETTINGS_PLUGINS_DIR', ms_plugins_dir)
config_h.set_quoted('MOBILE_SETTINGS_PHOSH_PLUGINS_DIR',
		    phosh_plugins_dep.get_variable(pkgconfig: 'lockscreen_plugins_dir'))
config_h.set_quoted('MOBILE_SETTINGS_PHOSH_PREFS_DIR',
		    phosh_plugins_dep.get_variable(pkgconfig: 'lockscreen_prefs_dir'))
config_h.set('HAVE_PHOSH_SETTINGS_SCHEMAS', phosh_settings_dep.found())
configure_file(
  output: 'mobile-settings-config.h',
  configuration: config_h,
)
add_project_arguments([
  '-I' + meson.project_build_root(),
], language: 'c')


subdir('data')
subdir('protocols')
subdir('src')
subdir('tests')
subdir('plugins')
subdir('po')

# Older meson can't handle gnome.post_install but that only
# matters for distro backports:
if meson.version().version_compare('>=0.59.0')
  gnome.post_install(
    gtk_update_icon_cache: true,
    update_desktop_database: true,
  )
endif
