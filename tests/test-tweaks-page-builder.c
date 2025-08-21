/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "pmos-tweaks/ms-tweaks-page-builder.c"

#include "utils/ms-tweaks-backend-dummy.h"


typedef struct {
  MsTweaksSetting *setting;
  MsTweaksBackend *backend;
  GValue *         value;
} PageBuilderTestFixture;


#define DEBUG_SETTING_NAME "Debug"


static void
test_page_builder_fixture_setup (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  fixture->setting = g_new0 (MsTweaksSetting, 1);
  fixture->backend = ms_tweaks_backend_dummy_new (fixture->setting);
  fixture->value = g_new0 (GValue, 1);

  fixture->setting->name = DEBUG_SETTING_NAME;

  fixture->setting->min = 0;
  fixture->setting->max = 10;
  fixture->setting->step = 1;

  g_value_init (fixture->value, G_TYPE_STRING);
  g_value_set_string (fixture->value, "Put Your Money On Me");
}


static void
test_page_builder_fixture_setup_with_map (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  test_page_builder_fixture_setup (fixture, unused);

  fixture->setting->map = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (fixture->setting->map, "Option 0", "Value 0");
  g_hash_table_insert (fixture->setting->map, "Option 1", "Value 1");
  g_hash_table_insert (fixture->setting->map, "Option 2", "Value 2");
}


static void
test_page_builder_fixture_teardown (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  if (fixture->setting->map)
    g_hash_table_destroy (fixture->setting->map);
  g_free (fixture->setting);
  g_clear_object (&fixture->backend);
  g_value_unset (fixture->value);
  g_free (fixture->value);
}


#define PAGE_BUILDER_TEST_ADD(name, test_func) g_test_add ((name), \
                                                           PageBuilderTestFixture, \
                                                           NULL, \
                                                           test_page_builder_fixture_setup, \
                                                           (test_func), \
                                                           test_page_builder_fixture_teardown)

#define PAGE_BUILDER_TEST_ADD_WITH_MAP(name, test_func) g_test_add ((name), \
                                                                    PageBuilderTestFixture, \
                                                                    NULL, \
                                                                    test_page_builder_fixture_setup_with_map, \
                                                                    (test_func), \
                                                                    test_page_builder_fixture_teardown)


