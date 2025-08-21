/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-parser"

#include "ms-tweaks-parser.h"

#include "ms-tweaks-utils.h"


typedef enum {
  MS_TWEAKS_MAPPING_STATE_KEY,
  MS_TWEAKS_MAPPING_STATE_VALUE,
} MsTweaksMappingState;


typedef enum {
  MS_TWEAKS_STATE_START,              /* Start state. */
  MS_TWEAKS_STATE_STREAM,             /* Start/end stream. */
  MS_TWEAKS_STATE_DOCUMENT,           /* Start/end document. */

  MS_TWEAKS_STATE_PAGE,               /* Page entry. */
  MS_TWEAKS_STATE_PAGE_NAME,          /* Page name. */
  MS_TWEAKS_STATE_PAGE_WEIGHT,        /* Page weight. */

  MS_TWEAKS_STATE_SECTION,            /* Section entry. */
  MS_TWEAKS_STATE_SECTION_NAME,	      /* Section name. */
  MS_TWEAKS_STATE_SECTION_WEIGHT,	    /* Section weight. */

  MS_TWEAKS_STATE_SETTING,            /* Setting information. */
  MS_TWEAKS_STATE_SETTING_NAME,	      /* Setting name. */
  MS_TWEAKS_STATE_SETTING_WEIGHT,	    /* Setting weight. */
  MS_TWEAKS_STATE_SETTING_TYPE,	      /* Setting type. */
  MS_TWEAKS_STATE_SETTING_GTYPE,      /* Setting GSettings type. */
  MS_TWEAKS_STATE_SETTING_STYPE,      /* Setting sysfs type. */
  MS_TWEAKS_STATE_SETTING_DATA,       /* Setting data source. */
  MS_TWEAKS_STATE_SETTING_MAP,        /* Mapping of multiple values. */
  MS_TWEAKS_STATE_SETTING_BACKEND,    /* Setting backend. */
  MS_TWEAKS_STATE_SETTING_HELP,	      /* Setting help message. */
  MS_TWEAKS_STATE_SETTING_DEFAULT,    /* Setting default value. */
  MS_TWEAKS_STATE_SETTING_KEY,        /* Setting key. */
  MS_TWEAKS_STATE_SETTING_READONLY,	  /* Setting readonly property. */
  MS_TWEAKS_STATE_SETTING_SOURCE_EXT, /* Setting source_ext property. */
  MS_TWEAKS_STATE_SETTING_SELECTOR,	  /* Setting selector (CSS backend). */
  MS_TWEAKS_STATE_SETTING_GUARD,      /* Setting guard (CSS backend). */
  MS_TWEAKS_STATE_SETTING_MULTIPLIER, /* Setting multiplier. */
  MS_TWEAKS_STATE_SETTING_MIN,        /* Setting minimum value. */
  MS_TWEAKS_STATE_SETTING_MAX,        /* Setting maximum value. */
  MS_TWEAKS_STATE_SETTING_STEP,	      /* Setting step value. */
  MS_TWEAKS_STATE_SETTING_CSS,        /* Setting CSS definition (CSS backend). */

  MS_TWEAKS_STATE_STOP,               /* End state. */
} MsTweaksState;


struct _MsTweaksParser {
  GObject parent_instance;

  MsTweaksState state;
  MsTweaksPage *current_page;
  MsTweaksSection *current_section;
  MsTweaksSetting *current_setting;
  GHashTable *page_table; /* key: char *, value: MsTweaksPage * */
  /* Used when parsing the "key" property of a setting. */
  gboolean in_setting_key_list;
  /* Used when parsing the "map" and "css" properties of a setting. */
  MsTweaksMappingState setting_mapping_state;
  /* Also used when parsing the "map" and "css" properties of a setting. */
  char *last_key_name;
};


G_DEFINE_TYPE (MsTweaksParser, ms_tweaks_parser, G_TYPE_OBJECT)


MsTweaksSetting *
ms_tweaks_setting_copy (MsTweaksSetting *setting)
{
  MsTweaksSetting *new_setting = g_object_new (MS_TYPE_TWEAKS_SETTING, NULL);

  new_setting->weight = setting->weight;
  new_setting->name = g_strdup (setting->name);
  new_setting->type = setting->type;
  new_setting->gtype = setting->gtype;
  new_setting->stype = setting->stype;
  new_setting->map = g_hash_table_ref (new_setting->map);
  new_setting->backend = setting->backend;
  new_setting->help = g_strdup (setting->help);
  new_setting->default_ = g_strdup (setting->default_);
  new_setting->key = g_ptr_array_ref (setting->key);
  new_setting->readonly = setting->readonly;
  new_setting->source_ext = setting->source_ext;
  new_setting->selector = g_strdup (setting->selector);
  new_setting->guard = g_strdup (setting->guard);
  new_setting->multiplier = setting->multiplier;
  new_setting->min = setting->min;
  new_setting->max = setting->max;
  new_setting->step = setting->step;
  new_setting->css = g_hash_table_ref (new_setting->css);

  return new_setting;
}


