/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-toplevel-tracker"

#include "mobile-settings-config.h"

#include "ms-toplevel-tracker.h"

#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"

enum {
  PROP_0,
  PROP_FOREIGN_TOPLEVEL_MANAGER,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


enum {
  APP_ID_ADDED,
  APP_ID_REMOVED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


typedef struct {
  char *app_id;
  char *title;

  struct zwlr_foreign_toplevel_handle_v1 *handle;
  MsToplevelTracker *tracker;
} MsToplevel;


struct _MsToplevelTracker {
  GObject               parent;

  GHashTable           *toplevels;
  GHashTable           *app_ids;

  struct zwlr_foreign_toplevel_manager_v1 *foreign_toplevel_manager;
};

G_DEFINE_TYPE (MsToplevelTracker, ms_toplevel_tracker, G_TYPE_OBJECT)


static void
handle_zwlr_foreign_toplevel_handle_title (
  void *data,
  struct zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1,
  const char* title)
{
  MsToplevel *toplevel = data;

  g_free (toplevel->title);
  toplevel->title = g_strdup (title);

  g_debug ("%p: Got title %s", zwlr_foreign_toplevel_handle_v1, title);
}


static void
handle_zwlr_foreign_toplevel_handle_app_id (void *data,
  struct zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1,
  const char* app_id)
{
  MsToplevel *toplevel = data;
  int num;

  g_free (toplevel->app_id);
  toplevel->app_id = g_strdup (app_id);

  num = GPOINTER_TO_INT (g_hash_table_lookup (toplevel->tracker->app_ids, app_id));
  if (num == 0) {
    g_signal_emit (toplevel->tracker, signals[APP_ID_ADDED], 0, app_id);
  }

  num++;
  g_debug ("%d toplevels with app-id %s", num, app_id);
  g_hash_table_insert (toplevel->tracker->app_ids, g_strdup (app_id), GINT_TO_POINTER (num));
}


static void
handle_zwlr_foreign_toplevel_handle_output_enter(void *data,
  struct zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1,
  struct wl_output *output)
{
}


static void
handle_zwlr_foreign_toplevel_handle_output_leave(void *data,
  struct zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1,
  struct wl_output *output)
{
}


static void
handle_zwlr_foreign_toplevel_handle_state (void *data,
  struct zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1,
  struct wl_array *state)
{
}


static void
handle_zwlr_foreign_toplevel_handle_done (void *data,
  struct zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1)
{
}


static void
handle_zwlr_foreign_toplevel_handle_closed (void *data,
  struct zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1)
{
  MsToplevel *toplevel = data;
  int num;

  g_return_if_fail (toplevel->handle == zwlr_foreign_toplevel_handle_v1);

  num = GPOINTER_TO_INT (g_hash_table_lookup (toplevel->tracker->app_ids, toplevel->app_id));
  num--;
  if (num == 0) {
    /* We're removing the last toplevel with that app-id */
    g_signal_emit (toplevel->tracker, signals[APP_ID_REMOVED], 0, toplevel->app_id);
    g_hash_table_remove (toplevel->tracker->app_ids, toplevel->app_id);
  } else {
    g_hash_table_insert (toplevel->tracker->app_ids, g_strdup (toplevel->app_id),
                         GINT_TO_POINTER (num));
  }

  g_debug ("%d toplevels with app-id %s remain", num, toplevel->app_id);
  if (g_hash_table_remove (toplevel->tracker->toplevels, toplevel->handle) == FALSE)
    g_warning ("Failed to find %p handle in toplevel tracker", toplevel->handle);
}


static const struct zwlr_foreign_toplevel_handle_v1_listener zwlr_foreign_toplevel_handle_listener = {
  handle_zwlr_foreign_toplevel_handle_title,
  handle_zwlr_foreign_toplevel_handle_app_id,
  handle_zwlr_foreign_toplevel_handle_output_enter,
  handle_zwlr_foreign_toplevel_handle_output_leave,
  handle_zwlr_foreign_toplevel_handle_state,
  handle_zwlr_foreign_toplevel_handle_done,
  handle_zwlr_foreign_toplevel_handle_closed
};


static void
toplevel_destroy (gpointer data)
{
  MsToplevel *toplevel = data;

  g_clear_pointer (&toplevel->app_id, g_free);
  g_clear_pointer (&toplevel->title, g_free);
  g_clear_pointer (&toplevel->handle, zwlr_foreign_toplevel_handle_v1_destroy);

  g_free (toplevel);
}


static MsToplevel *
ms_toplevel_new (struct zwlr_foreign_toplevel_handle_v1 *handle, MsToplevelTracker *tracker)
{
  MsToplevel *toplevel = g_new0 (MsToplevel, 1);

  toplevel->handle = handle;
  toplevel->tracker = tracker;

  zwlr_foreign_toplevel_handle_v1_add_listener (toplevel->handle,
                                                &zwlr_foreign_toplevel_handle_listener, toplevel);
  return toplevel;
}


static void
handle_zwlr_foreign_toplevel_manager_toplevel (void *data,
  struct zwlr_foreign_toplevel_manager_v1 *zwlr_foreign_toplevel_manager_v1,
  struct zwlr_foreign_toplevel_handle_v1  *handle)
{
  MsToplevelTracker *self = MS_TOPLEVEL_TRACKER (data);
  MsToplevel *toplevel;

