/*
 Copyright (C) 2007-2009 Christian Dywan <christian@twotoasts.de>
 Copyright (C) 2008 Dale Whittaker <dayul@users.sf.net>
 Copyright (C) 2009 Jérôme Geulfucci <jeromeg@xfce.org>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
*/

#if HAVE_CONFIG_H
    #include <config.h>
#endif

#include "midori-browser.h"

#include "midori-array.h"
#include "midori-view.h"
#include "midori-preferences.h"
#include "midori-panel.h"
#include "midori-locationaction.h"
#include "midori-searchaction.h"
#include "midori-stock.h"

#include "gtkiconentry.h"
#include "marshal.h"
#include "sokoke.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
    #include <unistd.h>
#endif

#ifdef HAVE_HILDON_2_2
    #include <dbus/dbus.h>
    #include <mce/mode-names.h>
    #include <mce/dbus-names.h>
    #define MCE_SIGNAL_MATCH "type='signal'," \
        "sender='"    MCE_SERVICE     "',"    \
        "path='"      MCE_SIGNAL_PATH "',"    \
        "interface='" MCE_SIGNAL_IF   "'"
    #include <gdk/gdkx.h>
    #include <X11/Xatom.h>
#endif

#if HAVE_SQLITE
    #include <sqlite3.h>
#endif

#include <inkview.h>

struct _MidoriBrowser
{
    #if HAVE_HILDON
    HildonWindow parent_instance;
    #else
    GtkWindow parent_instance;
    #endif

    GtkActionGroup* action_group;
    GtkWidget* menubar;
    GtkWidget* menu_tools;
    GtkWidget* throbber;
    GtkWidget* navigationbar;
    GtkWidget* bookmarkbar;
    GtkToolItem* homepage;

    GtkWidget* panel;
    GtkWidget* notebook;

    GtkWidget* inspector;
    GtkWidget* inspector_view;

    GtkWidget* find;
    GtkWidget* find_text;
    GtkToolItem* find_case;
    GtkToolItem* find_highlight;
    GtkToolItem* find_close;
    gboolean find_typing;

    GtkWidget* statusbar;
    GtkWidget* statusbar_contents;
    GtkWidget* transferbar;
    GtkWidget* transferbar_clear;
    GtkWidget* progressbar;
    gchar* statusbar_text;

    gint last_window_width, last_window_height;
    guint alloc_timeout;
    guint panel_timeout;

    gint clear_private_data;

    MidoriWebSettings* settings;
    KatzeArray* proxy_array;
    KatzeArray* bookmarks;
    KatzeArray* trash;
    KatzeArray* search_engines;
    KatzeArray* history;
    gboolean show_tabs;

    KatzeNet* net;

    gboolean show_navigationbar;
    gboolean show_statusbar;
    gboolean speed_dial_in_new_tabs;
    gboolean progress_in_location;
    guint maximum_history_age;
    gchar* location_entry_search;
    gchar* news_aggregator;
};

#if HAVE_HILDON
G_DEFINE_TYPE (MidoriBrowser, midori_browser, HILDON_TYPE_WINDOW)
#else
G_DEFINE_TYPE (MidoriBrowser, midori_browser, GTK_TYPE_WINDOW)
#endif

enum
{
    PROP_0,

    PROP_MENUBAR,
    PROP_NAVIGATIONBAR,
    PROP_NOTEBOOK,
    PROP_PANEL,
    PROP_URI,
    PROP_TAB,
    PROP_LOAD_STATUS,
    PROP_STATUSBAR,
    PROP_STATUSBAR_TEXT,
    PROP_SETTINGS,
    PROP_BOOKMARKS,
    PROP_TRASH,
    PROP_SEARCH_ENGINES,
    PROP_HISTORY,
    PROP_SHOW_TABS,
};

enum
{
    NEW_WINDOW,
    ADD_TAB,
    REMOVE_TAB,
    ACTIVATE_ACTION,
    CONTEXT_READY,
    ADD_DOWNLOAD,
    SEND_NOTIFICATION,
    POPULATE_TOOL_MENU,
    QUIT,

    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void
midori_browser_dispose (GObject* object);

static void
midori_browser_finalize (GObject* object);

static void
midori_browser_set_property (GObject*      object,
                             guint         prop_id,
                             const GValue* value,
                             GParamSpec*   pspec);

static void
midori_browser_get_property (GObject*    object,
                             guint       prop_id,
                             GValue*     value,
                             GParamSpec* pspec);

static void
midori_browser_new_history_item (MidoriBrowser* browser,
                                 KatzeItem**    item);

static void
_midori_browser_set_toolbar_style (MidoriBrowser*     browser,
                                   MidoriToolbarStyle toolbar_style);

GtkWidget*
midori_panel_construct_menu_item (MidoriPanel*    panel,
                                  MidoriViewable* viewable);

static void
midori_browser_settings_notify (MidoriWebSettings* web_settings,
                                GParamSpec*        pspec,
                                MidoriBrowser*     browser);

static void
midori_browser_set_bookmarks (MidoriBrowser* browser,
                              KatzeArray*    bookmarks);

static void
midori_browser_add_download_item (MidoriBrowser*  browser,
                                  WebKitDownload* download);

GdkPixbuf*
midori_search_action_get_icon (KatzeItem*    item,
                               GtkWidget*    widget,
                               const gchar** icon_name);

static void
_midori_browser_find_done (MidoriBrowser* browser);

static gboolean
_action_menus_activate_item_alt (GtkAction*     action,
                                 KatzeItem*     item,
                                 guint          button,
                                 MidoriBrowser* browser);

#define _action_by_name(brwsr, nme) \
    gtk_action_group_get_action (brwsr->action_group, nme)
#define _action_set_sensitive(brwsr, nme, snstv) \
    gtk_action_set_sensitive (_action_by_name (brwsr, nme), snstv);
#define _action_set_visible(brwsr, nme, vsbl) \
    gtk_action_set_visible (_action_by_name (brwsr, nme), vsbl);
#define _action_set_active(brwsr, nme, actv) \
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION ( \
    _action_by_name (brwsr, nme)), actv);

static void
_toggle_tabbar_smartly (MidoriBrowser* browser)
{
    guint n;
    gboolean always_show_tabbar;

    if (!browser->show_tabs)
        return;

    n = gtk_notebook_get_n_pages (GTK_NOTEBOOK (browser->notebook));
    if (n < 2)
    {
        g_object_get (browser->settings, "always-show-tabbar",
            &always_show_tabbar, NULL);
        if (always_show_tabbar)
            n++;
    }
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (browser->notebook), n > 1);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (browser->notebook), n > 1);
}

static void
_midori_browser_update_actions (MidoriBrowser* browser)
{
    guint n;
    gboolean trash_empty;

    _toggle_tabbar_smartly (browser);
    n = gtk_notebook_get_n_pages (GTK_NOTEBOOK (browser->notebook));
    _action_set_sensitive (browser, "TabPrevious", n > 1);
    _action_set_sensitive (browser, "TabNext", n > 1);

    if (browser->trash)
    {
        trash_empty = katze_array_is_empty (browser->trash);
        _action_set_sensitive (browser, "UndoTabClose", !trash_empty);
        _action_set_sensitive (browser, "Trash", !trash_empty);
    }
}

static gboolean
midori_browser_view_can_set_encoding (MidoriView* view)
{
    static GParamSpec* pspec = NULL;
    if (G_UNLIKELY (!pspec))
        pspec = g_object_class_find_property (
        g_type_class_peek (WEBKIT_TYPE_WEB_VIEW), "custom-encoding");
    /* Web views and zoom, source views can't, so zoom means encoding */
    return pspec && midori_view_can_zoom_in (view);
}

static void
_midori_browser_update_interface (MidoriBrowser* browser)
{
    GtkWidget* widget;
    MidoriView* view;
    gboolean loading;
    gboolean can_reload;
    GtkAction* action;

    widget = midori_browser_get_current_tab (browser);
    view = MIDORI_VIEW (widget);
    loading = midori_view_get_load_status (view) != MIDORI_LOAD_FINISHED;
    can_reload = midori_view_can_reload (view);

    _action_set_sensitive (browser, "Reload", can_reload);
    _action_set_sensitive (browser, "Stop", can_reload && loading);
    _action_set_sensitive (browser, "Back", midori_view_can_go_back (view));
    _action_set_sensitive (browser, "Forward", midori_view_can_go_forward (view));
    _action_set_sensitive (browser, "Previous",
        midori_view_get_previous_page (view) != NULL);
    _action_set_sensitive (browser, "Next",
        midori_view_get_next_page (view) != NULL);

    gtk_action_set_visible (_action_by_name (browser, "AddSpeedDial"),
        browser->speed_dial_in_new_tabs && !midori_view_is_blank (view));
    /* Currently views that don't support source, don't support
       saving either. If that changes, we need to think of something. */
    _action_set_sensitive (browser, "SaveAs", midori_view_can_view_source (view));
    _action_set_sensitive (browser, "Print", midori_view_can_print (view));
    _action_set_sensitive (browser, "ZoomIn", midori_view_can_zoom_in (view));
    _action_set_sensitive (browser, "ZoomOut", midori_view_can_zoom_out (view));
    _action_set_sensitive (browser, "ZoomNormal",
        midori_view_get_zoom_level (view) != 1.0f);
    _action_set_sensitive (browser, "Encoding",
        midori_browser_view_can_set_encoding (view));
    _action_set_sensitive (browser, "SourceView",
        midori_view_can_view_source (view));
    _action_set_sensitive (browser, "Find",
        midori_view_can_find (view));
    _action_set_sensitive (browser, "FindNext",
        midori_view_can_find (view));
    _action_set_sensitive (browser, "FindPrevious",
        midori_view_can_find (view));
    gtk_widget_set_sensitive (GTK_WIDGET (browser->find_highlight),
        midori_view_can_find (view));

    action = gtk_action_group_get_action (browser->action_group, "ReloadStop");
    if (!loading)
    {
        g_object_set (action,
                      "stock-id", GTK_STOCK_REFRESH,
                      "tooltip", _("Reload the current page"),
                      "sensitive", can_reload, NULL);

        gtk_widget_hide (browser->progressbar);
        if (!browser->show_navigationbar && !browser->show_statusbar)
            gtk_widget_hide (browser->navigationbar);
    }
    else
    {
        g_object_set (action,
                      "stock-id", GTK_STOCK_STOP,
                      "tooltip", _("Stop loading the current page"), NULL);

        if (!browser->progress_in_location || !GTK_WIDGET_VISIBLE (browser->navigationbar))
            gtk_widget_show (browser->progressbar);
        if (!GTK_WIDGET_VISIBLE (browser->statusbar) &&
            !GTK_WIDGET_VISIBLE (browser->navigationbar) &&
            browser->progress_in_location)
            gtk_widget_show (browser->navigationbar);
        if (browser->progress_in_location)
        {
            action = _action_by_name (browser, "Location");
            midori_location_action_set_progress (MIDORI_LOCATION_ACTION (action),
                midori_view_get_progress (view));
        }
    }

    #if HAVE_HILDON
    #if HILDON_CHECK_VERSION (2, 2, 0)
    hildon_gtk_window_set_progress_indicator (GTK_WINDOW (browser), loading);
    #endif
    #else
    gtk_widget_set_sensitive (browser->throbber, loading);
    katze_throbber_set_animated (KATZE_THROBBER (browser->throbber), loading);
    #endif

    action = _action_by_name (browser, "Location");
    if (g_object_get_data (G_OBJECT (view), "news-feeds"))
    {
        midori_location_action_set_secondary_icon (
            MIDORI_LOCATION_ACTION (action), STOCK_NEWS_FEED);
        gtk_action_set_sensitive (_action_by_name (browser, "AddNewsFeed"), TRUE);
    }
    else
    {
        midori_location_action_set_secondary_icon (
            MIDORI_LOCATION_ACTION (action), GTK_STOCK_JUMP_TO);
        gtk_action_set_sensitive (_action_by_name (browser, "AddNewsFeed"), FALSE);
    }
}

static void
_midori_browser_set_statusbar_text (MidoriBrowser* browser,
                                    const gchar*   text)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    gboolean is_location = widget ?
        GTK_IS_COMBO_BOX (gtk_widget_get_parent (widget)) : FALSE;

    katze_assign (browser->statusbar_text, sokoke_format_uri_for_display (text));

    if (!GTK_WIDGET_VISIBLE (browser->statusbar) && !is_location)
    {
        GtkAction* action = _action_by_name (browser, "Location");
        MidoriLocationAction* location_action = MIDORI_LOCATION_ACTION (action);
        if (text && *text)
        {
            midori_location_action_set_uri (location_action, text);
            midori_location_action_set_icon (location_action, NULL);
            midori_location_action_set_secondary_icon (location_action, NULL);
        }
        else
        {
            GtkWidget* view = midori_browser_get_current_tab (browser);
            if (G_LIKELY (view))
            {
                if (g_object_get_data (G_OBJECT (view), "news-feeds"))
                    midori_location_action_set_secondary_icon (
                        location_action, STOCK_NEWS_FEED);
                else
                    midori_location_action_set_secondary_icon (
                        location_action, GTK_STOCK_JUMP_TO);
                midori_location_action_set_uri (location_action,
                    midori_view_get_display_uri (MIDORI_VIEW (view)));
                midori_location_action_set_icon (location_action,
                    midori_view_get_icon (MIDORI_VIEW (view)));
            }
        }
    }
    else
    {
        gtk_statusbar_pop (GTK_STATUSBAR (browser->statusbar), 1);
        gtk_statusbar_push (GTK_STATUSBAR (browser->statusbar), 1,
                            browser->statusbar_text ? browser->statusbar_text : "");
    }
}

static void
_midori_browser_set_current_page_smartly (MidoriBrowser* browser,
                                          gint           n)
{
    if (!katze_object_get_boolean (browser->settings,
        "open-tabs-in-the-background"))
        midori_browser_set_current_page (browser, n);
}

static void
_midori_browser_update_progress (MidoriBrowser* browser,
                                 MidoriView*    view)
{
    MidoriLocationAction* action;
    gdouble progress;
    gchar* message;

    action = MIDORI_LOCATION_ACTION (_action_by_name (browser, "Location"));
    progress = midori_view_get_progress (view);
    /* When we are finished, we don't want to *see* progress anymore */
    if (midori_view_get_load_status (view) == MIDORI_LOAD_FINISHED)
        progress = 0.0;
    if (progress > 0.0)
    {
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (browser->progressbar),
                                       progress);
        message = g_strdup_printf (_("%d%% loaded"), (gint)(progress * 100));
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (browser->progressbar),
                                   message);
        g_free (message);
        if (!browser->progress_in_location)
            progress = 0.0;
        midori_location_action_set_progress (action, progress);
    }
    else
    {
        gtk_progress_bar_pulse (GTK_PROGRESS_BAR (browser->progressbar));
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (browser->progressbar),
                                   NULL);
        midori_location_action_set_progress (action, 0.0);
    }
}

static void
midori_browser_update_history_title (MidoriBrowser* browser,
                                     KatzeItem*     item)
{
    #if HAVE_SQLITE
    sqlite3* db;
    static sqlite3_stmt* stmt = NULL;

    db = g_object_get_data (G_OBJECT (browser->history), "db");
    if (!stmt)
    {
        const gchar* sqlcmd;

        sqlcmd = "UPDATE history SET title=? WHERE uri = ? and date=?";
        sqlite3_prepare_v2 (db, sqlcmd, -1, &stmt, NULL);
    }
    sqlite3_bind_text (stmt, 1, katze_item_get_name (item), -1, 0);
    sqlite3_bind_text (stmt, 2, katze_item_get_uri (item), -1, 0);
    sqlite3_bind_int64 (stmt, 3, katze_item_get_added (item));

    if (sqlite3_step (stmt) != SQLITE_DONE)
        g_printerr (_("Failed to update title: %s\n"), sqlite3_errmsg (db));
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    #endif
}

static void
_midori_browser_activate_action (MidoriBrowser* browser,
                                 const gchar*   name)
{
    GtkAction* action = _action_by_name (browser, name);
    if (action)
        gtk_action_activate (action);
    else
        g_warning (_("Unexpected action '%s'."), name);
}

static void
midori_view_notify_icon_cb (MidoriView*    view,
                            GParamSpec*    pspec,
                            MidoriBrowser* browser)
{
    const gchar* uri;
    GtkAction* action;

    if (midori_browser_get_current_tab (browser) != (GtkWidget*)view)
        return;

    uri = midori_view_get_display_uri (view);
    action = _action_by_name (browser, "Location");
    if (browser->maximum_history_age)
        midori_location_action_set_icon_for_uri (
        MIDORI_LOCATION_ACTION (action), midori_view_get_icon (view), uri);
    midori_location_action_set_icon (MIDORI_LOCATION_ACTION (action),
                                     midori_view_get_icon (view));
}

static void
midori_view_notify_load_status_cb (GtkWidget*      widget,
                                   GParamSpec*     pspec,
                                   MidoriBrowser*  browser)
{
    MidoriView* view = MIDORI_VIEW (widget);
    const gchar* uri;
    GtkAction* action;

    uri = midori_view_get_display_uri (view);
    action = _action_by_name (browser, "Location");

    if (midori_view_get_load_status (view) == MIDORI_LOAD_COMMITTED)
        midori_location_action_add_uri (MIDORI_LOCATION_ACTION (action), uri);

    if (widget == midori_browser_get_current_tab (browser))
    {
        if (midori_view_get_load_status (view) == MIDORI_LOAD_COMMITTED)
        {
            midori_location_action_set_uri (
                MIDORI_LOCATION_ACTION (action), uri);
            midori_location_action_set_secondary_icon (
                MIDORI_LOCATION_ACTION (action), GTK_STOCK_JUMP_TO);
            g_object_notify (G_OBJECT (browser), "uri");
        }

        _midori_browser_update_interface (browser);
        _midori_browser_set_statusbar_text (browser, NULL);

        /* This is a hack to ensure that the address entry is focussed
           with speed dial open. */
        if (midori_view_is_blank (view))
            gtk_action_activate (_action_by_name (browser, "Location"));
    }

    g_object_notify (G_OBJECT (browser), "load-status");
}

static void
midori_view_notify_progress_cb (GtkWidget*     view,
                                GParamSpec*    pspec,
                                MidoriBrowser* browser)
{
   if (view == midori_browser_get_current_tab (browser))
        _midori_browser_update_progress (browser, MIDORI_VIEW (view));
}

static void
midori_view_context_ready_cb (GtkWidget*     view,
                              JSContextRef   js_context,
                              MidoriBrowser* browser)
{
    g_signal_emit (browser, signals[CONTEXT_READY], 0, js_context);
}

static void
midori_view_notify_uri_cb (GtkWidget*     view,
                           GParamSpec*    pspec,
                           MidoriBrowser* browser)
{
    if (view == midori_browser_get_current_tab (browser))
    {
        const gchar* uri = midori_view_get_display_uri (MIDORI_VIEW (view));
        GtkAction* action = _action_by_name (browser, "Location");
        midori_location_action_set_uri (MIDORI_LOCATION_ACTION (action), uri);
        _midori_browser_update_interface (browser);
    }
}

static void
midori_view_notify_title_cb (GtkWidget*     widget,
                             GParamSpec*    pspec,
                             MidoriBrowser* browser)
{
    MidoriView* view = MIDORI_VIEW (widget);
    const gchar* uri;
    const gchar* title;
    GtkAction* action;
    gchar* window_title;

    uri = midori_view_get_display_uri (view);
    title = midori_view_get_display_title (view);
    action = _action_by_name (browser, "Location");
    if (browser->maximum_history_age)
        midori_location_action_set_title_for_uri (
        MIDORI_LOCATION_ACTION (action), title, uri);
    if (midori_view_get_load_status (view) == MIDORI_LOAD_COMMITTED)
    {
        KatzeItem* item;
        KatzeItem* proxy;

        if (browser->history && browser->maximum_history_age)
        {
            item = g_object_get_data (G_OBJECT (view), "history-item-added");
            proxy = midori_view_get_proxy_item (view);
            if (item && katze_item_get_added (item) == katze_item_get_added (proxy))
            {
                katze_item_set_name (item, katze_item_get_name (proxy));
                midori_browser_update_history_title (browser, item);
            }
            else
            {
                katze_object_assign (item, katze_item_copy (proxy));
                midori_browser_new_history_item (browser, &item);
                g_object_ref (item);
                g_object_set_data_full (G_OBJECT (view), "history-item-added",
                                        item, (GDestroyNotify)g_object_unref);
            }
        }
    }

    if (widget == midori_browser_get_current_tab (browser))
    {
        window_title = g_strconcat (title, " - ",
            g_get_application_name (), NULL);
        gtk_window_set_title (GTK_WINDOW (browser), window_title);
        g_free (window_title);
    }
}

static void
midori_view_notify_zoom_level_cb (GtkWidget*     view,
                                  GParamSpec*    pspec,
                                  MidoriBrowser* browser)
{
    if (view == midori_browser_get_current_tab (browser))
        _action_set_sensitive (browser, "ZoomNormal",
            midori_view_get_zoom_level (MIDORI_VIEW (view)) != 1.0f);
}

static void
midori_view_notify_statusbar_text_cb (GtkWidget*     view,
                                      GParamSpec*    pspec,
                                      MidoriBrowser* browser)
{
    gchar* text;

    if (view == midori_browser_get_current_tab (browser))
    {
        g_object_get (view, "statusbar-text", &text, NULL);
        _midori_browser_set_statusbar_text (browser, text);
        g_free (text);
    }
}

static void
midori_browser_edit_bookmark_uri_changed_cb (GtkEntry*      entry,
                                             GtkDialog*     dialog)
{
    const gchar* uri = gtk_entry_get_text (entry);
    gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_ACCEPT,
        uri && (g_str_has_prefix (uri, "http://")
        || g_str_has_prefix (uri, "https://")
        || g_str_has_prefix (uri, "file://")
        || g_str_has_prefix (uri, "data:")
        || g_str_has_prefix (uri, "javascript:")));
}

/* Private function, used by MidoriBookmarks and MidoriHistory */
/* static */ void
midori_browser_edit_bookmark_dialog_new (MidoriBrowser* browser,
                                         KatzeItem*     bookmark,
                                         gboolean       new_bookmark,
                                         gboolean       is_folder)
{
    const gchar* title;
    GtkWidget* dialog;
    GtkWidget* content_area;
    GtkSizeGroup* sizegroup;
    GtkWidget* view;
    GtkWidget* hbox;
    GtkWidget* label;
    const gchar* value;
    GtkWidget* entry_title;
    GtkWidget* entry_desc;
    GtkWidget* entry_uri;
    GtkWidget* combo_folder;
    GtkWidget* check_toolbar;
    GtkWidget* check_app;

    if (!browser->bookmarks || !GTK_WIDGET_VISIBLE (browser))
        return;

    if (is_folder)
        title = new_bookmark ? _("New folder") : _("Edit folder");
    else
        title = new_bookmark ? _("New bookmark") : _("Edit bookmark");
    dialog = gtk_dialog_new_with_buttons (
        title, GTK_WINDOW (browser),
        GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        new_bookmark ? GTK_STOCK_ADD : GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    gtk_window_set_icon_name (GTK_WINDOW (dialog),
        new_bookmark ? GTK_STOCK_ADD : GTK_STOCK_REMOVE);
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
    gtk_container_set_border_width (GTK_CONTAINER (content_area), 5);
    sizegroup =  gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    if (!bookmark)
    {
        view = midori_browser_get_current_tab (browser);
        if (is_folder)
        {
            bookmark = (KatzeItem*)katze_array_new (KATZE_TYPE_ARRAY);
            katze_item_set_name (bookmark,
                midori_view_get_display_title (MIDORI_VIEW (view)));
        }
        else
            bookmark = g_object_new (KATZE_TYPE_ITEM,
                "uri", midori_view_get_display_uri (MIDORI_VIEW (view)),
                "name", midori_view_get_display_title (MIDORI_VIEW (view)), NULL);
    }

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
    label = gtk_label_new_with_mnemonic (_("_Title:"));
    gtk_size_group_add_widget (sizegroup, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    entry_title = gtk_entry_new ();
    gtk_entry_set_activates_default (GTK_ENTRY (entry_title), TRUE);
    value = katze_item_get_name (bookmark);
    gtk_entry_set_text (GTK_ENTRY (entry_title), value ? value : "");
    gtk_box_pack_start (GTK_BOX (hbox), entry_title, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER (content_area), hbox);
    gtk_widget_show_all (hbox);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
    label = gtk_label_new_with_mnemonic (_("_Description:"));
    gtk_size_group_add_widget (sizegroup, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    entry_desc = gtk_entry_new ();
    gtk_entry_set_activates_default (GTK_ENTRY (entry_desc), TRUE);
    if (bookmark)
    {
        value = katze_item_get_text (bookmark);
        gtk_entry_set_text (GTK_ENTRY (entry_desc), value ? value : "");
    }
    gtk_box_pack_start (GTK_BOX (hbox), entry_desc, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER (content_area), hbox);
    gtk_widget_show_all (hbox);

    entry_uri = NULL;
    if (!KATZE_IS_ARRAY (bookmark))
    {
        hbox = gtk_hbox_new (FALSE, 8);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
        label = gtk_label_new_with_mnemonic (_("_Address:"));
        gtk_size_group_add_widget (sizegroup, label);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        entry_uri = gtk_entry_new ();
        #if HAVE_HILDON
        HildonGtkInputMode mode = hildon_gtk_entry_get_input_mode (GTK_ENTRY (entry_uri));
        mode &= ~HILDON_GTK_INPUT_MODE_AUTOCAP;
        hildon_gtk_entry_set_input_mode (GTK_ENTRY (entry_uri), mode);
        #endif
        gtk_entry_set_activates_default (GTK_ENTRY (entry_uri), TRUE);
        gtk_entry_set_text (GTK_ENTRY (entry_uri), katze_item_get_uri (bookmark));
        midori_browser_edit_bookmark_uri_changed_cb (GTK_ENTRY (entry_uri),
                                                     GTK_DIALOG (dialog));
        g_signal_connect (entry_uri, "changed",
            G_CALLBACK (midori_browser_edit_bookmark_uri_changed_cb), dialog);
        gtk_box_pack_start (GTK_BOX (hbox), entry_uri, TRUE, TRUE, 0);
        gtk_container_add (GTK_CONTAINER (content_area), hbox);
        gtk_widget_show_all (hbox);
    }

    combo_folder = NULL;
    if (1)
    {
        GtkListStore* model;
        GtkCellRenderer* renderer;
        KatzeItem* item;
        guint i, n;

        hbox = gtk_hbox_new (FALSE, 8);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
        label = gtk_label_new_with_mnemonic (_("_Folder:"));
        gtk_size_group_add_widget (sizegroup, label);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
        combo_folder = gtk_combo_box_new_with_model (GTK_TREE_MODEL (model));
        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_folder), renderer, TRUE);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo_folder), renderer, "text", 0);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo_folder), renderer, "ellipsize", 1);
        gtk_list_store_insert_with_values (model, NULL, G_MAXINT,
            0, _("Toplevel folder"), 1, PANGO_ELLIPSIZE_END, -1);
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_folder), 0);

        i = 0;
        n = 1;
        while ((item = katze_array_get_nth_item (browser->bookmarks, i++)))
        {
            if (KATZE_IS_ARRAY (item) && item != bookmark)
            {
                const gchar* name = katze_item_get_name (item);
                gtk_list_store_insert_with_values (model, NULL, G_MAXINT,
                    0, name, 1, PANGO_ELLIPSIZE_END, -1);
                if (katze_item_get_parent (bookmark) == item)
                    gtk_combo_box_set_active (GTK_COMBO_BOX (combo_folder), n);
                n++;
            }
        }
        if (n < 2)
            gtk_widget_set_sensitive (combo_folder, FALSE);
        gtk_box_pack_start (GTK_BOX (hbox), combo_folder, TRUE, TRUE, 0);
        gtk_container_add (GTK_CONTAINER (content_area), hbox);
        gtk_widget_show_all (hbox);
    }

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 1);
    label = gtk_label_new (NULL);
    gtk_size_group_add_widget (sizegroup, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    check_toolbar = gtk_check_button_new_with_mnemonic (_("Show in the tool_bar"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_toolbar),
        katze_item_get_meta_string (bookmark, "toolbar") != NULL);
    gtk_box_pack_start (GTK_BOX (hbox), check_toolbar, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER (content_area), hbox);
    gtk_widget_show_all (hbox);

    check_app = NULL;
    if (!KATZE_IS_ARRAY (bookmark))
    {
        hbox = gtk_hbox_new (FALSE, 8);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 1);
        label = gtk_label_new (NULL);
        gtk_size_group_add_widget (sizegroup, label);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        check_app = gtk_check_button_new_with_mnemonic (_("Run as _web application"));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_app),
            katze_item_get_meta_string (bookmark, "app") != NULL);
        gtk_box_pack_start (GTK_BOX (hbox), check_app, TRUE, TRUE, 0);
        gtk_container_add (GTK_CONTAINER (content_area), hbox);
        gtk_widget_show_all (hbox);
    }

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        gchar* selected;
        KatzeArray* folder;

        katze_item_set_name (bookmark,
            gtk_entry_get_text (GTK_ENTRY (entry_title)));
        katze_item_set_text (bookmark,
            gtk_entry_get_text (GTK_ENTRY (entry_desc)));
        katze_item_set_meta_integer (bookmark, "toolbar",
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_toolbar))
            ? 1 : -1);
        if (!KATZE_IS_ARRAY (bookmark))
        {
            katze_item_set_uri (bookmark,
                gtk_entry_get_text (GTK_ENTRY (entry_uri)));
            katze_item_set_meta_integer (bookmark, "app",
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_app))
                ? 1 : -1);
        }

        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_toolbar)))
            if (!GTK_WIDGET_VISIBLE (browser->bookmarkbar))
                _action_set_active (browser, "Bookmarkbar", TRUE);

        folder = browser->bookmarks;
        selected = gtk_combo_box_get_active_text (GTK_COMBO_BOX (combo_folder));
        if (strcmp (selected, _("Toplevel folder")))
        {
            guint i = 0;
            KatzeItem* item;
            while ((item = katze_array_get_nth_item (browser->bookmarks, i++)))
                if (KATZE_IS_ARRAY (item))
                    if (!g_strcmp0 (katze_item_get_name (item), selected))
                    {
                        folder = KATZE_ARRAY (item);
                        break;
                    }
        }
        g_free (selected);
        if (folder != katze_item_get_parent (bookmark) && folder != browser->bookmarks)
        {
            katze_array_remove_item (katze_item_get_parent (bookmark), bookmark);
            new_bookmark = TRUE;
        }
        if (new_bookmark)
            katze_array_add_item (folder, bookmark);
    }
    gtk_widget_destroy (dialog);
}

static void
midori_view_add_bookmark_cb (GtkWidget*   menuitem,
                             const gchar* uri,
                             GtkWidget*   view)
{
    KatzeItem* item;
    MidoriBrowser* browser;

    item = katze_item_new ();
    katze_item_set_uri (item, uri);
    browser = midori_browser_get_for_widget (menuitem);
    midori_browser_edit_bookmark_dialog_new (browser, item, TRUE, FALSE);
}

#if !WEBKIT_CHECK_VERSION (1, 1, 3)
static void
midori_browser_save_transfer_cb (KatzeNetRequest* request,
                                 gchar*           filename)
{
    FILE* fp;
    size_t ret;

    if (request->data)
    {
        if ((fp = fopen (filename, "wb")))
        {
            ret = fwrite (request->data, 1, request->length, fp);
            fclose (fp);
            if ((ret - request->length) != 0)
            {
                /* Once we have a download interface this should be
                   indicated graphically. */
                g_warning ("Error writing to file %s "
                           "in midori_browser_save_transfer_cb", filename);
            }
        }
    }
    g_free (filename);
}
#endif

