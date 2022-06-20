/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_COMPOSITOR_PANEL (ms_compositor_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsCompositorPanel, ms_compositor_panel, MS, COMPOSITOR_PANEL, AdwBin)

MsCompositorPanel *ms_compositor_panel_new (void);

G_END_DECLS
