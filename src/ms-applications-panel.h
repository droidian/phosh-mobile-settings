/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_APPLICATIONS_PANEL (ms_applications_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsApplicationsPanel, ms_applications_panel, MS, APPLICATIONS_PANEL, AdwBin)

MsApplicationsPanel *ms_applications_panel_new (void);

G_END_DECLS