static void
midori_browser_save_uri (MidoriBrowser* browser,
                         const gchar*   uri)
{
    static gchar* last_dir = NULL;
    gboolean folder_set = FALSE;
    GtkWidget* dialog;
    gchar* filename;
    gchar* dirname;
    gchar* last_slash;
    gchar* folder;

    if (!GTK_WIDGET_VISIBLE (browser))
        return;

    dialog = sokoke_file_chooser_dialog_new (_("Save file as"),
        GTK_WINDOW (browser), GTK_FILE_CHOOSER_ACTION_SAVE);

    if (uri)
    {
        /* Base the start folder on the current view's uri if it is local */
        filename = g_filename_from_uri (uri, NULL, NULL);
        if (filename)
        {
            dirname = g_path_get_dirname (filename);
            if (dirname && g_file_test (dirname, G_FILE_TEST_IS_DIR))
            {
                gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), dirname);
                folder_set = TRUE;
            }

            g_free (dirname);
            g_free (filename);
        }

        /* Try to provide a good default filename */
        filename = g_filename_from_uri (uri, NULL, NULL);
        if (!filename && (last_slash = g_strrstr (uri, "/")))
        {
            if (last_slash[0] == '/')
                last_slash++;
            filename = g_strdup (last_slash);
        }
        else
            filename = g_strdup (uri);
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), filename);
        g_free (filename);
    }

    if (!folder_set && last_dir && *last_dir)
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), last_dir);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
        #if WEBKIT_CHECK_VERSION (1, 1, 3)
        WebKitNetworkRequest* request;
        WebKitDownload* download;
        gchar* destination;
        #endif

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
        #if WEBKIT_CHECK_VERSION (1, 1, 3)
        request = webkit_network_request_new (uri);
        download = webkit_download_new (request);
        g_object_unref (request);
        destination = g_filename_to_uri (filename, NULL, NULL);
        webkit_download_set_destination_uri (download, destination);
        g_free (destination);
        midori_browser_add_download_item (browser, download);
        webkit_download_start (download);
        #else
        katze_net_load_uri (browser->net, uri, NULL,
            (KatzeNetTransferCb)midori_browser_save_transfer_cb, filename);
        #endif

        g_free (last_dir);
        last_dir = folder;
    }
    gtk_widget_destroy (dialog);
}

static void
midori_view_save_as_cb (GtkWidget*   menuitem,
                        const gchar* uri,
                        GtkWidget*   view)
{
    MidoriBrowser* browser;

    browser = midori_browser_get_for_widget (menuitem);
    midori_browser_save_uri (browser, uri);
}

static gchar*
midori_browser_speed_dial_get_next_free_slot (void)
{
    GRegex* regex;
    GMatchInfo* match_info;
    gchar* speed_dial_body;
    gchar* body_fname;
    gchar* slot_id = NULL;

    body_fname = g_build_filename (sokoke_set_config_dir (NULL),
                                   "speeddial.json", NULL);

    if (g_access (body_fname, F_OK) != 0)
    {
        gchar* filename = g_build_filename ("midori", "res", "speeddial.json", NULL);
        gchar* filepath = sokoke_find_data_filename (filename);
        g_free (filename);
        if (g_file_get_contents (filepath, &speed_dial_body, NULL, NULL))
        {
            g_file_set_contents (body_fname, speed_dial_body, -1, NULL);

            g_free (speed_dial_body);
        }
        g_free (filepath);
        g_free (body_fname);
        return g_strdup ("s1");
    }
    else
        g_file_get_contents (body_fname, &speed_dial_body, NULL, NULL);

    regex = g_regex_new ("\"id\":\"(s[1-9])\",\"href\":\"#\"",
                         G_REGEX_MULTILINE, 0, NULL);

    if (g_regex_match (regex, speed_dial_body, 0, &match_info))
    {
        slot_id = g_match_info_fetch (match_info, 1);
        g_match_info_free (match_info);
    }

    if (!g_strcmp0 (slot_id, ""))
        g_free (slot_id);

    g_free (body_fname);
    g_free (speed_dial_body);
    g_free (regex);

    return slot_id;
}

static void
midori_browser_add_speed_dial (MidoriBrowser* browser)
{
    GdkPixbuf* img;
    gchar* replace_from;
    gchar* replace_by;
    gsize len;

    GtkWidget* view = midori_browser_get_current_tab (browser);

    gchar* uri = g_strdup (midori_view_get_display_uri (MIDORI_VIEW (view)));
    gchar* title = g_strdup (midori_view_get_display_title (MIDORI_VIEW (view)));
    gchar* slot_id = midori_browser_speed_dial_get_next_free_slot ();

    GRegex* reg_quotes = g_regex_new ("'", 0, 0, NULL);
    GRegex* reg_others = g_regex_new ("[\\\"\\\\]", 0, 0, NULL);
    gchar* temp_title = g_regex_replace_literal (reg_others, title,
                                                 -1, 0, " ", 0, NULL);
    g_free (title);
    title = g_regex_replace_literal (reg_quotes, temp_title, -1, 0,
                                     "\\\\'", 0, NULL);

    g_free (temp_title);
    g_regex_unref (reg_quotes);
    g_regex_unref (reg_others);

    if (slot_id == NULL)
    {
        g_free (uri);
        g_free (title);
        return;
    }

    if ((len = g_utf8_strlen (title, -1)) > 15)
    {
        /**
          * The case when a quote was escaped with a backslash and the
          * backslash becomes the last character of the ellipsized string.
          * This causes JSON parsing to fail.
          * For example: "My Foo Bar \'b\..."
          **/
        GRegex* reg_unsafe = g_regex_new ("([\\\\]+\\.)", 0, 0, NULL);

        gchar* temp;
        gchar* ellipsized = g_malloc0 ( len + 1);

        g_utf8_strncpy (ellipsized, title, 15);
        g_free (title);

        temp = g_strdup_printf ("%s...", ellipsized);
        g_free  (ellipsized);

        title = g_regex_replace_literal (reg_unsafe, temp, -1, 0, ".", 0, NULL);
        g_free (temp);

        g_regex_unref (reg_unsafe);
    }

    if ((img = midori_view_get_snapshot (MIDORI_VIEW (view), 160, 107)))
    {
        GRegex* regex;
        gchar* replace;
        gchar* file_content;
        gchar* encoded;
        gchar* speed_dial_body;
        gchar* body_fname;
        gsize sz;

        body_fname = g_build_filename (sokoke_set_config_dir (NULL),
                                       "speeddial.json", NULL);

        if (g_file_get_contents (body_fname, &speed_dial_body, NULL, NULL))
        {
            gint i;

            gdk_pixbuf_save_to_buffer (img, &file_content, &sz, "png", NULL, NULL);
            encoded = g_base64_encode ((guchar *)file_content, sz);

            replace_from = g_strdup_printf (
                "\\{\"id\":\"%s\",\"href\":\"#\",\"title\":\"\",\"img\":\"\"\\}",
                slot_id);
            replace_by = g_strdup_printf (
                "{\"id\":\"%s\",\"href\":\"%s\",\"title\":\"%s\",\"img\":\"%s\"}",
                slot_id, uri, title, encoded);

            regex = g_regex_new (replace_from, G_REGEX_MULTILINE, 0, NULL);
            replace = g_regex_replace (regex, speed_dial_body, -1,
                                       1, replace_by, 0, NULL);

            g_file_set_contents (body_fname, replace, -1, NULL);

            i = 0;
            while ((view = gtk_notebook_get_nth_page (GTK_NOTEBOOK (
                                                      browser->notebook), i++)))
                if (midori_view_is_blank (MIDORI_VIEW (view)))
                    midori_view_reload (MIDORI_VIEW (view), FALSE);

            g_object_unref (img);
            g_regex_unref (regex);
            g_free (encoded);
            g_free (file_content);
            g_free (speed_dial_body);
            g_free (replace_from);
            g_free (replace_by);
            g_free (replace);
        }
        g_free (body_fname);
    }
}


static void
midori_view_add_speed_dial_cb (GtkWidget*   menuitem,
                              const gchar* uri,
                              GtkWidget*   view)
{
    MidoriBrowser* browser;

    browser = midori_browser_get_for_widget (menuitem);
    midori_browser_add_speed_dial (browser);
}


static gboolean
midori_browser_tab_leave_notify_event_cb (GtkWidget*        widget,
                                          GdkEventCrossing* event,
                                          MidoriBrowser*    browser)
{
    _midori_browser_set_statusbar_text (browser, NULL);
    return TRUE;
}

static void
midori_view_activate_action_cb (GtkWidget*     view,
                                const gchar*   action,
                                MidoriBrowser* browser)
{
    _midori_browser_activate_action (browser, action);
}

static void
midori_view_attach_inspector_cb (GtkWidget*     view,
                                 GtkWidget*     inspector_view,
                                 MidoriBrowser* browser)
{
    GtkWidget* toplevel;
    GtkWidget* scrolled;

    toplevel = gtk_widget_get_toplevel (inspector_view);
    gtk_widget_hide (toplevel);
    scrolled = gtk_widget_get_parent (browser->inspector_view);
    gtk_widget_destroy (browser->inspector_view);
    gtk_widget_reparent (inspector_view, scrolled);
    gtk_widget_show_all (browser->inspector);
    browser->inspector_view = inspector_view;
    gtk_widget_destroy (toplevel);
}

static void
midori_browser_view_copy_history (GtkWidget* view_to,
                                  GtkWidget* view_from,
                                  gboolean   omit_last)
{
    WebKitWebView* copy_from;
    WebKitWebBackForwardList* list_from;
    WebKitWebView* copy_to;
    WebKitWebBackForwardList* list_to;
    guint length_from;
    gint i;

    copy_from = WEBKIT_WEB_VIEW (gtk_bin_get_child (GTK_BIN (view_from)));
    list_from = webkit_web_view_get_back_forward_list (copy_from);
    copy_to = WEBKIT_WEB_VIEW (gtk_bin_get_child (GTK_BIN (view_to)));
    list_to = webkit_web_view_get_back_forward_list (copy_to);
    length_from = webkit_web_back_forward_list_get_back_length (list_from);

    g_return_if_fail (!webkit_web_back_forward_list_get_back_length (list_to));

    for (i = -length_from; i <= (omit_last ? -1 : 0); i++)
    {
        webkit_web_back_forward_list_add_item (list_to,
            webkit_web_back_forward_list_get_nth_item (list_from, i));
    }
}

static void
midori_view_new_tab_cb (GtkWidget*     view,
                        const gchar*   uri,
                        gboolean       background,
                        MidoriBrowser* browser)
{
    gint n = midori_browser_add_uri (browser, uri);
    midori_browser_view_copy_history (midori_browser_get_nth_tab (browser, n),
                                      view, FALSE);

    if (!background)
        midori_browser_set_current_page (browser, n);
}

static void
midori_view_new_window_cb (GtkWidget*     view,
                           const gchar*   uri,
                           MidoriBrowser* browser)
{
    MidoriBrowser* new_browser;
    g_signal_emit (browser, signals[NEW_WINDOW], 0, NULL, &new_browser);
    midori_browser_add_uri (new_browser, uri);
}

static void
midori_view_new_view_cb (GtkWidget*     view,
                         GtkWidget*     new_view,
                         MidoriNewView  where,
                         MidoriBrowser* browser)
{
    midori_browser_view_copy_history (new_view, view, TRUE);
    if (where == MIDORI_NEW_VIEW_WINDOW)
    {
        MidoriBrowser* new_browser;
        g_signal_emit (browser, signals[NEW_WINDOW], 0, NULL, &new_browser);
        midori_browser_add_tab (new_browser, new_view);
        midori_browser_set_current_tab (new_browser, new_view);
    }
    else if (gtk_widget_get_parent (new_view) != browser->notebook)
    {
        gint n = midori_browser_add_tab (browser, new_view);
        if (where != MIDORI_NEW_VIEW_BACKGROUND)
            midori_browser_set_current_page (browser, n);
    }
}

#if WEBKIT_CHECK_VERSION (1, 1, 3)
static void
midori_browser_download_notify_progress_cb (WebKitDownload* download,
                                            GParamSpec*     pspec,
                                            GtkWidget*      progress)
{
    gchar* current;
    gchar* total;
    gchar* size_text;
    gchar* text;

    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
        webkit_download_get_progress (download));

    current = g_format_size_for_display (webkit_download_get_current_size (download));
    total = g_format_size_for_display (webkit_download_get_total_size (download));
    size_text = g_strdup_printf (_("%s of %s"), current, total);
    g_free (current);
    g_free (total);
    text = g_strdup_printf ("%s (%s)",
        gtk_progress_bar_get_text (GTK_PROGRESS_BAR (progress)),
        size_text);
    gtk_widget_set_tooltip_text (progress, text);
}

static void
midori_browser_download_notify_status_cb (WebKitDownload* download,
                                          GParamSpec*     pspec,
                                          GtkWidget*      button)
{
    GtkWidget* icon;

    switch (webkit_download_get_status (download))
    {
        case WEBKIT_DOWNLOAD_STATUS_FINISHED:
        {
            MidoriBrowser* browser = midori_browser_get_for_widget (button);

            icon = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
            gtk_button_set_image (GTK_BUTTON (button), icon);
            if (g_object_get_data (G_OBJECT (download), "open-download"))
                gtk_button_clicked (GTK_BUTTON (button));
            else
                g_object_set_data (G_OBJECT (gtk_widget_get_parent (button)),
                                   "done", (void*)1);

            if (katze_object_get_boolean (browser->settings, "notify-transfer-completed"))
            {
                const gchar* uri = webkit_download_get_destination_uri (download);
                gchar* path = soup_uri_decode (uri);
                gchar* filename = g_strrstr (path, "/") + 1;
                gchar* msg = g_strdup_printf (
                    _("The file '<b>%s</b>' has been downloaded."), filename);
                g_free (path);

                g_signal_emit (browser, signals[SEND_NOTIFICATION], 0,
                    _("Transfer completed"), msg);

                g_free (msg);
            }
            break;
        }
        case WEBKIT_DOWNLOAD_STATUS_CANCELLED:
        case WEBKIT_DOWNLOAD_STATUS_ERROR:
            icon = gtk_image_new_from_stock (GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU);
            gtk_button_set_image (GTK_BUTTON (button), icon);
            g_object_set_data (G_OBJECT (gtk_widget_get_parent (button)),
                               "done", (void*)1);
            break;
        default:
            break;
    }
}

static void
midori_browser_download_button_clicked_cb (GtkWidget*      button,
                                           WebKitDownload* download)
{
    MidoriBrowser* browser;
    GList* buttons;

    browser = midori_browser_get_for_widget (button);

    switch (webkit_download_get_status (download))
    {
        case WEBKIT_DOWNLOAD_STATUS_STARTED:
            webkit_download_cancel (download);
            g_object_set_data (G_OBJECT (gtk_widget_get_parent (button)),
                               "done", (void*)1);
            break;
        case WEBKIT_DOWNLOAD_STATUS_FINISHED:
        {
            const gchar* uri = webkit_download_get_destination_uri (download);
            if (sokoke_show_uri (gtk_widget_get_screen (button),
                uri, gtk_get_current_event_time (), NULL))
                gtk_widget_destroy (gtk_widget_get_parent (button));
            break;
        }
        case WEBKIT_DOWNLOAD_STATUS_CANCELLED:
            gtk_widget_destroy (gtk_widget_get_parent (button));
        default:
            break;
    }

    buttons = gtk_container_get_children (GTK_CONTAINER (browser->transferbar));
    if (g_list_length (buttons) == 1)
        gtk_widget_hide (browser->transferbar_clear);
    g_list_free (buttons);
}

static void
midori_browser_add_download_item (MidoriBrowser*  browser,
                                  WebKitDownload* download)
{
    GtkWidget* box;
    GtkWidget* icon;
    GtkWidget* button;
    GtkWidget* progress;
    const gchar* uri;
    gint width;

    box = gtk_hbox_new (FALSE, 0);
    /* icon = gtk_image_new_from_stock (STOCK_TRANSFER, GTK_ICON_SIZE_MENU);
    gtk_box_pack_start (GTK_BOX (box), icon, FALSE, FALSE, 0); */
    progress = gtk_progress_bar_new ();
    gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (progress),
                                    PANGO_ELLIPSIZE_MIDDLE);
    if ((uri = webkit_download_get_destination_uri (download)))
    {
        gchar* path = soup_uri_decode (uri);
        gchar* filename = g_strrstr (path, "/") + 1;
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), filename);
        g_free (path);
    }
    else
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress),
            webkit_download_get_suggested_filename (download));
    sokoke_widget_get_text_size (progress, "M", &width, NULL);
    gtk_widget_set_size_request (progress, width * 10, -1);
    /* Avoid a bug in WebKit */
    if (webkit_download_get_status (download) != WEBKIT_DOWNLOAD_STATUS_CREATED)
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
            webkit_download_get_progress (download));
    gtk_box_pack_start (GTK_BOX (box), progress, FALSE, FALSE, 0);
    icon = gtk_image_new_from_stock (GTK_STOCK_CANCEL, GTK_ICON_SIZE_MENU);
    button = gtk_button_new ();
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
    gtk_container_add (GTK_CONTAINER (button), icon);
    gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
    gtk_widget_show_all (box);
    gtk_box_pack_start (GTK_BOX (browser->transferbar), box, FALSE, FALSE, 0);
    gtk_widget_show (browser->transferbar_clear);

    g_signal_connect (download, "notify::progress",
        G_CALLBACK (midori_browser_download_notify_progress_cb), progress);
    g_signal_connect (download, "notify::status",
        G_CALLBACK (midori_browser_download_notify_status_cb), button);
    g_signal_connect (button, "clicked",
        G_CALLBACK (midori_browser_download_button_clicked_cb), download);
}

static void
midori_view_download_save_as_response_cb (GtkWidget*      dialog,
                                          gint            response,
                                          MidoriBrowser*  browser)
{
    WebKitDownload* download = g_object_get_data (G_OBJECT (dialog), "download");
    if (response == GTK_RESPONSE_OK)
    {
        gchar* uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
        webkit_download_set_destination_uri (download, uri);
        midori_browser_add_download_item (browser, download);
        g_free (uri);
        webkit_download_start (download);
    }
    else
        g_object_unref (download);
    gtk_widget_hide (dialog);
}

static gboolean
midori_view_download_requested_cb (GtkWidget*      view,
                                   WebKitDownload* download,
                                   MidoriBrowser*  browser)
{
/*
    g_signal_emit (browser, signals[ADD_DOWNLOAD], 0, download);
    if (!webkit_download_get_destination_uri (download))
    {
        gchar* folder;
        if (g_object_get_data (G_OBJECT (download), "save-as-download"))
        {
            static GtkWidget* dialog = NULL;

            if (!dialog)
            {
                dialog = sokoke_file_chooser_dialog_new (_("Save file"),
                    GTK_WINDOW (browser), GTK_FILE_CHOOSER_ACTION_SAVE);
                gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
                folder = katze_object_get_string (browser->settings, "download-folder");
                gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), folder);
                g_free (folder);
                g_signal_connect (dialog, "response",
                    G_CALLBACK (midori_view_download_save_as_response_cb), browser);
            }
            g_object_set_data (G_OBJECT (dialog), "download", download);
            gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                webkit_download_get_suggested_filename (download));
            gtk_widget_show (dialog);
        }
        else
        {
            gchar* filename;
            gchar* uri;

            if (g_object_get_data (G_OBJECT (download), "open-download"))
                folder = g_strdup (g_get_tmp_dir ());
            else
                folder = katze_object_get_string (browser->settings, "download-folder");
            filename = g_build_filename (folder,
                webkit_download_get_suggested_filename (download), NULL);
            g_free (folder);
            uri = g_filename_to_uri (filename, NULL, NULL);
            g_free (filename);
            webkit_download_set_destination_uri (download, uri);
            g_free (uri);
            midori_browser_add_download_item (browser, download);
        }
    }
    return TRUE;
*/
    g_signal_emit (browser, signals[ADD_DOWNLOAD], 0, download);
    if (!webkit_download_get_destination_uri (download))
    {
        gchar* folder;
        if (g_object_get_data (G_OBJECT (download), "save-as-download"))
        {
            static GtkWidget* dialog = NULL;

            if (!dialog)
            {
                dialog = sokoke_file_chooser_dialog_new (_("Save file"),
                    GTK_WINDOW (browser), GTK_FILE_CHOOSER_ACTION_SAVE);
                gtk_file_chooser_set_do_overwrite_confirmation (
                    GTK_FILE_CHOOSER (dialog), TRUE);
                gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
                folder = katze_object_get_string (browser->settings, "download-folder");
                gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), folder);
                g_free (folder);
                g_signal_connect (dialog, "destroy",
                                  G_CALLBACK (gtk_widget_destroyed), &dialog);
                g_signal_connect (dialog, "response",
                    G_CALLBACK (midori_view_download_save_as_response_cb), browser);
            }
            g_object_set_data (G_OBJECT (dialog), "download", download);
            gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                webkit_download_get_suggested_filename (download));
            gtk_widget_show (dialog);
        }
        else
        {
            const gchar* suggested;
            gchar* basename;
            gchar* filename;
            gchar* uri;

            if (g_object_get_data (G_OBJECT (download), "open-download"))
                folder = g_strdup (g_get_tmp_dir ());
            else
                folder = katze_object_get_string (browser->settings, "download-folder");
            suggested = webkit_download_get_suggested_filename (download);

	    size_t suggestedLen;
	    if (!strcmp("ibseReader", GetDefaultUserAgent()) && (suggestedLen = strlen(suggested)) > 3 &&
		!strcmp(".acsm", suggested + suggestedLen - 5))
	    {
		struct stat folderstat;
		gchar* new_folder = g_strconcat(folder, "/licenze", NULL);
		g_free(folder);
		folder = new_folder;
		if (stat(folder, &folderstat) == -1)
			mkdir(folder, 0777);
	    }

            /* The suggested name may contain a folder name */
            basename = g_path_get_basename (suggested);
            filename = g_build_filename (folder, basename, NULL);
            g_free (basename);
            /* If the filename exists, choose a different name  */
            if (g_access (filename, F_OK) == 0)
            {
                /* Put the number in front of the extension */
                gchar* extension = strrchr (filename, '.');
                gsize length = extension ? (gsize)(extension - filename) : strlen (filename);
                do
                {
                    if (g_ascii_isdigit (filename[length - 1]))
                        filename[length - 1] += 1; /* FIXME: This will increment '9' to ':' */
                    else
                    {
                        gchar* new_filename;
                        if (extension)
                        {
                            /* Change the '.' to a '\0' to put the 0 in between */
                            *extension++ = '\0';
                            new_filename= g_strconcat (filename, "0.", extension, NULL);
                        }
                        else
                            new_filename = g_strconcat (filename, "0", NULL);
                        katze_assign (filename, new_filename);
                        if (extension)
                        {
                            extension = strrchr (filename, '.');
                            length = extension - filename;
                        }
                        else
                            length = strlen (filename);
                    }
                }
                while (g_access (filename, F_OK) == 0);
            }
            g_free (folder);
            uri = g_filename_to_uri (filename, NULL, NULL);
            g_free (filename);
	    webkit_download_set_destination_uri (download, uri);
            g_free (uri);
	    midori_browser_add_download_item (browser, download);
        }
    }
    return TRUE;
}
#endif

static void
midori_browser_transferbar_clear_clicked_cb (GtkWidget*     button,
                                             MidoriBrowser* browser)
{
    GList* buttons;
    guint i;
    GtkWidget* item;

    buttons = gtk_container_get_children (GTK_CONTAINER (browser->transferbar));
    i = 0;
    while ((item = g_list_nth_data (buttons, i++)))
        if (g_object_get_data (G_OBJECT (item), "done"))
            gtk_widget_destroy (item);
    g_list_free (buttons);
    buttons = gtk_container_get_children (GTK_CONTAINER (browser->transferbar));
    if (g_list_length (buttons) == 1)
        gtk_widget_hide (browser->transferbar_clear);
    g_list_free (buttons);
}

static void
midori_view_search_text_cb (GtkWidget*     view,
                            gboolean       found,
                            gchar*         typing,
                            MidoriBrowser* browser)
{
    const gchar* text;
    gboolean case_sensitive;
    gboolean highlight;

    if (typing)
    {
        gint position = -1;

        browser->find_typing = TRUE;
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (
            browser->find_highlight), TRUE);
        gtk_widget_hide (GTK_WIDGET (browser->find_case));
        gtk_widget_hide (GTK_WIDGET (browser->find_highlight));
        gtk_widget_hide (GTK_WIDGET (browser->find_close));
        if (!GTK_WIDGET_VISIBLE (browser->find))
            gtk_entry_set_text (GTK_ENTRY (browser->find_text), "");
        gtk_widget_show (browser->find);
        gtk_window_set_focus (GTK_WINDOW (browser), browser->find_text);
        gtk_editable_insert_text (GTK_EDITABLE (browser->find_text), typing, -1, &position);
        gtk_editable_set_position (GTK_EDITABLE (browser->find_text), -1);
    }
    if (GTK_WIDGET_VISIBLE (browser->find) && !typing)
    {
        #if !HAVE_HILDON
        gtk_icon_entry_set_icon_from_stock (GTK_ICON_ENTRY (browser->find_text),
            GTK_ICON_ENTRY_PRIMARY, (found) ? GTK_STOCK_FIND : GTK_STOCK_STOP);
        #endif
        text = gtk_entry_get_text (GTK_ENTRY (browser->find_text));
        case_sensitive = gtk_toggle_tool_button_get_active (
            GTK_TOGGLE_TOOL_BUTTON (browser->find_case));
        midori_view_mark_text_matches (MIDORI_VIEW (view), text, case_sensitive);
        highlight = gtk_toggle_tool_button_get_active (
            GTK_TOGGLE_TOOL_BUTTON (browser->find_highlight));
        midori_view_set_highlight_text_matches (MIDORI_VIEW (view), highlight);
    }
}

static gboolean
midori_browser_tab_destroy_cb (GtkWidget*     widget,
                               MidoriBrowser* browser)
{
    KatzeItem* item;
    const gchar* uri;

    if (browser->proxy_array && MIDORI_IS_VIEW (widget))
    {
        item = midori_view_get_proxy_item (MIDORI_VIEW (widget));
        uri = katze_item_get_uri (item);
        if (browser->trash && !midori_view_is_blank (MIDORI_VIEW (widget)))
            katze_array_add_item (browser->trash, item);
        katze_array_remove_item (browser->proxy_array, item);
        g_object_unref (item);
    }

    _midori_browser_update_actions (browser);

    /* This callback must only be called once, but we need to ensure
       that "remove-tab" is emitted in any case */
    g_signal_handlers_disconnect_by_func (widget,
        midori_browser_tab_destroy_cb, browser);

    g_signal_emit (browser, signals[REMOVE_TAB], 0, widget);

    /* We don't ever want to be in a situation with no tabs,
       so just create an empty one if the last one is closed.
       The only exception is when we are closing the window,
       which is indicated by the proxy array having been unset. */
    if (browser->proxy_array && !midori_browser_get_current_tab (browser))
        midori_browser_add_uri (browser, "");
    return FALSE;
}

static void
_midori_browser_add_tab (MidoriBrowser* browser,
                         GtkWidget*     view)
{
    GtkNotebook* notebook = GTK_NOTEBOOK (browser->notebook);
    GtkWidget* tab_label;
    KatzeItem* item;
    guint n;

    GTK_WIDGET_SET_FLAGS (view, GTK_CAN_FOCUS);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (view),
                                         GTK_SHADOW_ETCHED_IN);

    tab_label = midori_view_get_proxy_tab_label (MIDORI_VIEW (view));

    item = midori_view_get_proxy_item (MIDORI_VIEW (view));
    g_object_ref (item);
    katze_array_add_item (browser->proxy_array, item);

    g_object_connect (view,
                      "signal::notify::icon",
                      midori_view_notify_icon_cb, browser,
                      "signal::notify::load-status",
                      midori_view_notify_load_status_cb, browser,
                      "signal::notify::progress",
                      midori_view_notify_progress_cb, browser,
                      "signal::context-ready",
                      midori_view_context_ready_cb, browser,
                      "signal::notify::uri",
                      midori_view_notify_uri_cb, browser,
                      "signal::notify::title",
                      midori_view_notify_title_cb, browser,
                      "signal::notify::zoom-level",
                      midori_view_notify_zoom_level_cb, browser,
                      "signal::notify::statusbar-text",
                      midori_view_notify_statusbar_text_cb, browser,
                      "signal::activate-action",
                      midori_view_activate_action_cb, browser,
                      "signal::attach-inspector",
                      midori_view_attach_inspector_cb, browser,
                      "signal::new-tab",
                      midori_view_new_tab_cb, browser,
                      "signal::new-window",
                      midori_view_new_window_cb, browser,
                      "signal::new-view",
                      midori_view_new_view_cb, browser,
                      #if WEBKIT_CHECK_VERSION (1, 1, 3)
                      "signal::download-requested",
                      midori_view_download_requested_cb, browser,
                      #endif
                      "signal::search-text",
                      midori_view_search_text_cb, browser,
                      "signal::add-bookmark",
                      midori_view_add_bookmark_cb, browser,
                      "signal::save-as",
                      midori_view_save_as_cb, browser,
                      "signal::add-speed-dial",
                      midori_view_add_speed_dial_cb, browser,
                      "signal::leave-notify-event",
                      midori_browser_tab_leave_notify_event_cb, browser,
                      NULL);

    if (!g_object_get_data (G_OBJECT (view), "midori-view-append") &&
        katze_object_get_boolean (browser->settings, "open-tabs-next-to-current"))
    {
        n = gtk_notebook_get_current_page (notebook);
        gtk_notebook_insert_page (notebook, view, tab_label, n + 1);
    }
    else
        gtk_notebook_append_page (notebook, view, tab_label);

    gtk_notebook_set_tab_reorderable (notebook, view, TRUE);
    gtk_notebook_set_tab_detachable (notebook, view, TRUE);

    /* We want the tab to be removed if the widget is destroyed */
    g_signal_connect (view, "destroy",
        G_CALLBACK (midori_browser_tab_destroy_cb), browser);

    _midori_browser_update_actions (browser);
}

static void
_midori_browser_remove_tab (MidoriBrowser* browser,
                            GtkWidget*     view)
{
    gtk_widget_destroy (view);
}

/**
 * midori_browser_foreach:
 * @browser: a #MidoriBrowser
 * @callback: a #GtkCallback
 * @callback_data: custom data
 *
 * Calls the specified callback for each view contained
 * in the browser.
 *
 * Since: 0.1.7
 **/
void
midori_browser_foreach (MidoriBrowser* browser,
                        GtkCallback    callback,
                        gpointer       callback_data)
{
  g_return_if_fail (MIDORI_IS_BROWSER (browser));

  gtk_container_foreach (GTK_CONTAINER (browser->notebook),
                         callback, callback_data);
}

static void
_midori_browser_quit (MidoriBrowser* browser)
{
    /* Nothing to do */
}

