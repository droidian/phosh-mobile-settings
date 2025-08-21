/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-utils"

#include "ms-tweaks-utils.h"


const char *
ms_tweaks_util_boolean_to_string (const gboolean value)
{
  if (value)
    return "true";
  else
    return "false";
}

/**
 * ms_tweaks_util_string_to_boolean:
 * @string: A string of the form "true" or "false"
 *
 * This function assumes that string is valid input. Any invalid input will result in FALSE being
 * returned.
 *
 * Returns: The boolean representation of the string
 */
gboolean
ms_tweaks_util_string_to_boolean (const char *string)
{
  if (g_str_equal (string, "true"))
    return TRUE;
  else
    return FALSE;
}

/**
 * ms_tweaks_util_get_key_by_value_string:
 * @hash_table: The GHashTable to find the key in.
 * @value_to_find: The value to find the key of.
 *
 * Finds the first instance of the given value in a GHashTable. This should only be used on hash
 * tables where you can assume that every value is unique as hash tables don't guarantee any
 * particular ordering. Additionally, this only works for hash tables where both the keys and values
 * are strings.
 *
 * Returns: The key string if one was found, otherwise NULL.
 */
char *
ms_tweaks_util_get_key_by_value_string (GHashTable           *hash_table,
                                        const char *restrict  value_to_find)
{
  char *restrict matching_key = NULL;
  gpointer key = NULL, value = NULL;
  GHashTableIter iter;

  g_assert (hash_table);
  g_assert (value_to_find);

  g_hash_table_iter_init (&iter, hash_table);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    if (g_str_equal (value, value_to_find)) {
      matching_key = key;
      break;
    }
  }

  return matching_key;
}


[[gnu::format (gnu_printf, 4, 5)]]
void
ms_tweaks_log (const char *restrict log_domain,
               GLogLevelFlags       log_level,
               const char *restrict name,
               const char *restrict format,
               ...)
{
  va_list args;
  char *restrict format_with_prefix = g_strconcat ("[Setting \"", name, "\"] ", format, NULL);

  va_start (args, format);
  g_logv (log_domain, log_level, format_with_prefix, args);
  va_end (args);
  g_free (format_with_prefix);
}
