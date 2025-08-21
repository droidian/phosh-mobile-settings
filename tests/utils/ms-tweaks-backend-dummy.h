/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#pragma once

#include "pmos-tweaks/ms-tweaks-backend-interface.h"

G_BEGIN_DECLS

#define MS_TYPE_TWEAKS_BACKEND_DUMMY ms_tweaks_backend_dummy_get_type ()
G_DECLARE_FINAL_TYPE (MsTweaksBackendDummy, ms_tweaks_backend_dummy, MS, TWEAKS_BACKEND_DUMMY, GObject)

MsTweaksBackend *ms_tweaks_backend_dummy_new (const MsTweaksSetting *setting_data);

G_END_DECLS
