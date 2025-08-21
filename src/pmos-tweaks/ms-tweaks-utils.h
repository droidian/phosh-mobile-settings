/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#pragma once

#include <glib.h>

/* Data conversion functions. */
const char *ms_tweaks_util_boolean_to_string (const gboolean value);

/* Value retrieval functions. */
char *ms_tweaks_util_get_key_by_value_string (GHashTable           *hash_table,
                                              const char *restrict  value_to_find);
gboolean ms_tweaks_util_string_to_boolean (const char *string);

/* Miscellaneous utilities. */
#define ms_tweaks_error(name, ...)    ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_ERROR, \
                                                     (name), \
                                                     __VA_ARGS__)
#define ms_tweaks_message(name, ...)  ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_MESSAGE, \
                                                     (name), \
                                                     __VA_ARGS__)
#define ms_tweaks_critical(name, ...) ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_CRITICAL, \
                                                     (name), \
                                                     __VA_ARGS__)
#define ms_tweaks_warning(name, ...)  ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_WARNING, \
                                                     (name), \
                                                     __VA_ARGS__)
#define ms_tweaks_info(name, ...)     ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_INFO, \
                                                     (name), \
                                                     __VA_ARGS__)
#define ms_tweaks_debug(name, ...)    ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_DEBUG, \
                                                     (name), \
                                                     __VA_ARGS__)

void ms_tweaks_log (const char     *restrict log_domain,
                    GLogLevelFlags           log_level,
                    const char     *restrict name,
                    const char     *restrict message,
                    ...);
