/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define MS_TYPE_TOPLEVEL_TRACKER (ms_toplevel_tracker_get_type ())

G_DECLARE_FINAL_TYPE (MsToplevelTracker, ms_toplevel_tracker, MS, TOPLEVEL_TRACKER, GObject)

MsToplevelTracker *ms_toplevel_tracker_new (gpointer foreign_toplevel_manager);
GStrv              ms_toplevel_tracker_get_app_ids (MsToplevelTracker *self);

G_END_DECLS
