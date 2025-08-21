/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-mappings"

#include "ms-tweaks-mappings.h"

#include "ms-tweaks-utils.h"

#include <inttypes.h>
#include <stdint.h>

static gboolean
ms_tweaks_mappings_string_to_boolean (const char *from)
{
  return ms_tweaks_util_string_to_boolean (from);
}


static char *
ms_tweaks_mappings_boolean_to_string (const gboolean from)
{
  /* Copy the string for consistency with other functions in this file that allocate one. */
  return g_strdup (ms_tweaks_util_boolean_to_string (from));
}


static double
ms_tweaks_mappings_string_to_double (const char *from)
{
  return g_strtod (from, NULL);
}


static char *
ms_tweaks_mappings_double_to_string (const double from)
{
  return g_strdup_printf ("%lf", from);
}


static char *
ms_tweaks_mappings_float_to_string (const float from)
{
  return g_strdup_printf ("%f", from);
}


static intmax_t
ms_tweaks_mappings_string_to_int (const char *from)
{
  return strtoimax (from, NULL, 10);
}


static char *
ms_tweaks_mappings_int_to_string (const gint from)
{
  return g_strdup_printf ("%d", from);
}


static uintmax_t
ms_tweaks_mappings_string_to_uint (const char *from)
{
  return strtoumax (from, NULL, 10);
}


static char *
ms_tweaks_mappings_uint_to_string (const guint from)
{
  return g_strdup_printf ("%d", from);
}


static char *
stringify_gvalue (GValue *value)
{
  GType value_gtype = G_VALUE_TYPE (value);
  char *normalised = NULL;

  switch (value_gtype) {
  case G_TYPE_BOOLEAN:
    normalised = ms_tweaks_mappings_boolean_to_string (g_value_get_boolean (value));
    break;
  case G_TYPE_DOUBLE:
    normalised = ms_tweaks_mappings_double_to_string (g_value_get_double (value));
    break;
  case G_TYPE_FLAGS:
    normalised = ms_tweaks_mappings_uint_to_string (g_value_get_flags (value));
    break;
  case G_TYPE_FLOAT:
    normalised = ms_tweaks_mappings_float_to_string (g_value_get_float (value));
    break;
  case G_TYPE_INT:
    normalised = ms_tweaks_mappings_int_to_string (g_value_get_int (value));
    break;
  case G_TYPE_STRING:
    normalised = g_strdup (g_value_get_string (value));
    break;
  case G_TYPE_UINT:
    normalised = ms_tweaks_mappings_uint_to_string (g_value_get_uint (value));
    break;
  default:
    g_warning_once ("Unsupported GType type: %s", g_type_name (value_gtype));
    break;
  }

  return normalised;
}

/**
 * ms_tweaks_mappings_handle_get:
 * @value: Value to reverse-map and convert.
 * @setting_data: Data to use for mapping and type information.
 *
 * Takes a GValue initialised with some data from a backend, converts it to a string, reverse-maps
 * it based on `setting_data->map` if it is set, and then converts it to the type appropriate for
 * its `setting_data->type` value.
 */
void
ms_tweaks_mappings_handle_get (GValue *value, const MsTweaksSetting *setting_data)
{
  g_autofree char *normalised = stringify_gvalue (value);
  char *mapped = normalised;

  /* setting_data->map has a different purpose in choice widgets than other ones, so don't use it
   * for this. */
  if (setting_data->map && setting_data->type != MS_TWEAKS_TYPE_CHOICE)
    mapped = ms_tweaks_util_get_key_by_value_string (setting_data->map, normalised);

  if (!mapped) {
    ms_tweaks_warning (setting_data->name, "Error when handling mappings");
    return;
  }

  g_value_unset (value);

  switch (setting_data->type) {
  case MS_TWEAKS_TYPE_BOOLEAN:
    g_value_init (value, G_TYPE_BOOLEAN);
    g_value_set_boolean (value, ms_tweaks_mappings_string_to_boolean (mapped));
    break;
  case MS_TWEAKS_TYPE_NUMBER:
    g_value_init (value, G_TYPE_DOUBLE);
    g_value_set_double (value, ms_tweaks_mappings_string_to_double (mapped));
    break;
  case MS_TWEAKS_TYPE_INFO:
  case MS_TWEAKS_TYPE_COLOR:
  case MS_TWEAKS_TYPE_CHOICE:
  case MS_TWEAKS_TYPE_FILE:
  case MS_TWEAKS_TYPE_FONT:
  case MS_TWEAKS_TYPE_UNKNOWN:
  default:
    g_value_init (value, G_TYPE_STRING);
    g_value_set_string (value, mapped);
    break;
  }
}

/**
 * ms_tweaks_mappings_handle_set:
 * @value: Value to map and convert.
 * @setting_data: Data to use for mapping and type information.
 *
 * Takes a GValue initialised with some data from a widget to prepare it for being provided to a
 * backend.
 */
void
ms_tweaks_mappings_handle_set (GValue *value, const MsTweaksSetting *setting_data)
{
  g_autofree char *normalised = stringify_gvalue (value);
  const char *mapped = normalised;

  /* setting_data->map has a different purpose in choice widgets than other ones, so don't use it
   * for this. */
  if (setting_data->map && setting_data->type != MS_TWEAKS_TYPE_CHOICE)
    mapped = g_hash_table_lookup (setting_data->map, normalised);

  g_value_unset (value);

  if (setting_data->backend == MS_TWEAKS_BACKEND_IDENTIFIER_GSETTINGS)
    switch (setting_data->gtype) {
    case MS_TWEAKS_GTYPE_BOOLEAN:
      g_value_init (value, G_TYPE_BOOLEAN);
      g_value_set_boolean (value, ms_tweaks_mappings_string_to_boolean (mapped));
      break;
    case MS_TWEAKS_GTYPE_DOUBLE:
      g_value_init (value, G_TYPE_DOUBLE);
      g_value_set_double (value, ms_tweaks_mappings_string_to_double (mapped));
      break;
    case MS_TWEAKS_GTYPE_FLAGS:
      g_value_init (value, G_TYPE_FLAGS);
      g_value_set_flags (value, ms_tweaks_mappings_string_to_uint (mapped));
      break;
    case MS_TWEAKS_GTYPE_NUMBER:
      g_value_init (value, G_TYPE_INT);
      g_value_set_int (value, ms_tweaks_mappings_string_to_int (mapped));
      break;
    case MS_TWEAKS_GTYPE_STRING:
    case MS_TWEAKS_GTYPE_UNKNOWN:
    default:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, mapped);
      break;
    }
  else {
    g_value_init (value, G_TYPE_STRING);
    g_value_set_string (value, mapped);
  }
}
