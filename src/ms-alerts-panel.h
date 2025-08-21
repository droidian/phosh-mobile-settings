/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ms-panel.h"

G_BEGIN_DECLS

#define MS_TYPE_ALERTS_PANEL (ms_alerts_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsAlertsPanel, ms_alerts_panel, MS, ALERTS_PANEL, MsPanel)

MsAlertsPanel *ms_alerts_panel_new (void);

G_END_DECLS
