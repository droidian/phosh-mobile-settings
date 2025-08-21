/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-backend-dummy"

#include "ms-tweaks-backend-dummy.h"


struct _MsTweaksBackendDummy {
  MsTweaksBackendInterface parent_interface;
};


static void
ms_tweaks_backend_dummy_interface_init (MsTweaksBackendInterface *iface)
{
}


G_DEFINE_TYPE_WITH_CODE (MsTweaksBackendDummy, ms_tweaks_backend_dummy, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MS_TYPE_TWEAKS_BACKEND,
                                                ms_tweaks_backend_dummy_interface_init))

static void
ms_tweaks_backend_dummy_init (MsTweaksBackendDummy *self)
{
}


static void
ms_tweaks_backend_dummy_class_init (MsTweaksBackendDummyClass *klass)
{
}


MsTweaksBackend *
ms_tweaks_backend_dummy_new (const MsTweaksSetting *setting_data)
{
  MsTweaksBackendDummy *self = g_object_new (MS_TYPE_TWEAKS_BACKEND_DUMMY, NULL);

  return MS_TWEAKS_BACKEND (self);
}
