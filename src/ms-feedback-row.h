/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_FEEDBACK_ROW (ms_feedback_row_get_type ())

G_DECLARE_FINAL_TYPE (MsFeedbackRow, ms_feedback_row, MS, FEEDBACK_ROW, AdwComboRow)

MsFeedbackRow *ms_feedback_row_new (void);

G_END_DECLS