static gboolean
midori_browser_key_press_event (GtkWidget*   widget,
                                GdkEventKey* event)
{
    GtkWindow* window = GTK_WINDOW (widget);
    GtkWidgetClass* widget_class;
    
    if (event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))
        if (sokoke_window_activate_key (window, event))
            return TRUE;

    if (gtk_window_propagate_key_event (window, event))
        return TRUE;

    if (!(event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
        if (sokoke_window_activate_key (window, event))
            return TRUE;

    if (event->state && gtk_window_propagate_key_event (window, event))
        return TRUE;

    widget_class = g_type_class_peek_static (g_type_parent (GTK_TYPE_WINDOW));
    return widget_class->key_press_event (widget, event);
}

static void
midori_browser_class_init (MidoriBrowserClass* class)
{
    GtkWidgetClass* gtkwidget_class;
    GObjectClass* gobject_class;
    GParamFlags flags;

    /**
     * MidoriBrowser::new-window:
     * @browser: the object on which the signal is emitted
     * @window: a new browser window, or %NULL
     *
     * Emitted when a new browser window was created.
     *
     * Note: Before 0.1.7 the second argument was an URI string.
     *
     * Note: Since 0.2.1 the return value is a #MidoriBrowser
     *
     * Return value: a new #MidoriBrowser
     */
    signals[NEW_WINDOW] = g_signal_new (
        "new-window",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        G_STRUCT_OFFSET (MidoriBrowserClass, new_window),
        0,
        NULL,
        midori_cclosure_marshal_OBJECT__OBJECT,
        MIDORI_TYPE_BROWSER, 1,
        MIDORI_TYPE_BROWSER);

    signals[ADD_TAB] = g_signal_new (
        "add-tab",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET (MidoriBrowserClass, add_tab),
        0,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        GTK_TYPE_WIDGET);

    signals[REMOVE_TAB] = g_signal_new (
        "remove-tab",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET (MidoriBrowserClass, remove_tab),
        0,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        GTK_TYPE_WIDGET);

    signals[ACTIVATE_ACTION] = g_signal_new (
        "activate-action",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET (MidoriBrowserClass, activate_action),
        0,
        NULL,
        g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_STRING);

    signals[CONTEXT_READY] = g_signal_new (
        "context-ready",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1,
        G_TYPE_POINTER);

    /**
     * MidoriBrowser::add-download:
     * @browser: the object on which the signal is emitted
     * @download: a new download
     *
     * Emitted when a new download was accepted and is
     * about to start, before the browser adds items
     * to the transferbar.
     *
     * Emitting this signal manually is equal to a
     * user initiating and confirming a download
     *
     * Note: This requires WebKitGTK+ 1.1.3.
     *
     * Since: 0.1.5
     */
    signals[ADD_DOWNLOAD] = g_signal_new (
        "add-download",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        G_TYPE_OBJECT);

    /**
     * MidoriBrowser::send-notification:
     * @browser: the object on which the signal is emitted
     * @title: the title for the notification
     * @message: the message for the notification
     *
     * Emitted when a browser wants to display a notification message,
     * e.g. when a download has been completed.
     *
     * Since: 0.1.7
     */
    signals[SEND_NOTIFICATION] = g_signal_new (
        "send-notification",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        0,
        NULL,
        midori_cclosure_marshal_VOID__STRING_STRING,
        G_TYPE_NONE, 2,
        G_TYPE_STRING,
        G_TYPE_STRING);

    /**
     * MidoriBrowser::populate-tool-menu:
     * @browser: the object on which the signal is emitted
     * @menu: the #GtkMenu to populate
     *
     * Emitted when a Tool menu is displayed, such as the
     * toplevel Tools in the menubar or the compact menu.
     *
     * Since: 0.1.9
     */
    signals[POPULATE_TOOL_MENU] = g_signal_new (
        "populate-tool-menu",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        GTK_TYPE_MENU);

    signals[QUIT] = g_signal_new (
        "quit",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET (MidoriBrowserClass, quit),
        0,
        NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    class->add_tab = _midori_browser_add_tab;
    class->remove_tab = _midori_browser_remove_tab;
    class->activate_action = _midori_browser_activate_action;
    class->quit = _midori_browser_quit;

    gtkwidget_class = GTK_WIDGET_CLASS (class);
    gtkwidget_class->key_press_event = midori_browser_key_press_event;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->dispose = midori_browser_dispose;
    gobject_class->finalize = midori_browser_finalize;
    gobject_class->set_property = midori_browser_set_property;
    gobject_class->get_property = midori_browser_get_property;

    flags = G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS;

    g_object_class_install_property (gobject_class,
                                     PROP_MENUBAR,
                                     g_param_spec_object (
                                     "menubar",
                                     "Menubar",
                                     "The menubar",
                                     GTK_TYPE_MENU_BAR,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_NAVIGATIONBAR,
                                     g_param_spec_object (
                                     "navigationbar",
                                     "Navigationbar",
                                     "The navigationbar",
                                     GTK_TYPE_TOOLBAR,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_NOTEBOOK,
                                     g_param_spec_object (
                                     "notebook",
                                     "Notebook",
                                     "The notebook containing the views",
                                     GTK_TYPE_NOTEBOOK,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_PANEL,
                                     g_param_spec_object (
                                     "panel",
                                     "Panel",
                                     "The side panel embedded in the browser",
                                     MIDORI_TYPE_PANEL,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_URI,
                                     g_param_spec_string (
                                     "uri",
                                     "URI",
                                     "The current URI",
                                     "",
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_TAB,
                                     g_param_spec_object (
                                     "tab",
                                     "Tab",
                                     "The current tab",
                                     GTK_TYPE_WIDGET,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_LOAD_STATUS,
                                     g_param_spec_enum (
                                     "load-status",
                                     "Load Status",
                                     "The current load status",
                                     MIDORI_TYPE_LOAD_STATUS,
                                     MIDORI_LOAD_FINISHED,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:statusbar:
    *
    * The widget representing the statusbar contents. This is
    * not an actual #GtkStatusbar but rather a #GtkBox.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_STATUSBAR,
                                     g_param_spec_object (
                                     "statusbar",
                                     "Statusbar",
                                     "The statusbar",
                                     GTK_TYPE_BOX,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:statusbar-text:
    *
    * The text that is displayed in the statusbar.
    *
    * This value reflects changes to the text visible in the statusbar, such
    * as the uri of a hyperlink the mouse hovers over or the description of
    * a menuitem.
    *
    * Setting this value changes the displayed text until the next change.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_STATUSBAR_TEXT,
                                     g_param_spec_string (
                                     "statusbar-text",
                                     "Statusbar Text",
                                     "The text that is displayed in the statusbar",
                                     "",
                                     flags));

    /**
    * MidoriBrowser:settings:
    *
    * An associated settings instance that is shared among all web views.
    *
    * Setting this value is propagated to every present web view. Also
    * every newly created web view will use this instance automatically.
    *
    * If no settings are specified a default will be used.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_SETTINGS,
                                     g_param_spec_object (
                                     "settings",
                                     "Settings",
                                     "The associated settings",
                                     MIDORI_TYPE_WEB_SETTINGS,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:bookmarks:
    *
    * The bookmarks folder, containing all bookmarks.
    *
    * This is actually a reference to a bookmarks instance,
    * so if bookmarks should be used it must be initially set.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_BOOKMARKS,
                                     g_param_spec_object (
                                     "bookmarks",
                                     "Bookmarks",
                                     "The bookmarks folder, containing all bookmarks",
                                     KATZE_TYPE_ARRAY,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:trash:
    *
    * The trash, that collects all closed tabs and windows.
    *
    * This is actually a reference to a trash instance, so if a trash should
    * be used it must be initially set.
    *
    * Note: In the future the trash might collect other types of items.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_TRASH,
                                     g_param_spec_object (
                                     "trash",
                                     "Trash",
                                     "The trash, collecting recently closed tabs and windows",
                                     KATZE_TYPE_ARRAY,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:search-engines:
    *
    * The list of search engines to be used for web search.
    *
    * This is actually a reference to a search engines instance,
    * so if search engines should be used it must be initially set.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_SEARCH_ENGINES,
                                     g_param_spec_object (
                                     "search-engines",
                                     "Search Engines",
                                     "The list of search engines to be used for web search",
                                     KATZE_TYPE_ARRAY,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:history:
    *
    * The list of history items.
    *
    * This is actually a reference to a history instance,
    * so if history should be used it must be initially set.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_HISTORY,
                                     g_param_spec_object (
                                     "history",
                                     "History",
                                     "The list of history items",
                                     KATZE_TYPE_ARRAY,
                                     flags));

    /**
     * MidoriBrowser:show-tabs:
     *
     * Whether or not to show tabs.
     *
     * If disabled, no tab labels are shown. This is intended for
     * extensions that want to provide alternative tab labels.
     *
     * Since 0.1.8
    */
    g_object_class_install_property (gobject_class,
                                     PROP_SHOW_TABS,
                                     g_param_spec_boolean (
                                     "show-tabs",
                                     "Show Tabs",
                                     "Whether or not to show tabs",
                                     FALSE, /*Modified by SBielov: TRUE,*/
                                     flags));

    /* Add 2px space between tool buttons */
    gtk_rc_parse_string (
        "style \"tool-button-style\"\n {\n"
        "GtkToolButton::icon-spacing = 2\n }\n"
        "widget \"MidoriBrowser.*.MidoriBookmarkbar.Gtk*ToolButton\" "
        "style \"tool-button-style\"\n"
        "widget \"MidoriBrowser.*.MidoriFindbar.Gtk*ToolButton\" "
        "style \"tool-button-style\"\n");
}

static void
_action_window_new_activate (GtkAction*     action,
                             MidoriBrowser* browser)
{
    MidoriBrowser* new_browser;
    g_signal_emit (browser, signals[NEW_WINDOW], 0, NULL, &new_browser);
    if (new_browser)
    {
        midori_browser_add_uri (new_browser, "");
        midori_browser_activate_action (new_browser, "Location");
    }
}

static void
_action_tab_new_activate (GtkAction*     action,
                          MidoriBrowser* browser)
{
    gint n = midori_browser_add_uri (browser, "");
    midori_browser_set_current_page (browser, n);
}

static void
_action_private_browsing_activate (GtkAction*     action,
                                   MidoriBrowser* browser)
{
    const gchar* uri = midori_browser_get_current_uri (browser);
    if (uri != NULL)
    {
        /* FIXME: Use the same binary that is running right now */
        if (*uri != '\0')
            sokoke_spawn_program ("midori -a", uri, FALSE);
        else
            sokoke_spawn_program ("midori -a", "about:blank", FALSE);
    }
}

static void
_action_open_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    static gchar* last_dir = NULL;
    gchar* uri = NULL;
    gboolean folder_set = FALSE;
    GtkWidget* dialog;
    GtkWidget* view;

    if (!GTK_WIDGET_VISIBLE (browser))
        return;

    dialog = sokoke_file_chooser_dialog_new (_("Open file"),
        GTK_WINDOW (browser), GTK_FILE_CHOOSER_ACTION_OPEN);

     /* base the start folder on the current view's uri if it is local */
     view = midori_browser_get_current_tab (browser);
     if ((uri = (gchar*)midori_view_get_display_uri (MIDORI_VIEW (view))))
     {
         gchar* filename = g_filename_from_uri (uri, NULL, NULL);
         if (filename)
         {
             gchar* dirname = g_path_get_dirname (filename);
             if (dirname && g_file_test (dirname, G_FILE_TEST_IS_DIR))
             {
                 gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), dirname);
                 folder_set = TRUE;
             }

             g_free (dirname);
             g_free (filename);
         }
     }

     if (!folder_set && last_dir && *last_dir)
         gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), last_dir);

     if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
     {
         gchar* folder;

         folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
         uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
         midori_browser_set_current_uri (browser, uri);

         g_free (last_dir);
         last_dir = folder;
         g_free (uri);
     }
    gtk_widget_destroy (dialog);
}

static void
_action_save_as_activate (GtkAction*     action,
                          MidoriBrowser* browser)
{
    midori_browser_save_uri (browser, midori_browser_get_current_uri (browser));
}

static void
_action_add_speed_dial_activate (GtkAction*     action,
                                MidoriBrowser* browser)
{
    midori_browser_add_speed_dial (browser);
}

static void
_action_add_desktop_shortcut_activate (GtkAction*     action,
                                       MidoriBrowser* browser)
{
    #if defined (GDK_WINDOWING_X11)
    /* TODO: Implement */
    #elif defined(GDK_WINDOWING_QUARTZ)
    /* TODO: Implement */
    #elif defined (GDK_WINDOWING_WIN32)
    /* TODO: Implement */
    #elif HAVE_HILDON
    /* TODO: Implement */
    #endif
}

static void
midori_browser_subscribe_to_news_feed (MidoriBrowser* browser,
                                       const gchar*   uri)
{
    if (browser->news_aggregator && *browser->news_aggregator)
        sokoke_spawn_program (browser->news_aggregator, uri, FALSE);
    else
        sokoke_message_dialog (GTK_MESSAGE_INFO, _("New feed"), uri);
}

static void
_action_add_news_feed_activate (GtkAction*     action,
                                MidoriBrowser* browser)
{
    GtkWidget* view;
    const gchar* uri;

    if (!(view = midori_browser_get_current_tab (browser)))
        return;
    if (!(uri = g_object_get_data (G_OBJECT (view), "news-feeds")))
        return;

    midori_browser_subscribe_to_news_feed (browser, uri);
}

static void
_action_compact_add_response_cb (GtkWidget* dialog,
                                 gint       response,
                                 gpointer   data)
{
    gtk_widget_destroy (dialog);
}

static void
_action_compact_add_activate (GtkAction*     action,
                              MidoriBrowser* browser)
{
    GtkWidget* dialog;
    GtkBox* box;
    const gchar* actions[] = { "BookmarkAdd", "AddSpeedDial",
                               "AddDesktopShortcut", "AddNewsFeed" };
    guint i;

    if (!GTK_WIDGET_VISIBLE (browser))
        return;

    dialog = g_object_new (GTK_TYPE_DIALOG,
        "transient-for", browser,
        "title", _("Add a new bookmark"), NULL);
    box = GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog)));

    for (i = 0; i < G_N_ELEMENTS (actions); i++)
    {
        gchar* label;
        GtkWidget* button;

        action = _action_by_name (browser, actions[i]);
        label = katze_object_get_string (action, "label");
        button = gtk_button_new_with_mnemonic (label);
        g_free (label);
        gtk_widget_set_name (button, "GtkButton-thumb");
        gtk_box_pack_start (box, button, TRUE, TRUE, 4);
        #if GTK_CHECK_VERSION (2, 16, 0)
        gtk_activatable_set_related_action (GTK_ACTIVATABLE (button), action);
        #else
        gtk_action_connect_proxy (action, button);
        #endif
        g_signal_connect_swapped (button, "clicked",
                                  G_CALLBACK (gtk_widget_destroy), dialog);
    }

    gtk_widget_show (dialog);
    g_signal_connect (dialog, "response",
                              G_CALLBACK (_action_compact_add_response_cb), NULL);
}

static void
_action_tab_close_activate (GtkAction*     action,
                            MidoriBrowser* browser)
{
    GtkWidget* widget = midori_browser_get_current_tab (browser);
    gtk_widget_destroy (widget);
}

static void
_action_window_close_activate (GtkAction*     action,
                               MidoriBrowser* browser)
{
  gtk_widget_destroy (GTK_WIDGET (browser));
}

static void
_action_print_activate (GtkAction*     action,
                        MidoriBrowser* browser)
{
    GtkWidget* view;

    if (!GTK_WIDGET_VISIBLE (browser))
        return;

    if ((view = midori_browser_get_current_tab (browser)))
        midori_view_print (MIDORI_VIEW (view));
}

static void
_action_quit_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
  g_signal_emit (browser, signals[QUIT], 0);
}

static void
_action_edit_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    #if WEBKIT_CHECK_VERSION (1, 1, 14)
    gboolean can_undo = FALSE, can_redo = FALSE;
    #endif
    gboolean can_cut = FALSE, can_copy = FALSE, can_paste = FALSE;
    gboolean has_selection, can_select_all = FALSE;

    if (WEBKIT_IS_WEB_VIEW (widget))
    {
        WebKitWebView* view = WEBKIT_WEB_VIEW (widget);
        #if WEBKIT_CHECK_VERSION (1, 1, 14)
        can_undo = webkit_web_view_can_undo (view);
        can_redo = webkit_web_view_can_redo (view);
        #endif
        can_cut = webkit_web_view_can_cut_clipboard (view);
        can_copy = webkit_web_view_can_copy_clipboard (view);
        can_paste = webkit_web_view_can_paste_clipboard (view);
        can_select_all = TRUE;
    }
    else if (GTK_IS_EDITABLE (widget))
    {
        GtkEditable* editable = GTK_EDITABLE (widget);
        has_selection = gtk_editable_get_selection_bounds (editable, NULL, NULL);
        can_cut = has_selection && gtk_editable_get_editable (editable);
        can_copy = has_selection;
        can_paste = gtk_editable_get_editable (editable);
        can_select_all = TRUE;
    }
    else if (GTK_IS_TEXT_VIEW (widget))
    {
        GtkTextView* text_view = GTK_TEXT_VIEW (widget);
        GtkTextBuffer* buffer = gtk_text_view_get_buffer (text_view);
        has_selection = gtk_text_buffer_get_has_selection (buffer);
        can_cut = gtk_text_view_get_editable (text_view);
        can_copy = has_selection;
        can_paste = gtk_text_view_get_editable (text_view) && has_selection;
        can_select_all = TRUE;
    }

    #if WEBKIT_CHECK_VERSION (1, 1, 14)
    _action_set_sensitive (browser, "Undo", can_undo);
    _action_set_sensitive (browser, "Redo", can_redo);
    #endif
    _action_set_sensitive (browser, "Cut", can_cut);
    _action_set_sensitive (browser, "Copy", can_copy);
    _action_set_sensitive (browser, "Paste", can_paste);
    _action_set_sensitive (browser, "Delete", can_cut);
    _action_set_sensitive (browser, "SelectAll", can_select_all);
}

#if WEBKIT_CHECK_VERSION (1, 1, 14)
static void
_action_undo_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (WEBKIT_IS_WEB_VIEW (widget))
        webkit_web_view_undo (WEBKIT_WEB_VIEW (widget));
}

static void
_action_redo_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (WEBKIT_IS_WEB_VIEW (widget))
        webkit_web_view_redo (WEBKIT_WEB_VIEW (widget));
}
#endif

static void
_action_cut_activate (GtkAction*     action,
                      MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (G_LIKELY (widget) && g_signal_lookup ("cut-clipboard", G_OBJECT_TYPE (widget)))
        g_signal_emit_by_name (widget, "cut-clipboard");
}

static void
_action_copy_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (G_LIKELY (widget) && g_signal_lookup ("copy-clipboard", G_OBJECT_TYPE (widget)))
        g_signal_emit_by_name (widget, "copy-clipboard");
}

static void
_action_paste_activate (GtkAction*     action,
                        MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (G_LIKELY (widget) && g_signal_lookup ("paste-clipboard", G_OBJECT_TYPE (widget)))
        g_signal_emit_by_name (widget, "paste-clipboard");
}

static void
_action_delete_activate (GtkAction*     action,
                         MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (G_LIKELY (widget))
    {
        if (WEBKIT_IS_WEB_VIEW (widget))
            webkit_web_view_delete_selection (WEBKIT_WEB_VIEW (widget));
        else if (GTK_IS_EDITABLE (widget))
            gtk_editable_delete_selection (GTK_EDITABLE (widget));
        else if (GTK_IS_TEXT_VIEW (widget))
            gtk_text_buffer_delete_selection (
                gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)), TRUE, FALSE);
    }
}

static void
_action_select_all_activate (GtkAction*     action,
                             MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (G_LIKELY (widget))
    {
        if (GTK_IS_EDITABLE (widget))
            gtk_editable_select_region (GTK_EDITABLE (widget), 0, -1);
        else if (g_signal_lookup ("select-all", G_OBJECT_TYPE (widget)))
        {
            if (GTK_IS_TEXT_VIEW (widget))
                g_signal_emit_by_name (widget, "select-all", TRUE);
            else if (GTK_IS_TREE_VIEW (widget))
            {
                gboolean dummy;
                g_signal_emit_by_name (widget, "select-all", &dummy);
            }
            else
                g_signal_emit_by_name (widget, "select-all");
        }
    }
}

static void
_action_find_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    if (GTK_WIDGET_VISIBLE (browser->find))
        _midori_browser_find_done (browser);
    else
    {
        GtkWidget* view;
        const gchar* text;

        #if !HAVE_HILDON
        gtk_icon_entry_set_icon_from_stock (GTK_ICON_ENTRY (browser->find_text),
                                            GTK_ICON_ENTRY_PRIMARY, GTK_STOCK_FIND);
        gtk_widget_show (GTK_WIDGET (browser->find_case));
        #endif
        gtk_widget_show (GTK_WIDGET (browser->find_highlight));
        gtk_widget_show (GTK_WIDGET (browser->find_close));
        view = midori_browser_get_current_tab (browser);
        if ((text = midori_view_get_selected_text (MIDORI_VIEW (view))))
            gtk_entry_set_text (GTK_ENTRY (browser->find_text), text);
        gtk_widget_show (browser->find);
    }

    gtk_widget_grab_focus (GTK_WIDGET (browser->find_text));
}

static void
_midori_browser_find (MidoriBrowser* browser,
                      gboolean       forward)
{
    const gchar* text;
    gboolean case_sensitive;
    GtkWidget* view;

    if (!(view = midori_browser_get_current_tab (browser)))
        return;

    text = gtk_entry_get_text (GTK_ENTRY (browser->find_text));
    case_sensitive = gtk_toggle_tool_button_get_active (
        GTK_TOGGLE_TOOL_BUTTON (browser->find_case));
    midori_view_search_text (MIDORI_VIEW (view), text, case_sensitive, forward);
}

static void
_midori_browser_find_done (MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    midori_view_unmark_text_matches (MIDORI_VIEW (view));
    gtk_widget_hide (browser->find);
    browser->find_typing = FALSE;
    gtk_window_set_focus (GTK_WINDOW (browser),
        gtk_bin_get_child (GTK_BIN (view)) ?
        gtk_bin_get_child (GTK_BIN (view)) : view);
}

static void
_action_find_next_activate (GtkAction*     action,
                            MidoriBrowser* browser)
{
  _midori_browser_find (browser, TRUE);
}

static void
midori_browser_find_text_changed_cb (GtkWidget*     entry,
                                     MidoriBrowser* browser)
{
    if (browser->find_typing)
    {
        GtkWidget* view = midori_browser_get_current_tab (browser);
        const gchar* text = gtk_entry_get_text (GTK_ENTRY (entry));
        midori_view_unmark_text_matches (MIDORI_VIEW (view));
        if (g_utf8_strlen (text, -1) > 2)
            _midori_browser_find (browser, TRUE);
    }
}

static gboolean
midori_browser_find_text_focus_out_event_cb (GtkWidget*     entry,
                                             GdkEventFocus* event,
                                             MidoriBrowser* browser)
{
    _midori_browser_find_done (browser);
    return FALSE;
}

static void
_action_find_previous_activate (GtkAction*     action,
                                MidoriBrowser* browser)
{
    _midori_browser_find (browser, FALSE);
}

static void
_find_highlight_toggled (GtkToggleToolButton* toolitem,
                         MidoriBrowser*       browser)
{
    GtkWidget* view;
    gboolean highlight;

    view = midori_browser_get_current_tab (browser);
    highlight = gtk_toggle_tool_button_get_active (toolitem);
    midori_view_set_highlight_text_matches (MIDORI_VIEW (view), highlight);
}

static gboolean
midori_browser_find_key_press_event_cb (GtkWidget*     toolbar,
                                        GdkEventKey*   event,
                                        MidoriBrowser* browser)
{
    if (event->keyval == GDK_Escape)
    {
        _midori_browser_find_done (browser);
        return TRUE;
    }

    return FALSE;
}

static void
midori_browser_find_button_close_clicked_cb (GtkWidget*     widget,
                                             MidoriBrowser* browser)
{
    _midori_browser_find_done (browser);
}

static void
midori_browser_navigationbar_notify_style_cb (GObject*       object,
                                              GParamSpec*    arg1,
                                              MidoriBrowser* browser)
{
    MidoriToolbarStyle toolbar_style;

    g_object_get (browser->settings, "toolbar-style", &toolbar_style, NULL);
    _midori_browser_set_toolbar_style (browser, toolbar_style);
}

static gboolean
midori_browser_toolbar_item_button_press_event_cb (GtkWidget*      toolitem,
                                                   GdkEventButton* event,
                                                   MidoriBrowser*  browser);

static void
_midori_browser_save_toolbar_items (MidoriBrowser* browser)
{
    GString* toolbar_items;
    GList* children;
    gchar* items;

    toolbar_items = g_string_new (NULL);
    children = gtk_container_get_children (GTK_CONTAINER (browser->navigationbar));
    for (; children != NULL; children = g_list_next (children))
    {
        GtkAction* action;

        action = gtk_activatable_get_related_action (GTK_ACTIVATABLE (children->data));
        /* If a widget has no action that is actually a bug, so warn about it */
        g_warn_if_fail (action != NULL);
        if (action)
        {
            g_string_append (toolbar_items, gtk_action_get_name (action));
            g_string_append (toolbar_items, ",");
        }
    }
    items = g_string_free (toolbar_items, FALSE);
    g_object_set (browser->settings, "toolbar-items", items, NULL);
    g_free (items);
}

/**
 * midori_browser_get_toolbar_actions:
 *
 * Retrieves a list of actions which are suitable for use in a toolbar.
 *
 * Return value: a NULL-terminated array of strings with actions
 *
 * Since: 0.1.8
 **/
const gchar**
midori_browser_get_toolbar_actions (MidoriBrowser* browser)
{
    static const gchar* actions[] = {
            "WindowNew", "TabNew", "Open", "SaveAs", "Print", "Find",
            "Fullscreen", "Preferences", "Window", "Bookmarks",
            "RecentlyVisited", "ReloadStop", "ZoomIn", "TabClose",
            "ZoomOut", "Separator", "Back", "Forward", "Homepage",
            "Panel", "Trash", "Search", "BookmarkAdd", "Previous", "Next", NULL };

    return actions;
}

