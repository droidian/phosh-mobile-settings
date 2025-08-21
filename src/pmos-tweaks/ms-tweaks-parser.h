/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#pragma once

#include <glib.h>
#include <glib-object.h>

typedef enum {
  MS_TWEAKS_BACKEND_IDENTIFIER_UNKNOWN,

  MS_TWEAKS_BACKEND_IDENTIFIER_CSS,
  MS_TWEAKS_BACKEND_IDENTIFIER_GSETTINGS,
  MS_TWEAKS_BACKEND_IDENTIFIER_GTK3SETTINGS,
  MS_TWEAKS_BACKEND_IDENTIFIER_HARDWAREINFO,
  MS_TWEAKS_BACKEND_IDENTIFIER_OSKSDL,
  MS_TWEAKS_BACKEND_IDENTIFIER_SOUNDTHEME,
  MS_TWEAKS_BACKEND_IDENTIFIER_SYSFS,
  MS_TWEAKS_BACKEND_IDENTIFIER_SYMLINK,
  MS_TWEAKS_BACKEND_IDENTIFIER_XRESOURCES,
} MsTweaksSettingBackend;

typedef enum {
  MS_TWEAKS_GTYPE_UNKNOWN,

  MS_TWEAKS_GTYPE_BOOLEAN,
  MS_TWEAKS_GTYPE_DOUBLE,
  MS_TWEAKS_GTYPE_FLAGS,
  MS_TWEAKS_GTYPE_NUMBER,
  MS_TWEAKS_GTYPE_STRING,
} MsTweaksSettingGsettingType;

typedef enum {
  MS_TWEAKS_STYPE_UNKNOWN,

  MS_TWEAKS_STYPE_INT,
  MS_TWEAKS_STYPE_STRING,
} MsTweaksSettingSysfsType;

typedef enum {
  MS_TWEAKS_TYPE_UNKNOWN,

  MS_TWEAKS_TYPE_BOOLEAN,
  MS_TWEAKS_TYPE_CHOICE,
  MS_TWEAKS_TYPE_COLOR,
  MS_TWEAKS_TYPE_FILE,
  MS_TWEAKS_TYPE_FONT,
  MS_TWEAKS_TYPE_INFO,
  MS_TWEAKS_TYPE_NUMBER,
} MsTweaksWidgetType;

G_BEGIN_DECLS

#define MS_TYPE_TWEAKS_SETTING (ms_tweaks_setting_get_type ())
#define MS_TYPE_TWEAKS_SECTION (ms_tweaks_section_get_type ())
#define MS_TYPE_TWEAKS_PAGE (ms_tweaks_page_get_type ())

typedef struct {
  int weight;
  char *name;
  MsTweaksWidgetType type;
  MsTweaksSettingGsettingType gtype;
  MsTweaksSettingSysfsType stype;
  GHashTable *map; /* key: char *, value: char * */
  MsTweaksSettingBackend backend;
  char *help;
  char *default_; /* "default" is a reserved keyword in C. */
  GPtrArray *key; /* Since key may be a list, always make it an array. */
  gboolean readonly;
  gboolean source_ext;
  char *selector;
  char *guard;
  int multiplier;
  double min;
  double max;
  double step;
  GHashTable *css; /* key: char *, value: char * */
} MsTweaksSetting;

typedef struct {
  int weight;
  char *name;
  GHashTable *setting_table; /* key: char *, value: MsTweaksSetting * */
} MsTweaksSection;

typedef struct {
  int weight;
  char *name;
  GHashTable *section_table; /* key: char *, value: MsTweaksSection * */
} MsTweaksPage;

GType ms_tweaks_setting_get_type (void);
GType ms_tweaks_section_get_type (void);
GType ms_tweaks_page_get_type (void);

MsTweaksSetting *ms_tweaks_setting_copy (MsTweaksSetting *setting);
MsTweaksSection *ms_tweaks_section_copy (MsTweaksSection *section);
MsTweaksPage *ms_tweaks_page_copy (MsTweaksPage *page);

void ms_tweaks_setting_free (MsTweaksSetting *setting);
void ms_tweaks_section_free (MsTweaksSection *section);
void ms_tweaks_page_free (MsTweaksPage *page);

#define MS_TYPE_TWEAKS_PARSER ms_tweaks_parser_get_type ()
G_DECLARE_FINAL_TYPE (MsTweaksParser, ms_tweaks_parser, MS, TWEAKS_PARSER, GObject)

MsTweaksParser *ms_tweaks_parser_new (void);

GHashTable *ms_tweaks_parser_get_page_table (MsTweaksParser *self);

GList *ms_tweaks_parser_sort_by_weight (GHashTable *hash_table);

G_END_DECLS
