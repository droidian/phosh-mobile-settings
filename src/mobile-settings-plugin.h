/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "ms-plugin-panel.h"

#include <glib.h>
#include <glib-object.h>

/* Extension point names */
#define MS_EXTENSION_POINT_DEVICE_PANEL "ms-device-panel"

G_BEGIN_DECLS

gboolean ms_plugin_check_device_support (const char * const *supported);

G_END_DECLS