static void
test_setting_data_to_boolean_widget (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = setting_data_to_boolean_widget (fixture->setting, fixture->backend, NULL);

  g_assert_true (ADW_IS_SWITCH_ROW (widget));
  g_assert_false (adw_switch_row_get_active (ADW_SWITCH_ROW (widget)));

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_boolean_widget_with_value (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  GValue *value = g_new0 (GValue, 1);
  GtkWidget *widget_1 = NULL;
  GtkWidget *widget_2 = NULL;

  g_value_init (value, G_TYPE_BOOLEAN);

  g_value_set_boolean (value, TRUE);
  widget_1 = setting_data_to_boolean_widget (fixture->setting, fixture->backend, value);

  g_value_set_boolean (value, FALSE);
  widget_2 = setting_data_to_boolean_widget (fixture->setting, fixture->backend, value);

  g_assert_true (ADW_IS_SWITCH_ROW (widget_1));
  g_assert_true (ADW_IS_SWITCH_ROW (widget_2));
  g_assert_true (adw_switch_row_get_active (ADW_SWITCH_ROW (widget_1)));
  g_assert_false (adw_switch_row_get_active (ADW_SWITCH_ROW (widget_2)));

  g_object_ref_sink (widget_1);
  g_object_ref_sink (widget_2);
  g_object_unref (widget_1);
  g_object_unref (widget_2);
  g_free (value);
}


static void
test_setting_data_to_choice_widget (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  g_test_expect_message ("ms-tweaks-page-builder",
                         G_LOG_LEVEL_WARNING,
                         "[Setting \"" DEBUG_SETTING_NAME "\"] Choice widget with NULL map — either the datasource failed or the markup is wrong");

  g_assert_false (setting_data_to_choice_widget (fixture->setting, fixture->backend, NULL));
}


static void
test_setting_data_to_choice_widget_with_map (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = NULL;

  widget = setting_data_to_choice_widget (fixture->setting, fixture->backend, NULL);

  g_assert_true (ADW_IS_COMBO_ROW (widget));

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_choice_widget_with_map_and_value (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  GtkStringObject *string_object = NULL;
  GtkWidget *widget_1 = NULL;
  GtkWidget *widget_2 = NULL;

  widget_1 = setting_data_to_choice_widget (fixture->setting, fixture->backend, fixture->value);

  g_value_set_string (fixture->value, "Value 2");
  widget_2 = setting_data_to_choice_widget (fixture->setting, fixture->backend, fixture->value);

  g_assert_true (ADW_IS_COMBO_ROW (widget_1));
  g_assert_true (ADW_IS_COMBO_ROW (widget_2));

  string_object = adw_combo_row_get_selected_item (ADW_COMBO_ROW (widget_1));
  g_assert_cmpstr (gtk_string_object_get_string (string_object), ==, "Option 1");

  string_object = adw_combo_row_get_selected_item (ADW_COMBO_ROW (widget_2));
  g_assert_cmpstr (gtk_string_object_get_string (string_object), ==, "Option 2");

  g_object_ref_sink (widget_1);
  g_object_ref_sink (widget_2);
  g_object_unref (widget_1);
  g_object_unref (widget_2);
}


static void
test_setting_data_to_file_widget (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPageBuilderOpenFilePickerMetadata *metadata = g_new (MsTweaksPageBuilderOpenFilePickerMetadata, 1);
  GtkWidget *widget = setting_data_to_file_widget (fixture->setting,
                                                   fixture->backend,
                                                   NULL,
                                                   metadata);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
  g_object_ref_sink (metadata->file_picker_label);
  g_object_unref (metadata->file_picker_label);
  g_free (metadata);
}


static void
test_setting_data_to_file_widget_with_value (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPageBuilderOpenFilePickerMetadata *metadata = g_new (MsTweaksPageBuilderOpenFilePickerMetadata, 1);
  GtkWidget *widget = setting_data_to_file_widget (fixture->setting,
                                                   fixture->backend,
                                                   fixture->value,
                                                   metadata);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
  g_object_ref_sink (metadata->file_picker_label);
  g_object_unref (metadata->file_picker_label);
  g_free (metadata);
}


static void
test_setting_data_to_font_widget (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = setting_data_to_font_widget (fixture->setting, fixture->backend, NULL);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_font_widget_with_value (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = setting_data_to_font_widget (fixture->setting,
                                                   fixture->backend,
                                                   fixture->value);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_info_widget (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  g_test_expect_message ("ms-tweaks-page-builder",
                         G_LOG_LEVEL_WARNING,
                         "[Setting \"" DEBUG_SETTING_NAME "\"] widget_value was NULL in setting_data_to_boolean_widget ()");

  g_assert_false (setting_data_to_info_widget (fixture->setting, NULL));
}


static void
test_setting_data_to_info_widget_with_value (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = setting_data_to_info_widget (fixture->setting, fixture->value);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_number_widget (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = setting_data_to_number_widget (fixture->setting, fixture->backend, NULL);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_number_widget_with_value (PageBuilderTestFixture *fixture, gconstpointer unused)
{
  GValue value = G_VALUE_INIT;
  GtkWidget *widget = NULL;

  g_value_init (&value, G_TYPE_DOUBLE);
  g_value_set_double (&value, 5);
  widget = setting_data_to_number_widget (fixture->setting, fixture->backend, &value);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
  g_value_unset (&value);
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  PAGE_BUILDER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-boolean-widget",
                         test_setting_data_to_boolean_widget);
  PAGE_BUILDER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-boolean-widget-with-value",
                         test_setting_data_to_boolean_widget_with_value);
  PAGE_BUILDER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-choice-widget",
                         test_setting_data_to_choice_widget);
  PAGE_BUILDER_TEST_ADD_WITH_MAP ("/phosh-mobile-settings/test-tweaks-setting-data-to-choice-widget-with-map",
                                  test_setting_data_to_choice_widget_with_map);
  PAGE_BUILDER_TEST_ADD_WITH_MAP ("/phosh-mobile-settings/test-tweaks-setting-data-to-choice-widget-with-map-and-value",
                                  test_setting_data_to_choice_widget_with_map_and_value);
  PAGE_BUILDER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-file-widget",
                         test_setting_data_to_file_widget);
  PAGE_BUILDER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-file-widget-with-value",
                         test_setting_data_to_file_widget_with_value);
  PAGE_BUILDER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-font-widget",
                         test_setting_data_to_font_widget);
  PAGE_BUILDER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-font-widget-with-value",
                         test_setting_data_to_font_widget_with_value);
  PAGE_BUILDER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-info-widget",
                         test_setting_data_to_info_widget);
  PAGE_BUILDER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-info-widget-with-value",
                         test_setting_data_to_info_widget_with_value);
  PAGE_BUILDER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-number-widget",
                         test_setting_data_to_number_widget);
  PAGE_BUILDER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-number-widget-with-value",
                         test_setting_data_to_number_widget_with_value);

  return g_test_run ();
}