static gboolean
midori_browser_toolbar_popup_context_menu_cb (GtkWidget*     widget,
                                              gint           x,
                                              gint           y,
                                              gint           button,
                                              MidoriBrowser* browser)
{
    GtkWidget* menu;
    GtkWidget* menuitem;

    /* Unsupported trick used for web app mode */
    if (g_object_get_data (G_OBJECT (browser), "locked"))
        return FALSE;

    menu = gtk_menu_new ();
    menuitem = sokoke_action_create_popup_menu_item (
        _action_by_name (browser, "Menubar"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    menuitem = sokoke_action_create_popup_menu_item (
        _action_by_name (browser, "Navigationbar"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    menuitem = sokoke_action_create_popup_menu_item (
        _action_by_name (browser, "Bookmarkbar"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    menuitem = sokoke_action_create_popup_menu_item (
        _action_by_name (browser, "Transferbar"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    menuitem = sokoke_action_create_popup_menu_item (
        _action_by_name (browser, "Statusbar"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

    katze_widget_popup (widget, GTK_MENU (menu), NULL,
        button == -1 ? KATZE_MENU_POSITION_LEFT : KATZE_MENU_POSITION_CURSOR);
    return TRUE;
}

static void
midori_browser_menu_item_select_cb (GtkWidget*     menuitem,
                                    MidoriBrowser* browser)
{
    GtkAction* action;
    gchar* tooltip;

    action = gtk_activatable_get_related_action (GTK_ACTIVATABLE (menuitem));
    tooltip = action ? katze_object_get_string (action, "tooltip") : NULL;
    if (!tooltip)
    {
        /* This is undocumented object data, used by KatzeArrayAction. */
        KatzeItem* item = g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
        if (item)
        {
            tooltip = g_strdup (katze_item_get_uri (item));
            sokoke_prefetch_uri (tooltip);
        }
    }
    _midori_browser_set_statusbar_text (browser, tooltip);
    g_free (tooltip);
}

static void
midori_browser_menu_item_deselect_cb (GtkWidget*     menuitem,
                                      MidoriBrowser* browser)
{
    _midori_browser_set_statusbar_text (browser, NULL);
}

static void
_action_trash_populate_popup (GtkAction*     action,
                              GtkMenu*       menu,
                              MidoriBrowser* browser)
{
    GList* children = gtk_container_get_children (GTK_CONTAINER (menu));
    guint i = 0;
    GtkWidget* menuitem;

    while ((menuitem = g_list_nth_data (children, i++)))
    {
        g_signal_connect (menuitem, "select",
            G_CALLBACK (midori_browser_menu_item_select_cb), browser);
        g_signal_connect (menuitem, "deselect",
            G_CALLBACK (midori_browser_menu_item_deselect_cb), browser);
    }
    g_list_free (children);

    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "TrashEmpty"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
}

static void
_action_trash_activate_item (GtkAction*     action,
                             KatzeItem*     item,
                             MidoriBrowser* browser)
{
    guint n = midori_browser_add_item (browser, item);
    midori_browser_set_current_page (browser, n);
    katze_array_remove_item (browser->trash, item);
    _midori_browser_update_actions (browser);
}

static gboolean
_action_trash_activate_item_alt (GtkAction*     action,
                                 KatzeItem*     item,
                                 guint          button,
                                 MidoriBrowser* browser)
{
    if (button == 2)
    {
        gint n;

        n = midori_browser_add_uri (browser, katze_item_get_uri (item));

        _midori_browser_set_current_page_smartly (browser, n);

        katze_array_remove_item (browser->trash, item);
        _midori_browser_update_actions (browser);

        return TRUE;
    }

    return FALSE;
}

static void
midori_browser_menu_item_activate_cb (GtkWidget*     menuitem,
                                      MidoriBrowser* browser)
{
    KatzeItem* item = g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
    midori_browser_set_current_uri (browser, katze_item_get_uri (item));
}

static gboolean
midori_browser_menu_item_button_press_cb (GtkWidget*      menuitem,
                                          GdkEventButton* event,
                                          MidoriBrowser*  browser)
{
    KatzeItem* item = g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
    return _action_menus_activate_item_alt (NULL, item, event->button, browser);
}

static void
_action_history_populate_popup (GtkAction*     action,
                                GtkMenu*       menu,
                                MidoriBrowser* browser)
{
    #if HAVE_SQLITE
    sqlite3* db;
    sqlite3_stmt* statement;
    gint result;
    const gchar* sqlcmd;

    db = g_object_get_data (G_OBJECT (browser->history), "db");
    sqlcmd = "SELECT uri, title, date FROM history "
             "GROUP BY uri ORDER BY date ASC LIMIT 10";
    result = sqlite3_prepare_v2 (db, sqlcmd, -1, &statement, NULL);
    if (result != SQLITE_OK)
    {
        g_print (_("Failed to execute database statement: %s\n"),
                 sqlite3_errmsg (db));
        return;
    }

    while ((result = sqlite3_step (statement)) == SQLITE_ROW)
    {
        const unsigned char* uri;
        const unsigned char* title;
        KatzeItem* item;
        GtkWidget* menuitem;
        GdkPixbuf* icon;
        GtkWidget* image;

        uri = sqlite3_column_text (statement, 0);
        title = sqlite3_column_text (statement, 1);

        item = katze_item_new ();
        katze_item_set_uri (item, (gchar*)uri);
        katze_item_set_name (item, (gchar*)title);

        menuitem = katze_image_menu_item_new_ellipsized ((gchar*)title);
        icon = katze_load_cached_icon ((gchar*)uri, GTK_WIDGET (browser));
        image = gtk_image_new_from_pixbuf (icon);
        g_object_unref (icon);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
        #if GTK_CHECK_VERSION (2, 16, 0)
        gtk_image_menu_item_set_always_show_image (
            GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
        #endif
        g_object_set_data_full (G_OBJECT (menuitem), "KatzeItem",
                                item, (GDestroyNotify)g_object_unref);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
        gtk_widget_show (menuitem);

        g_signal_connect (menuitem, "activate",
            G_CALLBACK (midori_browser_menu_item_activate_cb), browser);
        g_signal_connect (menuitem, "button-press-event",
            G_CALLBACK (midori_browser_menu_item_button_press_cb), browser);
        g_signal_connect (menuitem, "select",
            G_CALLBACK (midori_browser_menu_item_select_cb), browser);
        g_signal_connect (menuitem, "deselect",
            G_CALLBACK (midori_browser_menu_item_deselect_cb), browser);
    }
    sqlite3_finalize (statement);
    #endif
}

static void
_action_history_activate_item (GtkAction*     action,
                               KatzeItem*     item,
                               MidoriBrowser* browser)
{
    midori_browser_set_current_uri (browser, katze_item_get_uri (item));
}

static void
_action_bookmarks_populate_popup (GtkAction*     action,
                                  GtkMenu*       menu,
                                  MidoriBrowser* browser)
{
    GList* children = gtk_container_get_children (GTK_CONTAINER (menu));
    guint i = 0;
    GtkWidget* menuitem;

    while ((menuitem = g_list_nth_data (children, i++)))
    {
        g_signal_connect (menuitem, "select",
            G_CALLBACK (midori_browser_menu_item_select_cb), browser);
        g_signal_connect (menuitem, "deselect",
            G_CALLBACK (midori_browser_menu_item_deselect_cb), browser);
    }
    g_list_free (children);

    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);

    if (katze_array_is_empty (browser->bookmarks))
    {
        menuitem = gtk_image_menu_item_new_with_label (_("Empty"));
        gtk_widget_set_sensitive (menuitem, FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
        gtk_widget_show (menuitem);
    }
    /*menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "BookmarksExport"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "BookmarksImport"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);*/

    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "BookmarkFolderAdd"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "BookmarkAdd"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
}

/* static */ void
midori_browser_open_bookmark (MidoriBrowser* browser,
                              KatzeItem*     item)
{
    const gchar* uri = katze_item_get_uri (item);
    gchar* uri_fixed;

    if (!(uri && *uri))
        return;

    /* Imported bookmarks may lack a protocol */
    uri_fixed = sokoke_magic_uri (uri);
    if (!uri_fixed)
        uri_fixed = g_strdup (uri);

    /* FIXME: Use the same binary that is running right now */
    if (katze_item_get_meta_integer (item, "app") != -1)
        sokoke_spawn_program ("midori -a", uri_fixed, FALSE);
    else
    {
        midori_browser_set_current_uri (browser, uri_fixed);
        gtk_widget_grab_focus (midori_browser_get_current_tab (browser));
    }
    g_free (uri_fixed);
}

static void
_action_bookmarks_activate_item (GtkAction*     action,
                                 KatzeItem*     item,
                                 MidoriBrowser* browser)
{
    midori_browser_open_bookmark (browser, item);
}

static void
_action_tools_populate_popup (GtkAction*     action,
                              GtkMenu*       menu,
                              MidoriBrowser* browser)
{
    static const GtkActionEntry actions[] = {
      { "ManageSearchEngines" },
      { "ClearPrivateData" },
      #if WEBKIT_CHECK_VERSION (1, 1, 17)
      { "InspectPage" },
      #endif
      { "-" },
      { NULL },
      { "p" },
      #ifdef G_OS_WIN32
      { NULL },
      { "Preferences" },
      #endif
    };
    guint i;

    for (i = 0; i < G_N_ELEMENTS (actions); i++)
    {
        GtkWidget* menuitem;
        if (actions[i].name != NULL)
        {
            if (actions[i].name[0] == '-')
            {
                g_signal_emit (browser, signals[POPULATE_TOOL_MENU], 0, menu);
                continue;
            }
            else if (actions[i].name[0] == 'p')
            {
                MidoriPanel* panel;
                gsize j;
                GtkWidget* widget;

                panel = MIDORI_PANEL (browser->panel);
                j = 0;
                while ((widget = midori_panel_get_nth_page (panel, j++)))
                {
                    menuitem = midori_panel_construct_menu_item (panel, MIDORI_VIEWABLE (widget));
                    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
                }
                continue;
            }
            menuitem = sokoke_action_create_popup_menu_item (
                _action_by_name (browser, actions[i].name));
        }
        else
            menuitem = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
        gtk_widget_show (menuitem);
    }
}

static void
midori_browser_bookmark_popup (GtkWidget*      widget,
                               GdkEventButton* event,
                               KatzeItem*      item,
                               MidoriBrowser*  browser);

static gboolean
_action_menus_activate_item_alt (GtkAction*     action,
                                 KatzeItem*     item,
                                 guint          button,
                                 MidoriBrowser* browser)
{
    if (button == 2)
    {
        gint n = midori_browser_add_uri (browser, katze_item_get_uri (item));
        _midori_browser_set_current_page_smartly (browser, n);

        return TRUE;
    }
    else if (button == 3)
    {
        GdkEvent* event = gtk_get_current_event ();
        midori_browser_bookmark_popup (gtk_get_event_widget (event),
            (GdkEventButton*)event, item, browser);
        return TRUE;
    }

    return FALSE;
}

static void
_action_window_populate_popup (GtkAction*     action,
                               GtkMenu*       menu,
                               MidoriBrowser* browser)
{
    GList* children = gtk_container_get_children (GTK_CONTAINER (menu));
    guint i = 0;
    GtkWidget* menuitem;

    while ((menuitem = g_list_nth_data (children, i++)))
    {
        g_signal_connect (menuitem, "select",
            G_CALLBACK (midori_browser_menu_item_select_cb), browser);
        g_signal_connect (menuitem, "deselect",
            G_CALLBACK (midori_browser_menu_item_deselect_cb), browser);
    }
    g_list_free (children);

    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "LastSession"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "TabCurrent"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "TabNext"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "TabPrevious"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
}

/*Modified by SBielov start*/
static void
_action_tabs_populate_popup (GtkAction*     action,
                               GtkMenu*       menu,
                               MidoriBrowser* browser)
{
    GList* children = gtk_container_get_children (GTK_CONTAINER (menu));
    guint i = 0;
    GtkWidget* menuitem;

    while ((menuitem = g_list_nth_data (children, i++)))
    {
        g_signal_connect (menuitem, "select",
            G_CALLBACK (midori_browser_menu_item_select_cb), browser);
        g_signal_connect (menuitem, "deselect",
            G_CALLBACK (midori_browser_menu_item_deselect_cb), browser);
    }
    g_list_free (children);
    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
}

static void
_action_quickmenu_populate_popup (GtkAction*     action,
                               GtkMenu*       menu,
                               MidoriBrowser* browser)
{
    GtkWidget* menuitem;

    //Quit
    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "Quit"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    //Preferences
    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "Preferences"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    //Bookmarks
    menuitem = gtk_action_create_menu_item (
         _action_by_name (browser, "Bookmarks"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    //Close tab
    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "TabClose"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    //New tab
    menuitem = gtk_action_create_menu_item (
        _action_by_name (browser, "TabNew"));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
}

static void
_action_window_activate_item (GtkAction*     action,
                              KatzeItem*     item,
                              MidoriBrowser* browser)
{
    guint i, n;
    GtkWidget* view;

    n = katze_array_get_length (browser->proxy_array);
    for (i = 0; i < n; i++)
    {
        view = gtk_notebook_get_nth_page (GTK_NOTEBOOK (browser->notebook), i);
        if (midori_view_get_proxy_item (MIDORI_VIEW (view)) == item)
            gtk_notebook_set_current_page (GTK_NOTEBOOK (browser->notebook), i);
    }
}

static void
_action_compact_menu_populate_popup (GtkAction*     action,
                                     GtkWidget*     menu,
                                     MidoriBrowser* browser)
{
    static const GtkActionEntry actions[] = {
      { "TabNew" },
      { "WindowNew" },
      { "PrivateBrowsing" },
      { "Open" },
      #if HAVE_HILDON
      { "Find" },
      #else
      { "Print" },
      { "About" },
      { NULL },
      { "Menubar" },
      { "Bookmarkbar" },
      { "Panel" },
      { "Statusbar" },
      { NULL },
      { "-" },
      { "ClearPrivateData" },
      #if WEBKIT_CHECK_VERSION (1, 1, 17)
      { "InspectPage" },
      #endif
      { "Fullscreen" },
      #endif
      { "Preferences" },
      #if HAVE_HILDON
      { "auto-load-images" },
      { "enable-scripts" },
      { "enable-plugins" },
      #endif
    };
    guint i;

    for (i = 0; i < G_N_ELEMENTS (actions); i++)
    {
        #ifdef HAVE_HILDON_2_2
        GtkAction* _action;
        gchar* label;
        GtkWidget* button;

        if (!actions[i].name)
            continue;
        _action = _action_by_name (browser, actions[i].name);
        if (_action)
        {
            label = katze_object_get_string (_action, "label");
            button = hildon_gtk_button_new (HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH);
            gtk_button_set_label (GTK_BUTTON (button), label);
            gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
            g_free (label);
            g_signal_connect_swapped (button, "clicked",
                G_CALLBACK (gtk_action_activate), _action);
        }
        else
            button = katze_property_proxy (browser->settings, actions[i].name, NULL);
        gtk_widget_show (button);
        hildon_app_menu_append (HILDON_APP_MENU (menu), GTK_BUTTON (button));
        #else
        GtkWidget* menuitem;
        if (actions[i].name != NULL)
        {
            if (actions[i].name[0] == '-')
            {
                g_signal_emit (browser, signals[POPULATE_TOOL_MENU], 0, menu);
                continue;
            }
            menuitem = sokoke_action_create_popup_menu_item (
                _action_by_name (browser, actions[i].name));
        }
        else
        {
            menuitem = gtk_separator_menu_item_new ();
            gtk_widget_show (menuitem);
        }
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
        #endif
    }
}

static void
midori_preferences_response_help_cb (GtkWidget*     preferences,
                                     gint           response,
                                     MidoriBrowser* browser)
{
    if (response == GTK_RESPONSE_HELP)
        gtk_action_activate (_action_by_name (browser, "HelpContents"));
}

static void
_action_preferences_activate (GtkAction*     action,
                              MidoriBrowser* browser)
{
    static GtkWidget* dialog = NULL;

    if (!GTK_WIDGET_VISIBLE (browser))
        return;

    if (!dialog)
    {
        dialog = midori_preferences_new (GTK_WINDOW (browser), browser->settings);
        g_signal_connect (dialog, "response",
            G_CALLBACK (midori_preferences_response_help_cb), browser);
        g_signal_connect (dialog, "destroy",
            G_CALLBACK (gtk_widget_destroyed), &dialog);
        gtk_widget_show (dialog);
    }
    else
        gtk_window_present (GTK_WINDOW (dialog));
}

static void
_action_menubar_activate (GtkToggleAction* action,
                          MidoriBrowser*   browser)
{
    gboolean active = gtk_toggle_action_get_active (action);
    if (active)
    {
        GtkContainer* navigationbar = GTK_CONTAINER (browser->navigationbar);
        GList* children = gtk_container_get_children (navigationbar);
        GtkAction* menu_action = _action_by_name (browser, "CompactMenu");
        for (; children != NULL; children = g_list_next (children))
        {
            GtkAction* action_;

            action_ = gtk_activatable_get_related_action (GTK_ACTIVATABLE (children->data));
            if (action_ == menu_action)
            {
                gtk_container_remove (navigationbar,
                    GTK_WIDGET (children->data));
                _midori_browser_save_toolbar_items (browser);
                break;
            }
        }
    }
    else
    {
        GtkAction* widget_action = _action_by_name (browser, "CompactMenu");
        GtkWidget* toolitem = gtk_action_create_tool_item (widget_action);
        gtk_toolbar_insert (GTK_TOOLBAR (browser->navigationbar),
                            GTK_TOOL_ITEM (toolitem), -1);
        g_signal_connect (gtk_bin_get_child (GTK_BIN (toolitem)),
            "button-press-event",
            G_CALLBACK (midori_browser_toolbar_item_button_press_event_cb),
            browser);
        _midori_browser_save_toolbar_items (browser);
    }

    g_object_set (browser->settings, "show-menubar", active, NULL);
    /* Make sure the menubar is uptodate in case no settings are set */
    sokoke_widget_set_visible (browser->menubar, active);

    g_object_set_data (G_OBJECT (browser), "midori-toolbars-visible",
        GTK_WIDGET_VISIBLE (browser->menubar)
        || GTK_WIDGET_VISIBLE (browser->navigationbar)
        ? (void*)0xdeadbeef : NULL);
}

static void
_action_navigationbar_activate (GtkToggleAction* action,
                                MidoriBrowser*   browser)
{
    gboolean active = gtk_toggle_action_get_active (action);
    g_object_set (browser->settings, "show-navigationbar", active, NULL);
    sokoke_widget_set_visible (browser->navigationbar, active);

    g_object_set_data (G_OBJECT (browser), "midori-toolbars-visible",
        GTK_WIDGET_VISIBLE (browser->menubar)
        || GTK_WIDGET_VISIBLE (browser->navigationbar)
        ? (void*)0xdeadbeef : NULL);
}

static void
_action_bookmarkbar_activate (GtkToggleAction* action,
                              MidoriBrowser*   browser)
{
    gboolean active = gtk_toggle_action_get_active (action);
    g_object_set (browser->settings, "show-bookmarkbar", active, NULL);
    sokoke_widget_set_visible (browser->bookmarkbar, active);
}

static void
_action_transferbar_activate (GtkToggleAction* action,
                              MidoriBrowser*   browser)
{
    gboolean active = gtk_toggle_action_get_active (action);
    g_object_set (browser->settings, "show-transferbar", active, NULL);
    sokoke_widget_set_visible (browser->transferbar, active);
}

static void
_action_statusbar_activate (GtkToggleAction* action,
                            MidoriBrowser*   browser)
{
    gboolean active = gtk_toggle_action_get_active (action);
    g_object_set (browser->settings, "show-statusbar", active, NULL);
    sokoke_widget_set_visible (browser->statusbar, active);
}

static void
_action_reload_stop_activate (GtkAction*     action,
                              MidoriBrowser* browser)
{
    gchar* stock_id;
    GtkWidget* view;
    GdkModifierType state = (GdkModifierType)0;
    gint x, y;
    gboolean from_cache;

    if (!(view = midori_browser_get_current_tab (browser)))
        return;

    g_object_get (action, "stock-id", &stock_id, NULL);

    /* Refresh or stop, depending on the stock id */
    if (!strcmp (stock_id, GTK_STOCK_REFRESH))
    {
        gdk_window_get_pointer (NULL, &x, &y, &state);
        from_cache = state & GDK_SHIFT_MASK;
        midori_view_reload (MIDORI_VIEW (view), !from_cache);
    }
    else
        midori_view_stop_loading (MIDORI_VIEW (view));
    g_free (stock_id);
}

static void
_action_zoom_in_activate (GtkAction*     action,
                          MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    if (view)
        midori_view_set_zoom_level (MIDORI_VIEW (view),
            midori_view_get_zoom_level (MIDORI_VIEW (view)) + 0.25f);
}

static void
_action_zoom_out_activate (GtkAction*     action,
                           MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    if (view)
        midori_view_set_zoom_level (MIDORI_VIEW (view),
            midori_view_get_zoom_level (MIDORI_VIEW (view)) - 0.25f);
}

static void
_action_zoom_normal_activate (GtkAction*     action,
                              MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    if (view)
        midori_view_set_zoom_level (MIDORI_VIEW (view), 1.0f);
}

static void
_action_view_encoding_activate (GtkAction*     action,
                                GtkAction*     current,
                                MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    if (view && midori_browser_view_can_set_encoding (MIDORI_VIEW (view)))
    {
        const gchar* name;
        GtkWidget* web_view;

        name = gtk_action_get_name (current);
        web_view = gtk_bin_get_child (GTK_BIN (view));
        if (!strcmp (name, "EncodingAutomatic"))
            g_object_set (web_view, "custom-encoding", NULL, NULL);
        else
        {
            const gchar* encoding;
            if (!strcmp (name, "EncodingChinese"))
                encoding = "BIG5";
            else if (!strcmp (name, "EncodingJapanese"))
                encoding = "SHIFT_JIS";
            else if (!strcmp (name, "EncodingKorean"))
                encoding = "EUC-KR";
            else if (!strcmp (name, "EncodingRussian"))
                encoding = "KOI8-R";
            else if (!strcmp (name, "EncodingUnicode"))
                encoding = "UTF-8";
            else if (!strcmp (name, "EncodingWestern"))
                encoding = "ISO-8859-1";
            else
                g_assert_not_reached ();
            g_object_set (web_view, "custom-encoding", encoding, NULL);
        }
    }
}

static gchar*
midori_browser_get_uri_extension (const gchar* uri)
{
    gchar* extension;
    gchar* slash;
    gchar* period;
    gchar* ext_end;
    gchar* tmp = g_strdup (uri);

    /* Find the last slash in the URI and search for the last period
       *after* the last slash. This is not completely accurate
       but should cover most (simple) URIs */
    slash = strrchr (tmp, '/');
    /* Huh, URI without slashes? */
    if (!slash)
        return g_strdup ("");

    ext_end = period = strrchr (slash, '.');
    if (!period)
       return g_strdup ("");

    /* Skip the period */
    ext_end++;
    /* If *ext_end is 0 here, the URI ended with a period, so skip */
    if (!*ext_end)
       return g_strdup ("");

    /* Find the end of the extension */
    while (*ext_end && g_ascii_isalnum (*ext_end))
        ext_end++;

    *ext_end = 0;
    extension = g_strdup (period);

    g_free (tmp);

    return extension;
}

static void
midori_browser_source_transfer_cb (KatzeNetRequest* request,
                                   MidoriBrowser*   browser)
{
    gchar* filename;
    gchar* extension;
    gchar* unique_filename;
    gchar* text_editor;
    gint fd;
    FILE* fp;
    size_t ret;

    if (request->data)
    {
        extension = midori_browser_get_uri_extension (request->uri);
        filename = g_strdup_printf ("%uXXXXXX%s",
                                    g_str_hash (request->uri), extension);
        g_free (extension);
        if (((fd = g_file_open_tmp (filename, &unique_filename, NULL)) != -1))
        {
            if ((fp = fdopen (fd, "w")))
            {
                ret = fwrite (request->data, 1, request->length, fp);
                fclose (fp);
                if ((ret - request->length) != 0)
                {
                    /* FIXME: Show an error in the graphical interface */
                    g_warning ("Error writing to file %s "
                               "in midori_browser_source_transfer_cb()", filename);
                }
                g_object_get (browser->settings,
                    "text-editor", &text_editor, NULL);
                if (text_editor && *text_editor)
                    sokoke_spawn_program (text_editor, unique_filename, TRUE);
                else
                    sokoke_show_uri (NULL, unique_filename,
                                     gtk_get_current_event_time (), NULL);

                g_free (unique_filename);
                g_free (text_editor);
            }
            close (fd);
        }
        g_free (filename);
    }
}

static void
_action_source_view_activate (GtkAction*     action,
                              MidoriBrowser* browser)
{
    GtkWidget* view;
    gchar* text_editor;
    const gchar* uri;

    if (!(view = midori_browser_get_current_tab (browser)))
        return;

    g_object_get (browser->settings, "text-editor", &text_editor, NULL);
    uri = midori_view_get_display_uri (MIDORI_VIEW (view));

    if (!(text_editor && *text_editor))
    {
        #if WEBKIT_CHECK_VERSION (1, 1, 14)
        GtkWidget* source;
        GtkWidget* web_view;

        source = midori_view_new (browser->net);
        midori_view_set_settings (MIDORI_VIEW (source), browser->settings);
        midori_view_set_uri (MIDORI_VIEW (source), "");
        web_view = gtk_bin_get_child (GTK_BIN (source));
        webkit_web_view_set_view_source_mode (WEBKIT_WEB_VIEW (web_view), TRUE);
        webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), uri);
        gtk_widget_show (source);
        midori_browser_add_tab (browser, source);
        g_free (text_editor);
        return;
        #else
        GError* error = NULL;

        /* FIXME: Handling http transparently in the function would be nice */
        if (g_str_has_prefix (uri, "file://"))
        {
            if (!sokoke_show_uri_with_mime_type (gtk_widget_get_screen (view),
                uri, "text/plain", gtk_get_current_event_time (), &error))
                sokoke_message_dialog (GTK_MESSAGE_ERROR,
                    _("Could not run external program."), error ? error->message : "");
            if (error)
                g_error_free (error);
            g_free (text_editor);
            return;
        }
        #endif
    }

    if (g_str_has_prefix (uri, "file://"))
    {
        gchar* filename = g_filename_from_uri (uri, NULL, NULL);
        sokoke_spawn_program (text_editor, filename, TRUE);
        g_free (filename);
        return;
    }
    katze_net_load_uri (browser->net, uri, NULL,
        (KatzeNetTransferCb)midori_browser_source_transfer_cb, browser);
    g_free (text_editor);
}

static void
_action_fullscreen_activate (GtkAction*     action,
                             MidoriBrowser* browser)
{
    GdkWindowState state;

    if (!GTK_WIDGET (browser)->window)
        return;

    state = gdk_window_get_state (GTK_WIDGET (browser)->window);
    if (state & GDK_WINDOW_STATE_FULLSCREEN)
        gtk_window_unfullscreen (GTK_WINDOW (browser));
    else
        gtk_window_fullscreen (GTK_WINDOW (browser));
}

#if WEBKIT_CHECK_VERSION (1, 1, 4)
static void
_action_scroll_somewhere_activate (GtkAction*     action,
                                   MidoriBrowser* browser)
{
    GtkWidget* view;
    WebKitWebView* web_view;
    const gchar* name;

    view = midori_browser_get_current_tab (browser);
    if (!view)
        return;
    web_view = WEBKIT_WEB_VIEW (gtk_bin_get_child (GTK_BIN (view)));
    name = gtk_action_get_name (action);

    if (g_str_equal (name, "ScrollLeft"))
        webkit_web_view_move_cursor (web_view, GTK_MOVEMENT_VISUAL_POSITIONS, -1);
    if (g_str_equal (name, "ScrollDown"))
        webkit_web_view_move_cursor (web_view, GTK_MOVEMENT_DISPLAY_LINES, 1);
    else if (g_str_equal (name, "ScrollUp"))
        webkit_web_view_move_cursor (web_view, GTK_MOVEMENT_DISPLAY_LINES, -1);
    else if (g_str_equal (name, "ScrollRight"))
        webkit_web_view_move_cursor (web_view, GTK_MOVEMENT_VISUAL_POSITIONS, 1);
}
#endif

static void
_action_back_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    GtkWidget* view;

    if (g_object_get_data (G_OBJECT (action), "midori-middle-click"))
    {
        g_object_set_data (G_OBJECT (action), "midori-middle-click", (void*)0);
        return;
    }

    view = midori_browser_get_current_tab (browser);
    if (view)
        midori_view_go_back (MIDORI_VIEW (view));
}

static void
_action_forward_activate (GtkAction*     action,
                          MidoriBrowser* browser)
{
    GtkWidget* view;

    if (g_object_get_data (G_OBJECT (action), "midori-middle-click"))
    {
        g_object_set_data (G_OBJECT (action), "midori-middle-click", (void*)0);
        return;
    }

    view = midori_browser_get_current_tab (browser);
    if (view)
        midori_view_go_forward (MIDORI_VIEW (view));
}

static void
_action_previous_activate (GtkAction*     action,
                           MidoriBrowser* browser)
{
    GtkWidget* view;

    if (g_object_get_data (G_OBJECT (action), "midori-middle-click"))
    {
        g_object_set_data (G_OBJECT (action), "midori-middle-click", (void*)0);
        return;
    }

    view = midori_browser_get_current_tab (browser);
    if (view)
    {
        gchar* uri = g_strdup (midori_view_get_previous_page (MIDORI_VIEW (view)));
        midori_view_set_uri (MIDORI_VIEW (view), uri);
        g_free (uri);
    }
}

static void
_action_next_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    GtkWidget* view;

    if (g_object_get_data (G_OBJECT (action), "midori-middle-click"))
    {
        g_object_set_data (G_OBJECT (action), "midori-middle-click", (void*)0);
        return;
    }

    view = midori_browser_get_current_tab (browser);
    if (view)
    {
        gchar* uri = g_strdup (midori_view_get_next_page (MIDORI_VIEW (view)));
        midori_view_set_uri (MIDORI_VIEW (view), uri);
        g_free (uri);
    }
}

static void
_action_homepage_activate (GtkAction*     action,
                           MidoriBrowser* browser)
{
    gchar* homepage;

    if (g_object_get_data (G_OBJECT (action), "midori-middle-click"))
    {
        g_object_set_data (G_OBJECT (action), "midori-middle-click", (void*)0);
        return;
    }

    g_object_get (browser->settings, "homepage", &homepage, NULL);
    midori_browser_set_current_uri (browser, homepage);
    g_free (homepage);
}

static void
_action_location_activate (GtkAction*     action,
                           MidoriBrowser* browser)
{
    if (!GTK_WIDGET_VISIBLE (browser->navigationbar))
        gtk_widget_show (browser->navigationbar);
}

static void
_action_location_active_changed (GtkAction*     action,
                                 gint           idx,
                                 MidoriBrowser* browser)
{
    const gchar* uri;

    if (idx > -1)
    {
        uri = midori_location_action_get_uri (MIDORI_LOCATION_ACTION (action));
        midori_browser_set_current_uri (browser, uri);
    }
}

static void
midori_browser_bookmark_homepage_clicked_cb (GtkToolItem*   button,
                                             MidoriBrowser* browser)
{
    gchar* homepage;
    g_object_get (browser->settings, "homepage", &homepage, NULL);
    midori_browser_set_current_uri (browser, homepage);
    g_free (homepage);
    gtk_widget_grab_focus (midori_browser_get_current_tab (browser));
}

static gboolean
midori_browser_bookmark_homepage_button_press_cb (GtkToolItem*    button,
                                                  GdkEventButton* event,
                                                  MidoriBrowser*  browser)
{
    if (event->button == 2)
    {
        gchar* homepage;
        guint n;

        g_object_get (browser->settings, "homepage", &homepage, NULL);
        n = midori_browser_add_uri (browser, homepage);
        g_free (homepage);
        _midori_browser_set_current_page_smartly (browser, n);
        return TRUE;
    }

    return FALSE;
}

static void
midori_browser_bookmarks_add_item_cb (KatzeArray* array,
                                      KatzeItem*  item,
                                      GtkWidget*  toolbar);

static void
_action_location_focus_in (GtkAction*     action,
                           MidoriBrowser* browser)
{
    midori_location_action_set_secondary_icon (
        MIDORI_LOCATION_ACTION (action), GTK_STOCK_JUMP_TO);
}

static void
_action_location_focus_out (GtkAction*     action,
                            MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);

    if (!browser->show_navigationbar)
        gtk_widget_hide (browser->navigationbar);

    if (g_object_get_data (G_OBJECT (view), "news-feeds"))
        midori_location_action_set_secondary_icon (
            MIDORI_LOCATION_ACTION (action), STOCK_NEWS_FEED);
    else
        midori_location_action_set_secondary_icon (
            MIDORI_LOCATION_ACTION (action), GTK_STOCK_JUMP_TO);
}

static void
_action_location_reset_uri (GtkAction*     action,
                            MidoriBrowser* browser)
{
    const gchar* uri;

    uri = midori_browser_get_current_uri (browser);
    midori_location_action_set_uri (MIDORI_LOCATION_ACTION (action), uri);
}



static void
_action_location_submit_uri (GtkAction*     action,
                             const gchar*   uri,
                             gboolean       new_tab,
                             MidoriBrowser* browser)
{
    gchar* stripped_uri;
    gchar* new_uri;
    gint n;

    stripped_uri = g_strdup (uri);
    g_strstrip (stripped_uri);
    new_uri = sokoke_magic_uri (stripped_uri);
    if (!new_uri)
    {
        gchar** parts;
        gchar* keywords = NULL;
        const gchar* search_uri = NULL;
        #if HAVE_SQLITE
        time_t now;
        gint64 day;
        sqlite3* db;
        static sqlite3_stmt* statement = NULL;
        #endif

        /* Do we have a keyword and a string? */
        parts = g_strsplit (stripped_uri, " ", 2);
        if (parts[0])
        {
            KatzeItem* item;
            if ((item = katze_array_find_token (browser->search_engines, parts[0])))
            {
                keywords = g_strdup (parts[1] ? parts[1] : "");
                search_uri = katze_item_get_uri (item);
            }
        }
        g_strfreev (parts);

        if (keywords)
            g_free (stripped_uri);
        else
        {
            keywords = stripped_uri;
            search_uri = browser->location_entry_search;
        }
        new_uri = sokoke_search_uri (search_uri, keywords);

        #if HAVE_SQLITE
        now = time (NULL);
        day = sokoke_time_t_to_julian (&now);
 
        db = g_object_get_data (G_OBJECT (browser->history), "db");
        if (!statement)
        {
             const gchar* sqlcmd;
             sqlcmd = "INSERT INTO search (keywords, uri, day) VALUES (?,?,?)";
             sqlite3_prepare_v2 (db, sqlcmd, strlen (sqlcmd) + 1, &statement, NULL);
        }
        sqlite3_bind_text (statement, 1, keywords, -1, 0);
        sqlite3_bind_text (statement, 2, search_uri, -1, 0);
        sqlite3_bind_int64 (statement, 3, day);
 
        if (sqlite3_step (statement) != SQLITE_DONE)
             g_printerr (_("Failed to insert new history item: %s\n"), sqlite3_errmsg (db));
        sqlite3_reset (statement);
        sqlite3_clear_bindings (statement);
        #endif

        g_free (keywords);
    }
    else
        g_free (stripped_uri);

    if (new_tab)
    {
        n = midori_browser_add_uri (browser, new_uri);
        midori_browser_set_current_page (browser, n);
    }
    else
        midori_browser_set_current_uri (browser, new_uri);
    g_free (new_uri);
    gtk_widget_grab_focus (midori_browser_get_current_tab (browser));
}

static void
midori_browser_news_feed_clicked_cb (GtkWidget*     menuitem,
                                     MidoriBrowser* browser)
{
    gchar* uri = g_object_get_data (G_OBJECT (menuitem), "uri");
    midori_browser_subscribe_to_news_feed (browser, uri);
}

static gboolean
_action_location_secondary_icon_released (GtkAction*     action,
                                          GtkWidget*     widget,
                                          MidoriBrowser* browser)
{
    GtkWidget* view;

    if ((view = midori_browser_get_current_tab (browser)))
    {
        const gchar* uri = midori_view_get_display_uri (MIDORI_VIEW (view));
        if (gtk_window_get_focus (GTK_WINDOW (browser)) == widget)
            _action_location_submit_uri (action, uri, FALSE, browser);
        else if ((uri = g_object_get_data (G_OBJECT (view), "news-feeds")))
        {
            KatzeArray* news_feeds;
            KatzeItem* item;
            KatzeItem* itemm;

            news_feeds = katze_object_get_object (G_OBJECT (view), "news-feeds");
            item = katze_array_get_nth_item (news_feeds, 0);
            if ((itemm = katze_array_get_nth_item (news_feeds, 1)))
            {
                guint i;
                GtkWidget* menu;
                GtkWidget* menuitem;

                menu = gtk_menu_new ();
                menuitem = gtk_menu_item_new_with_label (katze_item_get_name (item));
                g_object_set_data_full (G_OBJECT (menuitem), "uri",
                    g_strdup (katze_item_get_uri (item)), (GDestroyNotify)g_free);
                g_signal_connect (menuitem, "activate",
                    G_CALLBACK (midori_browser_news_feed_clicked_cb), browser);
                gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
                menuitem = gtk_menu_item_new_with_label (katze_item_get_name (itemm));
                g_object_set_data_full (G_OBJECT (menuitem), "uri",
                    g_strdup (katze_item_get_uri (itemm)), (GDestroyNotify)g_free);
                g_signal_connect (menuitem, "activate",
                    G_CALLBACK (midori_browser_news_feed_clicked_cb), browser);
                gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
                i = 2;
                while ((itemm = katze_array_get_nth_item (news_feeds, i++)))
                {
                    menuitem = gtk_menu_item_new_with_label (
                        katze_item_get_name (itemm));
                    g_object_set_data_full (G_OBJECT (menuitem), "uri",
                        g_strdup (katze_item_get_uri (itemm)), (GDestroyNotify)g_free);
                    g_signal_connect (menuitem, "activate",
                        G_CALLBACK (midori_browser_news_feed_clicked_cb), browser);
                    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
                }
                sokoke_container_show_children (GTK_CONTAINER (menu));
                katze_widget_popup (widget, GTK_MENU (menu), NULL,
                                    KATZE_MENU_POSITION_RIGHT);
            }
            else
                midori_browser_subscribe_to_news_feed (browser, uri);
            g_object_unref (news_feeds);
        }
        else
            _action_location_submit_uri (action, uri, FALSE, browser);
        return TRUE;
    }

    return FALSE;
}

static void
_action_search_submit (GtkAction*     action,
                       const gchar*   keywords,
                       gboolean       new_tab,
                       MidoriBrowser* browser)
{
    guint last_web_search;
    KatzeItem* item;
    const gchar* url;
    gchar* search;

    g_object_get (browser->settings, "last-web-search", &last_web_search, NULL);
    item = katze_array_get_nth_item (browser->search_engines, last_web_search);
    if (item)
        url = katze_item_get_uri (item);
    else /* The location entry search is our fallback */
        url = browser->location_entry_search;

    search = sokoke_search_uri (url, keywords);

    if (new_tab)
        midori_browser_add_uri (browser, search);
    else
        midori_browser_set_current_uri (browser, search);

    g_free (search);
}

static void
_action_search_activate (GtkAction*     action,
                         MidoriBrowser* browser)
{
    GSList* proxies = gtk_action_get_proxies (action);
    guint i = 0;
    GtkWidget* proxy;
    const gchar* uri;
    gchar* search;

    while (((proxy = g_slist_nth_data (proxies, i++))))
        if (GTK_IS_TOOL_ITEM (proxy))
        {
            if (!GTK_WIDGET_VISIBLE (browser->navigationbar))
                gtk_widget_show (browser->navigationbar);
            return;
        }

    /* Load default search engine in current tab */
    uri = browser->location_entry_search;
    search = sokoke_search_uri (uri ? uri : "", "");
    midori_browser_set_current_uri (browser, search);
    gtk_widget_grab_focus (midori_browser_get_current_tab (browser));
    g_free (search);
}

static void
_action_search_notify_current_item (GtkAction*     action,
                                    GParamSpec*    pspec,
                                    MidoriBrowser* browser)
{
    MidoriSearchAction* search_action;
    KatzeItem* item;
    guint idx;

    search_action = MIDORI_SEARCH_ACTION (action);
    item = midori_search_action_get_current_item (search_action);
    if (item)
        idx = katze_array_get_item_index (browser->search_engines, item);
    else
        idx = 0;

    g_object_set (browser->settings, "last-web-search", idx, NULL);
}

static void
_action_search_notify_default_item (GtkAction*     action,
                                    GParamSpec*    pspec,
                                    MidoriBrowser* browser)
{
    MidoriSearchAction* search_action;
    KatzeItem* item;

    search_action = MIDORI_SEARCH_ACTION (action);
    item = midori_search_action_get_default_item (search_action);
    if (item)
        g_object_set (browser->settings, "location-entry-search",
                      katze_item_get_uri (item), NULL);
}

static void
_action_search_focus_out (GtkAction*     action,
                          MidoriBrowser* browser)
{
    if (GTK_WIDGET_VISIBLE (browser->statusbar) && !browser->show_navigationbar)
        gtk_widget_hide (browser->navigationbar);
}

static void
midori_browser_bookmark_popup_item (GtkWidget*     menu,
                                    const gchar*   stock_id,
                                    const gchar*   label,
                                    KatzeItem*     item,
                                    gpointer       callback,
                                    MidoriBrowser* browser)
{
    const gchar* uri;
    GtkWidget* menuitem;

    uri = katze_item_get_uri (item);

    menuitem = gtk_image_menu_item_new_from_stock (stock_id, NULL);
    if (label)
        gtk_label_set_text_with_mnemonic (GTK_LABEL (gtk_bin_get_child (
        GTK_BIN (menuitem))), label);
    if (!strcmp (stock_id, GTK_STOCK_EDIT))
        gtk_widget_set_sensitive (menuitem,
            KATZE_IS_ARRAY (item) || uri != NULL);
    else if (!KATZE_IS_ARRAY (item) && strcmp (stock_id, GTK_STOCK_DELETE))
        gtk_widget_set_sensitive (menuitem, uri != NULL);
    g_object_set_data (G_OBJECT (menuitem), "KatzeItem", item);
    g_signal_connect (menuitem, "activate", G_CALLBACK (callback), browser);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
}

static void
midori_browser_bookmark_open_activate_cb (GtkWidget*     menuitem,
                                          MidoriBrowser* browser)
{
    KatzeItem* item;
    const gchar* uri;

    item = (KatzeItem*)g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
    uri = katze_item_get_uri (item);

    if (uri && *uri)
        midori_browser_set_current_uri (browser, uri);
}

static void
midori_browser_bookmark_open_in_tab_activate_cb (GtkWidget*     menuitem,
                                                 MidoriBrowser* browser)
{
    KatzeItem* item;
    const gchar* uri;
    guint n;

    item = (KatzeItem*)g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
    if (KATZE_IS_ARRAY (item))
    {
        KatzeItem* child;
        guint i = 0;

        while ((child = katze_array_get_nth_item (KATZE_ARRAY (item), i)))
        {
            if ((uri = katze_item_get_uri (child)) && *uri)
            {
                n = midori_browser_add_item (browser, child);
                _midori_browser_set_current_page_smartly (browser, n);
            }
            i++;
        }
    }
    else
    {
        if ((uri = katze_item_get_uri (item)) && *uri)
        {
            n = midori_browser_add_item (browser, item);
            _midori_browser_set_current_page_smartly (browser, n);
        }
    }
}

static void
midori_browser_bookmark_open_in_window_activate_cb (GtkWidget*     menuitem,
                                                    MidoriBrowser* browser)
{
    KatzeItem* item;
    const gchar* uri;

    item = (KatzeItem*)g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
    uri = katze_item_get_uri (item);

    if (uri && *uri)
    {
        MidoriBrowser* new_browser;
        g_signal_emit (browser, signals[NEW_WINDOW], 0, NULL, &new_browser);
        midori_browser_add_uri (new_browser, uri);
    }
}

static void
midori_browser_bookmark_edit_activate_cb (GtkWidget*     menuitem,
                                          MidoriBrowser* browser)
{
    KatzeItem* item;
    const gchar* uri;

    item = (KatzeItem*)g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
    uri = katze_item_get_uri (item);

    if (KATZE_IS_ARRAY (item))
        midori_browser_edit_bookmark_dialog_new (browser, item, FALSE, TRUE);
    else if (uri && *uri)
        midori_browser_edit_bookmark_dialog_new (browser, item, FALSE, FALSE);
}

static void
midori_browser_bookmark_delete_activate_cb (GtkWidget*     menuitem,
                                            MidoriBrowser* browser)
{
    KatzeItem* item;
    const gchar* uri;
    KatzeItem* parent;

    item = (KatzeItem*)g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
    uri = katze_item_get_uri (item);

    /* FIXME: Even toplevel items should technically have a parent */
    g_return_if_fail (katze_item_get_parent (item));

    parent = katze_item_get_parent (item);
    katze_array_remove_item (KATZE_ARRAY (parent), item);
}

static void
midori_browser_bookmark_popup (GtkWidget*      widget,
                               GdkEventButton* event,
                               KatzeItem*      item,
                               MidoriBrowser*  browser)
{
    GtkWidget* menu;
    GtkWidget* menuitem;

    menu = gtk_menu_new ();
    if (KATZE_IS_ARRAY (item))
        midori_browser_bookmark_popup_item (menu,
            STOCK_TAB_NEW, _("Open all in _Tabs"),
            item, midori_browser_bookmark_open_in_tab_activate_cb, browser);
    else
    {
        midori_browser_bookmark_popup_item (menu, GTK_STOCK_OPEN, NULL,
            item, midori_browser_bookmark_open_activate_cb, browser);
        midori_browser_bookmark_popup_item (menu,
            STOCK_TAB_NEW, _("Open in New _Tab"),
            item, midori_browser_bookmark_open_in_tab_activate_cb, browser);
        midori_browser_bookmark_popup_item (menu,
            STOCK_WINDOW_NEW, _("Open in New _Window"),
            item, midori_browser_bookmark_open_in_window_activate_cb, browser);
    }
    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
    midori_browser_bookmark_popup_item (menu, GTK_STOCK_EDIT, NULL,
        item, midori_browser_bookmark_edit_activate_cb, browser);
    midori_browser_bookmark_popup_item (menu, GTK_STOCK_DELETE, NULL,
        item, midori_browser_bookmark_delete_activate_cb, browser);

    katze_widget_popup (widget, GTK_MENU (menu), event, KATZE_MENU_POSITION_CURSOR);
}

static gboolean
midori_browser_menu_button_press_event_cb (GtkWidget*      toolitem,
                                           GdkEventButton* event,
                                           MidoriBrowser*  browser)
{
    /* GtkMenuBar catches button events on children with submenus,
       so we need to see if the actual widget is the menubar, and if
       it is an item, we forward it to the actual widget. */
    toolitem = gtk_get_event_widget ((GdkEvent*)event);

    if (GTK_IS_MENU_BAR (toolitem) && event->button == 3)
    {
        midori_browser_toolbar_popup_context_menu_cb (
            GTK_IS_BIN (toolitem) && gtk_bin_get_child (GTK_BIN (toolitem)) ?
                gtk_widget_get_parent (toolitem) : toolitem,
            event->x, event->y, event->button, browser);
        return TRUE;
    }
    else if (GTK_IS_MENU_ITEM (toolitem) && event->button == 3)
    {
        gboolean handled;
        g_signal_emit_by_name (toolitem, "button-press-event", event, &handled);
        return handled;
    }
    return FALSE;
}

static gboolean
midori_browser_menu_middle_click_on_navigation_action (MidoriBrowser* browser,
                                                       GtkAction*     action)
{
    const gchar* name;
    gchar* homepage;

    g_return_val_if_fail (action != NULL, FALSE);

    g_object_get (browser->settings, "homepage", &homepage, NULL);

    name = gtk_action_get_name (action);

    /* We use a trick here to implement middle click and prevent the default
       "activate" callback from being invoked. We set "midori-middle-click"
       as GObject data and check the value in the "activate" callback. */

    if (g_str_equal (name, "Homepage"))
    {
        gint n;

        n = midori_browser_add_uri (browser, homepage);
        _midori_browser_set_current_page_smartly (browser, n);

        g_object_set_data (G_OBJECT (action), "midori-middle-click", (void*)1);

        return TRUE;
    }
    else if (g_str_equal (name, "Back"))
    {
        GtkWidget* view;
        WebKitWebBackForwardList* back_forward_list;
        WebKitWebHistoryItem* back_item;
        const gchar* back_uri;
        gint n;

        view = gtk_bin_get_child (GTK_BIN (midori_browser_get_current_tab (browser)));

        back_forward_list =
            webkit_web_view_get_back_forward_list (WEBKIT_WEB_VIEW (view));

        back_item = webkit_web_back_forward_list_get_back_item (back_forward_list);
        back_uri = webkit_web_history_item_get_uri (back_item);

        n = midori_browser_add_uri (browser, back_uri);
        _midori_browser_set_current_page_smartly (browser, n);

        g_object_set_data (G_OBJECT (action), "midori-middle-click", (void*)1);

        return TRUE;
    }
    else if (g_str_equal (name, "Forward"))
    {
        GtkWidget *view;
        WebKitWebBackForwardList *back_forward_list;
        WebKitWebHistoryItem *forward_item;
        const gchar *forward_uri;
        gint n;

        view = gtk_bin_get_child (GTK_BIN (midori_browser_get_current_tab (browser)));

        back_forward_list =
            webkit_web_view_get_back_forward_list (WEBKIT_WEB_VIEW (view));

        forward_item =
            webkit_web_back_forward_list_get_forward_item (back_forward_list);
        forward_uri = webkit_web_history_item_get_uri (forward_item);

        n = midori_browser_add_uri (browser, forward_uri);
        _midori_browser_set_current_page_smartly (browser, n);

        g_object_set_data (G_OBJECT (action), "midori-middle-click", (void*)1);

        return TRUE;
    }
    else if (g_str_equal (name, "Previous"))
    {
        GtkWidget *view;
        gint n;

        view = midori_browser_get_current_tab (browser);
        n = midori_browser_add_uri (browser,
            midori_view_get_previous_page (MIDORI_VIEW (view)));
        _midori_browser_set_current_page_smartly (browser, n);

        g_object_set_data (G_OBJECT (action), "midori-middle-click", (void*)1);

        return TRUE;
    }
    else if (g_str_equal (name, "Next"))
    {
        GtkWidget *view;
        gint n;

        view = midori_browser_get_current_tab (browser);
        n = midori_browser_add_uri (browser,
            midori_view_get_next_page (MIDORI_VIEW (view)));
        _midori_browser_set_current_page_smartly (browser, n);

        g_object_set_data (G_OBJECT (action), "midori-middle-click", (void*)1);

        return TRUE;
    }

    g_free (homepage);

    return FALSE;
}

static gboolean
midori_browser_menu_item_middle_click_event_cb (GtkWidget*      toolitem,
                                                GdkEventButton* event,
                                                MidoriBrowser*  browser)
{
    if (event->button == 2)
    {
        GtkAction* action;

        action = gtk_activatable_get_related_action (GTK_ACTIVATABLE (toolitem));

        return midori_browser_menu_middle_click_on_navigation_action (browser, action);
    }
    return FALSE;
}

static gboolean
midori_browser_bookmarkbar_item_button_press_event_cb (GtkWidget*      toolitem,
                                                       GdkEventButton* event,
                                                       MidoriBrowser*  browser)
{
    KatzeItem* item;
    gint n;

    if (event->button == 2)
    {
        item = (KatzeItem*)g_object_get_data (G_OBJECT (toolitem), "KatzeItem");
        if (katze_item_get_uri (item))
        {
            n = midori_browser_add_uri (browser, katze_item_get_uri (item));
            _midori_browser_set_current_page_smartly (browser, n);

            return TRUE;
        }
    }
    else if (event->button == 3)
    {
        item = (KatzeItem*)g_object_get_data (G_OBJECT (toolitem), "KatzeItem");
        midori_browser_bookmark_popup (toolitem, NULL, item, browser);
        return TRUE;
    }
    return FALSE;
}

static void
_action_bookmark_add_activate (GtkAction*     action,
                               MidoriBrowser* browser)
{
    midori_browser_edit_bookmark_dialog_new (browser, NULL, TRUE, FALSE);
}

static void
_action_bookmark_folder_add_activate (GtkAction*     action,
                                      MidoriBrowser* browser)
{
    midori_browser_edit_bookmark_dialog_new (browser, NULL, TRUE, TRUE);
}

static void
_action_bookmarks_import_activate (GtkAction*     action,
                                   MidoriBrowser* browser)
{
    typedef struct
    {
        const gchar* path;
        const gchar* name;
        const gchar* icon;
    } BookmarkClient;
    static const BookmarkClient bookmark_clients[] = {
        { ".local/share/data/Arora/bookmarks.xbel", N_("Arora"), "arora" },
        { ".kazehakase/bookmarks.xml", N_("Kazehakase"), "kazehakase-icon" },
        { ".opera/bookmarks.adr", N_("Opera"), "opera" },
        { ".kde/share/apps/konqueror/bookmarks.xml", N_("Konqueror"), "konqueror" },
        { ".gnome2/epiphany/bookmarks.rdf", N_("Epiphany"), "epiphany" },
    };

    GtkWidget* dialog;
    GtkWidget* content_area;
    GtkSizeGroup* sizegroup;
    GtkWidget* hbox;
    GtkWidget* label;
    GtkWidget* combo;
    GtkComboBox* combobox;
    GtkListStore* model;
    GtkCellRenderer* renderer;
    GtkComboBox* combobox_folder;
    gint icon_width = 16;
    guint i;
    KatzeItem* item;

    if (!browser->bookmarks || !GTK_WIDGET_VISIBLE (browser))
        return;

    dialog = gtk_dialog_new_with_buttons (
        _("Import bookmarks..."), GTK_WINDOW (browser),
        GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        _("_Import bookmarks"), GTK_RESPONSE_ACCEPT,
        NULL);
    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    gtk_window_set_icon_name (GTK_WINDOW (dialog), STOCK_BOOKMARKS);

    gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
    gtk_container_set_border_width (GTK_CONTAINER (content_area), 5);
    sizegroup =  gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    label = gtk_label_new_with_mnemonic (_("_Application:"));
    gtk_size_group_add_widget (sizegroup, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    model = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_INT);
    combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (model));
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer, "icon-name", 1);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer, "width", 3);
    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer, "text", 0);
    combobox = GTK_COMBO_BOX (combo);
    gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (browser)),
                                       GTK_ICON_SIZE_MENU, &icon_width, NULL);
    for (i = 0; i < G_N_ELEMENTS (bookmark_clients); i++)
    {
        gchar* path = g_build_filename (g_get_home_dir (),
                                        bookmark_clients[i].path, NULL);
        if (g_access (path, F_OK) == 0)
            gtk_list_store_insert_with_values (model, NULL, G_MAXINT,
                0, _(bookmark_clients[i].name), 1, bookmark_clients[i].icon,
                2, path, 3, icon_width, -1);
        g_free (path);
    }
    gtk_list_store_insert_with_values (model, NULL, G_MAXINT,
        0, _("Custom..."), 1, NULL, 2, NULL, 3, icon_width, -1);
    gtk_combo_box_set_active (combobox, 0);
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER (content_area), hbox);
    gtk_widget_show_all (hbox);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    label = gtk_label_new_with_mnemonic (_("_Folder:"));
    gtk_size_group_add_widget (sizegroup, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    combo = gtk_combo_box_new_text ();
    combobox_folder = GTK_COMBO_BOX (combo);
    gtk_combo_box_append_text (combobox_folder, _("Toplevel folder"));
    gtk_combo_box_set_active (combobox_folder, 0);
    i = 0;
    while ((item = katze_array_get_nth_item (browser->bookmarks, i++)))
    {
        if (KATZE_IS_ARRAY (item))
        {
            const gchar* name = katze_item_get_name (item);
            if (name)
                gtk_combo_box_append_text (combobox_folder, name);
        }
    }
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER (content_area), hbox);
    gtk_widget_show_all (hbox);
    /* FIXME: Importing into a subfolder doesn't work */
    gtk_widget_set_sensitive (combo, FALSE);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        GtkTreeIter iter;
        gchar* path;
        gchar* selected;
        KatzeArray* folder;
        GError* error;

        gtk_combo_box_get_active_iter (combobox, &iter);
        gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 2, &path, -1);

        selected = gtk_combo_box_get_active_text (combobox_folder);
        folder = browser->bookmarks;
        if (g_strcmp0 (selected, _("Toplevel folder")))
        {
            i = 0;
            while ((item = katze_array_get_nth_item (browser->bookmarks, i++)))
                if (KATZE_IS_ARRAY (item))
                    if (!g_strcmp0 (katze_item_get_name (item), selected))
                    {
                        folder = KATZE_ARRAY (item);
                        break;
                    }
        }
        g_free (selected);

        gtk_widget_destroy (dialog);
        if (!path)
        {
            GtkWidget* file_dialog;

            file_dialog = sokoke_file_chooser_dialog_new (_("Save file as"),
                GTK_WINDOW (browser), GTK_FILE_CHOOSER_ACTION_OPEN);
            if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_OK)
                path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog));
            gtk_widget_destroy (file_dialog);
        }

        error = NULL;
        if (path && !midori_array_from_file (folder, path, NULL, &error))
        {
            sokoke_message_dialog (GTK_MESSAGE_ERROR,
                _("Failed to import bookmarks"), error ? error->message : "");
            if (error)
                g_error_free (error);
        }
        g_free (path);
    }
    else
        gtk_widget_destroy (dialog);
}

