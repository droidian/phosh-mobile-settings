/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#pragma once

#include "ms-tweaks-parser.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define MS_TYPE_TWEAKS_BACKEND ms_tweaks_backend_get_type ()
G_DECLARE_INTERFACE (MsTweaksBackend, ms_tweaks_backend, MS, TWEAKS_BACKEND, GObject)

/**
 * MsTweaksBackendInterface:
 * @parent_iface: The parent interface.
 * @get_value: Get the "value" of the backend. This does not necessarily correspond to any
 *             particular property but rather some value derived from the properties given to the
 *             backend in its constructor.
 * @set_value: Same as `get_value ()`, except it sets the value based on the same principles.
 * @get_setting_data: Should return the instance of `MsTweaksSetting` that was provided in the
 *                    backend's constructor.
 * @get_key: Should return the `key` property of the backend in string format as opposed to the
 *           `GPtrArray` format it originally is in. Generally, this would be achieved by the
 *           backend picking one key that it is going to use out of the ones specified. However, the
 *           string representation may also include other transformations, such as expanding tildes
 *           into full home directory paths.
 * @get_name: Should return the name from the `MsTweaksSetting` instance that was provided in the
 *            constructor.
 * @set_source_ext: Should set the `source_ext` property.
 *
 * All virtual functions but `set_source_ext ()` need to be implemented by backends. Additionally,
 * backends should generally follow these principles:
 *
 * - Only duplicate properties from the setting data if you want to have it mutable or change the
 *   value somehow, e.g. turning `key` into a `char *` instead of a `GPtrArray`.
 * - Return a MsTweaksBackend from the new () function, not the child type.
 */
struct _MsTweaksBackendInterface
{
  GTypeInterface parent_iface;

  GValue *                (* get_value) (MsTweaksBackend *self);
  void                    (* set_value) (MsTweaksBackend *self, GValue *value);

  const MsTweaksSetting * (* get_setting_data) (MsTweaksBackend *self);

  char *                  (* get_key) (MsTweaksBackend *self);
  char *                  (* get_name) (MsTweaksBackend *self);

  void                    (* set_source_ext) (MsTweaksBackend *self, gboolean new_value);
};

G_END_DECLS
