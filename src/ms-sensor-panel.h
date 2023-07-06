/*
 * Copyright (C) 2023 Evangelos Ribeiro Tzaras <devrtz@fortysixandtwo.eu>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_SENSOR_PANEL (ms_sensor_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsSensorPanel, ms_sensor_panel, MS, SENSOR_PANEL, AdwBin)

MsSensorPanel *ms_sensor_panel_new (void);

G_END_DECLS
