/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_FEEDBACK_PANEL (ms_feedback_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsFeedbackPanel, ms_feedback_panel, MS, FEEDBACK_PANEL, AdwBin)

MsFeedbackPanel *ms_feedback_panel_new (void);


G_END_DECLS