  toplevel = ms_toplevel_new (handle, self);
  g_hash_table_insert (self->toplevels, toplevel, toplevel);

  g_debug ("Got toplevel %p", toplevel);
}


static void
handle_zwlr_foreign_toplevel_manager_finished (void *data,
  struct zwlr_foreign_toplevel_manager_v1 *zwlr_foreign_toplevel_manager_v1)
{
  g_debug ("wlr_foreign_toplevel_manager_finished");
}


static const struct zwlr_foreign_toplevel_manager_v1_listener zwlr_foreign_toplevel_manager_listener = {
  handle_zwlr_foreign_toplevel_manager_toplevel,
  handle_zwlr_foreign_toplevel_manager_finished,
};


static void
ms_toplevel_tracker_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  MsToplevelTracker *self = MS_TOPLEVEL_TRACKER (object);

  switch (property_id) {
  case PROP_FOREIGN_TOPLEVEL_MANAGER:
    self->foreign_toplevel_manager = g_value_get_pointer (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_toplevel_tracker_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  MsToplevelTracker *self = MS_TOPLEVEL_TRACKER (object);

  switch (property_id) {
  case PROP_FOREIGN_TOPLEVEL_MANAGER:
    g_value_set_pointer (value, self->foreign_toplevel_manager);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_toplevel_tracker_constructed (GObject *object)
{
  MsToplevelTracker *self = MS_TOPLEVEL_TRACKER(object);

  G_OBJECT_CLASS (ms_toplevel_tracker_parent_class)->constructed (object);

  zwlr_foreign_toplevel_manager_v1_add_listener (self->foreign_toplevel_manager,
                                                 &zwlr_foreign_toplevel_manager_listener, self);
}


static void
ms_toplevel_tracker_finalize (GObject *object)
{
  MsToplevelTracker *self = MS_TOPLEVEL_TRACKER(object);

  g_clear_pointer (&self->toplevels, g_hash_table_destroy);
  g_clear_pointer (&self->app_ids, g_hash_table_destroy);

  G_OBJECT_CLASS (ms_toplevel_tracker_parent_class)->finalize (object);
}


static void
ms_toplevel_tracker_class_init (MsToplevelTrackerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ms_toplevel_tracker_get_property;
  object_class->set_property = ms_toplevel_tracker_set_property;
  object_class->constructed = ms_toplevel_tracker_constructed;
  object_class->finalize = ms_toplevel_tracker_finalize;

  props[PROP_FOREIGN_TOPLEVEL_MANAGER] =
    g_param_spec_pointer ("foreign-toplevel-tracker", "", "",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[APP_ID_ADDED] = g_signal_new ("app-id-added",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0, /* class offset */
                                        NULL, /* accumulator */
                                        NULL, /* accu_data */
                                        NULL, /* marshaller */
                                        G_TYPE_NONE, /* return */
                                        1, /* n_params */
                                        G_TYPE_STRING);

  signals[APP_ID_REMOVED] = g_signal_new ("app-id-removed",
                                          G_TYPE_FROM_CLASS (klass),
                                          G_SIGNAL_RUN_LAST,
                                          0, /* class_offset */
                                          NULL, /* accumulator */
                                          NULL, /* accu_data */
                                          NULL, /* marshaller */
                                          G_TYPE_NONE, /* return */
                                          1, /* n_params */
                                          G_TYPE_STRING);
}


static void
ms_toplevel_tracker_init (MsToplevelTracker *self)
{
  self->toplevels = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, toplevel_destroy);
  self->app_ids = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}


MsToplevelTracker *
ms_toplevel_tracker_new (gpointer foreign_toplevel_manager)
{
  return MS_TOPLEVEL_TRACKER (g_object_new (MS_TYPE_TOPLEVEL_TRACKER,
                                            "foreign-toplevel-tracker", foreign_toplevel_manager,
                                            NULL));
}


GStrv
ms_toplevel_tracker_get_app_ids (MsToplevelTracker *self)
{
  g_autoptr (GList) keys = NULL;
  GPtrArray *app_ids = NULL;

  keys = g_hash_table_get_keys (self->app_ids);

  app_ids = g_ptr_array_new ();
  for (GList *elem = keys; elem; elem = elem->next)
    g_ptr_array_add (app_ids, g_strdup (elem->data));

  g_ptr_array_add (app_ids, NULL);

  return (char **)g_ptr_array_steal (app_ids, NULL);
}
