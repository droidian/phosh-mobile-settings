/*
 * Copyright (C) 2025 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_OSK_ADD_SHORTCUT_DIALOG (ms_osk_add_shortcut_dialog_get_type ())

G_DECLARE_FINAL_TYPE (MsOskAddShortcutDialog, ms_osk_add_shortcut_dialog,
                      MS, OSK_ADD_SHORTCUT_DIALOG, AdwDialog)

GtkWidget *ms_osk_add_shortcut_dialog_new (void);

G_END_DECLS
