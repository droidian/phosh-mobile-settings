/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_PANEL (ms_panel_get_type ())

G_DECLARE_DERIVABLE_TYPE (MsPanel, ms_panel, MS, PANEL, AdwBin)

struct _MsPanelClass {
  AdwBinClass parent_class;
};

GtkStringList *ms_panel_get_keywords (MsPanel *self);
void           ms_panel_set_keywords (MsPanel *self, GtkStringList *keywords);

G_END_DECLS
