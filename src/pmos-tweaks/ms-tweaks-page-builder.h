/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#pragma once

#include "ms-tweaks-parser.h"

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_TWEAKS_PREFERENCES_PAGE ms_tweaks_preferences_page_get_type ()
G_DECLARE_FINAL_TYPE (MsTweaksPreferencesPage, ms_tweaks_preferences_page, MS, TWEAKS_PREFERENCES_PAGE, AdwPreferencesPage)

MsTweaksPreferencesPage *ms_tweaks_preferences_page_new (const MsTweaksPage *data);

G_END_DECLS