static void
_action_bookmarks_export_activate (GtkAction*     action,
                                   MidoriBrowser* browser)
{
    GtkWidget* file_dialog;
    gchar* path = NULL;
    GError* error;

    if (!browser->bookmarks || !GTK_WIDGET_VISIBLE (browser))
        return;

    file_dialog = sokoke_file_chooser_dialog_new (_("Save file as"),
        GTK_WINDOW (browser), GTK_FILE_CHOOSER_ACTION_SAVE);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_dialog),
                                       "bookmarks.xbel");
    if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_OK)
        path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog));
    gtk_widget_destroy (file_dialog);

    if (path == NULL)
        return;

    error = NULL;
    if (!midori_array_to_file (browser->bookmarks, path, "xbel", &error))
    {
        sokoke_message_dialog (GTK_MESSAGE_ERROR,
            _("Failed to export bookmarks"), error ? error->message : "");
        if (error)
            g_error_free (error);
    }
    g_free (path);
}

static void
_action_manage_search_engines_activate (GtkAction*     action,
                                        MidoriBrowser* browser)
{
    static GtkWidget* dialog = NULL;

    if (!GTK_WIDGET_VISIBLE (browser))
        return;

    if (!dialog)
    {
        dialog = midori_search_action_get_dialog (
            MIDORI_SEARCH_ACTION (_action_by_name (browser, "Search")));
        g_signal_connect (dialog, "destroy",
            G_CALLBACK (gtk_widget_destroyed), &dialog);
        gtk_widget_show (dialog);
    }
    else
        gtk_window_present (GTK_WINDOW (dialog));
}

static void
midori_browser_clear_private_data_response_cb (GtkWidget*     dialog,
                                               gint           response_id,
                                               MidoriBrowser* browser)
{
    if (response_id == GTK_RESPONSE_ACCEPT)
    {
        GtkToggleButton* button;
        gint clear_prefs = MIDORI_CLEAR_NONE;
        gint saved_prefs = MIDORI_CLEAR_NONE;

        g_object_get (browser->settings, "clear-private-data", &saved_prefs, NULL);

        #if HAVE_SQLITE
        button = g_object_get_data (G_OBJECT (dialog), "history");
        if (gtk_toggle_button_get_active (button))
        {
            const gchar* sqlcmd;
            sqlite3* db;
            char* errmsg = NULL;

            db = g_object_get_data (G_OBJECT (browser->history), "db");
            sqlcmd = "DELETE FROM history";

            if (sqlite3_exec (db, sqlcmd, NULL, NULL, &errmsg) != SQLITE_OK)
            {
                g_printerr (_("Failed to remove history item: %s\n"), errmsg);
                sqlite3_free (errmsg);
            }
            clear_prefs |= MIDORI_CLEAR_HISTORY;
        }
        #endif
        button = g_object_get_data (G_OBJECT (dialog), "cookies");
        if (gtk_toggle_button_get_active (button))
        {
            SoupSession* session = katze_net_get_session (browser->net);
            SoupSessionFeature* jar = soup_session_get_feature (session,
                SOUP_TYPE_COOKIE_JAR);
            GSList* cookies = soup_cookie_jar_all_cookies (SOUP_COOKIE_JAR (jar));
            SoupCookie* cookie;
            gsize i = 0;
            while ((cookie = g_slist_nth_data (cookies, i++)))
            {
                soup_cookie_jar_delete_cookie (SOUP_COOKIE_JAR (jar), cookie);
                soup_cookie_free (cookie);
            }
            g_slist_free (cookies);
            clear_prefs |= MIDORI_CLEAR_COOKIES;
        }
        button = g_object_get_data (G_OBJECT (dialog), "flash-cookies");
        if (gtk_toggle_button_get_active (button))
        {
            gchar* cache = g_build_filename (g_get_home_dir (), ".macromedia",
                                             "Flash_Player", NULL);
            sokoke_remove_path (cache, TRUE);
            g_free (cache);
            clear_prefs |= MIDORI_CLEAR_FLASH_COOKIES;
        }
        button = g_object_get_data (G_OBJECT (dialog), "website-icons");
        if (gtk_toggle_button_get_active (button))
        {
            gchar* cache = g_build_filename (g_get_user_cache_dir (),
                                             PACKAGE_NAME, "icons", NULL);
            sokoke_remove_path (cache, TRUE);
            g_free (cache);
            clear_prefs |= MIDORI_CLEAR_WEBSITE_ICONS;
        }
        button = g_object_get_data (G_OBJECT (dialog), "trash");
        if (gtk_toggle_button_get_active (button))
        {
            if (browser->trash)
            {
                katze_array_clear (browser->trash);
                _midori_browser_update_actions (browser);
            }
            clear_prefs |= MIDORI_CLEAR_TRASH;
        }
        button = g_object_get_data (G_OBJECT (dialog), "web-cache");
        if (gtk_toggle_button_get_active (button))
        {
            gchar* cache = g_build_filename (g_get_user_cache_dir (),
                                             PACKAGE_NAME, "web", NULL);
            sokoke_remove_path (cache, TRUE);
            g_free (cache);
            clear_prefs |= MIDORI_CLEAR_WEB_CACHE;
        }

        if (clear_prefs != saved_prefs)
        {
            clear_prefs |= (saved_prefs & MIDORI_CLEAR_ON_QUIT);
            g_object_set (browser->settings, "clear-private-data", clear_prefs, NULL);
        }
    }
    if (response_id != GTK_RESPONSE_DELETE_EVENT)
        gtk_widget_destroy (dialog);
}

static void
midori_browser_clear_on_quit_toggled_cb (GtkToggleButton*   button,
                                         MidoriWebSettings* settings)
{
    gint clear_prefs = MIDORI_CLEAR_NONE;
    g_object_get (settings, "clear-private-data", &clear_prefs, NULL);
    clear_prefs ^= MIDORI_CLEAR_ON_QUIT;
    g_object_set (settings, "clear-private-data", clear_prefs, NULL);
}

static void
_action_clear_private_data_activate (GtkAction*     action,
                                     MidoriBrowser* browser)
{
    static GtkWidget* dialog = NULL;

    if (!GTK_WIDGET_VISIBLE (browser))
        return;

    if (!dialog)
    {
        GtkWidget* content_area;
        GdkScreen* screen;
        GtkIconTheme* icon_theme;
        GtkSizeGroup* sizegroup;
        GtkWidget* hbox;
        GtkWidget* alignment;
        GtkWidget* vbox;
        GtkWidget* icon;
        GtkWidget* label;
        GtkWidget* button;

        gint clear_prefs = MIDORI_CLEAR_NONE;
        g_object_get (browser->settings, "clear-private-data", &clear_prefs, NULL);

        dialog = gtk_dialog_new_with_buttons (_("Clear Private Data"),
            GTK_WINDOW (browser),
            GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            _("_Clear private data"), GTK_RESPONSE_ACCEPT, NULL);
        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
        gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), FALSE);
        screen = gtk_widget_get_screen (GTK_WIDGET (browser));
        if (screen)
        {
            icon_theme = gtk_icon_theme_get_for_screen (screen);
            gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_CLEAR);
        }
        sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
        hbox = gtk_hbox_new (FALSE, 4);
        icon = gtk_image_new_from_stock (GTK_STOCK_CLEAR, GTK_ICON_SIZE_DIALOG);
        gtk_size_group_add_widget (sizegroup, icon);
        gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);
        label = gtk_label_new (_("Clear the following data:"));
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
        hbox = gtk_hbox_new (FALSE, 4);
        icon = gtk_image_new ();
        gtk_size_group_add_widget (sizegroup, icon);
        gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);
        vbox = gtk_vbox_new (TRUE, 4);
        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 6, 12, 0);
        #if HAVE_SQLITE
        button = gtk_check_button_new_with_mnemonic (_("History"));
        if ((clear_prefs & MIDORI_CLEAR_HISTORY) == MIDORI_CLEAR_HISTORY)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
        g_object_set_data (G_OBJECT (dialog), "history", button);
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        #endif
        button = gtk_check_button_new_with_mnemonic (_("Cookies"));
        if ((clear_prefs & MIDORI_CLEAR_COOKIES) == MIDORI_CLEAR_COOKIES)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
        g_object_set_data (G_OBJECT (dialog), "cookies", button);
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        button = gtk_check_button_new_with_mnemonic (_("'Flash' Cookies"));
        if ((clear_prefs & MIDORI_CLEAR_FLASH_COOKIES) == MIDORI_CLEAR_FLASH_COOKIES)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
        g_object_set_data (G_OBJECT (dialog), "flash-cookies", button);
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        button = gtk_check_button_new_with_mnemonic (_("Website icons"));
        if ((clear_prefs & MIDORI_CLEAR_WEBSITE_ICONS) == MIDORI_CLEAR_WEBSITE_ICONS)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
        g_object_set_data (G_OBJECT (dialog), "website-icons", button);
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        button = gtk_check_button_new_with_mnemonic (_("_Closed Tabs"));
        if ((clear_prefs & MIDORI_CLEAR_TRASH) == MIDORI_CLEAR_TRASH)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
        g_object_set_data (G_OBJECT (dialog), "trash", button);
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        button = gtk_check_button_new_with_mnemonic (_("Web Cache"));
        if ((clear_prefs & MIDORI_CLEAR_WEB_CACHE) == MIDORI_CLEAR_WEB_CACHE)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
        g_object_set_data (G_OBJECT (dialog), "web-cache", button);
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        gtk_container_add (GTK_CONTAINER (alignment), vbox);
        gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
        button = gtk_check_button_new_with_mnemonic (_("Clear private data when _quitting Midori"));
        if ((clear_prefs & MIDORI_CLEAR_ON_QUIT) == MIDORI_CLEAR_ON_QUIT)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
        g_signal_connect (button, "toggled",
            G_CALLBACK (midori_browser_clear_on_quit_toggled_cb), browser->settings);
        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 2, 0);
        gtk_container_add (GTK_CONTAINER (alignment), button);
        gtk_box_pack_start (GTK_BOX (content_area), alignment, FALSE, FALSE, 0);
        gtk_widget_show_all (content_area);

        g_signal_connect (dialog, "response",
            G_CALLBACK (midori_browser_clear_private_data_response_cb), browser);
        g_signal_connect (dialog, "destroy",
            G_CALLBACK (gtk_widget_destroyed), &dialog);
        gtk_widget_show (dialog);
    }
    else
        gtk_window_present (GTK_WINDOW (dialog));
}

#if WEBKIT_CHECK_VERSION (1, 1, 17)
static void
_action_inspect_page_activate (GtkAction*     action,
                               MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    WebKitWebView* web_view = WEBKIT_WEB_VIEW (gtk_bin_get_child (GTK_BIN (view)));
    WebKitWebInspector* inspector = webkit_web_view_get_inspector (web_view);
    webkit_web_inspector_show (inspector);
}
#endif

static void
_action_tab_previous_activate (GtkAction*     action,
                               MidoriBrowser* browser)
{
    gint n = gtk_notebook_get_current_page (GTK_NOTEBOOK (browser->notebook));
    gtk_notebook_set_current_page (GTK_NOTEBOOK (browser->notebook), n - 1);
}

static void
_action_tab_next_activate (GtkAction*     action,
                           MidoriBrowser* browser)
{
    /* Advance one tab or jump to the first one if we are at the last one */
    gint n = gtk_notebook_get_current_page (GTK_NOTEBOOK (browser->notebook));
    if (n == gtk_notebook_get_n_pages (GTK_NOTEBOOK (browser->notebook)) - 1)
        n = -1;
    gtk_notebook_set_current_page (GTK_NOTEBOOK (browser->notebook), n + 1);
}

static void
_action_tab_current_activate (GtkAction*     action,
                              MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    GtkWidget* child = gtk_bin_get_child (GTK_BIN (view));
    gtk_widget_grab_focus (child ? child : view);
}

static const gchar* credits_authors[] = {
    "Christian Dywan <christian@twotoasts.de>", NULL };
static const gchar* credits_documenters[] = {
    "Christian Dywan <christian@twotoasts.de>", NULL };
static const gchar* credits_artists[] = {
    "Nancy Runge <nancy@twotoasts.de>", NULL };

static const gchar* license =
 "This library is free software; you can redistribute it and/or\n"
 "modify it under the terms of the GNU Lesser General Public\n"
 "License as published by the Free Software Foundation; either\n"
 "version 2.1 of the License, or (at your option) any later version.\n";

static void
_action_about_activate_link (GtkAboutDialog* about,
                             const gchar*    uri,
                             gpointer        user_data)
{
    MidoriBrowser* browser;
    gint n;

    browser = MIDORI_BROWSER (user_data);
    n = midori_browser_add_uri (browser, uri);
    midori_browser_set_current_page (browser, n);
}

static void
_action_about_activate_email (GtkAboutDialog* about,
                              const gchar*    uri,
                              gpointer        user_data)
{
    /* Some email clients need the 'mailto' to function properly */
    gchar* newuri = NULL;
    if (!g_str_has_prefix (uri, "mailto:"))
        newuri = g_strconcat ("mailto:", uri, NULL);

    sokoke_show_uri (NULL, newuri ? newuri : uri, GDK_CURRENT_TIME, NULL);
    g_free (newuri);
}

static void
_action_about_activate (GtkAction*     action,
                        MidoriBrowser* browser)
{
    gchar* comments = g_strdup_printf ("GTK+ %d.%d.%d, WebKitGTK+ %d.%d.%d\n%s",
        GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION,
        WEBKIT_MAJOR_VERSION, WEBKIT_MINOR_VERSION, WEBKIT_MICRO_VERSION,
        _("A lightweight web browser."));

    gtk_about_dialog_set_email_hook (_action_about_activate_email, NULL, NULL);
    gtk_about_dialog_set_url_hook (_action_about_activate_link, browser, NULL);
    gtk_show_about_dialog (GTK_WINDOW (browser),
        "logo-icon-name", gtk_window_get_icon_name (GTK_WINDOW (browser)),
        "name", PACKAGE_NAME,
        "version", PACKAGE_VERSION,
        "comments", comments,
        "copyright", "Copyright © 2007-2009 Christian Dywan",
        "website", "http://www.twotoasts.de",
        "authors", credits_authors,
        "documenters", credits_documenters,
        "artists", credits_artists,
        "license", license,
        "wrap-license", TRUE,
        "translator-credits", _("translator-credits"),
        NULL);
}

static void
_action_help_link_activate (GtkAction*     action,
                            MidoriBrowser* browser)
{
    const gchar* action_name;
    const gchar* uri;
    gint n;
    #if defined (G_OS_WIN32) && defined (DOCDIR)
    gchar* free_uri = NULL;
    #endif

    action_name = gtk_action_get_name (action);
    if  (!strncmp ("HelpContents", action_name, 12))
    {
        #ifdef G_OS_WIN32
        {
            #ifdef DOCDIR
            gchar* path = sokoke_find_data_filename ("doc/midori/user/midori.html");
            uri = free_uri = g_filename_to_uri (path, NULL, NULL);
            if (g_access (path, F_OK) != 0)
            {
                if (g_access (DOCDIR "/midori/user/midori.html", F_OK) == 0)
                    uri = "file://" DOCDIR "/midori/user/midori.html";
                else
            #endif
                    uri = "error:nodocs share/doc/midori/user/midori.html";
            #ifdef DOCDIR
            }
            g_free (path);
            #endif
        }
        #else
        #ifdef DOCDIR
        uri = "file://" DOCDIR "/midori/user/midori.html";
        if (g_access (DOCDIR "/midori/user/midori.html", F_OK) != 0)
        #endif
            uri = "error:nodocs " DOCDIR "/midori/user/midori.html";
        #endif
    }
    else if  (!strncmp ("HelpFAQ", action_name, 7))
        uri = "http://wiki.xfce.org/_export/xhtml/midori_faq";
    else if  (!strncmp ("HelpBugs", action_name, 8))
        uri = "http://www.twotoasts.de/bugs/";
    else
        uri = NULL;

    if (uri)
    {
        n = midori_browser_add_uri (browser, uri);
        midori_browser_set_current_page (browser, n);

        #if defined (G_OS_WIN32) && defined (DOCDIR)
        g_free (free_uri);
        #endif
    }
}

static void
_action_panel_activate (GtkToggleAction* action,
                        MidoriBrowser*   browser)
{
    gboolean active = gtk_toggle_action_get_active (action);
    g_object_set (browser->settings, "show-panel", active, NULL);
    sokoke_widget_set_visible (browser->panel, active);
}

static gboolean
midori_browser_panel_timeout (GtkWidget* hpaned)
{
    gint position = gtk_paned_get_position (GTK_PANED (hpaned));
    MidoriBrowser* browser = midori_browser_get_for_widget (hpaned);
    g_object_set (browser->settings, "last-panel-position", position, NULL);
    browser->panel_timeout = 0;
    return FALSE;
}

static void
midori_panel_notify_position_cb (GObject*       hpaned,
                                 GParamSpec*    pspec,
                                 MidoriBrowser* browser)
{
    if (!browser->panel_timeout)
        browser->panel_timeout = g_timeout_add_full (G_PRIORITY_LOW, 5000,
            (GSourceFunc)midori_browser_panel_timeout, hpaned, NULL);
}

