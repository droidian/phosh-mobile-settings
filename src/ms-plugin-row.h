/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_PLUGIN_ROW (ms_plugin_row_get_type ())

G_DECLARE_FINAL_TYPE (MsPluginRow, ms_plugin_row, MS, PLUGIN_ROW, AdwActionRow)

const char    *ms_plugin_row_get_name      (MsPluginRow *self);
gboolean       ms_plugin_row_get_enabled   (MsPluginRow *self);

G_END_DECLS
