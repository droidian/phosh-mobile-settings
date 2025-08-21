/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "pmos-tweaks/ms-tweaks-mappings.h"


typedef struct {
  GValue          *value;
  MsTweaksSetting *setting;
} MappingTestFixture;


static void
test_mappings_fixture_setup (MappingTestFixture *fixture, gconstpointer unused)
{
  fixture->value = g_new0 (GValue, 1);
  fixture->setting = g_new0 (MsTweaksSetting, 1);
}


static void
test_mappings_fixture_setup_with_map (MappingTestFixture *fixture, gconstpointer unused)
{
  test_mappings_fixture_setup (fixture, unused);
  fixture->setting->map = g_hash_table_new (g_str_hash, g_str_equal);
}


#define BOOLEAN_TRUE_STRING "straight fax"
#define BOOLEAN_FALSE_STRING "that's cap"


static void
test_mappings_fixture_setup_boolean (MappingTestFixture *fixture, gconstpointer unused)
{
  test_mappings_fixture_setup_with_map (fixture, unused);

  fixture->setting->type = MS_TWEAKS_TYPE_BOOLEAN;

  /* Valid string -> boolean mappings. */
  g_hash_table_insert (fixture->setting->map, "true", BOOLEAN_TRUE_STRING);
  g_hash_table_insert (fixture->setting->map, "false", BOOLEAN_FALSE_STRING);

  /* Invalid string -> boolean mappings. */
  g_hash_table_insert (fixture->setting->map, "what", "what the heck");
  g_hash_table_insert (fixture->setting->map, "sorry", "it is okay");
}


#define CHOICE_KEY_STRING_1 "a key"
#define CHOICE_KEY_STRING_2 "yet another"
#define CHOICE_VALUE_STRING_1 "some value"
#define CHOICE_VALUE_STRING_2 "anyhow, another one"


static void
test_mappings_fixture_setup_choice (MappingTestFixture *fixture, gconstpointer unused)
{
  test_mappings_fixture_setup_with_map (fixture, unused);

  fixture->setting->type = MS_TWEAKS_TYPE_CHOICE;

  g_hash_table_insert (fixture->setting->map, CHOICE_KEY_STRING_1, CHOICE_VALUE_STRING_1);
  g_hash_table_insert (fixture->setting->map, CHOICE_KEY_STRING_2, CHOICE_VALUE_STRING_2);
}


static void
test_mappings_fixture_setup_number (MappingTestFixture *fixture, gconstpointer unused)
{
  test_mappings_fixture_setup (fixture, unused);

  fixture->setting->type = MS_TWEAKS_TYPE_NUMBER;
}


static void
test_mappings_fixture_setup_gtype_boolean (MappingTestFixture *fixture, gconstpointer unused)
{
  test_mappings_fixture_setup_with_map (fixture, unused);

  /* Test GType handling. */
  fixture->setting->backend = MS_TWEAKS_BACKEND_IDENTIFIER_GSETTINGS;

  /* Test boolean GType handling. */
  fixture->setting->gtype = MS_TWEAKS_GTYPE_BOOLEAN;

  /* Valid string -> boolean mappings. */
  g_hash_table_insert (fixture->setting->map, "tea_is_nice", "true");
  g_hash_table_insert (fixture->setting->map, "pollution_is_nice", "false");

  /* Invalid string -> boolean mappings. */
  g_hash_table_insert (fixture->setting->map, "towels", "what the heck");
  g_hash_table_insert (fixture->setting->map, "sorry", "it is okay");
}


static void
test_mappings_fixture_teardown (MappingTestFixture *fixture, gconstpointer unused)
{
  if (fixture->setting->map)
    g_hash_table_destroy (fixture->setting->map);

  g_value_unset (fixture->value);
  g_free (fixture->value);
  g_free (fixture->setting);
}


#define MAPPING_TEST_ADD(name, setup_func, test_func) g_test_add ((name), \
                                                                  MappingTestFixture, \
                                                                  NULL, \
                                                                  (setup_func), \
                                                                  (test_func), \
                                                                  test_mappings_fixture_teardown)

/**
 * test_mappings_handle_get_boolean_type_choice:
 * Ensures that mappings aren't applied for "choice" type widgets.
 */
