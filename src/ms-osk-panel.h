/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_OSK_PANEL (ms_osk_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsOskPanel, ms_osk_panel, MS, OSK_PANEL, AdwBin)

MsOskPanel *ms_osk_panel_new (void);

G_END_DECLS
