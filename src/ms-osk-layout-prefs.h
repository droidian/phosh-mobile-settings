/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_OSK_LAYOUT_PREFS (ms_osk_layout_prefs_get_type ())

G_DECLARE_FINAL_TYPE (MsOskLayoutPrefs, ms_osk_layout_prefs, MS, OSK_LAYOUT_PREFS,
                      AdwPreferencesGroup)

MsOskLayoutPrefs *ms_osk_layout_prefs_new (void);
void              ms_osk_layout_prefs_load_osk_layouts (MsOskLayoutPrefs *self);

G_END_DECLS