static void
midori_panel_notify_page_cb (MidoriPanel*   panel,
                             GParamSpec*    pspec,
                             MidoriBrowser* browser)
{
    gint page = katze_object_get_boolean (panel, "page");
    if (page > -1)
        g_object_set (browser->settings, "last-panel-page", page, NULL);
}

static void
midori_panel_notify_show_titles_cb (MidoriPanel*   panel,
                                    GParamSpec*    pspec,
                                    MidoriBrowser* browser)
{
    gboolean show_titles = katze_object_get_boolean (panel, "show-titles");
    g_signal_handlers_block_by_func (browser->settings,
        midori_browser_settings_notify, browser);
    g_object_set (browser->settings, "compact-sidepanel", !show_titles, NULL);
    g_signal_handlers_unblock_by_func (browser->settings,
        midori_browser_settings_notify, browser);
}

static void
midori_panel_notify_show_controls_cb (MidoriPanel*   panel,
                                      GParamSpec*    pspec,
                                      MidoriBrowser* browser)
{
    gboolean show_controls = katze_object_get_boolean (panel, "show-controls");
    g_signal_handlers_block_by_func (browser->settings,
        midori_browser_settings_notify, browser);
    g_object_set (browser->settings, "show-panel-controls", show_controls, NULL);
    g_signal_handlers_unblock_by_func (browser->settings,
        midori_browser_settings_notify, browser);
}

static void
midori_panel_notify_right_aligned_cb (MidoriPanel*   panel,
                                      GParamSpec*    pspec,
                                      MidoriBrowser* browser)
{
    gboolean right_aligned = katze_object_get_boolean (panel, "right-aligned");
    GtkWidget* hpaned = gtk_widget_get_parent (browser->panel);
    GtkWidget* vpaned = gtk_widget_get_parent (browser->notebook);
    gint paned_position = gtk_paned_get_position (GTK_PANED (hpaned));
    gint paned_size = hpaned->allocation.width;

    g_object_set (browser->settings, "right-align-sidepanel", right_aligned, NULL);

    g_object_ref (browser->panel);
    g_object_ref (vpaned);
    gtk_container_remove (GTK_CONTAINER (hpaned), browser->panel);
    gtk_container_remove (GTK_CONTAINER (hpaned), vpaned);
    if (right_aligned)
    {
        gtk_paned_pack1 (GTK_PANED (hpaned), vpaned, TRUE, FALSE);
        gtk_paned_pack2 (GTK_PANED (hpaned), browser->panel, FALSE, FALSE);
    }
    else
    {
        gtk_paned_pack1 (GTK_PANED (hpaned), browser->panel, FALSE, FALSE);
        gtk_paned_pack2 (GTK_PANED (hpaned), vpaned, TRUE, FALSE);
    }
    gtk_paned_set_position (GTK_PANED (hpaned), paned_size - paned_position);
    g_object_unref (browser->panel);
    g_object_unref (vpaned);
}

static gboolean
midori_panel_close_cb (MidoriPanel*   panel,
                       MidoriBrowser* browser)
{
    _action_set_active (browser, "Panel", FALSE);
    return FALSE;
}

static void
gtk_notebook_switch_page_cb (GtkWidget*       notebook,
                             GtkNotebookPage* page,
                             guint            page_num,
                             MidoriBrowser*   browser)
{
    GtkWidget* widget;
    GtkAction* action;
    const gchar* text;

    if (!(widget = midori_browser_get_current_tab (browser)))
        return;

    action = _action_by_name (browser, "Location");
    text = midori_location_action_get_text (MIDORI_LOCATION_ACTION (action));
    g_object_set_data_full (G_OBJECT (widget), "midori-browser-typed-text",
                            g_strdup (text), g_free);
}

static void
gtk_notebook_switch_page_after_cb (GtkWidget*       notebook,
                                   GtkNotebookPage* page,
                                   guint            page_num,
                                   MidoriBrowser*   browser)
{
    GtkWidget* widget;
    MidoriView* view;
    const gchar* uri;
    GtkAction* action;
    const gchar* title;
    gchar* window_title;

    if (!(widget = midori_browser_get_current_tab (browser)))
        return;

    view = MIDORI_VIEW (widget);
    uri = g_object_get_data (G_OBJECT (widget), "midori-browser-typed-text");
    if (!uri)
        uri = midori_view_get_display_uri (view);
    action = _action_by_name (browser, "Location");
    midori_location_action_set_text (MIDORI_LOCATION_ACTION (action), uri);
    midori_location_action_set_icon (MIDORI_LOCATION_ACTION (action),
                                     midori_view_get_icon (view));

    title = midori_view_get_display_title (view);
    window_title = g_strconcat (title, " - ", g_get_application_name (), NULL);
    gtk_window_set_title (GTK_WINDOW (browser), window_title);
    g_free (window_title);

    if (browser->proxy_array)
        katze_item_set_meta_integer (KATZE_ITEM (browser->proxy_array), "current",
                                     midori_browser_get_current_page (browser));
    g_object_freeze_notify (G_OBJECT (browser));
    g_object_notify (G_OBJECT (browser), "uri");
    g_object_notify (G_OBJECT (browser), "tab");
    g_object_thaw_notify (G_OBJECT (browser));

    _midori_browser_set_statusbar_text (browser, NULL);
    _midori_browser_update_interface (browser);
    _midori_browser_update_progress (browser, view);
}

static void
midori_browser_notebook_page_reordered_cb (GtkNotebook*   notebook,
                                           MidoriView*    view,
                                           guint          page_num,
                                           MidoriBrowser* browser)
{
    KatzeItem* item = midori_view_get_proxy_item (view);
    katze_array_move_item (browser->proxy_array, item, page_num);

    g_object_freeze_notify (G_OBJECT (browser));
    g_object_notify (G_OBJECT (browser), "uri");
    g_object_notify (G_OBJECT (browser), "tab");
    g_object_thaw_notify (G_OBJECT (browser));
}

static gboolean
midori_browser_notebook_button_press_event_after_cb (GtkNotebook*    notebook,
                                                     GdkEventButton* event,
                                                     MidoriBrowser*  browser)
{
    if (event->window != notebook->event_window)
        return FALSE;

    /* FIXME: Handle double click only when it wasn't handled by GtkNotebook */

    /* Open a new tab on double click or middle mouse click */
    if (/*(event->type == GDK_2BUTTON_PRESS && event->button == 1)
    || */(event->type == GDK_BUTTON_PRESS && event->button == 2))
    {
        gint n;
        GtkWidget* view;

        view = g_object_new (MIDORI_TYPE_VIEW,
                             "settings", browser->settings,
                             "net", browser->net, NULL);
        midori_view_set_uri (MIDORI_VIEW (view), "");
        gtk_widget_show (view);
        g_object_set_data (G_OBJECT (view), "midori-view-append", (void*)1);
        n = midori_browser_add_tab (browser, view);
        midori_browser_set_current_page (browser, n);

        return TRUE;
    }

    return FALSE;
}

static void
_action_undo_tab_close_activate (GtkAction*     action,
                                 MidoriBrowser* browser)
{
    guint last;
    KatzeItem* item;
    guint n;

    if (!browser->trash)
        return;

    /* Reopen the most recent trash item */
    last = katze_array_get_length (browser->trash) - 1;
    item = katze_array_get_nth_item (browser->trash, last);
    n = midori_browser_add_item (browser, item);
    midori_browser_set_current_page (browser, n);
    katze_array_remove_item (browser->trash, item);
    _midori_browser_update_actions (browser);
}

static void
_action_trash_empty_activate (GtkAction*     action,
                              MidoriBrowser* browser)
{
    if (browser->trash)
    {
        katze_array_clear (browser->trash);
        _midori_browser_update_actions (browser);
    }
}

static const GtkActionEntry entries[] = {
 { "File", NULL, N_("_File") },
 { "WindowNew", STOCK_WINDOW_NEW,
   N_("New _Window"), "<Ctrl>n",
   N_("Open a new window"), G_CALLBACK (_action_window_new_activate) },
 { "TabNew", STOCK_TAB_NEW,
   NULL, "<Ctrl>t",
   N_("Open a new tab"), G_CALLBACK (_action_tab_new_activate) },
 { "PrivateBrowsing", NULL,
   N_("P_rivate Browsing"), "<Ctrl><Shift>n",
   N_("Don't save any private data while browsing"),
   G_CALLBACK (_action_private_browsing_activate), },
 { "Open", GTK_STOCK_OPEN,
   NULL, "<Ctrl>o",
   N_("Open a file"), G_CALLBACK (_action_open_activate) },
 { "SaveAs", GTK_STOCK_SAVE_AS,
   NULL, "<Ctrl>s",
   N_("Save to a file"), G_CALLBACK (_action_save_as_activate) },
 { "AddSpeedDial", NULL,
   N_("Add to Speed _dial"), "<Ctrl>h",
   N_("Add shortcut to speed dial"), G_CALLBACK (_action_add_speed_dial_activate) },
 { "AddDesktopShortcut", NULL,
   N_("Add Shortcut to the _desktop"), "<Ctrl>h",
   N_("Add shortcut to the desktop"), G_CALLBACK (_action_add_desktop_shortcut_activate) },
 { "AddNewsFeed", NULL,
   N_("Subscribe to News _feed"), NULL,
   N_("Subscribe to this news feed"), G_CALLBACK (_action_add_news_feed_activate) },
 { "CompactAdd", GTK_STOCK_ADD,
  NULL, NULL,
  NULL, G_CALLBACK (_action_compact_add_activate) },
 { "TabClose", GTK_STOCK_CLOSE,
   N_("_Close Tab"), "<Ctrl>w",
   N_("Close the current tab"), G_CALLBACK (_action_tab_close_activate) },
 { "WindowClose", NULL,
   N_("C_lose Window"), "<Ctrl><Shift>w",
   N_("Close this window"), G_CALLBACK (_action_window_close_activate) },
 { "Print", GTK_STOCK_PRINT,
   NULL, "<Ctrl>p",
   N_("Print the current page"), G_CALLBACK (_action_print_activate) },
 { "Quit", GTK_STOCK_QUIT,
   NULL, "<Ctrl>q",
   N_("Quit the application"), G_CALLBACK (_action_quit_activate) },

 { "Edit", NULL, N_("_Edit"), NULL, NULL, G_CALLBACK (_action_edit_activate) },
 #if WEBKIT_CHECK_VERSION (1, 1, 14)
 { "Undo", GTK_STOCK_UNDO,
   NULL, "<Ctrl>z",
   N_("Undo the last modification"), G_CALLBACK (_action_undo_activate) },
   { "Redo", GTK_STOCK_REDO,
   NULL, "<Ctrl><Shift>z",
   N_("Redo the last modification"), G_CALLBACK (_action_redo_activate) },
 #endif
 { "Cut", GTK_STOCK_CUT,
   NULL, "<Ctrl>x",
   N_("Cut the selected text"), G_CALLBACK (_action_cut_activate) },
 { "Copy", GTK_STOCK_COPY,
   NULL, "<Ctrl>c",
   N_("Copy the selected text"), G_CALLBACK (_action_copy_activate) },
 { "Copy_", GTK_STOCK_COPY,
   NULL, "<Ctrl>c",
   N_("Copy the selected text"), G_CALLBACK (_action_copy_activate) },
 { "Paste", GTK_STOCK_PASTE,
   NULL, "<Ctrl>v",
   N_("Paste text from the clipboard"), G_CALLBACK (_action_paste_activate) },
 { "Delete", GTK_STOCK_DELETE,
   NULL, NULL,
   N_("Delete the selected text"), G_CALLBACK (_action_delete_activate) },
 { "SelectAll", GTK_STOCK_SELECT_ALL,
   NULL, "<Ctrl>a",
   N_("Select all text"), G_CALLBACK (_action_select_all_activate) },
 { "Find", GTK_STOCK_FIND,
   NULL, "<Ctrl>f",
   N_("Find a word or phrase in the page"), G_CALLBACK (_action_find_activate) },
 { "FindNext", GTK_STOCK_GO_FORWARD,
   N_("Find _Next"), "<Ctrl>g",
   N_("Find the next occurrence of a word or phrase"), G_CALLBACK (_action_find_next_activate) },
 { "FindPrevious", GTK_STOCK_GO_BACK,
   N_("Find _Previous"), "<Ctrl><Shift>g",
   N_("Find the previous occurrence of a word or phrase"),
   G_CALLBACK (_action_find_previous_activate) },
 { "Preferences", GTK_STOCK_PREFERENCES,
   NULL, "<Ctrl><Alt>p",
   N_("Configure the application preferences"), G_CALLBACK (_action_preferences_activate) },

 { "View", NULL, N_("_View") },
 { "Toolbars", NULL, N_("_Toolbars") },
 { "Reload", GTK_STOCK_REFRESH,
   NULL, "<Ctrl>r",
   N_("Reload the current page"), G_CALLBACK (_action_reload_stop_activate) },
 { "Stop", GTK_STOCK_STOP,
   NULL, "Escape",
   N_("Stop loading the current page"), G_CALLBACK (_action_reload_stop_activate) },
 { "ReloadStop", GTK_STOCK_STOP,
   NULL, "<Ctrl>r",
   N_("Reload the current page"), G_CALLBACK (_action_reload_stop_activate) },
 { "ZoomIn", GTK_STOCK_ZOOM_IN,
   NULL, "<Ctrl>plus",
   N_("Increase the zoom level"), G_CALLBACK (_action_zoom_in_activate) },
 { "ZoomOut", GTK_STOCK_ZOOM_OUT,
   NULL, "<Ctrl>minus",
   N_("Decrease the zoom level"), G_CALLBACK (_action_zoom_out_activate) },
 { "ZoomNormal", GTK_STOCK_ZOOM_100,
   NULL, "<Ctrl>0",
   N_("Reset the zoom level"), G_CALLBACK (_action_zoom_normal_activate) },
 { "Encoding", NULL, N_("_Encoding") },
 { "SourceView", NULL,
   N_("View So_urce"), "<Ctrl><Alt>U",
   N_("View the source code of the page"), G_CALLBACK (_action_source_view_activate) },
 { "Fullscreen", GTK_STOCK_FULLSCREEN,
   NULL, "F11",
   N_("Toggle fullscreen view"), G_CALLBACK (_action_fullscreen_activate) },
 #if WEBKIT_CHECK_VERSION (1, 1, 4)
 { "ScrollLeft", NULL,
   N_("Scroll _Left"), "Left"/*"h"*/,
   N_("Scroll to the left"), G_CALLBACK (_action_scroll_somewhere_activate) },
 { "ScrollDown", NULL,
   N_("Scroll _Down"), "Page_Down"/*"j"*/,
   N_("Scroll down"), G_CALLBACK (_action_scroll_somewhere_activate) },
 { "ScrollUp", NULL,
   N_("Scroll _Up"), "Page_Up"/*"k"*/,
   N_("Scroll up"), G_CALLBACK (_action_scroll_somewhere_activate) },
 { "ScrollRight", NULL,
   N_("Scroll _Right"), "Right"/*"l"*/,
   N_("Scroll to the right"), G_CALLBACK (_action_scroll_somewhere_activate) },
 #endif

 { "Go", NULL, N_("_Go") },
 { "Back", GTK_STOCK_GO_BACK,
   NULL, "<Alt>Left",
   N_("Go back to the previous page"), G_CALLBACK (_action_back_activate) },
 { "Forward", GTK_STOCK_GO_FORWARD,
   NULL, "<Alt>Right",
   N_("Go forward to the next page"), G_CALLBACK (_action_forward_activate) },
 { "Previous", GTK_STOCK_MEDIA_PREVIOUS,
   NULL, "<Ctrl>Left",
 /* i18n: Visit the previous logical page, ie. in a forum or blog */
   N_("Go to the previous sub-page"), G_CALLBACK (_action_previous_activate) },
 { "Next", GTK_STOCK_MEDIA_NEXT,
   NULL, "<Ctrl>Right",
 /* i18n: Visit the following logical page, ie. in a forum or blog */
   N_("Go to the next sub-page"), G_CALLBACK (_action_next_activate) },
 { "Homepage", STOCK_HOMEPAGE,
   NULL, "<Alt>Home",
   N_("Go to your homepage"), G_CALLBACK (_action_homepage_activate) },
 { "TrashEmpty", GTK_STOCK_CLEAR,
   N_("Empty Trash"), "",
   N_("Delete the contents of the trash"), G_CALLBACK (_action_trash_empty_activate) },
 { "UndoTabClose", GTK_STOCK_UNDELETE,
   N_("Undo _Close Tab"), "<Ctrl><Shift>t",
   N_("Open the last closed tab"), G_CALLBACK (_action_undo_tab_close_activate) },

 { "BookmarkAdd", STOCK_BOOKMARK_ADD,
   NULL, "<Ctrl>d",
   N_("Add a new bookmark"), G_CALLBACK (_action_bookmark_add_activate) },
 { "BookmarkFolderAdd", NULL,
   N_("Add a new _folder"), "",
   N_("Add a new bookmark folder"), G_CALLBACK (_action_bookmark_folder_add_activate) },
 { "BookmarksImport", NULL,
   N_("_Import bookmarks"), "",
   NULL, G_CALLBACK (_action_bookmarks_import_activate) },
 { "BookmarksExport", NULL,
   N_("_Export bookmarks"), "",
   NULL, G_CALLBACK (_action_bookmarks_export_activate) },
 { "ManageSearchEngines", GTK_STOCK_PROPERTIES,
   N_("_Manage Search Engines"), "<Ctrl><Alt>s",
   N_("Add, edit and remove search engines..."),
   G_CALLBACK (_action_manage_search_engines_activate) },
 { "ClearPrivateData", NULL,
   N_("_Clear Private Data"), "<Ctrl><Shift>Delete",
   N_("Clear private data..."),
   G_CALLBACK (_action_clear_private_data_activate) },
 #if WEBKIT_CHECK_VERSION (1, 1, 17)
 { "InspectPage", NULL,
   N_("_Inspect Page"), "<Ctrl><Shift>i",
   N_("Inspect page details and access developer tools..."),
   G_CALLBACK (_action_inspect_page_activate) },
 #endif

 { "TabPrevious", GTK_STOCK_GO_BACK,
   N_("_Previous Tab"), "<Ctrl>Page_Up",
   N_("Switch to the previous tab"), G_CALLBACK (_action_tab_previous_activate) },
 { "TabNext", GTK_STOCK_GO_FORWARD,
   N_("_Next Tab"), "<Ctrl>Page_Down",
   N_("Switch to the next tab"), G_CALLBACK (_action_tab_next_activate) },
 { "TabCurrent", NULL,
   N_("Focus _Current Tab"), "<Ctrl>Home",
   N_("Focus the current tab"), G_CALLBACK (_action_tab_current_activate) },
 { "LastSession", NULL,
   N_("Open last _session"), NULL,
   N_("Open the tabs saved in the last session"), NULL },

 { "Help", NULL, N_("_Help") },
 { "HelpContents", GTK_STOCK_HELP,
   N_("_Contents"), "F1",
   N_("Show the documentation"), G_CALLBACK (_action_help_link_activate) },
 { "HelpFAQ", NULL,
   N_("_Frequent Questions"), NULL,
   N_("Show the Frequently Asked Questions"), G_CALLBACK (_action_help_link_activate) },
 { "HelpBugs", NULL,
   N_("_Report a Bug"), NULL,
   N_("Open Midori's bug tracker"), G_CALLBACK (_action_help_link_activate) },
 { "About", GTK_STOCK_ABOUT,
   NULL, "",
   N_("Show information about the program"), G_CALLBACK (_action_about_activate) },
 { "Dummy", NULL, "Dummy" },
 };
 static const guint entries_n = G_N_ELEMENTS (entries);

static const GtkToggleActionEntry toggle_entries[] = {
 { "Menubar", NULL,
   N_("_Menubar"), "",
   N_("Show menubar"), G_CALLBACK (_action_menubar_activate),
   FALSE },
 { "Navigationbar", NULL,
   N_("_Navigationbar"), "",
   N_("Show navigationbar"), G_CALLBACK (_action_navigationbar_activate),
   FALSE },
 { "Panel", GTK_STOCK_INDENT,
   N_("Side_panel"), "F9",
   N_("Show sidepanel"), G_CALLBACK (_action_panel_activate),
   FALSE },
 { "Bookmarkbar", NULL,
   N_("_Bookmarkbar"), "",
   N_("Show bookmarkbar"), G_CALLBACK (_action_bookmarkbar_activate),
   FALSE },
 { "Transferbar", NULL,
   N_("_Transferbar"), "",
   N_("Show transferbar"), G_CALLBACK (_action_transferbar_activate),
   FALSE },
 { "Statusbar", NULL,
   N_("_Statusbar"), "",
   N_("Show statusbar"), G_CALLBACK (_action_statusbar_activate),
   FALSE },
 };
 static const guint toggle_entries_n = G_N_ELEMENTS (toggle_entries);

static const GtkRadioActionEntry encoding_entries[] = {
{ "EncodingAutomatic", NULL,
    N_("_Automatic"), "",
    NULL, 1 },
 { "EncodingChinese", NULL,
    N_("Chinese (BIG5)"), "",
    NULL, 1 },
 { "EncodingJapanese", NULL,
 /* i18n: A double underscore "__" is used to prevent an unwanted mnemonic */
    N_("Japanese (SHIFT__JIS)"), "",
    NULL, 1 },
 { "EncodingKorean", NULL,
    N_("Korean (EUC-KR)"), "",
    NULL, 1 },
 { "EncodingRussian", NULL,
    N_("Russian (KOI8-R)"), "",
    NULL, 1 },
 { "EncodingUnicode", NULL,
    N_("Unicode (UTF-8)"), "",
    NULL, 1 },
 { "EncodingWestern", NULL,
    N_("Western (ISO-8859-1)"), "",
    NULL, 1 },
 { "EncodingCustom", NULL,
    N_("Custom..."), "",
    NULL, 1 },
 };
 static const guint encoding_entries_n = G_N_ELEMENTS (encoding_entries);

static void
midori_browser_window_state_event_cb (MidoriBrowser*       browser,
                                      GdkEventWindowState* event)
{
    MidoriWindowState window_state = MIDORI_WINDOW_NORMAL;
    if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)
        window_state = MIDORI_WINDOW_MINIMIZED;
    else if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
        window_state = MIDORI_WINDOW_MAXIMIZED;
    else if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)
        window_state = MIDORI_WINDOW_FULLSCREEN;
    g_object_set (browser->settings, "last-window-state", window_state, NULL);

    if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
    {
        if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)
        {
            gtk_widget_hide (browser->menubar);
        }
        else
        {
            if (katze_object_get_boolean (browser->settings, "show-menubar"))
                gtk_widget_show (browser->menubar);
        }
    }
}

static gboolean
midori_browser_alloc_timeout (MidoriBrowser* browser)
{
    GtkWidget* widget = GTK_WIDGET (browser);
    GdkWindowState state = gdk_window_get_state (widget->window);

    if (!(state &
        (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN)))
    {
        if (widget->allocation.width != browser->last_window_width)
        {
            browser->last_window_width = widget->allocation.width;
            g_object_set (browser->settings,
                "last-window-width", browser->last_window_width, NULL);
        }
        if (widget->allocation.height != browser->last_window_height)
        {
            browser->last_window_height = widget->allocation.height;
            g_object_set (browser->settings,
                "last-window-height", widget->allocation.height, NULL);
        }
    }

    browser->alloc_timeout = 0;
    return FALSE;
}

static void
midori_browser_size_allocate_cb (MidoriBrowser* browser,
                                 GtkAllocation* allocation)
{
    GtkWidget* widget = GTK_WIDGET (browser);

    if (GTK_WIDGET_REALIZED (widget) && !browser->alloc_timeout)
    {
        gpointer last_page;

        if ((last_page = g_object_get_data (G_OBJECT (browser), "last-page")))
        {
            midori_panel_set_current_page (MIDORI_PANEL (browser->panel),
                GPOINTER_TO_INT (last_page));
                g_object_set_data (G_OBJECT (browser), "last-page", NULL);
        }

        browser->alloc_timeout = g_timeout_add_full (G_PRIORITY_LOW, 5000,
            (GSourceFunc)midori_browser_alloc_timeout, browser, NULL);
    }
}

static void
midori_browser_destroy_cb (MidoriBrowser* browser)
{
    if (browser->bookmarks)
        midori_browser_set_bookmarks (browser, NULL);

    if (G_UNLIKELY (browser->panel_timeout))
        g_source_remove (browser->panel_timeout);
    if (G_UNLIKELY (browser->alloc_timeout))
        g_source_remove (browser->alloc_timeout);

    /* Destroy panel first, so panels don't need special care */
    gtk_widget_destroy (browser->panel);
    /* Destroy tabs second, so child widgets don't need special care */
    gtk_container_foreach (GTK_CONTAINER (browser->notebook),
                           (GtkCallback) gtk_widget_destroy, NULL);
}

static const gchar* ui_markup =
 "<ui>"
  "<menubar>"
   "<menu action='File'>"
    "<menuitem action='WindowNew'/>"
    "<menuitem action='TabNew'/>"
    "<menuitem action='PrivateBrowsing'/>"
    "<separator/>"
    "<menuitem action='Open'/>"
    "<separator/>"
    "<menuitem action='SaveAs'/>"
    "<menuitem action='AddSpeedDial'/>"
    "<menuitem action='AddDesktopShortcut'/>"
    "<separator/>"
    "<menuitem action='TabClose'/>"
    "<menuitem action='WindowClose'/>"
    "<separator/>"
    "<menuitem action='Print'/>"
    "<separator/>"
    "<menuitem action='Quit'/>"
   "</menu>"
   "<menu action='Edit'>"
    #if WEBKIT_CHECK_VERSION (1, 1, 14)
    "<menuitem action='Undo'/>"
    "<menuitem action='Redo'/>"
    "<separator/>"
    #endif
    "<menuitem action='Cut'/>"
    "<menuitem action='Copy'/>"
    "<menuitem action='Paste'/>"
    "<menuitem action='Delete'/>"
    "<separator/>"
    "<menuitem action='SelectAll'/>"
    "<separator/>"
    "<menuitem action='Find'/>"
    "<menuitem action='FindNext'/>"
    #ifndef G_OS_WIN32
    "<separator/>"
    "<menuitem action='Preferences'/>"
    #endif
   "</menu>"
   "<menu action='View'>"
    "<menu action='Toolbars'>"
     "<menuitem action='Menubar'/>"
     "<menuitem action='Navigationbar'/>"
     "<menuitem action='Bookmarkbar'/>"
     "<menuitem action='Transferbar'/>"
     "<menuitem action='Statusbar'/>"
    "</menu>"
    "<menuitem action='Panel'/>"
    "<separator/>"
    "<menuitem action='Stop'/>"
    "<menuitem action='Reload'/>"
    "<separator/>"
    "<menuitem action='ZoomIn'/>"
    "<menuitem action='ZoomOut'/>"
    "<menuitem action='ZoomNormal'/>"
    "<separator/>"
    "<menu action='Encoding'>"
     "<menuitem action='EncodingAutomatic'/>"
     "<menuitem action='EncodingChinese'/>"
     "<menuitem action='EncodingJapanese'/>"
     "<menuitem action='EncodingKorean'/>"
     "<menuitem action='EncodingRussian'/>"
     "<menuitem action='EncodingUnicode'/>"
     "<menuitem action='EncodingWestern'/>"
     "<menuitem action='EncodingCustom'/>"
    "</menu>"
    "<menuitem action='SourceView'/>"
    "<menuitem action='Fullscreen'/>"
   "</menu>"
   "<menu action='Go'>"
    "<menuitem action='Back'/>"
    "<menuitem action='Forward'/>"
    "<menuitem action='Previous'/>"
    "<menuitem action='Next'/>"
    "<menuitem action='Homepage'/>"
    "<menuitem action='Location'/>"
    "<menuitem action='Search'/>"
    "<menuitem action='Trash'/>"
    "<menuitem action='RecentlyVisited'/>"
   "</menu>"
   "<menuitem action='Bookmarks'/>"
   "<menuitem action='Tools'/>"
   "<menuitem action='Window'/>"
   "<menu action='Help'>"
    "<menuitem action='HelpContents'/>"
    "<menuitem action='HelpFAQ'/>"
    "<menuitem action='HelpBugs'/>"
    "<separator/>"
    "<menuitem action='About'/>"
   "</menu>"
   /* For accelerators to work all actions need to be used
     *somewhere* in the UI definition */
   "<menu action='Dummy'>"
    "<menuitem action='ScrollLeft'/>"
    "<menuitem action='ScrollDown'/>"
    "<menuitem action='ScrollUp'/>"
    "<menuitem action='ScrollRight'/>"
    "<menuitem action='FindPrevious'/>"
    "<menuitem action='BookmarkAdd'/>"
    "<menuitem action='BookmarkFolderAdd'/>"
    "<menuitem action='BookmarksImport'/>"
    "<menuitem action='BookmarksExport'/>"
    "<menuitem action='ManageSearchEngines'/>"
    "<menuitem action='ClearPrivateData'/>"
    "<menuitem action='TabPrevious'/>"
    "<menuitem action='TabNext'/>"
    "<menuitem action='TabCurrent'/>"
    "<menuitem action='LastSession'/>"
    "<menuitem action='UndoTabClose'/>"
    "<menuitem action='TrashEmpty'/>"
    "<menuitem action='Preferences'/>"
   "</menu>"
  "</menubar>"
  "<toolbar name='toolbar_navigation'>"
  "</toolbar>"
 "</ui>";

static void
midori_browser_realize_cb (GtkStyle*      style,
                           MidoriBrowser* browser)
{
    GdkScreen* screen;
    GtkIconTheme* icon_theme;
    #ifdef HAVE_HILDON_2_2
    /* hildon_gtk_window_enable_zoom_keys */
    guint32 set = 1;
    gdk_property_change (GTK_WIDGET (browser)->window,
                         gdk_atom_intern ("_HILDON_ZOOM_KEY_ATOM", FALSE),
                         gdk_x11_xatom_to_atom (XA_INTEGER),
                         32, GDK_PROP_MODE_REPLACE,
                         (const guchar *) &set, 1);
    #endif

    screen = gtk_widget_get_screen (GTK_WIDGET (browser));
    if (screen)
    {
        icon_theme = gtk_icon_theme_get_for_screen (screen);
        if (gtk_icon_theme_has_icon (icon_theme, "midori"))
            gtk_window_set_icon_name (GTK_WINDOW (browser), "midori");
        else
            gtk_window_set_icon_name (GTK_WINDOW (browser), "web-browser");
    }
}

static void
midori_browser_entry_clear_icon_released_cb (GtkIconEntry* entry,
                                             gint          icon_pos,
                                             gint          button,
                                             gpointer      user_data)
{
    if (icon_pos == GTK_ICON_ENTRY_SECONDARY)
        gtk_entry_set_text (GTK_ENTRY (entry), "");
}

static void
midori_browser_new_history_item (MidoriBrowser* browser,
                                 KatzeItem**    item)
{
    time_t now;
    gint64 day;
    #if HAVE_SQLITE
    sqlite3* db;
    static sqlite3_stmt* stmt = NULL;
    #endif

    now = time (NULL);
    katze_item_set_added (*item, now);
    day = sokoke_time_t_to_julian (&now);

    #if HAVE_SQLITE
    db = g_object_get_data (G_OBJECT (browser->history), "db");
    if (!stmt)
    {
        const gchar* sqlcmd;

        sqlcmd = "INSERT INTO history (uri, title, date, day) VALUES (?,?,?,?)";
        sqlite3_prepare_v2 (db, sqlcmd, -1, &stmt, NULL);
    }
    sqlite3_bind_text (stmt, 1, katze_item_get_uri (*item), -1, 0);
    sqlite3_bind_text (stmt, 2, katze_item_get_name (*item), -1, 0);
    sqlite3_bind_int64 (stmt, 3, katze_item_get_added (*item));
    sqlite3_bind_int64 (stmt, 4, day);

    if (sqlite3_step (stmt) != SQLITE_DONE)
        g_printerr (_("Failed to insert new history item: %s\n"), sqlite3_errmsg (db));
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    #endif
}

static void
midori_browser_history_remove_item_cb (KatzeArray*    folder,
                                       KatzeItem*     item,
                                       MidoriBrowser* browser)
{
    GtkAction* action = _action_by_name (browser, "RecentlyVisited");
    if ((KatzeArray*)item == katze_array_action_get_array (KATZE_ARRAY_ACTION (action)))
        g_object_set (action, "array", NULL, NULL);
}

static void
midori_browser_history_clear_cb (KatzeArray*    history,
                                 MidoriBrowser* browser)
{
    GtkAction* location_action = _action_by_name (browser, "Location");
    midori_location_action_clear (MIDORI_LOCATION_ACTION (location_action));
}

