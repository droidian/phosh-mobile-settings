/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "ms-tweaks-backend-interface.h"

G_DEFINE_INTERFACE (MsTweaksBackend, ms_tweaks_backend, G_TYPE_OBJECT)


static void
ms_tweaks_backend_default_init (MsTweaksBackendInterface *iface)
{}
