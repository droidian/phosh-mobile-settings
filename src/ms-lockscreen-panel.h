/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_LOCKSCREEN_PANEL (ms_lockscreen_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsLockscreenPanel, ms_lockscreen_panel, MS, LOCKSCREEN_PANEL, AdwBin)

MsLockscreenPanel *ms_lockscreen_panel_new (void);

G_END_DECLS
