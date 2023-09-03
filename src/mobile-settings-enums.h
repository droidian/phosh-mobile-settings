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

/**
 * MsPhoshNotifyScreenWakeupFlags:
 * @MS_PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_NONE: No wakeup
 * @MS_PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_URGENCY: Wakeup screen based on notification urgency
 * @MS_PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_CATEGORY: Wakeup screen based on notification category
 * @MS_PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_ANY: Wakeup screen on any notification
 *
 * What notification properties trigger screen wakeup
 */
typedef enum {
  MS_PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_NONE     = 0, /*< skip >*/
  MS_PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_ANY      = (1 << 0),
  MS_PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_URGENCY  = (1 << 1),
  MS_PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_CATEGORY = (1 << 2),
} MsPhoshNotifyScreenWakeupFlags;

G_END_DECLS
