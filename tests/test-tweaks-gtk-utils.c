/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "pmos-tweaks/ms-tweaks-gtk-utils.h"


static void
test_gdkrgba_to_rgb_hex_string (void)
{
  GdkRGBA from = { .red = 1.0, .green = 1.0, .blue = 1.0 };
  char *to = NULL;

  to = ms_tweaks_util_gdkrgba_to_rgb_hex_string (&from);

  g_assert_cmpstr ("#ffffff", ==, to);

  g_free (to);

  from.red = 0.0;
  from.green = 0.0;
  from.blue = 0.0;

  to = ms_tweaks_util_gdkrgba_to_rgb_hex_string (&from);

  g_assert_cmpstr ("#000000", ==, to);

  g_free (to);

  from.red = 0.5;
  from.green = 0.5;
  from.blue = 0.5;

  to = ms_tweaks_util_gdkrgba_to_rgb_hex_string (&from);

  g_assert_cmpstr ("#808080", ==, to);

  g_free (to);

  from.red = 1.0;
  from.green = 0.0;
  from.blue = 0.5;

  to = ms_tweaks_util_gdkrgba_to_rgb_hex_string (&from);

  g_assert_cmpstr ("#ff0080", ==, to);

  g_free (to);

  from.red = g_test_rand_double_range (0.0, 1.0);
  from.green = g_test_rand_double_range (0.0, 1.0);
  from.blue = g_test_rand_double_range (0.0, 1.0);

  to = ms_tweaks_util_gdkrgba_to_rgb_hex_string (&from);

  g_assert_nonnull (to);
  g_assert_cmpint (7, ==, strlen (to));
  g_assert_cmpint ('#', ==, to[0]);

  g_free (to);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh-mobile-settings/test-tweaks-gtk-utils-gdkrgba-to-gdb-hex-string",
                   test_gdkrgba_to_rgb_hex_string);

  return g_test_run ();
}
