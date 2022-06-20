/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <mobile-settings-plugin.h>

G_BEGIN_DECLS

#define MS_TYPE_PLUGIN_LIBREM5_PANEL (ms_plugin_librem5_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsPluginLibrem5Panel, ms_plugin_librem5_panel, MS, PLUGIN_LIBREM5_PANEL, MsPluginPanel)

MsPluginLibrem5Panel *ms_plugin_librem5_panel_new (void);

G_END_DECLS