static void
test_mappings_handle_get_boolean_type_choice (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, CHOICE_VALUE_STRING_1);
  ms_tweaks_mappings_handle_get (value, setting);
  g_assert_cmpstr (g_value_get_string (value), ==, CHOICE_VALUE_STRING_1);
}


static void
test_mappings_handle_set_boolean_type_choice (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, CHOICE_KEY_STRING_2);
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_cmpstr (g_value_get_string (value), ==, CHOICE_KEY_STRING_2);
}


#define TEST_FLOAT_DOUBLE_REPR 34.4
#define TEST_FLOAT_STRING_REPR "34.400000"
#define TEST_FLOAT_STRING_REPR_SHORT "34.4"


static void
test_mappings_handle_get_type_number (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, TEST_FLOAT_STRING_REPR);
  ms_tweaks_mappings_handle_get (value, setting);
  g_assert_cmpfloat_with_epsilon (g_value_get_double (value), TEST_FLOAT_DOUBLE_REPR, FLT_TRUE_MIN);

  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, TEST_FLOAT_STRING_REPR_SHORT);
  ms_tweaks_mappings_handle_get (value, setting);
  g_assert_cmpfloat_with_epsilon (g_value_get_double (value), TEST_FLOAT_DOUBLE_REPR, FLT_TRUE_MIN);
}


static void
test_mappings_handle_set_type_number (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, TEST_FLOAT_DOUBLE_REPR);
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_cmpstr (TEST_FLOAT_STRING_REPR, ==, g_value_get_string (value));
}


static void
test_mappings_handle_get_boolean (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  /* Test valid mappings. */
  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, BOOLEAN_TRUE_STRING);
  ms_tweaks_mappings_handle_get (value, setting);
  g_assert_true (g_value_get_boolean (value));

  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, BOOLEAN_FALSE_STRING);
  ms_tweaks_mappings_handle_get (value, setting);
  g_assert_false (g_value_get_boolean (value));

  /* Test invalid mappings. These always result in the value being set to false and no error being
   * logged. Might be worth logging some error and testing that it happens in the future. */
  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "what the heck");
  ms_tweaks_mappings_handle_get (value, setting);
  g_assert_false (g_value_get_boolean (value));

  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "it is okay");
  ms_tweaks_mappings_handle_get (value, setting);
  g_assert_false (g_value_get_boolean (value));
}

/**
 * test_mappings_handle_set_gtype_boolean:
 * `gtype` affects the value rather than the key (compared to `type`), so we should expect to input
 * anything and get a boolean back.
 */
static void
test_mappings_handle_set_gtype_boolean (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  /* Test valid mappings. */
  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "tea_is_nice");
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_true (g_value_get_boolean (value));

  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "pollution_is_nice");
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_false (g_value_get_boolean (value));

  g_value_unset (value);

  /* Test invalid mappings. These always result in the value being set to false and no error being
   * logged. Might be worth logging some error and testing that it happens in the future. */
  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "towels");
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_false (g_value_get_boolean (value));

  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "sorry");
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_false (g_value_get_boolean (value));
}


#define TEST_STRING "Stadsbana"


static void
test_mappings_handle_get_null_map (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, TEST_STRING);
  ms_tweaks_mappings_handle_get (value, setting);
  g_assert_cmpstr (g_value_get_string (value), ==, TEST_STRING);
}


static void
test_mappings_handle_set_null_map (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, TEST_STRING);
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_cmpstr (g_value_get_string (value), ==, TEST_STRING);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-get-boolean-type-choice",
                    test_mappings_fixture_setup_choice,
                    test_mappings_handle_get_boolean_type_choice);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-set-boolean-type-choice",
                    test_mappings_fixture_setup_choice,
                    test_mappings_handle_set_boolean_type_choice);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-get-type-number",
                    test_mappings_fixture_setup_number,
                    test_mappings_handle_get_type_number);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-set-type-number",
                    test_mappings_fixture_setup_number,
                    test_mappings_handle_set_type_number);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-get-boolean",
                    test_mappings_fixture_setup_boolean,
                    test_mappings_handle_get_boolean);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-set-gtype-boolean",
                    test_mappings_fixture_setup_gtype_boolean,
                    test_mappings_handle_set_gtype_boolean);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-get-null-map",
                    test_mappings_fixture_setup,
                    test_mappings_handle_get_null_map);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-set-null-map",
                    test_mappings_fixture_setup,
                    test_mappings_handle_set_null_map);

  return g_test_run ();
}
