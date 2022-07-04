/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-head-tracker"

#include "mobile-settings-config.h"

#include "ms-head-tracker.h"

#include "protocols/wlr-output-management-unstable-v1-client-protocol.h"

enum {
  PROP_0,
  PROP_OUTPUT_MANAGER,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


enum {
  HEAD_ADDED,
  HEAD_REMOVED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


struct _MsHeadTracker {
  GObject               parent;

  GPtrArray            *heads;
  GPtrArray            *heads_added;

  struct zwlr_output_manager_v1 *output_manager;
};

G_DEFINE_TYPE (MsHeadTracker, ms_head_tracker, G_TYPE_OBJECT)

G_DEFINE_BOXED_TYPE (MsHead, ms_head, ms_head_ref, ms_head_unref);


static void
zwlr_output_mode_v1_handle_size (void *data, struct zwlr_output_mode_v1 *wlr_mode,
                                 int32_t width, int32_t height)
{
}


static void
zwlr_output_mode_v1_handle_refresh (void                       *data,
                                    struct zwlr_output_mode_v1 *wlr_mode,
                                    int32_t                     refresh)
{
}


static void
zwlr_output_mode_v1_handle_preferred (void                       *data,
                                      struct zwlr_output_mode_v1 *wlr_mode)
{
}


static void
zwlr_output_mode_v1_handle_finished (void                       *data,
                                     struct zwlr_output_mode_v1 *wlr_mode)
{
  zwlr_output_mode_v1_destroy (wlr_mode);
}


static const struct zwlr_output_mode_v1_listener mode_listener = {
  .size = zwlr_output_mode_v1_handle_size,
  .refresh = zwlr_output_mode_v1_handle_refresh,
  .preferred = zwlr_output_mode_v1_handle_preferred,
  .finished = zwlr_output_mode_v1_handle_finished,
};



static void
handle_zwlr_output_head_name (
  void *data,
  struct zwlr_output_head_v1 *zwlr_output_head_v1,
  const char* name)
{
  MsHead *head = data;

  g_free (head->name);
  head->name = g_strdup (name);

  g_debug ("%p: Got name %s", zwlr_output_head_v1, name);
}


static void
handle_zwlr_output_head_description(void *data,
                                    struct zwlr_output_head_v1 *zwlr_output_head_v1,
                                    const char*description)
{
}


static void
handle_zwlr_output_head_physical_size (void *data,
                                       struct zwlr_output_head_v1 *zwlr_output_head_v1,
                                       int32_t width,
                                       int32_t height)
{
}

static void
handle_zwlr_output_head_mode (void *data,
                              struct zwlr_output_head_v1 *zwlr_output_head_v1,
                              struct zwlr_output_mode_v1 *mode)
{
  zwlr_output_mode_v1_add_listener (mode, &mode_listener, NULL);
}

static void
handle_zwlr_output_head_enabled (void *data,
                                 struct zwlr_output_head_v1 *zwlr_output_head_v1,
                                 int32_t enabled)
{
}


static void
handle_zwlr_output_head_current_mode (void *data,
                                      struct zwlr_output_head_v1 *zwlr_output_head_v1,
                                      struct zwlr_output_mode_v1 *mode)
{
}

static void
handle_zwlr_output_head_position (void *data,
                                  struct zwlr_output_head_v1 *zwlr_output_head_v1,
                                  int32_t x,
                                  int32_t y)
{
}


static void
handle_zwlr_output_head_transform (void *data,
                                   struct zwlr_output_head_v1 *zwlr_output_head_v1,
                                   int32_t transform)
{
}


static void
handle_zwlr_output_head_scale (void *data,
                               struct zwlr_output_head_v1 *zwlr_output_head_v1,
                               wl_fixed_t scale)
{
}


static void
handle_zwlr_output_head_finished (void *data,
                                  struct zwlr_output_head_v1 *zwlr_output_head_v1)
{
  MsHead *head = data;
  guint index;

  if (g_ptr_array_find (head->tracker->heads, head, &index) == FALSE) {
    g_warning ("Trying to remove inexistent head %p", head);
    return;
  }

  g_signal_emit (head->tracker, signals[HEAD_REMOVED], 0, head);

  g_ptr_array_remove_index_fast (head->tracker->heads, index);
}


static void
handle_zwlr_output_head_make (void *data,
                              struct zwlr_output_head_v1 *zwlr_output_head_v1,
                              const char *make)
{
  MsHead *head = data;

  g_free (head->make);
  head->make = g_strdup (make);

  g_debug ("%p: Got make %s", zwlr_output_head_v1, make);
}


static void
handle_zwlr_output_head_model (void *data,
                              struct zwlr_output_head_v1 *zwlr_output_head_v1,
                              const char *model)
{
  MsHead *head = data;

  g_free (head->model);
  head->model = g_strdup (model);

  g_debug ("%p: Got model %s", zwlr_output_head_v1, model);
}


static void
handle_zwlr_output_head_serial_number (void *data,
                                       struct zwlr_output_head_v1 *zwlr_output_head_v1,
                                       const char *serial_number)
{
  MsHead *head = data;

  g_free (head->serial_number);
  head->serial_number = g_strdup (serial_number);

  g_debug ("%p: Got serial number %s", zwlr_output_head_v1, serial_number);
}



static const struct zwlr_output_head_v1_listener zwlr_output_head_v1_listener = {
  handle_zwlr_output_head_name,
  handle_zwlr_output_head_description,
  handle_zwlr_output_head_physical_size,
  handle_zwlr_output_head_mode,
  handle_zwlr_output_head_enabled,
  handle_zwlr_output_head_current_mode,
  handle_zwlr_output_head_position,
  handle_zwlr_output_head_transform,
  handle_zwlr_output_head_scale,
  handle_zwlr_output_head_finished,
  handle_zwlr_output_head_make,
  handle_zwlr_output_head_model,
  handle_zwlr_output_head_serial_number,
};


static void
ms_head_destroy (gpointer data)
{
  MsHead *head = data;

  g_debug ("Destroying head %s", head->name);
  g_clear_pointer (&head->name, g_free);
  g_clear_pointer (&head->make, g_free);
  g_clear_pointer (&head->model, g_free);
  g_clear_pointer (&head->serial_number, g_free);
  g_clear_pointer (&head->head, zwlr_output_head_v1_destroy);
}


static MsHead *
ms_head_new (struct zwlr_output_head_v1 *zwlr_output_head_v1, MsHeadTracker *tracker)
{
  MsHead *head = g_new0 (MsHead, 1);

  g_atomic_ref_count_init (&head->ref_count);
  head->head = zwlr_output_head_v1;
  head->tracker = tracker;

  zwlr_output_head_v1_add_listener (head->head,
                                    &zwlr_output_head_v1_listener, head);
  return head;
}


static void
handle_zwlr_output_manager_head (void *data,
  struct zwlr_output_manager_v1 *zwlr_foreign_head_manager_v1,
  struct zwlr_output_head_v1     *zwlr_output_head_v1)
{
  MsHeadTracker *self = MS_HEAD_TRACKER (data);
  MsHead *head;

