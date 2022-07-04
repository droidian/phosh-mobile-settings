/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define MS_TYPE_HEAD_TRACKER (ms_head_tracker_get_type ())

G_DECLARE_FINAL_TYPE (MsHeadTracker, ms_head_tracker, MS, HEAD_TRACKER, GObject)

typedef struct {
  gatomicrefcount      ref_count;
  char                *name;
  char                *make;
  char                *model;
  char                *serial_number;
  struct zwlr_output_head_v1 *head;
  MsHeadTracker *tracker;
} MsHead;

GType ms_head_get_type (void) G_GNUC_CONST;
#define MS_TYPE_HEAD (ms_head_get_type ())

MsHead *ms_head_ref (MsHead *self);
void    ms_head_unref (MsHead *self);

MsHeadTracker *ms_head_tracker_new (gpointer foreign_head_manager);
GPtrArray     *ms_head_tracker_get_heads (MsHeadTracker *self);

G_END_DECLS
