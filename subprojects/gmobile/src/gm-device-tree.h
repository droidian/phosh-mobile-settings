/*
 * Copyright (C) 2022 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_GMOBILE_INSIDE) && !defined(GMOBILE_COMPILATION)
#error "Only <gmobile.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

GStrv       gm_device_tree_get_compatibles (const char *sysfs_root, GError **err);

G_END_DECLS
