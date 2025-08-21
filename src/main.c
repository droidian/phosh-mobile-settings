/*
 * Copyright (C) 2022,2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include <glib/gi18n.h>

#include "mobile-settings-config.h"
#include "mobile-settings-application.h"
#include "ms-main.h"


int
main (int argc, char *argv[])
{
  g_autoptr (MobileSettingsApplication) app = NULL;
  g_autoptr (GError) err = NULL;
  int ret;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  ms_init ();

  app = mobile_settings_application_new (MOBILE_SETTINGS_APP_ID);
  ret = g_application_run (G_APPLICATION (app), argc, argv);

  ms_uninit ();

  return ret;
}
