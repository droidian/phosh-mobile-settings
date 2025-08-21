/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "pmos-tweaks/ms-tweaks-parser.h"


#define SETTING_COUNT 3


static void
test_sort_settings_by_weight (void)
{
  /* This code was written with ease of reading in mind rather than optimising for using GList in a
   * performant way. */
  GHashTable *unsorted_settings = g_hash_table_new (g_direct_hash, g_direct_equal);
  GList *sorted_settings_result = NULL;
  GList *sorted_settings_good = NULL;
  MsTweaksSetting settings[SETTING_COUNT];

  settings[0].weight = 10;
  settings[1].weight = 0;
  settings[2].weight = 90;

  for (int i = 0; i < SETTING_COUNT; i++)
    g_hash_table_add (unsorted_settings, &settings[i]);

  sorted_settings_good = g_list_append (sorted_settings_good, &settings[1]);
  sorted_settings_good = g_list_append (sorted_settings_good, &settings[0]);
  sorted_settings_good = g_list_append (sorted_settings_good, &settings[2]);

  sorted_settings_result = ms_tweaks_parser_sort_by_weight (unsorted_settings);

  g_assert_cmpint (g_list_length (sorted_settings_result), ==, SETTING_COUNT);
  g_assert_cmpint (g_list_length (sorted_settings_good), ==, SETTING_COUNT);

  for (int i = 0; i < SETTING_COUNT; i++)
    g_assert_true (g_list_nth_data (sorted_settings_result, i) == g_list_nth_data (sorted_settings_good, i));

  g_list_free (sorted_settings_good);
  g_list_free (sorted_settings_result);
  g_hash_table_destroy (unsorted_settings);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh-mobile-settings/test-tweaks-parser-sort-by-weight",
                   test_sort_settings_by_weight);

  return g_test_run ();
}
