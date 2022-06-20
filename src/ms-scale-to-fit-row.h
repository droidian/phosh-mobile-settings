/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_SCALE_TO_FIT_ROW (ms_scale_to_fit_row_get_type ())

G_DECLARE_FINAL_TYPE (MsScaleToFitRow, ms_scale_to_fit_row, MS, SCALE_TO_FIT_ROW, AdwComboRow)

MsScaleToFitRow *ms_scale_to_fit_row_new (const char *app_id);
const char      *ms_scale_to_fit_row_get_app_id (MsScaleToFitRow *self);

G_END_DECLS
