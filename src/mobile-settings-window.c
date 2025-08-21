/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */


#define G_LOG_DOMAIN "mobile-settings-window"

#include "mobile-settings-config.h"
#include "mobile-settings-application.h"
#include "mobile-settings-window.h"

#include "ms-compositor-panel.h"
#include "ms-feedback-panel.h"
#include "ms-plugin-panel.h"

#include "pmos-tweaks/ms-tweaks-page-builder.h"
#include "pmos-tweaks/ms-tweaks-parser.h"
#include "pmos-tweaks/ms-tweaks-utils.h"

#include <glib/gi18n.h>


struct _MobileSettingsWindow {
  AdwApplicationWindow     parent_instance;

  GtkSearchBar            *search_bar;
  GtkSearchEntry          *search_entry;

  AdwNavigationSplitView  *split_view;
  GtkStack                *stack;
  MsPanelSwitcher         *panel_switcher;

  GSettings               *settings;
  MsTweaksParser          *ms_tweaks_parser;
};

G_DEFINE_TYPE (MobileSettingsWindow, mobile_settings_window, ADW_TYPE_APPLICATION_WINDOW)


static void
on_search_entry_changed (GtkSearchEntry *search_entry,
                         MobileSettingsWindow *self)
{
  ms_panel_switcher_set_search_query (self->panel_switcher,
                                      gtk_editable_get_text (GTK_EDITABLE (search_entry)));
}


static void
on_search_entry_activated (GtkSearchEntry *search_entry,
                           MobileSettingsWindow *self)
{
  ms_panel_switcher_set_active_panel_index (self->panel_switcher, 0);
}


static void
show_content_cb (MobileSettingsWindow *self)
{
  const char *panelname;

  adw_navigation_split_view_set_show_content (self->split_view, TRUE);

  panelname = gtk_stack_get_visible_child_name (self->stack);

  g_settings_set_string (self->settings, "last-panel", panelname);

  /* Clear search entry to display all panels again */
  if (gtk_search_bar_get_search_mode (self->search_bar)) {
    gtk_search_bar_set_search_mode (self->search_bar, FALSE);
    gtk_editable_delete_text (GTK_EDITABLE (self->search_entry), 0, -1);
  }
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
add_ms_tweaks_page (gpointer value, gpointer user_data)
{
  MobileSettingsWindow *self = MOBILE_SETTINGS_WINDOW (user_data);
  MsTweaksPage *page_data = (MsTweaksPage *) value;
  MsTweaksPreferencesPage *page_widget = ms_tweaks_preferences_page_new (page_data);

  if (page_widget) {
    GtkStackPage *stack_page = gtk_stack_add_titled (self->stack,
                                                     GTK_WIDGET (page_widget), page_data->name,
                                                     page_data->name);
    /* TODO: Read icon from base64 property of settings definitions. */
    gtk_stack_page_set_icon_name (stack_page, "applications-science-symbolic");
  }
}


static void
ms_settings_window_constructed (GObject *object)
{
  MobileSettingsWindow *self = MOBILE_SETTINGS_WINDOW (object);
  MobileSettingsApplication *app = MOBILE_SETTINGS_APPLICATION (g_application_get_default ());
  GtkWidget *device_panel;
  GList *pages_sorted_by_weight = NULL;
  GHashTable *parser_page_table = NULL;

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

  parser_page_table = ms_tweaks_parser_get_page_table (self->ms_tweaks_parser);
  pages_sorted_by_weight = ms_tweaks_parser_sort_by_weight (parser_page_table);

  g_list_foreach (pages_sorted_by_weight, add_ms_tweaks_page, self);
}


static void
ms_settings_window_dispose (GObject *object)
{
  MobileSettingsWindow *self = MOBILE_SETTINGS_WINDOW (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->ms_tweaks_parser);

  G_OBJECT_CLASS (mobile_settings_window_parent_class)->dispose (object);
}


static void
mobile_settings_window_class_init (MobileSettingsWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = ms_settings_window_constructed;
  object_class->dispose = ms_settings_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/mobile-settings-window.ui");
  gtk_widget_class_bind_template_child (widget_class, MobileSettingsWindow, search_bar);
  gtk_widget_class_bind_template_child (widget_class, MobileSettingsWindow, search_entry);
  gtk_widget_class_bind_template_child (widget_class, MobileSettingsWindow, split_view);
  gtk_widget_class_bind_template_child (widget_class, MobileSettingsWindow, stack);
  gtk_widget_class_bind_template_child (widget_class, MobileSettingsWindow, panel_switcher);

  gtk_widget_class_bind_template_callback (widget_class, on_search_entry_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_search_entry_activated);
  gtk_widget_class_bind_template_callback (widget_class, show_content_cb);
  gtk_widget_class_bind_template_callback (widget_class, stack_child_to_tile);
}

static void
mobile_settings_window_init (MobileSettingsWindow *self)
{
  self->settings = g_settings_new ("mobi.phosh.MobileSettings");
  self->ms_tweaks_parser = ms_tweaks_parser_new ();

  gtk_widget_init_template (GTK_WIDGET (self));
  show_content_cb (self);

  gtk_search_bar_set_key_capture_widget (self->search_bar, GTK_WIDGET (self));
}


GtkSelectionModel *
mobile_settings_window_get_stack_pages (MobileSettingsWindow *self)
{
  g_assert (MOBILE_SETTINGS_IS_WINDOW (self));

  return gtk_stack_get_pages (self->stack);
}


MsPanelSwitcher *
mobile_settings_window_get_panel_switcher (MobileSettingsWindow *self)
{
  g_assert (MOBILE_SETTINGS_IS_WINDOW (self));

  return self->panel_switcher;
}