MsTweaksSection *
ms_tweaks_section_copy (MsTweaksSection *section)
{
  MsTweaksSection *new_section = g_object_new (MS_TYPE_TWEAKS_SECTION, NULL);

  new_section->weight = section->weight;
  new_section->name = g_strdup (section->name);
  new_section->setting_table = g_hash_table_ref (new_section->setting_table);

  return new_section;
}


MsTweaksPage *
ms_tweaks_page_copy (MsTweaksPage *page)
{
  MsTweaksPage *new_page = g_object_new (MS_TYPE_TWEAKS_PAGE, NULL);

  new_page->weight = page->weight;
  new_page->name = g_strdup (page->name);
  new_page->section_table = g_hash_table_ref (new_page->section_table);

  return new_page;
}


void
ms_tweaks_setting_free (MsTweaksSetting *setting)
{
  /* Free string properties. */
  g_free (setting->name);
  g_free (setting->help);
  g_free (setting->default_);
  g_free (setting->selector);
  g_free (setting->guard);

  /* Free hash table properties. */
  g_clear_pointer (&setting->map, g_hash_table_unref);
  g_clear_pointer (&setting->css, g_hash_table_unref);

  /* Free pointer array properties. */
  g_clear_pointer (&setting->key, g_ptr_array_unref);
}


void
ms_tweaks_section_free (MsTweaksSection *section)
{
  g_free (section->name);

  g_clear_pointer (&section->setting_table, g_hash_table_unref);
}


void
ms_tweaks_page_free (MsTweaksPage *page)
{
  g_free (page->name);

  g_clear_pointer (&page->section_table, g_hash_table_unref);
}


G_DEFINE_BOXED_TYPE (MsTweaksSetting, ms_tweaks_setting, ms_tweaks_setting_copy, ms_tweaks_setting_free)
G_DEFINE_BOXED_TYPE (MsTweaksSection, ms_tweaks_section, ms_tweaks_section_copy, ms_tweaks_section_free)
G_DEFINE_BOXED_TYPE (MsTweaksPage, ms_tweaks_page, ms_tweaks_page_copy, ms_tweaks_page_free)


GHashTable *
ms_tweaks_parser_get_page_table (MsTweaksParser *self)
{
  return self->page_table;
}


static void
ms_tweaks_parser_init (MsTweaksParser *self)
{
  self->state = MS_TWEAKS_STATE_START;
  self->page_table = g_hash_table_new_full (g_str_hash,
                                            g_str_equal,
                                            g_free,
                                            (GDestroyNotify) ms_tweaks_page_free);
  /* Initialise it to key as we will always start with a key in mappings. */
  self->setting_mapping_state = MS_TWEAKS_MAPPING_STATE_KEY;
}


static void
ms_tweaks_parser_dispose (GObject *object)
{
  MsTweaksParser *self = MS_TWEAKS_PARSER (object);

  g_clear_pointer (&self->last_key_name, g_free);
  g_clear_pointer (&self->page_table, g_hash_table_unref);
}


static void
ms_tweaks_parser_class_init (MsTweaksParserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ms_tweaks_parser_dispose;
}


static gint
compare_by_weight (gconstpointer structure_a, gconstpointer structure_b)
{
  const int *restrict weight_a;
  const int *restrict weight_b;

  /* The cast below only works if these assertions hold true, if they fail the sorting will be
   * completely broken. */
  G_STATIC_ASSERT (offsetof (MsTweaksSetting, weight) == 0);
  G_STATIC_ASSERT (offsetof (MsTweaksSection, weight) == 0);
  G_STATIC_ASSERT (offsetof (MsTweaksPage, weight) == 0);

  weight_a = structure_a;
  weight_b = structure_b;

  return *weight_a - *weight_b;
}

/**
 * ms_tweaks_parser_sort_by_weight:
 * @hash_table: Hash table containing either MsTweaksSetting, MsTweaksSection, or MsTweaksPage
 *              values.
 *
 * Returns: The entries of the hash table sorted by their weight property.
 */
GList *
ms_tweaks_parser_sort_by_weight (GHashTable *hash_table)
{
  GList *hash_table_values = g_hash_table_get_values (hash_table);

  return g_list_sort (hash_table_values, compare_by_weight);
}


MsTweaksParser *
ms_tweaks_parser_new (void)
{
  return g_object_new (MS_TYPE_TWEAKS_PARSER, NULL);
}
