/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_SOUND_ROW (ms_sound_row_get_type ())

G_DECLARE_FINAL_TYPE (MsSoundRow, ms_sound_row, MS, SOUND_ROW, AdwActionRow)

MsSoundRow *ms_sound_row_new (void);
void        ms_sound_row_set_filename (MsSoundRow *self, const char *filename);

G_END_DECLS
