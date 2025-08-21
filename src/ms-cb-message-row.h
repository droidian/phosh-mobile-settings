/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once


#include <adwaita.h>
#include <gtk/gtk.h>

#define LIBCELLBROADCAST_USE_UNSTABLE_API
#include <libcellbroadcast.h>

G_BEGIN_DECLS

#define MS_TYPE_CB_MESSAGE_ROW (ms_cb_message_row_get_type ())

G_DECLARE_FINAL_TYPE (MsCbMessageRow, ms_cb_message_row, MS, CB_MESSAGE_ROW,
                      AdwExpanderRow)

MsCbMessageRow      *ms_cb_message_row_new                 (LcbMessage     *message);
LcbMessage          *ms_cb_message_row_get_message         (MsCbMessageRow *self);

G_END_DECLS
