/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_CONVERGENCE_PANEL (ms_convergence_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsConvergencePanel, ms_convergence_panel, MS, CONVERGENCE_PANEL, AdwBin)

MsConvergencePanel *ms_convergence_panel_new (void);

G_END_DECLS
