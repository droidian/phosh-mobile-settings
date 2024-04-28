/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_TOPBAR_PANEL (ms_topbar_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsTopbarPanel, ms_topbar_panel, MS, TOPBAR_PANEL, AdwBin)

G_END_DECLS
