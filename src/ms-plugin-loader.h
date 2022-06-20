/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include <gtk/gtk.h>
#include "mobile-settings-plugin.h"

G_BEGIN_DECLS

#define MS_TYPE_PLUGIN_LOADER ms_plugin_loader_get_type ()

G_DECLARE_FINAL_TYPE (MsPluginLoader, ms_plugin_loader, MS, PLUGIN_LOADER, GObject)

MsPluginLoader *ms_plugin_loader_new (const char * const * plugin_dirs, const char *extension_point);
GtkWidget *ms_plugin_loader_load_plugin (MsPluginLoader *self);

G_END_DECLS
