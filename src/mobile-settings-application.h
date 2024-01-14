/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "ms-head-tracker.h"
#include "ms-toplevel-tracker.h"

#include <adwaita.h>

G_BEGIN_DECLS

#define MOBILE_SETTINGS_TYPE_APPLICATION (mobile_settings_application_get_type ())

G_DECLARE_FINAL_TYPE (MobileSettingsApplication, mobile_settings_application, MOBILE_SETTINGS, APPLICATION, AdwApplication)

MobileSettingsApplication *mobile_settings_application_new (gchar *application_id);
GtkWidget *mobile_settings_application_get_device_panel  (MobileSettingsApplication *self);
MsToplevelTracker *mobile_settings_application_get_toplevel_tracker (MobileSettingsApplication *self);
MsHeadTracker     *mobile_settings_application_get_head_tracker (MobileSettingsApplication *self);
GStrv mobile_settings_application_get_wayland_protocols (MobileSettingsApplication *self);
guint32 mobile_settings_application_get_wayland_protocol_version (MobileSettingsApplication *self,
                                                                  const char *protocol);

G_END_DECLS
