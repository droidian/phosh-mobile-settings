/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_OSK_ADD_LAYOUT_DIALOG (ms_osk_add_layout_dialog_get_type ())

G_DECLARE_FINAL_TYPE (MsOskAddLayoutDialog, ms_osk_add_layout_dialog, MS, OSK_ADD_LAYOUT_DIALOG,
                      AdwDialog)

GtkWidget *ms_osk_add_layout_dialog_new (GListModel *layouts);

G_END_DECLS
