/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-convergence-panel"

#include "mobile-settings-config.h"

#include "mobile-settings-application.h"
#include "ms-convergence-panel.h"
#include "ms-scale-to-fit-row.h"
#include "ms-util.h"

/* Verbatim from convergence */
#define TOUCH_MAPPING_SCHEMA_ID "org.gnome.desktop.peripherals.touchscreen"
#define TOUCH_MAPPING_PATH_PREFIX "/org/gnome/desktop/peripherals/touchscreens/"
#define TOUCH_MAPPING_KEY "output"


typedef struct {
  char *name;

  char *make;
  char *model;
  char *serial;

  guint touch_usb_vendor;
  guint touch_usb_id;
} MsDock;


struct _MsConvergencePanel {
  AdwBin     parent;

  const MsDock *dock;
  AdwPreferencesGroup *dock_pref_group;
  GtkStack  *dock_stack;
  GSettings *touch_settings;
  GtkListBox *docks_listbox;
  AdwActionRow *map_touch_screen_row;
  GtkWidget *map_touch_screen_switch;


  MsHeadTracker *tracker;
};

G_DEFINE_TYPE (MsConvergencePanel, ms_convergence_panel, ADW_TYPE_BIN)


/*
 * This table lists dock outputs and their touch controllers.
 * The edid information can be queried like:
 *
 *  edid-decode /sys/devices/platform/soc@0/32c00000.bus/32e00000.display-controller/drm/card2/card2-DP-1/edid
 *
 * To locate the edid blob in /sys you can use
 *
 *  find /sys/devices | grep /edid
 *
 * Note that current phoc/wlroots provides the information from the "Detailed Timing Descriptors" if available
 * (not "Vendor & Product Identification"). However we list the bits from the "Vendor & Product Identification"
 * in the comment as well.
 *
 * To figure out the touch controller:
 *  lsusb -t  -v
 */
static const MsDock docks[] = {
  {
    /*
     * Manufacturer: YUK
     * Model: 16628
     */
    .name = "Nexdock 360",
    .make = "Unknown",
    .model = "NexDock",
    .serial = "8R33926O00Q",

    .touch_usb_vendor = 0x27c0,
    .touch_usb_id = 0x0819,
  },
  /* more docks go here */
};


static gboolean
ms_dock_has_touch (const MsDock *dock)
{
  return dock->touch_usb_vendor && dock->touch_usb_id;
}


static const MsDock *
find_dock (MsHead *head)
{
  for (int i = 0; i < G_N_ELEMENTS (docks); i++) {
    if ((STR_IS_NULL_OR_EMPTY (docks[i].make) || g_strcmp0 (docks[i].make, head->make) == 0) &&
        (STR_IS_NULL_OR_EMPTY (docks[i].model) || g_strcmp0 (docks[i].model, head->model) == 0) &&
        (STR_IS_NULL_OR_EMPTY (docks[i].serial) || g_strcmp0 (docks[i].serial, head->serial_number) == 0)) {
      return &docks[i];
    }
  }

  return NULL;
}


static gboolean
touch_mapping_get (GValue *value,
                   GVariant *variant,
                   gpointer user_data)
{
  g_autofree const char **vals = NULL;
  gsize len;
  gboolean mapped;

  vals = g_variant_get_strv (variant, &len);
  if (len != 3) {
    g_warning ("Can't convert touch mapping");
    return FALSE;
  }

  /* TODO: what if it's mapped but not to our output? */
  if  (STR_IS_NULL_OR_EMPTY (vals[0]) &&
       STR_IS_NULL_OR_EMPTY (vals[1]) &&
       STR_IS_NULL_OR_EMPTY (vals[2])) {
    mapped = FALSE;
  } else {
    mapped = TRUE;
  }

  g_value_set_boolean (value, mapped);

  return TRUE;
}


static GVariant *
touch_mapping_set (const GValue *value,
                   const GVariantType *expected_type,
                   gpointer user_data)
{
  MsConvergencePanel *self = MS_CONVERGENCE_PANEL (user_data);
  GVariantBuilder builder;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));

  if (g_value_get_boolean (value)) {
    g_variant_builder_add_value (&builder, g_variant_new ("s", self->dock->make));
    g_variant_builder_add_value (&builder, g_variant_new ("s", self->dock->model));
    g_variant_builder_add_value (&builder, g_variant_new ("s", self->dock->serial));
  } else {
    g_variant_builder_add_value (&builder, g_variant_new ("s", ""));
    g_variant_builder_add_value (&builder, g_variant_new ("s", ""));
    g_variant_builder_add_value (&builder, g_variant_new ("s", ""));
  }

  return g_variant_builder_end (&builder);
}