static void
midori_browser_set_history (MidoriBrowser* browser,
                            KatzeArray*    history)
{
    time_t now;
    gint64 day;

    if (browser->history == history)
        return;

    if (browser->history)
        g_signal_handlers_disconnect_by_func (browser->history,
                                              midori_browser_history_clear_cb,
                                              browser);
    if (history)
        g_object_ref (history);
    katze_object_assign (browser->history, history);
    g_object_set (_action_by_name (browser, "RecentlyVisited"),
                  "array", history, NULL);

    midori_browser_history_clear_cb (history, browser);

    if (!history)
        return;

    g_signal_connect (browser->history, "remove-item",
        G_CALLBACK (midori_browser_history_remove_item_cb), browser);
    g_signal_connect (browser->history, "clear",
                      G_CALLBACK (midori_browser_history_clear_cb), browser);

    now = time (NULL);
    day = sokoke_time_t_to_julian (&now);

    g_object_set (_action_by_name (browser, "Location"), "history",
                  browser->history, NULL);
}

static void
midori_browser_accel_switch_tab_activate_cb (GtkAccelGroup*  accel_group,
                                             GObject*        acceleratable,
                                             guint           keyval,
                                             GdkModifierType modifiers)
{
    GtkAccelGroupEntry* entry;

    if ((entry = gtk_accel_group_query (accel_group, keyval, modifiers, NULL)))
    {
        gint n;
        MidoriBrowser* browser;
        GtkWidget* view;

        n = keyval - GDK_0;
        if (n == 0)
            n = 10;
        browser = g_object_get_data (G_OBJECT (accel_group), "midori-browser");
        if ((view = gtk_notebook_get_nth_page (GTK_NOTEBOOK (browser->notebook),
                                          n - 1)))
            midori_browser_set_current_tab (browser, view);
    }
}

static void
midori_browser_ui_manager_connect_proxy_cb (GtkUIManager*  ui_manager,
                                            GtkAction*     action,
                                            GtkWidget*     proxy,
                                            MidoriBrowser* browser)
{
    if (GTK_IS_MENU_ITEM (proxy))
    {
        g_signal_connect (proxy, "select",
            G_CALLBACK (midori_browser_menu_item_select_cb), browser);
        g_signal_connect (proxy, "deselect",
            G_CALLBACK (midori_browser_menu_item_deselect_cb), browser);
    }
}

static void
midori_browser_ui_manager_disconnect_proxy_cb (GtkUIManager*  ui_manager,
                                               GtkAction*     action,
                                               GtkWidget*     proxy,
                                               MidoriBrowser* browser)
{
    if (GTK_IS_MENU_ITEM (proxy))
    {
        g_signal_handlers_disconnect_by_func (proxy,
            midori_browser_menu_item_select_cb, browser);
        g_signal_handlers_disconnect_by_func (proxy,
            midori_browser_menu_item_deselect_cb, browser);
    }
}

#ifdef HAVE_HILDON_2_2
static void
midori_browser_set_portrait_mode (MidoriBrowser* browser,
                                  gboolean       portrait)
{
    if (portrait)
        hildon_gtk_window_set_portrait_flags (GTK_WINDOW (browser),
                                              HILDON_PORTRAIT_MODE_REQUEST);
    else
        hildon_gtk_window_set_portrait_flags (GTK_WINDOW (browser),
                                              ~HILDON_PORTRAIT_MODE_REQUEST);
    _action_set_visible (browser, "Tools", !portrait);
    _action_set_visible (browser, "CompactAdd", !portrait);
    _action_set_visible (browser, "Back", !portrait);
    _action_set_visible (browser, "SourceView", !portrait);
    _action_set_visible (browser, "Fullscreen", !portrait);
}

static DBusHandlerResult
midori_browser_mce_filter_cb (DBusConnection* connection,
                              DBusMessage*    message,
                              gpointer        data)
{
    if (dbus_message_is_signal (message, MCE_SIGNAL_IF, MCE_DEVICE_ORIENTATION_SIG))
    {
        DBusError error;
        char *rotation, *stand, *face;
        int x, y, z;

        dbus_error_init (&error);
        if (dbus_message_get_args (message,
                                   &error,
                                   DBUS_TYPE_STRING, &rotation,
                                   DBUS_TYPE_STRING, &stand,
                                   DBUS_TYPE_STRING, &face,
                                   DBUS_TYPE_INT32,  &x,
                                   DBUS_TYPE_INT32,  &y,
                                   DBUS_TYPE_INT32,  &z, DBUS_TYPE_INVALID))
        {
            gboolean portrait = !strcmp (rotation, MCE_ORIENTATION_PORTRAIT);
            midori_browser_set_portrait_mode (MIDORI_BROWSER (data), portrait);
        }
        else
        {
            g_warning ("%s: %s\n", error.name, error.message);
            dbus_error_free (&error);
        }
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
#endif

static void
midori_browser_init (MidoriBrowser* browser)
{
    GtkWidget* vbox;
    GtkUIManager* ui_manager;
    GtkAccelGroup* accel_group;
    guint i;
    GError* error;
    GtkAction* action;
    #if !HAVE_HILDON
    GtkWidget* menuitem;
    #endif
    GtkWidget* homepage;
    GtkWidget* back;
    GtkWidget* forward;
    GtkSettings* gtk_settings;
    GtkWidget* hpaned;
    GtkWidget* vpaned;
    GtkToolItem* toolitem;
    GtkRcStyle* rcstyle;
    GtkWidget* label;
    GtkWidget* scrolled;
    GtkWidget* icon;

    browser->net = katze_net_new ();

    browser->settings = midori_web_settings_new ();
    browser->proxy_array = katze_array_new (KATZE_TYPE_ARRAY);
    browser->bookmarks = NULL;
    browser->trash = NULL;
    browser->search_engines = NULL;

    /* Setup the window metrics */
    g_signal_connect (browser, "realize",
                      G_CALLBACK (midori_browser_realize_cb), browser);
    g_signal_connect (browser, "window-state-event",
                      G_CALLBACK (midori_browser_window_state_event_cb), NULL);
    g_signal_connect (browser, "size-allocate",
                      G_CALLBACK (midori_browser_size_allocate_cb), NULL);
    g_signal_connect (browser, "destroy",
                      G_CALLBACK (midori_browser_destroy_cb), NULL);
    gtk_window_set_role (GTK_WINDOW (browser), "browser");
    gtk_window_set_icon_name (GTK_WINDOW (browser), "web-browser");
    gtk_window_set_title (GTK_WINDOW (browser), g_get_application_name ());
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (browser), vbox);
    gtk_widget_show (vbox);

    /* Let us see some ui manager magic */
    browser->action_group = gtk_action_group_new ("Browser");
    gtk_action_group_set_translation_domain (browser->action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (browser->action_group,
                                  entries, entries_n, browser);
    gtk_action_group_add_toggle_actions (browser->action_group,
        toggle_entries, toggle_entries_n, browser);
    gtk_action_group_add_radio_actions (browser->action_group,
        encoding_entries, encoding_entries_n, 0,
        G_CALLBACK (_action_view_encoding_activate), browser);
    ui_manager = gtk_ui_manager_new ();
    g_signal_connect (ui_manager, "connect-proxy",
        G_CALLBACK (midori_browser_ui_manager_connect_proxy_cb), browser);
    g_signal_connect (ui_manager, "disconnect-proxy",
        G_CALLBACK (midori_browser_ui_manager_disconnect_proxy_cb), browser);
    accel_group = gtk_ui_manager_get_accel_group (ui_manager);
    gtk_window_add_accel_group (GTK_WINDOW (browser), accel_group);
    gtk_ui_manager_insert_action_group (ui_manager, browser->action_group, 0);

    g_object_set_data (G_OBJECT (accel_group), "midori-browser", browser);
    for (i = 0; i < 10; i++)
    {
        gchar* accel_path = g_strdup_printf ("<Manual>/Browser/SwitchTab%d", i);
        GClosure* closure = g_cclosure_new (
            G_CALLBACK (midori_browser_accel_switch_tab_activate_cb),
            browser, NULL);
        gtk_accel_map_add_entry (accel_path, GDK_0 + i, GDK_MOD1_MASK);
        gtk_accel_group_connect_by_path (accel_group, accel_path, closure);
        g_free (accel_path);
    }

    error = NULL;
    if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_markup, -1, &error))
    {
        /* TODO: Should this be a message dialog? When does this happen? */
        g_message ("User interface couldn't be created: %s", error->message);
        g_error_free (error);
    }

    /* Hide the 'Dummy' which only holds otherwise unused actions */
    g_object_set (_action_by_name (browser, "Dummy"), "visible", FALSE, NULL);

    action = g_object_new (KATZE_TYPE_SEPARATOR_ACTION,
        "name", "Separator",
        "label", _("_Separator"),
        NULL);
    gtk_action_group_add_action (browser->action_group, action);
    g_object_unref (action);

    action = g_object_new (MIDORI_TYPE_LOCATION_ACTION,
        "name", "Location",
        "label", _("_Location..."),
        "stock-id", GTK_STOCK_JUMP_TO,
        "tooltip", _("Open a particular location"),
        NULL);
    g_object_connect (action,
                      "signal::activate",
                      _action_location_activate, browser,
                      "signal::active-changed",
                      _action_location_active_changed, browser,
                      "signal::focus-in",
                      _action_location_focus_in, browser,
                      "signal::focus-out",
                      _action_location_focus_out, browser,
                      "signal::reset-uri",
                      _action_location_reset_uri, browser,
                      "signal::submit-uri",
                      _action_location_submit_uri, browser,
                      "signal-after::secondary-icon-released",
                      _action_location_secondary_icon_released, browser,
                      NULL);
    gtk_action_group_add_action_with_accel (browser->action_group,
        action, "<Ctrl>L");
    g_object_unref (action);

    action = g_object_new (MIDORI_TYPE_SEARCH_ACTION,
        "name", "Search",
        "label", _("_Web Search..."),
        "stock-id", GTK_STOCK_FIND,
        "tooltip", _("Run a web search"),
        NULL);
    g_object_connect (action,
                      "signal::activate",
                      _action_search_activate, browser,
                      "signal::submit",
                      _action_search_submit, browser,
                      "signal::focus-out",
                      _action_search_focus_out, browser,
                      "signal::notify::current-item",
                      _action_search_notify_current_item, browser,
                      "signal::notify::default-item",
                      _action_search_notify_default_item, browser,
                      NULL);
    gtk_action_group_add_action_with_accel (browser->action_group,
        action, "<Ctrl>K");
    g_object_unref (action);

    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
        "name", "Trash",
        "stock-id", STOCK_USER_TRASH,
        "tooltip", _("Reopen a previously closed tab or window"),
        NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_trash_populate_popup, browser,
                      "signal::activate-item",
                      _action_trash_activate_item, browser,
                      "signal::activate-item-alt",
                      _action_trash_activate_item_alt, browser,
                      NULL);
    gtk_action_group_add_action_with_accel (browser->action_group, action, "");
    g_object_unref (action);

    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
        "name", "RecentlyVisited",
        "label", _("_Recently visited pages"),
        "stock-id", STOCK_HISTORY,
        "tooltip", _("Reopen pages that you visited earlier"),
        NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_history_populate_popup, browser,
                      "signal::activate-item",
                      _action_history_activate_item, browser,
                      "signal::activate-item-alt",
                      _action_menus_activate_item_alt, browser,
                      NULL);
    gtk_action_group_add_action_with_accel (browser->action_group, action, "");
    g_object_unref (action);

    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
        "name", "Bookmarks",
        "label", _("_Bookmarks"),
        "stock-id", STOCK_BOOKMARKS,
        "tooltip", _("Show the saved bookmarks"),
        NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_bookmarks_populate_popup, browser,
                      "signal::activate-item",
                      _action_bookmarks_activate_item, browser,
                      "signal::activate-item-alt",
                      _action_menus_activate_item_alt, browser,
                      NULL);
    gtk_action_group_add_action_with_accel (browser->action_group, action, "");
    g_object_unref (action);

    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
        "name", "Tools",
        "label", _("_Tools"),
        "stock-id", GTK_STOCK_PREFERENCES,
        "array", katze_array_new (KATZE_TYPE_ITEM),
        NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_tools_populate_popup, browser,
                      NULL);
    gtk_action_group_add_action (browser->action_group, action);
    g_object_unref (action);

    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
        "name", "Window",
        "label", _("_Window"),
        "stock-id", GTK_STOCK_INDEX,
        "tooltip", _("Show a list of all open tabs"),
        "array", browser->proxy_array,
        NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_window_populate_popup, browser,
                      "signal::activate-item",
                      _action_window_activate_item, browser,
                      NULL);
    gtk_action_group_add_action_with_accel (browser->action_group, action, "");
    g_object_unref (action);

    /*Modified by SBielov start*/
    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
        "name", "Tabs",
        "label", _("_Tabs"),
        "stock-id", GTK_STOCK_INDEX,
        "tooltip", _("Show a list of all open tabs"),
        "array", browser->proxy_array,
        NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_tabs_populate_popup, browser,
                      "signal::activate-item",
                      _action_window_activate_item, browser,
                      NULL);
    gtk_action_group_add_action_with_accel (browser->action_group, action, "");
    g_object_unref (action);
    
    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
        "name", "QuickMenu",
        "label", _("_Menu"),
        "stock-id", GTK_STOCK_INDEX,
        "tooltip", _("Show quick menu"),
        "array", katze_array_new (KATZE_TYPE_ITEM),
        NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_quickmenu_populate_popup, browser,
                      NULL);
    gtk_action_group_add_action_with_accel (browser->action_group, action, "");
    g_object_unref (action);
    /*Modified by SBielov end*/
    
    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
        "name", "CompactMenu",
        "label", _("_Menu"),
        "stock-id", STOCK_WEB_BROWSER,
        "tooltip", _("Menu"),
        "array", katze_array_new (KATZE_TYPE_ITEM),
        NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_compact_menu_populate_popup, browser,
                      NULL);
    gtk_action_group_add_action (browser->action_group, action);
    g_object_unref (action);

    /* Create the menubar */
    browser->menubar = gtk_ui_manager_get_widget (ui_manager, "/menubar");
    gtk_box_pack_start (GTK_BOX (vbox), browser->menubar, FALSE, FALSE, 0);
    gtk_widget_hide (browser->menubar);
    #if HAVE_HILDON
    #if HILDON_CHECK_VERSION (2, 2, 0)
    browser->menubar = hildon_app_menu_new ();
    _action_compact_menu_populate_popup (NULL, browser->menubar, browser);
    hildon_window_set_app_menu (HILDON_WINDOW (browser), HILDON_APP_MENU (browser->menubar));
    #else
    browser->menubar = gtk_menu_new ();
    _action_compact_menu_populate_popup (NULL, browser->menubar, browser);
    hildon_window_set_menu (HILDON_WINDOW (browser), GTK_MENU (browser->menubar));
    #endif
    hildon_program_add_window (hildon_program_get_instance (),
                               HILDON_WINDOW (browser));
    #else
    g_signal_connect (browser->menubar, "button-press-event",
        G_CALLBACK (midori_browser_menu_button_press_event_cb), browser);

    menuitem = gtk_menu_item_new ();
    gtk_widget_show (menuitem);
    browser->throbber = katze_throbber_new ();
    gtk_widget_show (browser->throbber);
    gtk_container_add (GTK_CONTAINER (menuitem), browser->throbber);
    gtk_widget_set_sensitive (menuitem, FALSE);
    gtk_menu_item_set_right_justified (GTK_MENU_ITEM (menuitem), TRUE);
    gtk_menu_shell_append (GTK_MENU_SHELL (browser->menubar), menuitem);
    #endif
    browser->menu_tools = gtk_menu_new ();

    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (
        gtk_ui_manager_get_widget (ui_manager, "/menubar/File/WindowNew")), NULL);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (
        gtk_ui_manager_get_widget (ui_manager, "/menubar/Go/Location")), NULL);

    homepage = gtk_ui_manager_get_widget (ui_manager, "/menubar/Go/Homepage");
    g_signal_connect (homepage, "button-press-event",
        G_CALLBACK (midori_browser_menu_item_middle_click_event_cb), browser);
    back = gtk_ui_manager_get_widget (ui_manager, "/menubar/Go/Back");
    g_signal_connect (back, "button-press-event",
        G_CALLBACK (midori_browser_menu_item_middle_click_event_cb), browser);
    forward = gtk_ui_manager_get_widget (ui_manager, "/menubar/Go/Forward");
    g_signal_connect (forward, "button-press-event",
        G_CALLBACK (midori_browser_menu_item_middle_click_event_cb), browser);
    forward = gtk_ui_manager_get_widget (ui_manager, "/menubar/Go/Previous");
    g_signal_connect (forward, "button-press-event",
        G_CALLBACK (midori_browser_menu_item_middle_click_event_cb), browser);
    forward = gtk_ui_manager_get_widget (ui_manager, "/menubar/Go/Next");
    g_signal_connect (forward, "button-press-event",
        G_CALLBACK (midori_browser_menu_item_middle_click_event_cb), browser);

    #if HAVE_HILDON
    _action_set_visible (browser, "Menubar", FALSE);
    #endif
    #if !WEBKIT_CHECK_VERSION (1, 1, 3)
    _action_set_visible (browser, "Transferbar", FALSE);
    #endif
    _action_set_sensitive (browser, "EncodingCustom", FALSE);
    _action_set_visible (browser, "LastSession", FALSE);
    /* FIXME: Show once implemented */
    _action_set_visible (browser, "AddDesktopShortcut", FALSE);

    /* Create the navigationbar */
    browser->navigationbar = gtk_ui_manager_get_widget (
        ui_manager, "/toolbar_navigation");
    /* FIXME: Settings should be connected with screen changes */
    gtk_settings = gtk_widget_get_settings (GTK_WIDGET (browser));
    if (gtk_settings)
        g_signal_connect (gtk_settings, "notify::gtk-toolbar-style",
            G_CALLBACK (midori_browser_navigationbar_notify_style_cb), browser);
    gtk_toolbar_set_show_arrow (GTK_TOOLBAR (browser->navigationbar), TRUE);
    g_object_set (_action_by_name (browser, "Back"), "is-important", TRUE, NULL);
    gtk_widget_hide (browser->navigationbar);
    g_signal_connect (browser->navigationbar, "popup-context-menu",
        G_CALLBACK (midori_browser_toolbar_popup_context_menu_cb), browser);
    #if HAVE_HILDON
    hildon_window_add_toolbar (HILDON_WINDOW (browser),
                               GTK_TOOLBAR (browser->navigationbar));
    #else
    gtk_box_pack_start (GTK_BOX (vbox), browser->navigationbar, FALSE, FALSE, 0);
    #endif

    #ifdef HAVE_HILDON_2_2
    DBusConnection* system_bus = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
    if (system_bus)
    {
        dbus_bus_add_match (system_bus, MCE_SIGNAL_MATCH, NULL);
        dbus_connection_add_filter (system_bus,
            midori_browser_mce_filter_cb, browser, NULL);
        hildon_gtk_window_set_portrait_flags (GTK_WINDOW (browser),
                                              HILDON_PORTRAIT_MODE_SUPPORT);
    }
    #endif

    /* Bookmarkbar */
    browser->bookmarkbar = gtk_toolbar_new ();
    gtk_widget_set_name (browser->bookmarkbar, "MidoriBookmarkbar");
    gtk_toolbar_set_icon_size (GTK_TOOLBAR (browser->bookmarkbar),
                               GTK_ICON_SIZE_MENU);
    gtk_toolbar_set_style (GTK_TOOLBAR (browser->bookmarkbar),
                           GTK_TOOLBAR_BOTH_HORIZ);
    browser->homepage = gtk_tool_button_new_from_stock (STOCK_HOMEPAGE);
    gtk_tool_item_set_is_important (browser->homepage, TRUE);
    gtk_widget_show (GTK_WIDGET (browser->homepage));
    g_signal_connect (browser->homepage, "clicked",
        G_CALLBACK (midori_browser_bookmark_homepage_clicked_cb), browser);
    g_signal_connect (gtk_bin_get_child (GTK_BIN (browser->homepage)), "button-press-event",
        G_CALLBACK (midori_browser_bookmark_homepage_button_press_cb), browser);
    gtk_toolbar_insert (GTK_TOOLBAR (browser->bookmarkbar), browser->homepage, -1);
    #if HAVE_HILDON
    hildon_window_add_toolbar (HILDON_WINDOW (browser),
                               GTK_TOOLBAR (browser->bookmarkbar));
    #else
    gtk_box_pack_start (GTK_BOX (vbox), browser->bookmarkbar, FALSE, FALSE, 0);
    #endif
    g_signal_connect (browser->bookmarkbar, "popup-context-menu",
        G_CALLBACK (midori_browser_toolbar_popup_context_menu_cb), browser);

    /* Create the panel */
    hpaned = gtk_hpaned_new ();
    g_signal_connect (hpaned, "notify::position",
                      G_CALLBACK (midori_panel_notify_position_cb),
                      browser);
    gtk_box_pack_start (GTK_BOX (vbox), hpaned, TRUE, TRUE, 0);
    gtk_widget_show (hpaned);
    browser->panel = g_object_new (MIDORI_TYPE_PANEL,
                                   "action-group", browser->action_group,
                                   "menu", browser->menu_tools,
                                   NULL);
    g_object_connect (browser->panel,
        "signal::notify::page",
        midori_panel_notify_page_cb, browser,
        "signal::notify::show-titles",
        midori_panel_notify_show_titles_cb, browser,
        "signal::notify::show-controls",
        midori_panel_notify_show_controls_cb, browser,
        "signal::notify::right-aligned",
        midori_panel_notify_right_aligned_cb, browser,
        "signal::close",
        midori_panel_close_cb, browser,
        NULL);
    gtk_paned_pack1 (GTK_PANED (hpaned), browser->panel, FALSE, FALSE);

    /* Notebook, containing all views */
    vpaned = gtk_vpaned_new ();
    gtk_paned_pack2 (GTK_PANED (hpaned), vpaned, TRUE, FALSE);
    gtk_widget_show (vpaned);
    browser->notebook = gtk_notebook_new ();
    /* Remove the inner border between scrollbars and the window border */
    rcstyle = gtk_rc_style_new ();
    rcstyle->xthickness = 0;
    gtk_widget_modify_style (browser->notebook, rcstyle);
    g_object_unref (rcstyle);
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (browser->notebook), TRUE);
    gtk_paned_pack1 (GTK_PANED (vpaned), browser->notebook, FALSE, FALSE);
    g_signal_connect (browser->notebook, "switch-page",
                      G_CALLBACK (gtk_notebook_switch_page_cb),
                      browser);
    g_signal_connect_after (browser->notebook, "switch-page",
                            G_CALLBACK (gtk_notebook_switch_page_after_cb),
                            browser);
    g_signal_connect (browser->notebook, "page-reordered",
                      G_CALLBACK (midori_browser_notebook_page_reordered_cb),
                      browser);
    g_signal_connect_after (browser->notebook, "button-press-event",
        G_CALLBACK (midori_browser_notebook_button_press_event_after_cb),
                      browser);
    gtk_widget_show (browser->notebook);

    /* Inspector container */
    browser->inspector = gtk_vbox_new (FALSE, 0);
    gtk_paned_pack2 (GTK_PANED (vpaned), browser->inspector, FALSE, FALSE);
    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GTK_WIDGET_SET_FLAGS (scrolled, GTK_CAN_FOCUS);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                         GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start (GTK_BOX (browser->inspector), scrolled, TRUE, TRUE, 0);
    browser->inspector_view = gtk_viewport_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), browser->inspector_view);

    /* Incremental findbar */
    browser->find = gtk_toolbar_new ();
    gtk_widget_set_name (browser->find, "MidoriFindbar");
    gtk_toolbar_set_icon_size (GTK_TOOLBAR (browser->find), GTK_ICON_SIZE_MENU);
    gtk_toolbar_set_style (GTK_TOOLBAR (browser->find), GTK_TOOLBAR_BOTH_HORIZ);
    g_signal_connect (browser->find, "key-press-event",
        G_CALLBACK (midori_browser_find_key_press_event_cb), browser);
    toolitem = gtk_tool_item_new ();
    gtk_container_set_border_width (GTK_CONTAINER (toolitem), 6);
    gtk_container_add (GTK_CONTAINER (toolitem),
        /* i18n: A panel at the bottom, to search text in pages */
        gtk_label_new_with_mnemonic (_("_Inline Find:")));
    gtk_toolbar_insert (GTK_TOOLBAR (browser->find), toolitem, -1);
    browser->find_text = gtk_icon_entry_new ();
    #if !HAVE_HILDON
    gtk_icon_entry_set_icon_from_stock (GTK_ICON_ENTRY (browser->find_text),
                                        GTK_ICON_ENTRY_PRIMARY,
                                        GTK_STOCK_FIND);
    gtk_icon_entry_set_icon_from_stock (GTK_ICON_ENTRY (browser->find_text),
                                        GTK_ICON_ENTRY_SECONDARY,
                                        GTK_STOCK_CLEAR);
    gtk_icon_entry_set_icon_highlight (GTK_ICON_ENTRY (browser->find_text),
                                       GTK_ICON_ENTRY_SECONDARY, TRUE);
    #endif
    g_signal_connect (browser->find_text, "icon-release",
        G_CALLBACK (midori_browser_entry_clear_icon_released_cb), NULL);
    g_signal_connect (browser->find_text, "activate",
        G_CALLBACK (_action_find_next_activate), browser);
    g_signal_connect (browser->find_text, "changed",
        G_CALLBACK (midori_browser_find_text_changed_cb), browser);
    g_signal_connect (browser->find_text, "focus-out-event",
        G_CALLBACK (midori_browser_find_text_focus_out_event_cb), browser);
    toolitem = gtk_tool_item_new ();
    gtk_container_add (GTK_CONTAINER (toolitem), browser->find_text);
    gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (browser->find), toolitem, -1);
    #if HAVE_HILDON
    browser->find_case = gtk_toggle_tool_button_new ();
    browser->find_highlight = gtk_toggle_tool_button_new ();
    #else
    toolitem = (GtkToolItem*)gtk_action_create_tool_item
        (_action_by_name (browser, "FindPrevious"));
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (toolitem), _("Previous"));
    gtk_tool_item_set_is_important (toolitem, TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (browser->find), toolitem, -1);
    toolitem = (GtkToolItem*)gtk_action_create_tool_item
        (_action_by_name (browser, "FindNext"));
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (toolitem), _("Next"));
    gtk_tool_item_set_is_important (toolitem, TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (browser->find), toolitem, -1);
    browser->find_case = gtk_toggle_tool_button_new_from_stock (
        GTK_STOCK_SPELL_CHECK);
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (browser->find_case), _("Match Case"));
    gtk_tool_item_set_is_important (GTK_TOOL_ITEM (browser->find_case), TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (browser->find), browser->find_case, -1);
    browser->find_highlight = gtk_toggle_tool_button_new_from_stock (
        GTK_STOCK_SELECT_ALL);
    #endif
    g_signal_connect (browser->find_highlight, "toggled",
                      G_CALLBACK (_find_highlight_toggled), browser);
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (browser->find_highlight),
                               _("Highlight Matches"));
    gtk_tool_item_set_is_important (GTK_TOOL_ITEM (browser->find_highlight), TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (browser->find), browser->find_highlight, -1);
    toolitem = gtk_separator_tool_item_new ();
    gtk_separator_tool_item_set_draw (
        GTK_SEPARATOR_TOOL_ITEM (toolitem), FALSE);
    #if !HAVE_HILDON
    gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolitem), TRUE);
    #endif
    gtk_toolbar_insert (GTK_TOOLBAR (browser->find), toolitem, -1);
    browser->find_close = gtk_tool_button_new_from_stock (GTK_STOCK_CLOSE);
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (browser->find_close),
                               _("Close Findbar"));
    g_signal_connect (browser->find_close, "clicked",
        G_CALLBACK (midori_browser_find_button_close_clicked_cb), browser);
    #if HAVE_OSX
    gtk_toolbar_insert (GTK_TOOLBAR (browser->find), browser->find_close, 0);
    #else
    gtk_toolbar_insert (GTK_TOOLBAR (browser->find), browser->find_close, -1);
    #endif
    sokoke_container_show_children (GTK_CONTAINER (browser->find));
    #if HAVE_HILDON
    hildon_window_add_toolbar (HILDON_WINDOW (browser),
                               GTK_TOOLBAR (browser->find));
    #else
    gtk_box_pack_start (GTK_BOX (vbox), browser->find, FALSE, FALSE, 0);
    #endif

    /* Statusbar */
    browser->statusbar = gtk_statusbar_new ();
    /* Rearrange the statusbar packing. This is necessariy to keep
        themes happy while there is no support from GtkStatusbar. */
    label = GTK_STATUSBAR (browser->statusbar)->label;
    if (GTK_IS_BOX (gtk_widget_get_parent (label)))
        browser->statusbar_contents = gtk_widget_get_parent (label);
    else
    {
        browser->statusbar_contents = gtk_hbox_new (FALSE, 4);
        gtk_widget_show (browser->statusbar_contents);
        g_object_ref (GTK_STATUSBAR (browser->statusbar)->label);
        gtk_container_remove (
            GTK_CONTAINER (GTK_STATUSBAR (browser->statusbar)->frame), label);
        gtk_box_pack_start (GTK_BOX (browser->statusbar_contents),
            label, TRUE, TRUE, 0);
        g_object_unref (label);
        gtk_container_add (GTK_CONTAINER (GTK_STATUSBAR (browser->statusbar)->frame),
                           browser->statusbar_contents);
    }
    gtk_box_pack_start (GTK_BOX (vbox), browser->statusbar, FALSE, FALSE, 0);

    browser->progressbar = gtk_progress_bar_new ();
    gtk_box_pack_start (GTK_BOX (browser->statusbar_contents),
                        browser->progressbar, FALSE, FALSE, 3);

    browser->transferbar = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (browser->statusbar_contents),
                        browser->transferbar, FALSE, FALSE, 3);
    gtk_widget_show (browser->transferbar);
    browser->transferbar_clear = gtk_button_new_with_label (_("Clear All"));
    icon = gtk_image_new_from_stock (GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU);
    gtk_button_set_image (GTK_BUTTON (browser->transferbar_clear), icon);
    g_signal_connect (browser->transferbar_clear, "clicked",
        G_CALLBACK (midori_browser_transferbar_clear_clicked_cb), browser);
    gtk_box_pack_end (GTK_BOX (browser->transferbar), browser->transferbar_clear,
                        FALSE, FALSE, 0);
    g_signal_connect (browser->statusbar, "button-press-event",
        G_CALLBACK (midori_browser_menu_button_press_event_cb), browser);

    g_object_unref (ui_manager);
}

static void
midori_browser_dispose (GObject* object)
{
    MidoriBrowser* browser = MIDORI_BROWSER (object);

    /* We are done, the session mustn't change anymore */
    katze_object_assign (browser->proxy_array, NULL);

    G_OBJECT_CLASS (midori_browser_parent_class)->dispose (object);
}

static void
midori_browser_finalize (GObject* object)
{
    MidoriBrowser* browser = MIDORI_BROWSER (object);

    g_signal_handlers_disconnect_by_func (browser->settings,
                                          midori_browser_settings_notify,
                                          browser);

    katze_assign (browser->statusbar_text, NULL);

    katze_object_assign (browser->settings, NULL);
    katze_object_assign (browser->bookmarks, NULL);
    katze_object_assign (browser->trash, NULL);
    katze_object_assign (browser->search_engines, NULL);
    katze_object_assign (browser->history, NULL);

    katze_object_assign (browser->net, NULL);

    katze_assign (browser->news_aggregator, NULL);

    G_OBJECT_CLASS (midori_browser_parent_class)->finalize (object);
}

static void
_midori_browser_set_toolbar_style (MidoriBrowser*     browser,
                                   MidoriToolbarStyle toolbar_style)
{
    GtkToolbarStyle gtk_toolbar_style;
    GtkIconSize icon_size;
    GtkSettings* gtk_settings = gtk_widget_get_settings (GTK_WIDGET (browser));
    g_object_get (gtk_settings, "gtk-toolbar-icon-size", &icon_size, NULL);
    if (toolbar_style == MIDORI_TOOLBAR_DEFAULT && gtk_settings)
    #ifdef G_OS_WIN32
        gtk_toolbar_style = GTK_TOOLBAR_ICONS;
    #else
        g_object_get (gtk_settings, "gtk-toolbar-style", &gtk_toolbar_style, NULL);
    #endif
    else
    {
        switch (toolbar_style)
        {
        case MIDORI_TOOLBAR_SMALL_ICONS:
            icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
        case MIDORI_TOOLBAR_ICONS:
            gtk_toolbar_style = GTK_TOOLBAR_ICONS;
            break;
        case MIDORI_TOOLBAR_TEXT:
            gtk_toolbar_style = GTK_TOOLBAR_TEXT;
            break;
        case MIDORI_TOOLBAR_BOTH:
            gtk_toolbar_style = GTK_TOOLBAR_BOTH;
            break;
        case MIDORI_TOOLBAR_BOTH_HORIZ:
        case MIDORI_TOOLBAR_DEFAULT:
            gtk_toolbar_style = GTK_TOOLBAR_BOTH_HORIZ;
        }
    }
    gtk_toolbar_set_style (GTK_TOOLBAR (browser->navigationbar),
                           gtk_toolbar_style);
    gtk_toolbar_set_icon_size (GTK_TOOLBAR (browser->navigationbar), icon_size);
}

