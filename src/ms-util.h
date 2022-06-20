/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include "mobile-settings-enums.h"

#include <glib.h>
#include <gio/gdesktopappinfo.h>

G_BEGIN_DECLS

#define STR_IS_NULL_OR_EMPTY(x) ((x) == NULL || (x)[0] == '\0')

gchar            *ms_munge_app_id (const gchar *app_id);
GDesktopAppInfo  *ms_get_desktop_app_info_for_app_id (const char *app_id);
MsFeedbackProfile ms_feedback_profile_from_name (const char *name);
char             *ms_feedback_profile_to_name (MsFeedbackProfile profile);

G_END_DECLS
