/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "ms-plugin-librem5-panel.h"
#include "ms-util.h"

#include "dbus/login1-manager-dbus.h"

#include <sensors/sensors.h>

#include <glib/gi18n.h>

#define CMDLINE_PATH "/proc/cmdline"
#define CMDLINE_MAX  1024

#define LOGIN_BUS_NAME "org.freedesktop.login1"
#define LOGIN_OBJECT_PATH "/org/freedesktop/login1"

static uint sensors_inited;

typedef enum {
  MS_TEMP_SENSOR_CPU = 0,
  MS_TEMP_SENSOR_GPU,
  MS_TEMP_SENSOR_VPU,
  MS_TEMP_SENSOR_FUEL_GAUGE,
  MS_TEMP_SENSOR_BATTERY,
  MS_TEMP_SENSOR_LAST = MS_TEMP_SENSOR_BATTERY,
} MsTempSensor;

typedef struct {
  const sensors_chip_name  *name;
  const sensors_subfeature *subfeature_temp;
  const sensors_subfeature *subfeature_temp_crit;
  GtkLabel *label;
  GtkImage *icon;
  AdwActionRow *row;
} MsSensor;

typedef struct {
  const char *name;
  const char *pretty;
} MsSensorMapping;

static const MsSensorMapping temp_sensor_mapping[] = {
  { "cpu_thermal", "cpu" },
  { "gpu_thermal", "gpu" },
  { "vpu_thermal", "vpu" },
  { "bq25890_charger", "battery"},
  { "max170xx_battery", "fuelgauge"},
};

struct _MsPluginLibrem5Panel {
  MsPluginPanel  parent;

  GtkLabel      *uboot_label;

  MsSensor       temp_sensors[MS_TEMP_SENSOR_LAST + 1];
  guint          update_timeout_id;

  GtkWidget                       *suspend_button;
  GCancellable                    *cancel;
  MsPluginLibrem5DBusLoginManager *logind_manager_proxy;
};

G_DEFINE_TYPE (MsPluginLibrem5Panel, ms_plugin_librem5_panel, MS_TYPE_PLUGIN_PANEL)


static void
on_suspend_finish (GObject              *object,
                   GAsyncResult         *res,
                   MsPluginLibrem5Panel *self)
{
  g_autoptr (GError) err = NULL;

  if (ms_plugin_librem5_dbus_login_manager_call_suspend_finish (
        MS_PLUGIN_LIBREM5_DBUS_LOGIN_MANAGER (object),
        res,
        &err) == FALSE) {
    g_warning ("Failed to suspend: %s", err->message);
  }
}


static void
on_suspend_clicked (MsPluginLibrem5Panel *self)
{
  ms_plugin_librem5_dbus_login_manager_call_suspend (
    self->logind_manager_proxy,
    TRUE,
    self->cancel,
    (GAsyncReadyCallback)on_suspend_finish,
    self);
}


static void
on_can_suspend_finish (GObject              *object,
                       GAsyncResult         *res,
                       MsPluginLibrem5Panel *self)
{
  g_autoptr (GError) err = NULL;
  g_autofree char *out = NULL;

  if (ms_plugin_librem5_dbus_login_manager_call_can_suspend_finish (
        MS_PLUGIN_LIBREM5_DBUS_LOGIN_MANAGER (object),
        &out,
        res,
        &err) == FALSE) {
    g_warning ("Failed to check suspend capabilities: %s", err->message);
    return;
  }

  g_debug ("CanSuspend: %s", out);

  if (g_strcmp0 (out, "yes"))
      gtk_widget_set_sensitive (self->suspend_button, TRUE);
}


static void
on_logind_manager_proxy_new_for_bus_finish (GObject              *object,
                                            GAsyncResult         *res,
                                            MsPluginLibrem5Panel *self)
{
  g_autoptr (GError) err = NULL;
  MsPluginLibrem5DBusLoginManager *manager;

  manager = ms_plugin_librem5_dbus_login_manager_proxy_new_for_bus_finish (res, &err);
  if (manager == NULL) {
    g_warning ("Failed to get login1 session proxy: %s", err->message);
    return;
  }

  self->logind_manager_proxy = manager;

  ms_plugin_librem5_dbus_login_manager_call_can_suspend (
    self->logind_manager_proxy,
    self->cancel,
    (GAsyncReadyCallback) on_can_suspend_finish,
    self);
}


