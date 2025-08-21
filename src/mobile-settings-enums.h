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

/**
 * MsPhoshNotificationUrgency:
 * @MS_PHOSH_NOTIFICATION_NONE: disables all wakeup based on notifications urgency
 * @MS_PHOSH_NOTIFICATION_URGENCY_LOW: low urgency
 * @MS_PHOSH_NOTIFICATION_URGENCY_NORMAL: normal urgency
 * @MS_PHOSH_NOTIFICATION_URGENCY_CRITICAL: critical urgency
 */

typedef enum {
  MS_PHOSH_NOTIFICATION_NONE = -1,
  MS_PHOSH_NOTIFICATION_URGENCY_LOW = 0,
  MS_PHOSH_NOTIFICATION_URGENCY_NORMAL,
  MS_PHOSH_NOTIFICATION_URGENCY_CRITICAL,
} MsPhoshNotificationUrgency;


/* TODO: Use headers from libcbd once that is released or usable as subproject */
typedef enum {
  MS_CHANNEL_MODE_NONE  = 0,
  MS_CHANNEL_MODE_COUNTRY  = 1,
} MsChannelMode;

typedef enum {
  MS_CHANNEL_LEVEL_UNKNOWN        = 0,
  MS_CHANNEL_LEVEL_PRESIDENTIAL   = (1 << 0),
  MS_CHANNEL_LEVEL_EXTREME        = (1 << 1),
  MS_CHANNEL_LEVEL_SEVERE         = (1 << 2),
  MS_CHANNEL_LEVEL_PUBLIC_SAFETY  = (1 << 3),
  MS_CHANNEL_LEVEL_AMBER          = (1 << 4),
  MS_CHANNEL_LEVEL_TEST           = (1 << 5),
} MsChannelLevel;

G_END_DECLS
