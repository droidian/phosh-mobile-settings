/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Gotam Gorabh <gautamy672@gmail.com>
 */

#include "ms-topbar-panel.h"
#include "ms-plugin-list-box.h"

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>


struct _MsTopbarPanel {
  AdwBin parent;
};

G_DEFINE_TYPE (MsTopbarPanel, ms_topbar_panel, ADW_TYPE_BIN)


static void
ms_topbar_panel_finalize (GObject *object)
{
}


static void
ms_topbar_panel_class_init (MsTopbarPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_topbar_panel_finalize;

  g_type_ensure (MS_TYPE_PLUGIN_LIST_BOX);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-topbar-panel.ui");
}


static void
ms_topbar_panel_init (MsTopbarPanel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