static gboolean
parse_uboot_version (MsPluginLibrem5Panel *self)
{
  gsize len;
  g_autofree gchar *cmdline = NULL;
  g_autoptr (GError) err = NULL;
  g_auto (GStrv) parts = NULL;

  if (g_file_test (CMDLINE_PATH, (G_FILE_TEST_EXISTS)) == FALSE)
    return FALSE;

  if (g_file_get_contents (CMDLINE_PATH, &cmdline, &len, &err) == FALSE) {
    g_warning ("Unable to read %s: %s", CMDLINE_PATH, err->message);
    return FALSE;
  }

  if (cmdline == NULL)
    return FALSE;

  parts = g_strsplit (cmdline, " ", -1);
  for (int i = 0; i < g_strv_length (parts); i++) {
    const char *version;

    if (g_str_has_prefix (parts[i], "u_boot_version=") == FALSE)
      continue;

    version = strchr (parts[i], '=');
    if (version != NULL) {
      version++;
      gtk_label_set_label (self->uboot_label, version);
      return TRUE;
    }
  }

  return FALSE;
}



static gboolean
on_update_timeout (gpointer user_data)
{
  MsPluginLibrem5Panel *self = MS_PLUGIN_LIBREM5_PANEL (user_data);
  double temp, crit;

  for (MsTempSensor i = 0; i <= MS_TEMP_SENSOR_LAST; i++) {
    g_autofree char *temp_msg = NULL;
    g_autofree char *crit_msg = NULL;
    gboolean icon_visible = FALSE;

    if (self->temp_sensors[i].name == NULL)
      continue;

    if (sensors_get_value (self->temp_sensors[i].name,
                           self->temp_sensors[i].subfeature_temp->number, &temp) < 0) {
      g_warning ("Failed to read temp for %s", self->temp_sensors[i].name->prefix);
      continue;
    }

    temp_msg = g_strdup_printf ("%.2f°C", temp);
    gtk_label_set_label (self->temp_sensors[i].label, temp_msg);

    if (self->temp_sensors[i].subfeature_temp_crit &&
        sensors_get_value (self->temp_sensors[i].name,
                           self->temp_sensors[i].subfeature_temp_crit->number, &crit) == 0) {
      crit_msg = g_strdup_printf (_("Critical temperature is %.2f°C"), crit);
      adw_action_row_set_subtitle (self->temp_sensors[i].row, crit_msg);
    }

    if (temp >= crit * 0.9)
      icon_visible = TRUE;

    gtk_widget_set_visible (GTK_WIDGET (self->temp_sensors[i].icon), icon_visible);
  }

  return TRUE;
}


static void
get_features (MsPluginLibrem5Panel *self, MsTempSensor num, const sensors_chip_name *name)
{
  int nr = 0;
  const sensors_feature *feature;
  const sensors_subfeature *subfeature;
  double val;

  do {
    feature = sensors_get_features(name, &nr);

    if (feature == NULL)
      break;

    subfeature = sensors_get_subfeature (name, feature, SENSORS_SUBFEATURE_TEMP_INPUT);
    if (subfeature == NULL)
      continue;

    if (sensors_get_value (name, subfeature->number, &val) < 0) {
      g_warning ("Failed tor read value for %s", name->prefix);
      continue;
    }

    g_debug ("chip: %s, feature: %s, subfeature: %s, value: %f", name->prefix, feature->name, subfeature->name, val);
    self->temp_sensors[num].name = name;
    self->temp_sensors[num].subfeature_temp = subfeature;

    subfeature = sensors_get_subfeature (name, feature, SENSORS_SUBFEATURE_TEMP_CRIT);
    if (subfeature != NULL) {
      self->temp_sensors[num].subfeature_temp_crit = subfeature;
    }

  } while (feature);
}


