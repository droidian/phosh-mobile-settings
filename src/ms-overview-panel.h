/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ms-panel.h"

G_BEGIN_DECLS

#define MS_TYPE_OVERVIEW_PANEL (ms_overview_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsOverviewPanel, ms_overview_panel, MS, OVERVIEW_PANEL, MsPanel)

MsOverviewPanel *ms_overview_panel_new (void);

G_END_DECLS
