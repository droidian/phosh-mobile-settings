/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "test-tweaks-utils"

#include "pmos-tweaks/ms-tweaks-utils.h"


#define TEST_WARNING_MESSAGE "scary warning: "
#define TEST_WARNING_MESSAGE_FMT TEST_WARNING_MESSAGE "%s"
#define TEST_WARNING_STRING "boooooo!!"
#define TEST_SETTING_NAME "Pageus"
#define TEST_FULL_WARNING_MESSAGE "[Setting \"" TEST_SETTING_NAME "\"] " TEST_WARNING_MESSAGE \
                                  TEST_WARNING_STRING


static void
test_tweaks_log (void)
{
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, TEST_FULL_WARNING_MESSAGE);
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, TEST_FULL_WARNING_MESSAGE);
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, TEST_FULL_WARNING_MESSAGE);
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, TEST_FULL_WARNING_MESSAGE);
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, TEST_FULL_WARNING_MESSAGE);

  ms_tweaks_message (TEST_SETTING_NAME, TEST_WARNING_MESSAGE_FMT, TEST_WARNING_STRING);
  ms_tweaks_critical (TEST_SETTING_NAME, TEST_WARNING_MESSAGE_FMT, TEST_WARNING_STRING);
  ms_tweaks_warning (TEST_SETTING_NAME, TEST_WARNING_MESSAGE_FMT, TEST_WARNING_STRING);
  ms_tweaks_info (TEST_SETTING_NAME, TEST_WARNING_MESSAGE_FMT, TEST_WARNING_STRING);
  ms_tweaks_debug (TEST_SETTING_NAME, TEST_WARNING_MESSAGE_FMT, TEST_WARNING_STRING);

  g_test_assert_expected_messages ();
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh-mobile-settings/test-tweaks-utils-log", test_tweaks_log);

  return g_test_run ();
}
