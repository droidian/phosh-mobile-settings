/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-main"

#include "ms-main.h"

#include "mobile-settings-config.h"
#include "mobile-settings-resources.h"
#include "mobile-settings-window.h"
#include "ms-alerts-panel.h"
#include "ms-compositor-panel.h"
#include "ms-convergence-panel.h"
#include "ms-features-panel.h"
#include "ms-feedback-panel.h"
#include "ms-topbar-panel.h"
#include "ms-overview-panel.h"
#include "ms-osk-panel.h"
#include "ms-lockscreen-panel.h"
#include "ms-sensor-panel.h"

#include <glib/gi18n.h>

static void
ms_init_types (void)
{
  g_type_ensure (MS_TYPE_ALERTS_PANEL);
  g_type_ensure (MS_TYPE_COMPOSITOR_PANEL);
  g_type_ensure (MS_TYPE_CONVERGENCE_PANEL);
  g_type_ensure (MS_TYPE_FEATURES_PANEL);
  g_type_ensure (MS_TYPE_FEEDBACK_PANEL);
  g_type_ensure (MS_TYPE_TOPBAR_PANEL);
  g_type_ensure (MS_TYPE_LOCKSCREEN_PANEL);
  g_type_ensure (MS_TYPE_OSK_PANEL);
  g_type_ensure (MS_TYPE_OVERVIEW_PANEL);
  g_type_ensure (MS_TYPE_SENSOR_PANEL);
}

/**
 * ms_init:
 *
 * Initialise the library. This ensures the available types and loads
 * the resources.
 */
void
ms_init (void)
{
  static gsize initialised = FALSE;

  if (g_once_init_enter (&initialised)) {
    textdomain (GETTEXT_PACKAGE);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);

    /*
     * libms is meant as static library so register resources explicitly,
     * otherwise they get dropped during static linking.
     */
    mobile_settings_register_resource ();

    ms_init_types ();
    g_once_init_leave (&initialised, TRUE);
  }
}

/**
 * ms_uninit:
 *
 * Free up resources.
 */
void
ms_uninit (void)
{
  mobile_settings_unregister_resource ();
}
