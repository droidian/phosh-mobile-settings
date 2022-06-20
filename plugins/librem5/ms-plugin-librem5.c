/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-plugin-librem5"

#include "mobile-settings-plugin.h"

#include "ms-plugin-librem5-panel.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

char **g_io_ms_plugin_librem5_query (void);


void
g_io_module_load (GIOModule *module)
{
  const char * const supported[] = { "purism,librem5", NULL };

  g_type_module_use (G_TYPE_MODULE (module));

  g_type_ensure (GTK_TYPE_CALENDAR);

  if (ms_plugin_check_device_support (supported) == FALSE)
    return;

  g_io_extension_point_implement (MS_EXTENSION_POINT_DEVICE_PANEL,
                                  MS_TYPE_PLUGIN_LIBREM5_PANEL,
                                  "device-panel-librem5",
                                  10);
}

void
g_io_module_unload (GIOModule *module)
{
}

char **
g_io_ms_plugin_librem5_query (void)
{
  char *extension_points[] = {MS_EXTENSION_POINT_DEVICE_PANEL, NULL};

  return g_strdupv (extension_points);
}