static void
init_sensors (MsPluginLibrem5Panel *self)
{
  int chipnum = 0;
  const sensors_chip_name *name;

  if (sensors_inited == 0)
    sensors_init (NULL);

  do {
    name = sensors_get_detected_chips (NULL, &chipnum);
    if (name == NULL)
      break;

    for (MsTempSensor i = 0; i <= MS_TEMP_SENSOR_LAST; i++) {
      if (g_str_has_prefix (name->prefix, temp_sensor_mapping[i].name)) {
        get_features (self, i, name);
        break;
      }
    }
  } while (name);
}


static void
ms_plugin_librem5_panel_realize (GtkWidget *widget)
{
  MsPluginLibrem5Panel *self = MS_PLUGIN_LIBREM5_PANEL (widget);

  GTK_WIDGET_CLASS (ms_plugin_librem5_panel_parent_class)->realize (widget);

  self->update_timeout_id = g_timeout_add_seconds (1, on_update_timeout, self);
  on_update_timeout (self);
}


static void
ms_plugin_librem5_panel_unrealize (GtkWidget *widget)
{
  MsPluginLibrem5Panel *self = MS_PLUGIN_LIBREM5_PANEL (widget);

  g_clear_handle_id (&self->update_timeout_id, g_source_remove);

  GTK_WIDGET_CLASS (ms_plugin_librem5_panel_parent_class)->unrealize (widget);
}


static void
ms_plugin_librem5_panel_finalize (GObject *object)
{
  MsPluginLibrem5Panel *self = MS_PLUGIN_LIBREM5_PANEL (object);

  if (sensors_inited) {
    if (sensors_inited == 1)
      sensors_cleanup();

    sensors_inited--;
  }

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);
  g_clear_object (&self->logind_manager_proxy);

  G_OBJECT_CLASS (ms_plugin_librem5_panel_parent_class)->finalize (object);
}


static void
ms_plugin_librem5_panel_class_init (MsPluginLibrem5PanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_plugin_librem5_panel_finalize;
  widget_class->realize = ms_plugin_librem5_panel_realize;
  widget_class->unrealize = ms_plugin_librem5_panel_unrealize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sigxcpu/MobileSettings/plugins/librem5/ui/ms-plugin-librem5-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, MsPluginLibrem5Panel, uboot_label);
  gtk_widget_class_bind_template_child (widget_class, MsPluginLibrem5Panel, suspend_button);

  for (int i = 0; i <= MS_TEMP_SENSOR_LAST; i++) {
    g_autofree char *name_label = g_strdup_printf ("%s_temp_label", temp_sensor_mapping[i].pretty);
    g_autofree char *name_icon = g_strdup_printf ("%s_temp_icon", temp_sensor_mapping[i].pretty);
    g_autofree char *name_row = g_strdup_printf ("%s_temp_row", temp_sensor_mapping[i].pretty);
    gtk_widget_class_bind_template_child_full (widget_class,
                                               name_label,
                                               FALSE,
                                               G_STRUCT_OFFSET(MsPluginLibrem5Panel, temp_sensors[i].label));
    gtk_widget_class_bind_template_child_full (widget_class,
                                               name_icon,
                                               FALSE,
                                               G_STRUCT_OFFSET(MsPluginLibrem5Panel, temp_sensors[i].icon));
    gtk_widget_class_bind_template_child_full (widget_class,
                                               name_row,
                                               FALSE,
                                               G_STRUCT_OFFSET(MsPluginLibrem5Panel, temp_sensors[i].row));
  }

  gtk_widget_class_bind_template_callback (widget_class, on_suspend_clicked);
}


static void
ms_plugin_librem5_panel_init (MsPluginLibrem5Panel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  parse_uboot_version (self);

  init_sensors (self);

  self->cancel = g_cancellable_new ();
  ms_plugin_librem5_dbus_login_manager_proxy_new_for_bus (
    G_BUS_TYPE_SYSTEM,
    G_DBUS_PROXY_FLAGS_NONE,
    LOGIN_BUS_NAME,
    LOGIN_OBJECT_PATH,
    self->cancel,
    (GAsyncReadyCallback) on_logind_manager_proxy_new_for_bus_finish,
    self);
}


MsPluginLibrem5Panel *
ms_plugin_librem5_panel_new (void)
{
  return MS_PLUGIN_LIBREM5_PANEL (g_object_new (MS_TYPE_PLUGIN_LIBREM5_PANEL, NULL));
}
