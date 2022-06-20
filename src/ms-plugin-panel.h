/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "ms-plugin-panel.h"

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_PLUGIN_PANEL (ms_plugin_panel_get_type ())

G_DECLARE_DERIVABLE_TYPE (MsPluginPanel, ms_plugin_panel, MS, PLUGIN_PANEL, AdwBin)


struct _MsPluginPanelClass {
  AdwBinClass parent_class;
};


MsPluginPanel *ms_plugin_panel_new (const char *title);
const char    *ms_plugin_panel_get_title (MsPluginPanel *self);

G_END_DECLS
