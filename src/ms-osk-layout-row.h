/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ms-osk-layout.h"

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_OSK_LAYOUT_ROW (ms_osk_layout_row_get_type ())

G_DECLARE_FINAL_TYPE (MsOskLayoutRow, ms_osk_layout_row, MS, OSK_LAYOUT_ROW, AdwActionRow)

GtkWidget       *ms_osk_layout_row_new            (MsOskLayout *layout);
MsOskLayout     *ms_osk_layout_row_get_layout     (MsOskLayoutRow *self);

G_END_DECLS
