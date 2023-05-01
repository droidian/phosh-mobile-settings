/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>
#include <gsound.h>


G_BEGIN_DECLS

#define MS_TYPE_FEEDBACK_PANEL (ms_feedback_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsFeedbackPanel, ms_feedback_panel, MS, FEEDBACK_PANEL, AdwBin)

MsFeedbackPanel *ms_feedback_panel_new (void);
void             ms_feedback_panel_play_sound_file (MsFeedbackPanel *self, const char *file);

G_END_DECLS
