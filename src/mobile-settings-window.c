/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "mobile-settings-config.h"
#include "mobile-settings-application.h"
#include "mobile-settings-window.h"

#include "ms-compositor-panel.h"
#include "ms-feedback-panel.h"
#include "ms-plugin-panel.h"

#include <glib/gi18n.h>


struct _MobileSettingsWindow {
  AdwApplicationWindow parent_instance;

  AdwNavigationSplitView  *split_view;
  GtkStack                *stack;
};

G_DEFINE_TYPE (MobileSettingsWindow, mobile_settings_window, ADW_TYPE_APPLICATION_WINDOW)


static void
on_visible_child_changed (MobileSettingsWindow *self)
{
  adw_navigation_split_view_set_show_content (self->split_view, TRUE);
}


static char *
stack_child_to_tile (gpointer target, GtkStack *stack, GtkWidget *child)
{
  const char *title;
  GtkStackPage *page;

  g_assert (GTK_IS_STACK (stack));
  g_assert (GTK_IS_WIDGET (child));

  page = gtk_stack_get_page (stack, child);
  title = gtk_stack_page_get_title (page);
  if (title == NULL)
    title = gtk_stack_page_get_name (page);

  return g_strdup (title);
}


static void
ms_settings_window_constructed (GObject *object)
{
  MobileSettingsWindow *self = MOBILE_SETTINGS_WINDOW (object);
  MobileSettingsApplication *app = MOBILE_SETTINGS_APPLICATION (g_application_get_default ());
  GtkWidget *device_panel;

  G_OBJECT_CLASS (mobile_settings_window_parent_class)->constructed (object);

  if (gtk_stack_get_child_by_name (self->stack, "device") == NULL) {
    const char *title;

    g_assert (GTK_IS_APPLICATION (app));
    device_panel = mobile_settings_application_get_device_panel (app);
    if (device_panel) {
      GtkStackPage *page;

      title = ms_plugin_panel_get_title (MS_PLUGIN_PANEL (device_panel));
      page = gtk_stack_add_titled (self->stack, device_panel, "device", title ?: _("Device"));
      gtk_stack_page_set_icon_name (page, "phone-symbolic");
    }
  }
}


static void
mobile_settings_window_class_init (MobileSettingsWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = ms_settings_window_constructed;

  g_type_ensure (MS_TYPE_COMPOSITOR_PANEL);
  g_type_ensure (MS_TYPE_FEEDBACK_PANEL);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/mobile-settings-window.ui");
  gtk_widget_class_bind_template_child (widget_class, MobileSettingsWindow, split_view);
  gtk_widget_class_bind_template_child (widget_class, MobileSettingsWindow, stack);
  gtk_widget_class_bind_template_callback (widget_class, on_visible_child_changed);
  gtk_widget_class_bind_template_callback (widget_class, stack_child_to_tile);
}

static void
mobile_settings_window_init (MobileSettingsWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  on_visible_child_changed (self);
}
