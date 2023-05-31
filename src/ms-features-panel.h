/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_FEATURES_PANEL (ms_features_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsFeaturesPanel, ms_features_panel, MS, FEATURES_PANEL, AdwBin)

MsFeaturesPanel *ms_features_panel_new (void);

G_END_DECLS
