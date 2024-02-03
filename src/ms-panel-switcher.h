/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_PANEL_SWITCHER (ms_panel_switcher_get_type ())

G_DECLARE_FINAL_TYPE (MsPanelSwitcher, ms_panel_switcher, MS, PANEL_SWITCHER, AdwBin)

MsPanelSwitcher *ms_panel_switcher_new (void);
void             ms_panel_switcher_set_stack (MsPanelSwitcher *self, GtkStack *stack);
gboolean         ms_panel_switcher_set_active_panel_name (MsPanelSwitcher *self, const char *panel);

G_END_DECLS
