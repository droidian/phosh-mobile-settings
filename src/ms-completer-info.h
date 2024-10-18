/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define MS_TYPE_COMPLETER_INFO (ms_completer_info_get_type ())

G_DECLARE_FINAL_TYPE (MsCompleterInfo, ms_completer_info, MS, COMPLETER_INFO, GObject)

MsCompleterInfo *ms_completer_info_new (void);
const char      *ms_completer_info_get_id (MsCompleterInfo *self);
const char      *ms_completer_info_get_name (MsCompleterInfo *self);
const char      *ms_completer_info_get_description (MsCompleterInfo *self);
const char      *ms_completer_info_get_comment (MsCompleterInfo *self);

G_END_DECLS
