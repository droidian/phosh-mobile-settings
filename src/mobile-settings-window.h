/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MOBILE_SETTINGS_TYPE_WINDOW (mobile_settings_window_get_type ())

G_DECLARE_FINAL_TYPE (MobileSettingsWindow, mobile_settings_window, MOBILE_SETTINGS, WINDOW, AdwApplicationWindow)

GtkSelectionModel *mobile_settings_window_get_stack_pages (MobileSettingsWindow *self);

G_END_DECLS