static void
on_head_added (MsConvergencePanel *self,
               MsHead *head)
{
  g_debug ("Got head %s", head->name);

  /* If we have a dock, keep it */
  if (self->dock)
    return;

  self->dock = find_dock (head);
  if (self->dock != NULL) {
    adw_preferences_group_set_title (self->dock_pref_group, self->dock->name);
    gtk_stack_set_visible_child_name (self->dock_stack, "dock");
    gtk_widget_set_sensitive (GTK_WIDGET (self->map_touch_screen_row),
                              ms_dock_has_touch (self->dock));
    if (ms_dock_has_touch (self->dock)) {
      g_autofree char *path = NULL;

      path = g_strdup_printf (TOUCH_MAPPING_PATH_PREFIX "%.4x:%.4x/",
                              self->dock->touch_usb_vendor, self->dock->touch_usb_id);

      self->touch_settings = g_settings_new_with_path (TOUCH_MAPPING_SCHEMA_ID, path);
      g_debug ("Dock touch settings path: %s", path);
      g_settings_bind_with_mapping (self->touch_settings,
                                    TOUCH_MAPPING_KEY,
                                    self->map_touch_screen_switch,
                                    "active",
                                    G_SETTINGS_BIND_DEFAULT,
                                    touch_mapping_get,
                                    touch_mapping_set,
                                    self,
                                    NULL);
    }
  }
}


static void
on_head_removed (MsConvergencePanel *self,
                 MsHead *head)
{
  g_debug ("Lost head: %s", head->name);

  if (find_dock (head) == FALSE)
    return;

  gtk_stack_set_visible_child_name (self->dock_stack, "empty");
  gtk_widget_set_sensitive (GTK_WIDGET (self->map_touch_screen_row), FALSE);
  g_clear_pointer (&self->touch_settings, g_object_unref);
  self->dock = NULL;
}


static void
on_head_tracker_changed (MsConvergencePanel *self, GParamSpec *spec, MobileSettingsApplication *app)
{
  MsHeadTracker *tracker = mobile_settings_application_get_head_tracker (app);
  GPtrArray *heads;

  if (tracker == NULL)
    return;

  self->tracker = g_object_ref (tracker);
  g_object_connect (self->tracker,
                    "swapped_object_signal::head-added",
                    G_CALLBACK (on_head_added),
                    self,
                    "swapped_object_signal::head-removed",
                    G_CALLBACK (on_head_removed),
                    self,
                    NULL);

  heads = ms_head_tracker_get_heads (self->tracker);
  for (int i = 0; i < heads->len; i++) {
    MsHead *head = g_ptr_array_index (heads, i);
    g_debug ("Initial head: %s", head->name);
    on_head_added (self, head);
  }
}


static void
ms_convergence_panel_finalize (GObject *object)
{
  MsConvergencePanel *self = MS_CONVERGENCE_PANEL (object);

  g_clear_object (&self->tracker);
  g_clear_object (&self->touch_settings);

  G_OBJECT_CLASS (ms_convergence_panel_parent_class)->finalize (object);
}


static void
ms_convergence_panel_class_init (MsConvergencePanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_convergence_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sigxcpu/MobileSettings/ui/ms-convergence-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, MsConvergencePanel, dock_pref_group);
  gtk_widget_class_bind_template_child (widget_class, MsConvergencePanel, dock_stack);
  gtk_widget_class_bind_template_child (widget_class, MsConvergencePanel, map_touch_screen_row);
  gtk_widget_class_bind_template_child (widget_class, MsConvergencePanel, map_touch_screen_switch);
}


static void
ms_convergence_panel_init (MsConvergencePanel *self)
{
  MobileSettingsApplication *app;

  gtk_widget_init_template (GTK_WIDGET (self));

  app = MOBILE_SETTINGS_APPLICATION (g_application_get_default ());
  g_signal_connect_swapped (app, "notify::head-tracker",
                            G_CALLBACK (on_head_tracker_changed), self);
  on_head_tracker_changed(self, NULL,
                          MOBILE_SETTINGS_APPLICATION (g_application_get_default ()));
}


MsConvergencePanel *
ms_convergence_panel_new (void)
{
  return MS_CONVERGENCE_PANEL (g_object_new (MS_TYPE_CONVERGENCE_PANEL, NULL));
}
