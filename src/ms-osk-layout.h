/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define MS_TYPE_OSK_LAYOUT (ms_osk_layout_get_type ())

G_DECLARE_FINAL_TYPE (MsOskLayout, ms_osk_layout, MS, OSK_LAYOUT, GObject)

MsOskLayout            *ms_osk_layout_new      (const char *name,
                                                const char *type,
                                                const char *id);
const char             *ms_osk_layout_get_id    (MsOskLayout *self);
const char             *ms_osk_layout_get_type_ (MsOskLayout *self);
const char             *ms_osk_layout_get_name  (MsOskLayout *self);

G_END_DECLS