  head = ms_head_new (zwlr_output_head_v1, self);
  g_ptr_array_add (self->heads_added, head);

  g_debug ("Got head %p", head);
}


static void
handle_zwlr_output_manager_done (void *data,
                                 struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
                                 uint32_t serial)
{
  MsHeadTracker *self = MS_HEAD_TRACKER (data);

  while (self->heads_added->len) {
    MsHead *head = g_ptr_array_steal_index_fast (self->heads_added, 0);

    g_ptr_array_add (self->heads, head);
    g_signal_emit (head->tracker, signals[HEAD_ADDED], 0, head);
  }
}


static void
handle_zwlr_output_manager_finished (void *data,
                                     struct zwlr_output_manager_v1 *zwlr_output_manager_v1)
{
  g_debug ("wlr_output_manager_finished");
}


static const struct zwlr_output_manager_v1_listener zwlr_output_manager_listener = {
  handle_zwlr_output_manager_head,
  handle_zwlr_output_manager_done,
  handle_zwlr_output_manager_finished,
};


static void
ms_head_tracker_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  MsHeadTracker *self = MS_HEAD_TRACKER (object);

  switch (property_id) {
  case PROP_OUTPUT_MANAGER:
    self->output_manager = g_value_get_pointer (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_head_tracker_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  MsHeadTracker *self = MS_HEAD_TRACKER (object);

  switch (property_id) {
  case PROP_OUTPUT_MANAGER:
    g_value_set_pointer (value, self->output_manager);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_head_tracker_constructed (GObject *object)
{
  MsHeadTracker *self = MS_HEAD_TRACKER(object);

  G_OBJECT_CLASS (ms_head_tracker_parent_class)->constructed (object);

  zwlr_output_manager_v1_add_listener (self->output_manager,
                                       &zwlr_output_manager_listener, self);
}


static void
ms_head_tracker_finalize (GObject *object)
{
  MsHeadTracker *self = MS_HEAD_TRACKER(object);

  g_clear_pointer (&self->heads, g_ptr_array_unref);
  g_clear_pointer (&self->heads_added, g_ptr_array_unref);

  G_OBJECT_CLASS (ms_head_tracker_parent_class)->finalize (object);
}


static void
ms_head_tracker_class_init (MsHeadTrackerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ms_head_tracker_get_property;
  object_class->set_property = ms_head_tracker_set_property;
  object_class->constructed = ms_head_tracker_constructed;
  object_class->finalize = ms_head_tracker_finalize;

  props[PROP_OUTPUT_MANAGER] =
    g_param_spec_pointer ("output-manager", "", "",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[HEAD_ADDED] = g_signal_new ("head-added",
                                      G_TYPE_FROM_CLASS (klass),
                                      G_SIGNAL_RUN_LAST,
                                      0, /* class offset */
                                      NULL, /* accumulator */
                                      NULL, /* accu_data */
                                      NULL, /* marshaller */
                                      G_TYPE_NONE, /* return */
                                      1, /* n_params */
                                      MS_TYPE_HEAD);

  signals[HEAD_REMOVED] = g_signal_new ("head-removed",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0, /* class_offset */
                                        NULL, /* accumulator */
                                        NULL, /* accu_data */
                                        NULL, /* marshaller */
                                        G_TYPE_NONE, /* return */
                                        1, /* n_params */
                                        MS_TYPE_HEAD);
}


static void
ms_head_tracker_init (MsHeadTracker *self)
{
  self->heads = g_ptr_array_new_full (1, (GDestroyNotify) ms_head_unref);
  self->heads_added = g_ptr_array_new_full (1, (GDestroyNotify) ms_head_unref);
}


MsHeadTracker *
ms_head_tracker_new (gpointer output_manager)
{
  return MS_HEAD_TRACKER (g_object_new (MS_TYPE_HEAD_TRACKER,
                                        "output-manager", output_manager,
                                        NULL));
}


GPtrArray *
ms_head_tracker_get_heads (MsHeadTracker *self)
{
  g_assert (MS_IS_HEAD_TRACKER (self));

  return self->heads;
}


MsHead *
ms_head_ref (MsHead *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ref_count, NULL);

  g_atomic_ref_count_inc (&self->ref_count);

  return self;
}

void
ms_head_unref (MsHead *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count);

  if (g_atomic_ref_count_dec (&self->ref_count))
    ms_head_destroy (self);
}
