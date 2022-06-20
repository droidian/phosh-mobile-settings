/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
  MS_FEEDBACK_PROFILE_FULL   = 0,
  MS_FEEDBACK_PROFILE_QUIET  = 1,
  MS_FEEDBACK_PROFILE_SILENT = 2,
} MsFeedbackProfile;

G_END_DECLS
