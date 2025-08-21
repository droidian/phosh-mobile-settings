/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "ms-tweaks-gtk-utils.h"


/**
 * ms_tweaks_util_gdkrgba_to_rgb_hex_string:
 * @from: The GdkRGBA to convert into a hexadecimal string.
 *
 * Takes a GdkRGBA struct and creates a hexadecimal string representation of the colour it denotes.
 *
 * Returns: A hexadecimal string of the form #RRGGBB (where R is red, G is green, B is blue).
 */
char *
ms_tweaks_util_gdkrgba_to_rgb_hex_string (const GdkRGBA *from)
{
  guchar red, green, blue;

  g_assert (from);

  red = roundf (from->red * 255.0f);
  green = roundf (from->green * 255.0f);
  blue = roundf (from->blue * 255.0f);

  return g_strdup_printf ("#%02hhx%02hhx%02hhx", red, green, blue);
}