static gboolean
midori_browser_toolbar_item_button_press_event_cb (GtkWidget*      toolitem,
                                                   GdkEventButton* event,
                                                   MidoriBrowser*  browser)
{
    if (event->button == 2)
    {
        GtkWidget* parent = gtk_widget_get_parent (toolitem);
        GtkAction* action = gtk_widget_get_action (parent);

        return midori_browser_menu_middle_click_on_navigation_action (browser, action);
    }
    else if (event->button == 3)
    {
        midori_browser_toolbar_popup_context_menu_cb (
            GTK_IS_BIN (toolitem) && gtk_bin_get_child (GTK_BIN (toolitem)) ?
                gtk_widget_get_parent (toolitem) : toolitem,
            event->x, event->y, event->button, browser);

        return TRUE;
    }
    return FALSE;
}

static void
_midori_browser_set_toolbar_items (MidoriBrowser* browser,
                                   const gchar*   items)
{
    gchar** names;
    gchar** name;
    GtkAction* action;
    GtkWidget* toolitem;

    gtk_container_foreach (GTK_CONTAINER (browser->navigationbar),
        (GtkCallback)gtk_widget_destroy, NULL);

    names = g_strsplit (items ? items : "", ",", 0);
    name = names;
    while (*name)
    {
        action = _action_by_name (browser, *name);
        if (action)
        {
            toolitem = gtk_action_create_tool_item (action);
            if (gtk_bin_get_child (GTK_BIN (toolitem)))
                g_signal_connect (gtk_bin_get_child (GTK_BIN (toolitem)),
                    "button-press-event",
                    G_CALLBACK (midori_browser_toolbar_item_button_press_event_cb),
                    browser);
            else
            {
                gtk_tool_item_set_use_drag_window (GTK_TOOL_ITEM (toolitem), TRUE);
                g_signal_connect (toolitem,
                    "button-press-event",
                    G_CALLBACK (midori_browser_toolbar_item_button_press_event_cb),
                    browser);
            }
            gtk_toolbar_insert (GTK_TOOLBAR (browser->navigationbar),
                                GTK_TOOL_ITEM (toolitem), -1);
        }
        name++;
    }
    g_strfreev (names);
}

static void
_midori_browser_update_settings (MidoriBrowser* browser)
{
    gboolean remember_last_window_size;
    MidoriWindowState last_window_state;
    gboolean compact_sidepanel, show_panel_controls;
    gboolean right_align_sidepanel, open_panels_in_windows;
    gint last_panel_position, last_panel_page;
    gboolean show_menubar, show_bookmarkbar;
    gboolean show_panel, show_transferbar;
    gchar* homepage;
    MidoriToolbarStyle toolbar_style;
    gchar* toolbar_items;
    gint last_web_search;
    gboolean close_buttons_on_tabs;
    GdkScreen* screen;
    gint default_width, default_height;
    KatzeItem* item;

    g_free (browser->news_aggregator);

    g_object_get (browser->settings,
                  "remember-last-window-size", &remember_last_window_size,
                  "last-window-width", &browser->last_window_width,
                  "last-window-height", &browser->last_window_height,
                  "last-window-state", &last_window_state,
                  "compact-sidepanel", &compact_sidepanel,
                  "show-panel-controls", &show_panel_controls,
                  "right-align-sidepanel", &right_align_sidepanel,
                  "open-panels-in-windows", &open_panels_in_windows,
                  "last-panel-position", &last_panel_position,
                  "last-panel-page", &last_panel_page,
                  "show-menubar", &show_menubar,
                  "show-navigationbar", &browser->show_navigationbar,
                  "show-bookmarkbar", &show_bookmarkbar,
                  "show-panel", &show_panel,
                  "show-transferbar", &show_transferbar,
                  "show-statusbar", &browser->show_statusbar,
                  "homepage", &homepage,
                  "speed-dial-in-new-tabs", &browser->speed_dial_in_new_tabs,
                  "toolbar-style", &toolbar_style,
                  "toolbar-items", &toolbar_items,
                  "last-web-search", &last_web_search,
                  "location-entry-search", &browser->location_entry_search,
                  "close-buttons-on-tabs", &close_buttons_on_tabs,
                  "progress-in-location", &browser->progress_in_location,
                  "maximum-history-age", &browser->maximum_history_age,
                  "news-aggregator", &browser->news_aggregator,
                  NULL);

    screen = gtk_window_get_screen (GTK_WINDOW (browser));
    default_width = gdk_screen_get_width (screen);// / 1.7;
    default_height = gdk_screen_get_height (screen);// / 1.7;

    //if (remember_last_window_size)
    //{
        //if (browser->last_window_width && browser->last_window_height)
        //    gtk_window_set_default_size (GTK_WINDOW (browser),
        //        browser->last_window_width, browser->last_window_height);
        //else
            gtk_window_set_default_size (GTK_WINDOW (browser),
                                         default_width, default_height);
        //switch (last_window_state)
        //{
        //    case MIDORI_WINDOW_MINIMIZED:
        //        gtk_window_iconify (GTK_WINDOW (browser));
        //        break;
        //    case MIDORI_WINDOW_MAXIMIZED:
        //        gtk_window_maximize (GTK_WINDOW (browser));
        //        break;
        //    case MIDORI_WINDOW_FULLSCREEN:
        //        gtk_window_fullscreen (GTK_WINDOW (browser));
        //        break;
        //    default:
                ;/* Do nothing. */
        //}
    //}

    _midori_browser_set_toolbar_style (browser, toolbar_style);
    _toggle_tabbar_smartly (browser);
    _midori_browser_set_toolbar_items (browser, toolbar_items);

    if (browser->search_engines)
    {
        guint i;

        item = katze_array_get_nth_item (browser->search_engines,
                                         last_web_search);
        if (item)
            midori_search_action_set_current_item (MIDORI_SEARCH_ACTION (
                _action_by_name (browser, "Search")), item);

        i = 0;
        while ((item = katze_array_get_nth_item (browser->search_engines, i++)))
            if (!g_strcmp0 (katze_item_get_uri (item), browser->location_entry_search))
            {
                midori_search_action_set_default_item (MIDORI_SEARCH_ACTION (
                _action_by_name (browser, "Search")), item);
                break;
            }
    }

    g_object_set (browser->panel, "show-titles", !compact_sidepanel,
        "show-controls", show_panel_controls,
        "right-aligned", right_align_sidepanel,
        "open-panels-in-windows", open_panels_in_windows, NULL);
    gtk_paned_set_position (GTK_PANED (gtk_widget_get_parent (browser->panel)),
                            last_panel_position);
    /* The browser may not yet be visible, which means that we can't set the
       page. So we set it in midori_browser_size_allocate_cb */
    if (GTK_WIDGET_VISIBLE (browser))
        midori_panel_set_current_page (MIDORI_PANEL (browser->panel), last_panel_page);
    else
        g_object_set_data (G_OBJECT (browser), "last-page",
                           GINT_TO_POINTER (last_panel_page));

    _action_set_active (browser, "Menubar", show_menubar);
    _action_set_active (browser, "Navigationbar", browser->show_navigationbar);
    _action_set_active (browser, "Bookmarkbar", show_bookmarkbar);
    _action_set_active (browser, "Panel", show_panel);
    #if WEBKIT_CHECK_VERSION (1, 1, 3)
    _action_set_active (browser, "Transferbar", show_transferbar);
    #endif
    _action_set_active (browser, "Statusbar", browser->show_statusbar);

    sokoke_widget_set_visible (GTK_WIDGET (browser->homepage),
                               homepage && *homepage);

    g_free (homepage);
    g_free (toolbar_items);
}

static void
midori_browser_settings_notify (MidoriWebSettings* web_settings,
                                GParamSpec*        pspec,
                                MidoriBrowser*     browser)
{
    const gchar* name;
    GValue value = {0, };

    name = g_intern_string (pspec->name);
    g_value_init (&value, pspec->value_type);
    g_object_get_property (G_OBJECT (web_settings), name, &value);

    if (name == g_intern_string ("toolbar-style"))
        _midori_browser_set_toolbar_style (browser, g_value_get_enum (&value));
    else if (name == g_intern_string ("toolbar-items"))
        _midori_browser_set_toolbar_items (browser, g_value_get_string (&value));
    else if (name == g_intern_string ("compact-sidepanel"))
    {
        g_signal_handlers_block_by_func (browser->panel,
            midori_panel_notify_show_titles_cb, browser);
        g_object_set (browser->panel, "show-titles",
                      !g_value_get_boolean (&value), NULL);
        g_signal_handlers_unblock_by_func (browser->panel,
            midori_panel_notify_show_titles_cb, browser);
    }
    else if (name == g_intern_string ("show-panel-controls"))
    {
        g_signal_handlers_block_by_func (browser->panel,
            midori_panel_notify_show_controls_cb, browser);
        g_object_set (browser->panel, "show-controls",
                      g_value_get_boolean (&value), NULL);
        g_signal_handlers_unblock_by_func (browser->panel,
            midori_panel_notify_show_controls_cb, browser);
    }
    else if (name == g_intern_string ("open-panels-in-windows"))
        g_object_set (browser->panel, "open-panels-in-windows",
                      g_value_get_boolean (&value), NULL);
    else if (name == g_intern_string ("always-show-tabbar"))
        _toggle_tabbar_smartly (browser);
    else if (name == g_intern_string ("show-menubar"))
        sokoke_widget_set_visible (browser->menubar, g_value_get_boolean (&value));
    else if (name == g_intern_string ("show-navigationbar"))
        browser->show_navigationbar = g_value_get_boolean (&value);
    else if (name == g_intern_string ("show-statusbar"))
        browser->show_statusbar = g_value_get_boolean (&value);
    else if (name == g_intern_string ("speed-dial-in-new-tabs"))
        browser->speed_dial_in_new_tabs = g_value_get_boolean (&value);
    else if (name == g_intern_string ("progress-in-location"))
        browser->progress_in_location = g_value_get_boolean (&value);
    else if (name == g_intern_string ("homepage"))
        sokoke_widget_set_visible (GTK_WIDGET (browser->homepage),
            g_value_get_string (&value) && *g_value_get_string (&value));
    else if (name == g_intern_string ("search-engines-in-completion"))
    {
        if (g_value_get_boolean (&value))
            midori_location_action_set_search_engines (MIDORI_LOCATION_ACTION (
                _action_by_name (browser, "Location")), browser->search_engines);
        else
            midori_location_action_set_search_engines (MIDORI_LOCATION_ACTION (
                _action_by_name (browser, "Location")), NULL);
    }
    else if (name == g_intern_string ("location-entry-search"))
    {
        katze_assign (browser->location_entry_search, g_value_dup_string (&value));
    }
    else if (name == g_intern_string ("maximum-history-age"))
        browser->maximum_history_age = g_value_get_boolean (&value);
    else if (name == g_intern_string ("news-aggregator"))
    {
        katze_assign (browser->news_aggregator, g_value_dup_string (&value));
    }
    else if (!g_object_class_find_property (G_OBJECT_GET_CLASS (web_settings),
                                             name))
         g_warning (_("Unexpected setting '%s'"), name);
    g_value_unset (&value);
}

static GtkWidget*
midori_browser_toolitem_for_item (KatzeItem* item,
                                  GtkWidget* toolbar)

{
    GList* children = gtk_container_get_children (GTK_CONTAINER (toolbar));
    while (children != NULL)
    {
        void* toolitem = children->data;
        void* found_item = g_object_get_data (G_OBJECT (toolitem), "KatzeItem");

        if (found_item == item)
            return toolitem;
        children = g_list_next (children);
    }
    return NULL;
}

static void
midori_browser_bookmarks_remove_item_cb (KatzeArray* array,
                                         KatzeItem*  removed_item,
                                         GtkWidget*  toolbar)
{
    GtkWidget* toolitem = midori_browser_toolitem_for_item (removed_item, toolbar);
    if (toolitem)
        gtk_widget_destroy (toolitem);
}

static void
midori_browser_bookmarks_meta_data_changed_toolbar_cb (KatzeItem*   item,
                                                       const gchar* key,
                                                       GtkWidget*   toolbar)
{
    MidoriBrowser* browser = midori_browser_get_for_widget (toolbar);

    if (katze_item_get_meta_string (item, "toolbar"))
    {
        GtkToolItem* toolitem;

        if ((toolitem = (GtkToolItem*)midori_browser_toolitem_for_item (item, toolbar)))
            return;

        toolitem = katze_array_action_create_tool_item_for (
            KATZE_ARRAY_ACTION (_action_by_name (browser, "Bookmarks")), item);
        g_object_set_data (G_OBJECT (toolitem), "KatzeItem", item);

        if (KATZE_IS_ARRAY (item) || katze_item_get_uri (item))
        {
            GtkWidget* child = gtk_bin_get_child (GTK_BIN (toolitem));
            g_signal_connect (child, "button-press-event",
                G_CALLBACK (midori_browser_bookmarkbar_item_button_press_event_cb),
                browser);
            g_object_set_data (G_OBJECT (child), "KatzeItem", item);
        }
        else /* Separator */
        {
            gtk_tool_item_set_use_drag_window (toolitem, TRUE);
            g_signal_connect (toolitem, "button-press-event",
                G_CALLBACK (midori_browser_bookmarkbar_item_button_press_event_cb),
                browser);
        }
        gtk_widget_show (GTK_WIDGET (toolitem));
        gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    }
    else
    {
        KatzeArray* array = katze_item_get_parent (item);
        midori_browser_bookmarks_remove_item_cb (array, item, toolbar);
    }
}

static void
midori_browser_bookmarks_connect_item (KatzeArray* array,
                                       KatzeItem*  item,
                                       GtkWidget*  toolbar)
{
    if (KATZE_IS_ARRAY (item))
    {
        KatzeItem* child;
        guint i = 0;
        while ((child = katze_array_get_nth_item ((KatzeArray*)item, i++)))
            midori_browser_bookmarks_connect_item ((KatzeArray*)item, child, toolbar);

        g_signal_connect_after (item, "add-item",
            G_CALLBACK (midori_browser_bookmarks_add_item_cb), toolbar);
        g_signal_connect_after (item, "remove-item",
            G_CALLBACK (midori_browser_bookmarks_remove_item_cb), toolbar);
    }

    if (katze_item_get_meta_string (item, "toolbar"))
        midori_browser_bookmarks_meta_data_changed_toolbar_cb (item, "toolbar", toolbar);
    g_signal_connect (item, "meta-data-changed::toolbar",
        G_CALLBACK (midori_browser_bookmarks_meta_data_changed_toolbar_cb), toolbar);
}

static void
midori_browser_bookmarks_add_item_cb (KatzeArray* array,
                                      KatzeItem*  item,
                                      GtkWidget*  toolbar)
{
    midori_browser_bookmarks_connect_item (array, item, toolbar);
}

static void
midori_browser_set_bookmarks (MidoriBrowser* browser,
                              KatzeArray*    bookmarks)
{
    if (browser->bookmarks == bookmarks)
        return;

    if (browser->bookmarks)
    {
        g_signal_handlers_disconnect_by_func (
            browser->bookmarks, midori_browser_bookmarks_add_item_cb, browser->bookmarkbar);
        g_signal_handlers_disconnect_by_func (
            browser->bookmarks, midori_browser_bookmarks_remove_item_cb, browser->bookmarkbar);
        /* FIXME: Disconnect recursively */
    }

    if (bookmarks)
        g_object_ref (bookmarks);
    katze_object_assign (browser->bookmarks, bookmarks);

    g_object_set (_action_by_name (browser, "Bookmarks"), "array",
                  browser->bookmarks, NULL);

    _action_set_sensitive (browser, "BookmarkAdd", FALSE);
    _action_set_sensitive (browser, "BookmarkFolderAdd", FALSE);

    if (!browser->bookmarks)
        return;

    midori_browser_bookmarks_connect_item (NULL, (KatzeItem*)browser->bookmarks,
                                           browser->bookmarkbar);
    g_signal_connect (browser->bookmarks, "add-item",
        G_CALLBACK (midori_browser_bookmarks_add_item_cb), browser->bookmarkbar);
    g_signal_connect (browser->bookmarks, "remove-item",
        G_CALLBACK (midori_browser_bookmarks_remove_item_cb), browser->bookmarkbar);

    _action_set_sensitive (browser, "BookmarkAdd", TRUE);
    _action_set_sensitive (browser, "BookmarkFolderAdd", TRUE);
}

static void
midori_browser_set_property (GObject*      object,
                             guint         prop_id,
                             const GValue* value,
                             GParamSpec*   pspec)
{
    MidoriBrowser* browser = MIDORI_BROWSER (object);
    guint last_web_search;
    KatzeItem* item;

    switch (prop_id)
    {
    case PROP_URI:
        midori_browser_set_current_uri (browser, g_value_get_string (value));
        break;
    case PROP_TAB:
        midori_browser_set_current_tab (browser, g_value_get_object (value));
        break;
    case PROP_STATUSBAR_TEXT:
        _midori_browser_set_statusbar_text (browser, g_value_get_string (value));
        break;
    case PROP_SETTINGS:
        g_signal_handlers_disconnect_by_func (browser->settings,
                                              midori_browser_settings_notify,
                                              browser);
        katze_object_assign (browser->settings, g_value_dup_object (value));
        if (!browser->settings)
            browser->settings = midori_web_settings_new ();

        _midori_browser_update_settings (browser);
        g_signal_connect (browser->settings, "notify",
            G_CALLBACK (midori_browser_settings_notify), browser);
        gtk_container_foreach (GTK_CONTAINER (browser->notebook),
            (GtkCallback) midori_view_set_settings, browser->settings);
        break;
    case PROP_BOOKMARKS:
        midori_browser_set_bookmarks (browser, g_value_get_object (value));
        break;
    case PROP_TRASH:
        /* FIXME: Disconnect handlers */
        katze_object_assign (browser->trash, g_value_dup_object (value));
        g_object_set (_action_by_name (browser, "Trash"),
                      "array", browser->trash, "reversed", TRUE,
                      NULL);
        /* FIXME: Connect to updates */
        _midori_browser_update_actions (browser);
        break;
    case PROP_SEARCH_ENGINES:
    {
        guint i;

        /* FIXME: Disconnect handlers */
        katze_object_assign (browser->search_engines, g_value_dup_object (value));
        if (katze_object_get_boolean (browser->settings,
                                      "search-engines-in-completion"))
            midori_location_action_set_search_engines (MIDORI_LOCATION_ACTION (
                _action_by_name (browser, "Location")), browser->search_engines);
        else
            midori_location_action_set_search_engines (MIDORI_LOCATION_ACTION (
                _action_by_name (browser, "Location")), NULL);
        midori_search_action_set_search_engines (MIDORI_SEARCH_ACTION (
            _action_by_name (browser, "Search")), browser->search_engines);
        /* FIXME: Connect to updates */

        if (browser->search_engines)
        {
            g_object_get (browser->settings, "last-web-search", &last_web_search, NULL);
            item = katze_array_get_nth_item (browser->search_engines, last_web_search);
            midori_search_action_set_current_item (MIDORI_SEARCH_ACTION (
                _action_by_name (browser, "Search")), item);

            i = 0;
            while ((item = katze_array_get_nth_item (browser->search_engines, i++)))
                if (!g_strcmp0 (katze_item_get_uri (item), browser->location_entry_search))
                {
                    midori_search_action_set_default_item (MIDORI_SEARCH_ACTION (
                    _action_by_name (browser, "Search")), item);
                    break;
                }
        }
        break;
    }
    case PROP_HISTORY:
        midori_browser_set_history (browser, g_value_get_object (value));
        break;
    case PROP_SHOW_TABS:
        browser->show_tabs = g_value_get_boolean (value);
        if (browser->show_tabs)
            _toggle_tabbar_smartly (browser);
        else
        {
            gtk_notebook_set_show_tabs (GTK_NOTEBOOK (browser->notebook), FALSE);
            gtk_notebook_set_show_border (GTK_NOTEBOOK (browser->notebook), FALSE);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
midori_browser_get_property (GObject*    object,
                             guint       prop_id,
                             GValue*     value,
                             GParamSpec* pspec)
{
    MidoriBrowser* browser = MIDORI_BROWSER (object);

    switch (prop_id)
    {
    case PROP_MENUBAR:
        g_value_set_object (value, browser->menubar);
        break;
    case PROP_NAVIGATIONBAR:
        g_value_set_object (value, browser->navigationbar);
        break;
    case PROP_NOTEBOOK:
        g_value_set_object (value, browser->notebook);
        break;
    case PROP_PANEL:
        g_value_set_object (value, browser->panel);
        break;
    case PROP_URI:
        g_value_set_string (value, midori_browser_get_current_uri (browser));
        break;
    case PROP_TAB:
        g_value_set_object (value, midori_browser_get_current_tab (browser));
        break;
    case PROP_LOAD_STATUS:
    {
        GtkWidget* view = midori_browser_get_current_tab (browser);
        if (view)
            g_value_set_enum (value,
                midori_view_get_load_status (MIDORI_VIEW (view)));
        else
            g_value_set_enum (value, MIDORI_LOAD_FINISHED);
        break;
    }
    case PROP_STATUSBAR:
        g_value_set_object (value, browser->statusbar_contents);
        break;
    case PROP_STATUSBAR_TEXT:
        g_value_set_string (value, browser->statusbar_text);
        break;
    case PROP_SETTINGS:
        g_value_set_object (value, browser->settings);
        break;
    case PROP_BOOKMARKS:
        g_value_set_object (value, browser->bookmarks);
        break;
    case PROP_TRASH:
        g_value_set_object (value, browser->trash);
        break;
    case PROP_SEARCH_ENGINES:
        g_value_set_object (value, browser->search_engines);
        break;
    case PROP_HISTORY:
        g_value_set_object (value, browser->history);
        break;
    case PROP_SHOW_TABS:
        g_value_set_boolean (value, browser->show_tabs);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/**
 * midori_browser_new:
 *
 * Creates a new browser widget.
 *
 * A browser is a window with a menubar, toolbars, a notebook, panels
 * and a statusbar. You should mostly treat it as an opaque widget.
 *
 * Return value: a new #MidoriBrowser
 **/
MidoriBrowser*
midori_browser_new (void)
{
    MidoriBrowser* browser = g_object_new (MIDORI_TYPE_BROWSER,
                                           NULL);
    return browser;
}

/**
 * midori_browser_add_tab:
 * @browser: a #MidoriBrowser
 * @widget: a view
 *
 * Appends a view in the form of a new tab and creates an
 * according item in the Window menu.
 *
 * Return value: the index of the new tab, or -1 in case of an error
 **/
gint
midori_browser_add_tab (MidoriBrowser* browser,
                        GtkWidget*     view)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), -1);
    g_return_val_if_fail (GTK_IS_WIDGET (view), -1);

    g_signal_emit (browser, signals[ADD_TAB], 0, view);
    return gtk_notebook_page_num (GTK_NOTEBOOK (browser->notebook), view);
}

/**
 * midori_browser_remove_tab:
 * @browser: a #MidoriBrowser
 * @widget: a view
 *
 * Removes an existing view from the browser,
 * including an associated menu item.
 **/
void
midori_browser_remove_tab (MidoriBrowser* browser,
                           GtkWidget*     view)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (GTK_IS_WIDGET (view));

    g_signal_emit (browser, signals[REMOVE_TAB], 0, view);
}

/**
 * midori_browser_add_item:
 * @browser: a #MidoriBrowser
 * @item: an item
 *
 * Appends a new view as described by @item.
 *
 * Return value: the index of the new tab, or -1 in case of an error
 **/
gint
midori_browser_add_item (MidoriBrowser* browser,
                         KatzeItem*     item)
{
    const gchar* uri;
    const gchar* title;
    GtkWidget* view;
    gint page;
    KatzeItem* proxy_item;
    GList* keys;

    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), -1);
    g_return_val_if_fail (KATZE_IS_ITEM (item), -1);

    uri = katze_item_get_uri (item);
    title = katze_item_get_name (item);
    view = g_object_new (MIDORI_TYPE_VIEW,
                         "title", title,
                         "settings", browser->settings,
                         "net", browser->net,
                         NULL);
    midori_view_set_uri (MIDORI_VIEW (view), uri);
    gtk_widget_show (view);

    /* FIXME: We should have public API for that */
    if (g_object_get_data (G_OBJECT (item), "midori-view-append"))
        g_object_set_data (G_OBJECT (view), "midori-view-append", (void*)1);

    page = midori_browser_add_tab (browser, view);
    proxy_item = midori_view_get_proxy_item (MIDORI_VIEW (view));
    if ((keys = katze_item_get_meta_keys (item)))
    {
        guint i = 0;
        const gchar* key;
        while ((key = g_list_nth_data (keys, i++)))
            katze_item_set_meta_string (proxy_item, key,
                katze_item_get_meta_string (item, key));
        g_list_free (keys);
    }
    return page;
}

/**
 * midori_browser_add_uri:
 * @browser: a #MidoriBrowser
 * @uri: an URI
 *
 * Appends an uri in the form of a new view.
 *
 * Return value: the index of the new view, or -1
 **/
gint
midori_browser_add_uri (MidoriBrowser* browser,
                        const gchar*   uri)
{
    GtkWidget* view;

    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), -1);
    g_return_val_if_fail (uri != NULL, -1);

    view = g_object_new (MIDORI_TYPE_VIEW, "settings", browser->settings,
                                           "net", browser->net, NULL);
    midori_view_set_uri (MIDORI_VIEW (view), uri);
    gtk_widget_show (view);

    return midori_browser_add_tab (browser, view);
}

/**
 * midori_browser_activate_action:
 * @browser: a #MidoriBrowser
 * @name: name of the action
 *
 * Activates the specified action.
 **/
void
midori_browser_activate_action (MidoriBrowser* browser,
                                const gchar*   name)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (name != NULL);

    g_signal_emit (browser, signals[ACTIVATE_ACTION], 0, name);
}

/**
 * midori_browser_get_action_group:
 * @browser: a #MidoriBrowser
 *
 * Retrieves the action group holding all actions used
 * by the browser. It allows obtaining individual
 * actions and adding new actions.
 *
 * Return value: the action group of the browser
 *
 * Since: 0.1.4
 **/
GtkActionGroup*
midori_browser_get_action_group (MidoriBrowser* browser)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);

    return browser->action_group;
}

/**
 * midori_browser_set_current_uri:
 * @browser: a #MidoriBrowser
 * @uri: an URI
 *
 * Loads the specified URI in the current view.
 *
 * If the current view is opaque, and cannot load
 * new pages, it will automatically open a new tab.
 **/
void
midori_browser_set_current_uri (MidoriBrowser* browser,
                                const gchar*   uri)
{
    GtkWidget* view;

    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (uri != NULL);

    if ((view = midori_browser_get_current_tab (browser)))
        midori_view_set_uri (MIDORI_VIEW (view), uri);
    else
        midori_browser_add_uri (browser, uri);
}

/**
 * midori_browser_get_current_uri:
 * @browser: a #MidoriBrowser
 *
 * Determines the URI loaded in the current view.
 *
 * If there is no view present at all, %NULL is returned.
 *
 * Return value: the current URI, or %NULL
 **/
const gchar*
midori_browser_get_current_uri (MidoriBrowser* browser)
{
    GtkWidget* view;

    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);

    if ((view = midori_browser_get_current_tab (browser)))
        return midori_view_get_display_uri (MIDORI_VIEW (view));
    return NULL;
}

/**
 * midori_browser_set_current_page:
 * @browser: a #MidoriBrowser
 * @n: the index of a page
 *
 * Switches to the page with the index @n.
 *
 * The widget will also grab the focus automatically.
 **/
void
midori_browser_set_current_page (MidoriBrowser* browser,
                                 gint           n)
{
    GtkWidget* view;
    GtkWidget* web_view;

    gtk_notebook_set_current_page (GTK_NOTEBOOK (browser->notebook), n);
    view = gtk_notebook_get_nth_page (GTK_NOTEBOOK (browser->notebook), n);
    if (view && midori_view_is_blank (MIDORI_VIEW (view)))
        gtk_action_activate (_action_by_name (browser, "Location"));
    else if ((web_view = gtk_bin_get_child (GTK_BIN (view))))
        gtk_widget_grab_focus (web_view);
    else
        gtk_widget_grab_focus (view);
}

/**
 * midori_browser_get_current_page:
 * @browser: a #MidoriBrowser
 *
 * Determines the currently selected page.
 *
 * If there is no page present at all, %NULL is returned.
 *
 * Return value: the selected page, or -1
 **/
gint
midori_browser_get_current_page (MidoriBrowser* browser)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), -1);

    return gtk_notebook_get_current_page (GTK_NOTEBOOK (browser->notebook));
}

/**
 * midori_browser_get_nth_tab:
 * @browser: a #MidoriBrowser
 * @page: the index of a tab
 *
 * Retrieves the tab at the position @page.
 *
 * If there is no page present at all, %NULL is returned.
 *
 * Return value: the selected page, or -1
 *
 * Since: 0.1.9
 **/
GtkWidget*
midori_browser_get_nth_tab (MidoriBrowser* browser,
                            gint           page)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);

    return gtk_notebook_get_nth_page (GTK_NOTEBOOK (browser->notebook), page);
}

/**
 * midori_browser_set_current_tab:
 * @browser: a #MidoriBrowser
 * @view: a #GtkWidget
 *
 * Switches to the page containing @view.
 *
 * The widget will also grab the focus automatically.
 **/
void
midori_browser_set_current_tab (MidoriBrowser* browser,
                                GtkWidget*     view)
{
    gint n;
    GtkWidget* web_view;

    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (GTK_IS_WIDGET (view));

    n = gtk_notebook_page_num (GTK_NOTEBOOK (browser->notebook), view);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (browser->notebook), n);
    if (view && midori_view_is_blank (MIDORI_VIEW (view)))
        gtk_action_activate (_action_by_name (browser, "Location"));
    else if ((web_view = gtk_bin_get_child (GTK_BIN (view))))
        gtk_widget_grab_focus (web_view);
    else
        gtk_widget_grab_focus (view);
}

/**
 * midori_browser_get_current_tab:
 * @browser: a #MidoriBrowser
 *
 * Retrieves the currently selected tab.
 *
 * If there is no tab present at all, %NULL is returned.
 *
 * See also midori_browser_get_current_page().
 *
 * Return value: the selected tab, or %NULL
 **/
GtkWidget*
midori_browser_get_current_tab (MidoriBrowser* browser)
{
    gint n;

    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);

    n = gtk_notebook_get_current_page (GTK_NOTEBOOK (browser->notebook));
    if (n >= 0)
        return gtk_notebook_get_nth_page (GTK_NOTEBOOK (browser->notebook), n);
    else
        return NULL;
}

/**
 * midori_browser_get_proxy_array:
 * @browser: a #MidoriBrowser
 *
 * Retrieves a proxy array representing the respective proxy items
 * of the present views that can be used for session management.
 *
 * The array is updated automatically.
 *
 * Note: Calling this function doesn't add a reference and the browser
 *       may release its reference at some point.
 *
 * Return value: the proxy #KatzeArray
 **/
KatzeArray*
midori_browser_get_proxy_array (MidoriBrowser* browser)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);

    return browser->proxy_array;
}

/**
 * midori_browser_get_for_widget:
 * @widget: a #GtkWidget
 *
 * Determines the browser appropriate for the specified widget.
 *
 * Return value: a #MidoriBrowser
 *
 * Since 0.1.7
 **/
MidoriBrowser*
midori_browser_get_for_widget (GtkWidget* widget)
{
    gpointer browser;

    g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

    browser = gtk_widget_get_toplevel (GTK_WIDGET (widget));
    if (!MIDORI_IS_BROWSER (browser))
    {
        if (!GTK_IS_WINDOW (browser))
            return NULL;

        browser = gtk_window_get_transient_for (GTK_WINDOW (browser));
        if (!MIDORI_IS_BROWSER (browser))
            return NULL;
    }

    return MIDORI_BROWSER (browser);
}

/**
 * midori_browser_quit:
 * @browser: a #MidoriBrowser
 *
 * Quits the browser, including any other browser windows.
 *
 * This function relys on the application implementing
 * the MidoriBrowser::quit signal. If the browser was added
 * to the MidoriApp, this is handled automatically.
 **/
void
midori_browser_quit (MidoriBrowser* browser)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));

    g_signal_emit (browser, signals[QUIT], 0);
}
