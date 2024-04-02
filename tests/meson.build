test_env_common = environment()
test_env_common.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env_common.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env_common.set('G_DEBUG', 'gc-friendly,fatal-warnings')
test_env_common.set('GSETTINGS_BACKEND', 'memory')
test_env_common.set('PYTHONDONTWRITEBYTECODE', 'yes')
test_env_common.set('MALLOC_CHECK_', '2')
test_env_common.set('NO_AT_BRIDGE', '1')
test_env_common.set('GTK_A11Y', 'none')

phoc = find_program('phoc')

if phoc.found()

  test_env_phoc = test_env_common
  test_env_phoc.set('XDG_RUNTIME_DIR', meson.current_build_dir())
  test_env_phoc.set('WLR_RENDERER', 'pixman')
  test_env_phoc.set('WLR_BACKENDS', 'headless')

  local_opts = [
    'debug-info',
    'help',
    'list',
    'version',
  ]

  # Execute these tests under phoc
  foreach opt : local_opts
    test('test-@0@'.format(opt), phoc,
         args: ['-E',
		'@0@/src/phosh-mobile-settings --@1@'.format(meson.build_root(), opt),
	       ],
         env: test_env_phoc)
  endforeach

endif
