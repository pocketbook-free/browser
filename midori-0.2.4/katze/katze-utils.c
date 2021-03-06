/*
 Copyright (C) 2007-2009 Christian Dywan <christian@twotoasts.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
*/

#include "katze-utils.h"

#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <string.h>

#if HAVE_CONFIG_H
    #include "config.h"
#endif

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#ifdef HAVE_HILDON_2_2
    #include <hildon/hildon.h>
#endif

#if !GTK_CHECK_VERSION (2, 18, 0)
    #define gtk_widget_get_has_window(wdgt) !GTK_WIDGET_NO_WINDOW (wdgt)
    #define gtk_widget_get_allocation(wdgt, alloc) *alloc = wdgt->allocation
#endif

static void
proxy_toggle_button_toggled_cb (GtkToggleButton* button,
                                GObject*         object)
{
    gboolean toggled;
    const gchar* property;

    #ifdef HAVE_HILDON_2_2
    if (HILDON_IS_CHECK_BUTTON (button))
        toggled = hildon_check_button_get_active (HILDON_CHECK_BUTTON (button));
    #else
    toggled = gtk_toggle_button_get_active (button);
    #endif
    property = g_object_get_data (G_OBJECT (button), "property");
    g_object_set (object, property, toggled, NULL);
}

static void
proxy_file_file_set_cb (GtkFileChooser* button,
                        GObject*        object)
{
    const gchar* file = gtk_file_chooser_get_filename (button);
    const gchar* property = g_object_get_data (G_OBJECT (button), "property");
    g_object_set (object, property, file, NULL);
}

static void
proxy_folder_file_set_cb (GtkFileChooser* button,
                          GObject*        object)
{
    const gchar* file = gtk_file_chooser_get_current_folder (button);
    const gchar* property = g_object_get_data (G_OBJECT (button), "property");
    g_object_set (object, property, file, NULL);
}

static void
proxy_uri_file_set_cb (GtkFileChooser* button,
                       GObject*        object)
{
    const gchar* file = gtk_file_chooser_get_uri (button);
    const gchar* property = g_object_get_data (G_OBJECT (button), "property");
    g_object_set (object, property, file, NULL);
}

static void
proxy_combo_box_text_changed_cb (GtkComboBox* button,
                                 GObject*     object)
{
    gchar* text = gtk_combo_box_get_active_text (button);
    const gchar* property = g_object_get_data (G_OBJECT (button), "property");
    g_object_set (object, property, text, NULL);
}

static const gchar*
katze_app_info_get_commandline (GAppInfo* info)
{
    const gchar* exe;

    #if GLIB_CHECK_VERSION (2, 20, 0)
    exe = g_app_info_get_commandline (info);
    #else
    exe = g_object_get_data (G_OBJECT (info), "katze-cmdline");
    #endif
    if (!exe)
        exe = g_app_info_get_executable (info);
    return exe;
}

static gboolean
proxy_entry_focus_out_event_cb (GtkEntry*      entry,
                                GdkEventFocus* event,
                                GObject*       object);

static void
proxy_combo_box_apps_changed_cb (GtkComboBox* button,
                                 GObject*     object)
{
    guint active = gtk_combo_box_get_active (button);
    GtkTreeModel* model = gtk_combo_box_get_model (button);
    GtkTreeIter iter;

    if (gtk_tree_model_iter_nth_child (model, &iter, NULL, active))
    {
        GAppInfo* info;
        gboolean use_entry;
        GtkWidget* child;
        const gchar* exe;
        const gchar* property = g_object_get_data (G_OBJECT (button), "property");

        gtk_tree_model_get (model, &iter, 0, &info, -1);

        use_entry = info && !g_app_info_get_icon (info);
        child = gtk_bin_get_child (GTK_BIN (button));
        if (use_entry && GTK_IS_CELL_VIEW (child))
        {
            GtkWidget* entry = gtk_entry_new ();
            exe = g_app_info_get_executable (info);
            if (exe && *exe && strcmp (exe, "%f"))
                gtk_entry_set_text (GTK_ENTRY (entry), exe);
            gtk_widget_show (entry);
            gtk_container_add (GTK_CONTAINER (button), entry);
            gtk_widget_grab_focus (entry);
            g_signal_connect (entry, "focus-out-event",
                G_CALLBACK (proxy_entry_focus_out_event_cb), object);
            g_object_set_data_full (G_OBJECT (entry), "property",
                                    g_strdup (property), g_free);
        }
        else if (!use_entry && GTK_IS_ENTRY (child))
        {
            /* Force the combo to change the item again */
            gtk_widget_destroy (child);
            gtk_combo_box_set_active (button, 0);
            gtk_combo_box_set_active_iter (button, &iter);
        }

        if (info)
        {
            exe = katze_app_info_get_commandline (info);
            g_object_set (object, property, exe, NULL);
            g_object_unref (info);
        }
        else
            g_object_set (object, property, "", NULL);
    }
}

static void
proxy_entry_activate_cb (GtkEntry* entry,
                         GObject*  object)
{
    const gchar* text = gtk_entry_get_text (entry);
    const gchar* property = g_object_get_data (G_OBJECT (entry), "property");
    g_object_set (object, property, text, NULL);
}

static gboolean
proxy_entry_focus_out_event_cb (GtkEntry*      entry,
                                GdkEventFocus* event,
                                GObject*       object)
{
    const gchar* text = gtk_entry_get_text (entry);
    const gchar* property = g_object_get_data (G_OBJECT (entry), "property");
    g_object_set (object, property, text, NULL);
    return FALSE;
}

static void
proxy_spin_button_changed_cb (GtkSpinButton* button,
                              GObject*       object)
{
    GObjectClass* class = G_OBJECT_GET_CLASS (object);
    const gchar* property = g_object_get_data (G_OBJECT (button), "property");
    GParamSpec* pspec = g_object_class_find_property (class, property);
    if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_INT)
    {
        gint value = gtk_spin_button_get_value_as_int (button);
        g_object_set (object, property, value, NULL);
    }
    else
    {
        gdouble value = gtk_spin_button_get_value (button);
        g_object_set (object, property, value, NULL);
    }
}

#ifdef HAVE_HILDON_2_2
static void
proxy_picker_button_changed_cb (HildonPickerButton* button,
                                GObject*            object)
{
    gint value = hildon_picker_button_get_active (button);
    const gchar* property = g_object_get_data (G_OBJECT (button), "property");
    g_object_set (object, property, value, NULL);
    /* FIXME: Implement custom-PROPERTY */
}
#else
static void
proxy_combo_box_changed_cb (GtkComboBox* button,
                            GObject*     object)
{
    gint value = gtk_combo_box_get_active (button);
    const gchar* property = g_object_get_data (G_OBJECT (button), "property");
    gint custom_value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                         "katze-custom-value"));
    const gchar* custom_property = g_object_get_data (G_OBJECT (button),
                                                      "katze-custom-property");

    if (custom_value)
    {
        GtkWidget* child = gtk_bin_get_child (GTK_BIN (button));

        if (value == custom_value && GTK_IS_CELL_VIEW (child))
        {
            GtkWidget* entry = gtk_entry_new ();
            gchar* custom_text = katze_object_get_string (object, custom_property);
            if (custom_text && *custom_text)
                gtk_entry_set_text (GTK_ENTRY (entry), custom_text);
            g_free (custom_text);
            gtk_widget_show (entry);
            gtk_container_add (GTK_CONTAINER (button), entry);
            gtk_widget_grab_focus (entry);
            g_signal_connect (entry, "focus-out-event",
                G_CALLBACK (proxy_entry_focus_out_event_cb), object);
            g_object_set_data_full (G_OBJECT (entry), "property",
                                    g_strdup (custom_property), g_free);
        }
        else if (value != custom_value && GTK_IS_ENTRY (child))
        {
            /* Force the combo to change the item again */
            gtk_widget_destroy (child);
            gtk_combo_box_set_active (button, value + 1);
            gtk_combo_box_set_active (button, value);
        }
    }

    g_object_set (object, property, value, NULL);

    if (custom_value)
    {
        #if GTK_CHECK_VERSION (2, 12, 0)
        if (value == custom_value)
            gtk_widget_set_tooltip_text (GTK_WIDGET (button), NULL);
        else
        {
            gchar* custom_text = katze_object_get_string (object, custom_property);
            gtk_widget_set_tooltip_text (GTK_WIDGET (button), custom_text);
            g_free (custom_text);
        }
        #endif
    }
}
#endif

static void
proxy_object_notify_boolean_cb (GObject*    object,
                                GParamSpec* pspec,
                                GtkWidget*  proxy)
{
    const gchar* property = g_object_get_data (G_OBJECT (proxy), "property");
    gboolean value = katze_object_get_boolean (object, property);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (proxy), value);
}

static void
proxy_object_notify_string_cb (GObject*    object,
                               GParamSpec* pspec,
                               GtkWidget*  proxy)
{
    const gchar* property = g_object_get_data (G_OBJECT (proxy), "property");
    gchar* value = katze_object_get_string (object, property);
    gtk_entry_set_text (GTK_ENTRY (proxy), value);
    g_free (value);
}

static void
proxy_widget_boolean_destroy_cb (GtkWidget* proxy,
                                 GObject*   object)
{
    g_signal_handlers_disconnect_by_func (object,
        proxy_object_notify_boolean_cb, proxy);
}

static void
proxy_widget_string_destroy_cb (GtkWidget* proxy,
                                 GObject*  object)
{
    g_signal_handlers_disconnect_by_func (object,
        proxy_object_notify_string_cb, proxy);
}

static GList*
katze_app_info_get_all_for_category (const gchar* category)
{
    GList* all_apps = g_app_info_get_all ();
    GList* apps = NULL;
    guint i = 0;
    GAppInfo* info;
    while ((info = g_list_nth_data (all_apps, i++)))
    {
        #ifdef GDK_WINDOWING_X11
        gchar* filename = g_strconcat ("applications/", g_app_info_get_id (info), NULL);
        GKeyFile* file = g_key_file_new ();

        if (g_key_file_load_from_data_dirs (file, filename, NULL, G_KEY_FILE_NONE, NULL))
        {
            gchar* cat = g_key_file_get_string (file, "Desktop Entry",
                                                "Categories", NULL);
            if (cat && g_strrstr (cat, category))
                apps = g_list_append (apps, info);

            g_free (cat);
        }
        g_key_file_free (file);
        g_free (filename);
        #else
        apps = g_list_append (apps, info);
        #endif
    }
    g_list_free (all_apps);
    return apps;
}

#if !GLIB_CHECK_VERSION (2, 20, 0)
/* Icon tokenization, for Glib < 2.20 */
static gboolean
g_icon_to_string_tokenized (GIcon   *icon,
                            GString *s)
{
  GPtrArray *tokens;
  guint i;

  if (G_IS_THEMED_ICON (icon))
    {
      guint n;
      const char * const *names;
      tokens = g_ptr_array_new ();
      g_object_get (icon, "names", &names, NULL);
      for (n = 0; names[n] != NULL; n++)
        g_ptr_array_add (tokens, g_strdup (names[n]));
    }
  else if (G_IS_FILE_ICON (icon))
    {
      tokens = g_ptr_array_new ();
      g_ptr_array_add (tokens,
                       g_file_get_uri (g_file_icon_get_file (G_FILE_ICON (icon))));
    }
  else
    return FALSE;

  g_string_append (s, g_type_name_from_instance ((GTypeInstance *)icon));

  for (i = 0; i < tokens->len; i++)
    {
      char *token = g_ptr_array_index (tokens, i);

      g_string_append_c (s, ' ');
      g_string_append_uri_escaped (s, token, "!$&'()*+,;=:@/", TRUE);

      g_free (token);
    }

  g_ptr_array_free (tokens, TRUE);

  return TRUE;
}

/* GIcon serialization, for Glib < 2.20 */
static gchar *
g_icon_to_string (GIcon *icon)
{
  gchar *ret = NULL;

  g_return_val_if_fail (G_IS_ICON (icon), NULL);

  if (G_IS_FILE_ICON (icon))
    {
      GFile *file = g_file_icon_get_file (G_FILE_ICON (icon));
      if (g_file_is_native (file))
        {
          ret = g_file_get_path (file);
          if (!g_utf8_validate (ret, -1, NULL))
            {
              g_free (ret);
              ret = NULL;
            }
        }
      else
        ret = g_file_get_uri (file);
    }
  else if (G_IS_THEMED_ICON (icon))
    {
      const char * const *names;
      g_object_get (icon, "names", &names, NULL);
      if (names && names[0] && names[0][0] != '.' &&
          g_utf8_validate (names[0], -1, NULL) && names[1] == NULL)
        ret = g_strdup (names[0]);
    }

  if (ret == NULL)
    {
      GString *s = g_string_new (". ");
      if (g_icon_to_string_tokenized (icon, s))
        ret = g_string_free (s, FALSE);
      else
        g_string_free (s, TRUE);
    }

  return ret;
}
#endif

/**
 * katze_property_proxy:
 * @object: a #GObject
 * @property: the name of a property
 * @hint: a special hint
 *
 * Create a widget of an appropriate type to represent the specified
 * object's property. If the property is writable changes of the value
 * through the widget will be reflected in the value of the property.
 *
 * Supported values for @hint are as follows:
 *     "blurb": the blurb of the property will be used to provide a kind
 *         of label, instead of the name.
 *     "file": the widget created will be particularly suitable for
 *         choosing an existing filename.
 *     "folder": the widget created will be particularly suitable for
 *         choosing an existing folder.
 *     "uri": the widget created will be particularly suitable for
 *         choosing an existing filename, encoded as an URI.
 *     "font": the widget created will be particularly suitable for
 *         choosing a variable-width font from installed fonts.
 *     Since 0.1.6 the following hints are also supported:
 *     "toggle": the widget created will be an empty toggle button. This
 *         is only supported with boolean properties.
 *         Since 0.1.8 "toggle" creates GtkCheckButton widgets without checkmarks.
 *     Since 0.2.0 the following hints are also supported:
 *     "font-monospace": the widget created will be particularly suitable for
 *         choosing a fixed-width font from installed fonts.
 *     Since 0.2.1 the following hints are also supported:
 *     "application-TYPE": the widget created will be particularly suitable
 *         for choosing an application to open TYPE files, ie. "text/plain".
 *     "application-CATEGORY": the widget created will be particularly suitable
 *         for choosing an application to open CATEGORY files, ie. "Network".
 *     "custom-PROPERTY": the last value of an enumeration will be the "custom"
 *         value, where the user may enter text freely, which then updates
 *         the property PROPERTY instead. This applies only to enumerations.
 *
 * Any other values for @hint are silently ignored.
 *
 * Since 0.1.2 strings without hints and booleans are truly synchronous
 *     including property changes causing the proxy to be updated.
 *
 * Since 0.2.1 the proxy may contain a label if the platform
 *     has according widgets.
 *
 * Return value: a new widget
 **/
GtkWidget*
katze_property_proxy (gpointer     object,
                      const gchar* property,
                      const gchar* hint)
{
    GObjectClass* class;
    GParamSpec* pspec;
    GType type;
    const gchar* nick;
    const gchar* _hint;
    GtkWidget* widget;
    gchar* string;

    g_return_val_if_fail (G_IS_OBJECT (object), NULL);

    class = G_OBJECT_GET_CLASS (object);
    pspec = g_object_class_find_property (class, property);
    if (!pspec)
    {
        g_warning (_("Property '%s' is invalid for %s"),
                   property, G_OBJECT_CLASS_NAME (class));
        return gtk_label_new (property);
    }

    type = G_PARAM_SPEC_TYPE (pspec);
    nick = g_param_spec_get_nick (pspec);
    _hint = g_intern_string (hint);
    if (_hint == g_intern_string ("blurb"))
        nick = g_param_spec_get_blurb (pspec);
    string = NULL;
    if (type == G_TYPE_PARAM_BOOLEAN)
    {
        gchar* notify_property;
        gboolean toggled = katze_object_get_boolean (object, property);

        #ifdef HAVE_HILDON_2_2
        if (_hint != g_intern_string ("toggle"))
        {
            widget = hildon_check_button_new (HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH);
            gtk_button_set_label (GTK_BUTTON (widget), gettext (nick));
            hildon_check_button_set_active (HILDON_CHECK_BUTTON (widget), toggled);
        }
        else
        #endif
        {
            widget = gtk_check_button_new ();
            if (_hint == g_intern_string ("toggle"))
                gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (widget), FALSE);
            else
                gtk_button_set_label (GTK_BUTTON (widget), gettext (nick));
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), toggled);
        }
        g_signal_connect (widget, "toggled",
                          G_CALLBACK (proxy_toggle_button_toggled_cb), object);
        notify_property = g_strdup_printf ("notify::%s", property);
        g_signal_connect (object, notify_property,
            G_CALLBACK (proxy_object_notify_boolean_cb), widget);
        g_signal_connect (widget, "destroy",
            G_CALLBACK (proxy_widget_boolean_destroy_cb), object);
        g_free (notify_property);
    }
    else if (type == G_TYPE_PARAM_STRING && _hint == g_intern_string ("file"))
    {
        string = katze_object_get_string (object, property);

        widget = gtk_file_chooser_button_new (_("Choose file"),
            GTK_FILE_CHOOSER_ACTION_OPEN);

        if (!string)
            string = g_strdup (G_PARAM_SPEC_STRING (pspec)->default_value);
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (widget),
                                       string ? string : "");
        if (pspec->flags & G_PARAM_WRITABLE)
            g_signal_connect (widget, "selection-changed",
                              G_CALLBACK (proxy_file_file_set_cb), object);
    }
    else if (type == G_TYPE_PARAM_STRING && _hint == g_intern_string ("folder"))
    {
        string = katze_object_get_string (object, property);

        widget = gtk_file_chooser_button_new (_("Choose folder"),
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
        if (!string)
            string = g_strdup (G_PARAM_SPEC_STRING (pspec)->default_value);
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
                                             string ? string : "");
        if (pspec->flags & G_PARAM_WRITABLE)
            g_signal_connect (widget, "selection-changed",
                              G_CALLBACK (proxy_folder_file_set_cb), object);
    }
    else if (type == G_TYPE_PARAM_STRING && _hint == g_intern_string ("uri"))
    {
        string = katze_object_get_string (object, property);

        widget = gtk_file_chooser_button_new (_("Choose file"),
            GTK_FILE_CHOOSER_ACTION_OPEN);
        if (!string)
            string = g_strdup (G_PARAM_SPEC_STRING (pspec)->default_value);
        gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (widget),
                                  string ? string : "");
        #if GTK_CHECK_VERSION (2, 12, 0)
        g_signal_connect (widget, "file-set",
                          G_CALLBACK (proxy_uri_file_set_cb), object);
        #else
        if (pspec->flags & G_PARAM_WRITABLE)
            g_signal_connect (widget, "selection-changed",
                              G_CALLBACK (proxy_uri_file_set_cb), object);
        #endif
    }
    else if (type == G_TYPE_PARAM_STRING && (_hint == g_intern_string ("font")
        || _hint == g_intern_string ("font-monospace")))
    {
        GtkComboBox* combo;
        gint n_families, i;
        PangoContext* context;
        PangoFontFamily** families;
        gboolean monospace = _hint == g_intern_string ("font-monospace");
        string = katze_object_get_string (object, property);

        widget = gtk_combo_box_new_text ();
        combo = GTK_COMBO_BOX (widget);
        context = gtk_widget_get_pango_context (widget);
        pango_context_list_families (context, &families, &n_families);
        if (!string)
            string = g_strdup (G_PARAM_SPEC_STRING (pspec)->default_value);
        /* 'sans' and 'sans-serif' are presumably the same */
        if (!g_strcmp0 (string, "sans-serif"))
            katze_assign (string, g_strdup ("sans"));
        if (string)
        {
            gint j = 0;
            for (i = 0; i < n_families; i++)
            {
                const gchar* font = pango_font_family_get_name (families[i]);
                if (monospace != pango_font_family_is_monospace (families[i]))
                    continue;
                gtk_combo_box_append_text (combo, font);
                if (!g_ascii_strcasecmp (font, string))
                    gtk_combo_box_set_active (combo, j);
                j++;
            }
        }
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (
            gtk_combo_box_get_model (combo)), 0, GTK_SORT_ASCENDING);
        g_signal_connect (widget, "changed",
                          G_CALLBACK (proxy_combo_box_text_changed_cb), object);
        g_free (families);
    }
    else if (type == G_TYPE_PARAM_STRING && hint && g_str_has_prefix (hint, "application-"))
    {
        GtkListStore* model;
        GtkCellRenderer* renderer;
        GtkComboBox* combo;
        GList* apps;
        const gchar* app_type = &hint[12];
        GtkSettings* settings;
        gint icon_width = 16;

        settings = gtk_settings_get_for_screen (gdk_screen_get_default ());
        gtk_icon_size_lookup_for_settings (settings, GTK_ICON_SIZE_MENU,
                                           &icon_width, NULL);

        model = gtk_list_store_new (4, G_TYPE_APP_INFO, G_TYPE_STRING,
                                       G_TYPE_STRING, G_TYPE_INT);
        widget = gtk_combo_box_new_with_model (GTK_TREE_MODEL (model));
        renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, FALSE);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (widget), renderer, "icon-name", 1);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (widget), renderer, "width", 3);
        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (widget), renderer, "text", 2);
        combo = GTK_COMBO_BOX (widget);
        apps = g_app_info_get_all_for_type (app_type);
        if (!apps)
            apps = katze_app_info_get_all_for_category (app_type);

        string = katze_object_get_string (object, property);
        if (!g_strcmp0 (string, ""))
            katze_assign (string, NULL);

        if (apps)
        {
            GtkTreeIter iter_none;
            gint i = 0;
            GAppInfo* info;

            gtk_list_store_insert_with_values (model, &iter_none, 0,
                0, NULL, 1, NULL, 2, _("None"), 3, icon_width, -1);

            while ((info = g_list_nth_data (apps, i++)))
            {
                const gchar* name = g_app_info_get_name (info);
                GIcon* icon = g_app_info_get_icon (info);
                gchar* icon_name;
                GtkTreeIter iter;

                if (!g_app_info_should_show (info))
                    continue;

                icon_name = icon ? g_icon_to_string (icon) : NULL;
                gtk_list_store_insert_with_values (model, &iter, G_MAXINT,
                    0, info, 1, icon_name, 2, name, 3, icon_width, -1);
                if (string && !strcmp (katze_app_info_get_commandline (info), string))
                    gtk_combo_box_set_active_iter (combo, &iter);

                g_free (icon_name);
            }

            info = g_app_info_create_from_commandline ("",
                "", G_APP_INFO_CREATE_NONE, NULL);
            gtk_list_store_insert_with_values (model, NULL, G_MAXINT,
                0, info, 1, NULL, 2, _("Custom..."), 3, icon_width, -1);
            g_object_unref (info);

            if (gtk_combo_box_get_active (combo) == -1)
            {
                if (string)
                {
                    GtkWidget* entry;
                    const gchar* exe;

                    info = g_app_info_create_from_commandline (string,
                        NULL, G_APP_INFO_CREATE_NONE, NULL);
                    #if !GLIB_CHECK_VERSION (2, 20, 0)
                    g_object_set_data (G_OBJECT (info), "katze-cmdline", string);
                    #endif
                    entry = gtk_entry_new ();
                    exe = g_app_info_get_executable (info);
                    if (exe && *exe && strcmp (exe, "%f"))
                        gtk_entry_set_text (GTK_ENTRY (entry), string);
                    gtk_widget_show (entry);
                    gtk_container_add (GTK_CONTAINER (combo), entry);
                    g_object_unref (info);
                    g_signal_connect (entry, "focus-out-event",
                        G_CALLBACK (proxy_entry_focus_out_event_cb), object);
                    g_object_set_data_full (G_OBJECT (entry), "property",
                                            g_strdup (property), g_free);
                }
                else
                    gtk_combo_box_set_active_iter (combo, &iter_none);
            }
        }
        g_list_free (apps);
        g_signal_connect (widget, "changed",
                          G_CALLBACK (proxy_combo_box_apps_changed_cb), object);
    }
    else if (type == G_TYPE_PARAM_STRING)
    {
        gchar* notify_property;

        widget = gtk_entry_new ();
        g_object_get (object, property, &string, NULL);
        if (!string)
            string = g_strdup (G_PARAM_SPEC_STRING (pspec)->default_value);
        gtk_entry_set_text (GTK_ENTRY (widget), string ? string : "");
        g_signal_connect (widget, "activate",
                          G_CALLBACK (proxy_entry_activate_cb), object);
        g_signal_connect (widget, "focus-out-event",
                          G_CALLBACK (proxy_entry_focus_out_event_cb), object);
        notify_property = g_strdup_printf ("notify::%s", property);
        g_signal_connect (object, notify_property,
            G_CALLBACK (proxy_object_notify_string_cb), widget);
        g_signal_connect (widget, "destroy",
            G_CALLBACK (proxy_widget_string_destroy_cb), object);
        g_free (notify_property);
    }
    else if (type == G_TYPE_PARAM_FLOAT)
    {
        gfloat value = katze_object_get_float (object, property);

        widget = gtk_spin_button_new_with_range (
            G_PARAM_SPEC_FLOAT (pspec)->minimum,
            G_PARAM_SPEC_FLOAT (pspec)->maximum, 1);
        /* Keep it narrow, 5 + 2 digits are usually fine */
        gtk_entry_set_width_chars (GTK_ENTRY (widget), 5 + 2);
        gtk_spin_button_set_digits (GTK_SPIN_BUTTON (widget), 2);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), value);
        g_signal_connect (widget, "value-changed",
                          G_CALLBACK (proxy_spin_button_changed_cb), object);
    }
    else if (type == G_TYPE_PARAM_INT)
    {
        gint value = katze_object_get_int (object, property);

        widget = gtk_spin_button_new_with_range (
            G_PARAM_SPEC_INT (pspec)->minimum,
            G_PARAM_SPEC_INT (pspec)->maximum, 1);
        #if HAVE_HILDON
        hildon_gtk_entry_set_input_mode (GTK_ENTRY (widget),
                                         HILDON_GTK_INPUT_MODE_NUMERIC);
        #endif
        /* Keep it narrow, 5 digits are usually fine */
        gtk_entry_set_width_chars (GTK_ENTRY (widget), 5);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), value);
        g_signal_connect (widget, "value-changed",
                          G_CALLBACK (proxy_spin_button_changed_cb), object);
    }
    else if (type == G_TYPE_PARAM_ENUM)
    {
        guint i;
        GEnumClass* enum_class = G_ENUM_CLASS (
            g_type_class_ref (pspec->value_type));
        gint value = katze_object_get_enum (object, property);
        const gchar* custom = NULL;

        if (hint && g_str_has_prefix (hint, "custom-"))
            custom = &hint[7];

        #ifdef HAVE_HILDON_2_2
        GtkWidget* selector;

        widget = hildon_picker_button_new (
            HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH,
            HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);
        selector = hildon_touch_selector_new_text ();
        hildon_button_set_title (HILDON_BUTTON (widget), gettext (nick));
        hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (widget),
                                           HILDON_TOUCH_SELECTOR (selector));
        #else
        widget = gtk_combo_box_new_text ();
        #endif
        for (i = 0; i < enum_class->n_values; i++)
        {
            const gchar* label = gettext (enum_class->values[i].value_nick);
            #ifdef HAVE_HILDON_2_2
            hildon_touch_selector_append_text (HILDON_TOUCH_SELECTOR (selector), label);
            #else
            gtk_combo_box_append_text (GTK_COMBO_BOX (widget), label);
            #endif
        }
        #ifdef HAVE_HILDON_2_2
        hildon_touch_selector_set_active (HILDON_TOUCH_SELECTOR (selector), 0, value);
        g_signal_connect (widget, "value-changed",
                          G_CALLBACK (proxy_picker_button_changed_cb), object);
        #else
        gtk_combo_box_set_active (GTK_COMBO_BOX (widget), value);
        g_signal_connect (widget, "changed",
                          G_CALLBACK (proxy_combo_box_changed_cb), object);
        #endif
        if (custom)
        {
            gchar* custom_text = katze_object_get_string (object, custom);

            if (value == (gint)(enum_class->n_values - 1))
            {
                GtkWidget* entry = gtk_entry_new ();
                gchar* text = katze_object_get_string (object, custom);
                if (text && *text)
                    gtk_entry_set_text (GTK_ENTRY (entry), text);
                gtk_widget_show (entry);
                gtk_container_add (GTK_CONTAINER (widget), entry);
                g_signal_connect (entry, "focus-out-event",
                    G_CALLBACK (proxy_entry_focus_out_event_cb), object);
                g_object_set_data_full (G_OBJECT (entry), "property",
                                        g_strdup (custom), g_free);
            }
            #if GTK_CHECK_VERSION (2, 12, 0)
            else
                gtk_widget_set_tooltip_text (widget, custom_text);
            #endif

            g_free (custom_text);

            g_object_set_data (G_OBJECT (widget), "katze-custom-value",
                               GINT_TO_POINTER (enum_class->n_values - 1));
            g_object_set_data (G_OBJECT (widget), "katze-custom-property",
                               (gpointer)custom);
        }
        g_type_class_unref (enum_class);
    }
    else
        widget = gtk_label_new (gettext (nick));
    g_free (string);

    #if GTK_CHECK_VERSION (2, 12, 0)
    if (!gtk_widget_get_tooltip_text (widget))
        gtk_widget_set_tooltip_text (widget, g_param_spec_get_blurb (pspec));
    #endif
    gtk_widget_set_sensitive (widget, pspec->flags & G_PARAM_WRITABLE);

    g_object_set_data_full (G_OBJECT (widget), "property",
                            g_strdup (property), g_free);

    return widget;
}

/**
 * katze_property_label:
 * @object: a #GObject
 * @property: the name of a property
 *
 * Create a label widget displaying the name of the specified object's property.
 *
 * Return value: a new label widget
 *
 * Since 0.2.1 the label will be empty if the property proxy for the
 *    same property would contain a label already.
 **/
GtkWidget*
katze_property_label (gpointer     object,
                      const gchar* property)
{
    GObjectClass* class;
    GParamSpec* pspec;
    const gchar* nick;
    GtkWidget* widget;

    g_return_val_if_fail (G_IS_OBJECT (object), NULL);

    class = G_OBJECT_GET_CLASS (object);
    pspec = g_object_class_find_property (class, property);
    if (!pspec)
    {
        g_warning (_("Property '%s' is invalid for %s"),
                   property, G_OBJECT_CLASS_NAME (class));
        return gtk_label_new (property);
    }

    #ifdef HAVE_HILDON_2_2
    if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_ENUM)
        return gtk_label_new (NULL);
    #endif

    nick = g_param_spec_get_nick (pspec);
    widget = gtk_label_new (nick);
    #if GTK_CHECK_VERSION (2, 12, 0)
    gtk_widget_set_tooltip_text (widget, g_param_spec_get_blurb (pspec));
    #endif
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);

    return widget;
}

typedef struct
{
     GtkWidget* widget;
     KatzeMenuPos position;
} KatzePopupInfo;

static void
katze_widget_popup_position_menu (GtkMenu*  menu,
                                  gint*     x,
                                  gint*     y,
                                  gboolean* push_in,
                                  gpointer  user_data)
{
    gint wx, wy;
    gint menu_width;
    GtkAllocation allocation;
    GtkRequisition menu_req;
    GtkRequisition widget_req;
    KatzePopupInfo* info = user_data;
    GtkWidget* widget = info->widget;
    gint widget_height;

    gtk_widget_get_allocation (widget, &allocation);

    /* Retrieve size and position of both widget and menu */
    if (!gtk_widget_get_has_window (widget))
    {
        gdk_window_get_position (gtk_widget_get_window (widget), &wx, &wy);
        wx += allocation.x;
        wy += allocation.y;
    }
    else
        gdk_window_get_origin (gtk_widget_get_window (widget), &wx, &wy);
    gtk_widget_size_request (GTK_WIDGET (menu), &menu_req);
    gtk_widget_size_request (widget, &widget_req);
    menu_width = menu_req.width;
    widget_height = widget_req.height; /* Better than allocation.height */

    /* Calculate menu position */
    if (info->position == KATZE_MENU_POSITION_CURSOR)
        ; /* Do nothing? */
    else if (info->position == KATZE_MENU_POSITION_RIGHT)
    {
        *x = wx + allocation.width - menu_width;
        *y = wy + widget_height;
    } else if (info->position == KATZE_MENU_POSITION_LEFT)
    {
        *x = wx;
        *y = wy + widget_height;
    }

    *push_in = TRUE;
}

/**
 * katze_widget_popup:
 * @widget: a widget
 * @menu: the menu to popup
 * @event: a button event, or %NULL
 * @pos: the preferred positioning
 *
 * Pops up the given menu relative to @widget. Use this
 * instead of writing custom positioning functions.
 *
 * Return value: a new label widget
 **/
void
katze_widget_popup (GtkWidget*      widget,
                    GtkMenu*        menu,
                    GdkEventButton* event,
                    KatzeMenuPos    pos)
{
    int button, event_time;
    if (event)
    {
        button = event->button;
        event_time = event->time;
    }
    else
    {
        button = 0;
        event_time = gtk_get_current_event_time ();
    }

    if (!gtk_menu_get_attach_widget (menu))
        gtk_menu_attach_to_widget (menu, widget, NULL);


    if (widget)
    {
        KatzePopupInfo info = { widget, pos };
        gtk_menu_popup (menu, NULL, NULL,
                        katze_widget_popup_position_menu, &info,
                        button, event_time);
    }
    else
        gtk_menu_popup (menu, NULL, NULL, NULL, NULL, button, event_time);
}

/**
 * katze_image_menu_item_new_ellipsized:
 * @label: a label or %NULL
 *
 * Creates an image menu item where the label is
 * reasonably ellipsized for you.
 *
 * Return value: a new label widget
 **/
GtkWidget*
katze_image_menu_item_new_ellipsized (const gchar* label)
{
    GtkWidget* menuitem;
    GtkWidget* label_widget;

    menuitem = gtk_image_menu_item_new ();
    label_widget = gtk_label_new (label);
    /* FIXME: Should text direction be respected here? */
    gtk_misc_set_alignment (GTK_MISC (label_widget), 0.0, 0.0);
    gtk_label_set_max_width_chars (GTK_LABEL (label_widget), 50);
    gtk_label_set_ellipsize (GTK_LABEL (label_widget), PANGO_ELLIPSIZE_MIDDLE);
    gtk_widget_show (label_widget);
    gtk_container_add (GTK_CONTAINER (menuitem), label_widget);

    return menuitem;
}

/**
 * katze_pixbuf_new_from_buffer:
 * @buffer: Buffer with image data
 * @length: Length of the buffer
 * @mime_type: a MIME type, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Creates a new #GdkPixbuf out of the specified buffer.
 *
 * You can specify a MIME type if looking at the buffer
 * is not enough to determine the right type.
 *
 * Return value: A newly-allocated #GdkPixbuf
 **/
GdkPixbuf*
katze_pixbuf_new_from_buffer (const guchar* buffer,
                              gsize         length,
                              const gchar*  mime_type,
                              GError**      error)
{
    /* Proposed for inclusion in GdkPixbuf
       See http://bugzilla.gnome.org/show_bug.cgi?id=74291 */
    GdkPixbufLoader* loader;
    GdkPixbuf* pixbuf;

    g_return_val_if_fail (buffer != NULL, NULL);
    g_return_val_if_fail (length > 0, NULL);

    if (mime_type)
    {
        loader = gdk_pixbuf_loader_new_with_mime_type (mime_type, error);
        if (!loader)
            return NULL;
    }
    else
        loader = gdk_pixbuf_loader_new ();
    if (!gdk_pixbuf_loader_write (loader, buffer, length, error))
    {
        g_object_unref (loader);
        return NULL;
    }
    if (!gdk_pixbuf_loader_close (loader, error))
    {
        g_object_unref (loader);
        return NULL;
    }

    pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    g_object_ref (pixbuf);
    g_object_unref (loader);
    return pixbuf;
}

/**
 * katze_tree_view_get_selected_iter:
 * @treeview: a #GtkTreeView
 * @model: a pointer to store the model, or %NULL
 * @iter: a pointer to store the iter, or %NULL
 *
 * Determines whether there is a selection in @treeview
 * and sets the @iter to the current selection.
 *
 * If there is a selection and @model is not %NULL, it is
 * set to the model, mainly for convenience.
 *
 * Either @model or @iter or both can be %NULL in which case
 * no value will be assigned in any case.
 *
 * Return value: %TRUE if there is a selection
 *
 * Since: 0.1.3
 **/
gboolean
katze_tree_view_get_selected_iter (GtkTreeView*   treeview,
                                   GtkTreeModel** model,
                                   GtkTreeIter*   iter)
{
    GtkTreeSelection* selection;

    g_return_val_if_fail (GTK_IS_TREE_VIEW (treeview), FALSE);

    if ((selection = gtk_tree_view_get_selection (treeview)))
        if (gtk_tree_selection_get_selected (selection, model, iter))
            return TRUE;
    return FALSE;
}

/**
 * katze_strip_mnemonics:
 * @original: a string with mnemonics
 *
 * Parses the given string for mnemonics in the form
 * "B_utton" or "Button (_U)" and returns a string
 * without any mnemonics.
 *
 * Return value: a newly allocated string without mnemonics
 *
 * Since: 0.1.8
 **/
gchar*
katze_strip_mnemonics (const gchar* original)
{
  /* A copy of _gtk_toolbar_elide_underscores
     Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
     Copied from GTK+ 2.17.1 */
  gchar *q, *result;
  const gchar *p, *end;
  gsize len;
  gboolean last_underscore;

  if (!original)
    return NULL;

  len = strlen (original);
  q = result = g_malloc (len + 1);
  last_underscore = FALSE;

  end = original + len;
  for (p = original; p < end; p++)
    {
      if (!last_underscore && *p == '_')
	last_underscore = TRUE;
      else
	{
	  last_underscore = FALSE;
	  if (original + 2 <= p && p + 1 <= end &&
              p[-2] == '(' && p[-1] == '_' && p[0] != '_' && p[1] == ')')
	    {
	      q--;
	      *q = '\0';
	      p++;
	    }
	  else
	    *q++ = *p;
	}
    }

  if (last_underscore)
    *q++ = '_';

  *q = '\0';

  return result;
}

/**
 * katze_object_has_property:
 * @object: a #GObject
 * @property: the name of the property
 *
 * Determine if @object has a property with the specified name.
 *
 * Return value: a boolean
 *
 * Since: 0.1.2
 **/
gboolean
katze_object_has_property (gpointer     object,
                           const gchar* property)
{
    GObjectClass* class;

    g_return_val_if_fail (G_IS_OBJECT (object), FALSE);

    class = G_OBJECT_GET_CLASS (object);
    return g_object_class_find_property (class, property) != NULL;
}

/**
 * katze_object_get_boolean:
 * @object: a #GObject
 * @property: the name of the property to get
 *
 * Retrieve the boolean value of the specified property.
 *
 * Return value: a boolean
 **/
gboolean
katze_object_get_boolean (gpointer     object,
                          const gchar* property)
{
    gboolean value = FALSE;

    g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
    /* FIXME: Check value type */

    g_object_get (object, property, &value, NULL);
    return value;
}

/**
 * katze_object_get_int:
 * @object: a #GObject
 * @property: the name of the property to get
 *
 * Retrieve the integer value of the specified property.
 *
 * Return value: an integer
 **/
gint
katze_object_get_int (gpointer     object,
                      const gchar* property)
{
    gint value = -1;

    g_return_val_if_fail (G_IS_OBJECT (object), -1);
    /* FIXME: Check value type */

    g_object_get (object, property, &value, NULL);
    return value;
}

/**
 * katze_object_get_float:
 * @object: a #GObject
 * @property: the name of the property to get
 *
 * Retrieve the float value of the specified property.
 *
 * Return value: a float
 **/
gfloat
katze_object_get_float (gpointer     object,
                        const gchar* property)
{
    gfloat value = -1.0f;

    g_return_val_if_fail (G_IS_OBJECT (object), -1.0f);
    /* FIXME: Check value type */

    g_object_get (object, property, &value, NULL);
    return value;
}

/**
 * katze_object_get_enum:
 * @object: a #GObject
 * @property: the name of the property to get
 *
 * Retrieve the enum value of the specified property.
 *
 * Return value: an enumeration
 **/
gint
katze_object_get_enum (gpointer     object,
                       const gchar* property)
{
    gint value = -1;

    g_return_val_if_fail (G_IS_OBJECT (object), -1);
    /* FIXME: Check value type */

    g_object_get (object, property, &value, NULL);
    return value;
}

/**
 * katze_object_get_string:
 * @object: a #GObject
 * @property: the name of the property to get
 *
 * Retrieve the string value of the specified property.
 *
 * Return value: a newly allocated string
 **/
gchar*
katze_object_get_string (gpointer     object,
                         const gchar* property)
{
    gchar* value = NULL;

    g_return_val_if_fail (G_IS_OBJECT (object), NULL);
    /* FIXME: Check value type */

    g_object_get (object, property, &value, NULL);
    return value;
}

/**
 * katze_object_get_object:
 * @object: a #GObject
 * @property: the name of the property to get
 *
 * Retrieve the object value of the specified property.
 *
 * Return value: an object
 **/
gpointer
katze_object_get_object (gpointer     object,
                         const gchar* property)
{
    GObject* value = NULL;

    g_return_val_if_fail (G_IS_OBJECT (object), NULL);
    /* FIXME: Check value type */

    g_object_get (object, property, &value, NULL);
    return value;
}

/**
 * katze_mkdir_with_parents:
 * @pathname: a pathname in the GLib file name encoding
 * @mode: permissions to use for newly created directories
 *
 * Create a directory if it doesn't already exist. Create intermediate
 * parent directories as needed, too.
 *
 * Similar to g_mkdir_with_parents() but returning early if the
 * @pathname refers to an existing directory.
 *
 * Returns: 0 if the directory already exists, or was successfully
 * created. Returns -1 if an error occurred, with errno set.
 *
 * Since: 0.2.1
 */
/* Creating directories recursively
   Copyright 2000 Red Hat, Inc.
   Originally copied from Glib 2.20, coding style adjusted
   Modified to determine file existence early and pathname must be != NULL */
int
katze_mkdir_with_parents (const gchar* pathname,
                          int          mode)
{
  gchar* fn, *p;

  /* Use g_access instead of g_file_test for better performance */
  if (g_access (pathname, F_OK) == 0)
      return 0;

  fn = g_strdup (pathname);

  if (g_path_is_absolute (fn))
    p = (gchar *) g_path_skip_root (fn);
  else
    p = fn;

  do
  {
      while (*p && !G_IS_DIR_SEPARATOR (*p))
          p++;

      if (!*p)
          p = NULL;
      else
          *p = '\0';

      if (g_access (fn, F_OK) != 0)
      {
          if (g_mkdir (fn, mode) == -1)
          {
              g_free (fn);
              return -1;
          }
      }
      else if (!g_file_test (fn, G_FILE_TEST_IS_DIR))
      {
          g_free (fn);
          return -1;
      }
      if (p)
      {
          *p++ = G_DIR_SEPARATOR;
          while (*p && G_IS_DIR_SEPARATOR (*p))
              p++;
      }
  }
  while (p);

  g_free (fn);

  return 0;
}

/**
 * katze_widget_has_touchscreen_mode:
 * @widget: a #GtkWidget, or %NULL
 *
 * Determines whether @widget should operate in touchscreen
 * mode, as determined by GtkSettings or the environment
 * variable MIDORI_TOUCHSCREEN.
 *
 * If @widget is %NULL, the default screen will be used.
 *
 * Returns: %TRUE if touchscreen mode should be used
 *
 * Since: 0.2.1
 */
gboolean
katze_widget_has_touchscreen_mode (GtkWidget* widget)
{
    /*const gchar* touchscreen = g_getenv ("MIDORI_TOUCHSCREEN");
    if (touchscreen && touchscreen[0] == '1')
        return TRUE;
    else if (touchscreen && touchscreen[0] == '0')
        return FALSE;
    else
    {
        GdkScreen* screen = widget && gtk_widget_has_screen (widget)
            ? gtk_widget_get_screen (widget) : gdk_screen_get_default ();
        GtkSettings* gtk_settings = gtk_settings_get_for_screen (screen);
        gboolean enabled;
        g_object_get (gtk_settings, "gtk-touchscreen-mode", &enabled, NULL);
        return enabled;
    }*/
    return TRUE;
}

/**
 * katze_load_cached_icon:
 * @uri: an URI string
 * @widget: a #GtkWidget, or %NULL
 *
 * Loads a cached icon for the specified @uri. If there is no
 * icon and @widget is specified, a default will be returned.
 *
 * Returns: a #GdkPixbuf, or %NULL
 *
 * Since: 0.2.2
 */
GdkPixbuf*
katze_load_cached_icon (const gchar* uri,
                        GtkWidget*   widget)
{
    GdkPixbuf* icon = NULL;

    if (g_str_has_prefix (uri, "http://"))
    {
        guint i;
        gchar* icon_uri;
        gchar* checksum;
        gchar* ext;
        gchar* filename;
        gchar* path;

        i = 8;
        while (uri[i] != '\0' && uri[i] != '/')
            i++;
        if (uri[i] == '/')
        {
            gchar* ticon_uri = g_strdup (uri);
            ticon_uri[i] = '\0';
            icon_uri = g_strdup_printf ("%s/favicon.ico", ticon_uri);
            g_free (ticon_uri);
        }
        else
            icon_uri = g_strdup_printf ("%s/favicon.ico", uri);

        checksum = g_compute_checksum_for_string (G_CHECKSUM_MD5, icon_uri, -1);
        ext = g_strrstr (icon_uri, ".");
        filename = g_strdup_printf ("%s%s", checksum, ext ? ext : "");
        g_free (icon_uri);
        g_free (checksum);
        path = g_build_filename (g_get_user_cache_dir (), PACKAGE_NAME,
                                 "icons", filename, NULL);
        if ((icon = gdk_pixbuf_new_from_file_at_size (path, 16, 16, NULL)))
        {
            g_free (path);
            return icon;
        }
        g_free (path);
    }

    return icon || !widget ? icon : gtk_widget_render_icon (widget,
        GTK_STOCK_FILE, GTK_ICON_SIZE_MENU, NULL);
}

/**
 * katze_collfold:
 * @str: a non-NULL UTF-8 string
 *
 * Computes a string without case and decomposited so
 * it can be used for comparison.
 *
 * Return value: a normalized string
 *
 * Since: 0.2.3
 **/
gchar*
katze_collfold (const gchar* str)
{
    GString* result = g_string_new (NULL);
    const gchar* p = str;

    while (*p)
    {
        gunichar ch = g_unichar_tolower (g_utf8_get_char (p));
        gsize len;
        gunichar* sch = g_unicode_canonical_decomposition (ch, &len);
        guint i = 0;
        while (i < len)
            g_string_append_unichar (result, sch[i++]);

        p = g_utf8_next_char (p);
    }

    return g_string_free (result, FALSE);
}

/**
 * katze_utf8_stristr:
 * @haystack: a non-NULL UTF-8 string
 * @needle: a normalized non-NULL UTF-8 string
 *
 * Determines whether @needle is in @haystack, disregarding
 * differences in case.
 *
 * Return value: %TRUE if @needle is found in @haystack
 *
 * Since: 0.2.3
 **/
gboolean
katze_utf8_stristr (const gchar* haystack,
                    const gchar* needle)
{
    #if 0 /* 0,000159 seconds */
    /* Too slow for use in completion */
    gchar* nhaystack = g_utf8_normalize (haystack, -1, G_NORMALIZE_DEFAULT);
    const gchar *p = nhaystack;
    gsize len = strlen (needle);
    gsize i;

    while (*p)
    {
        for (i = 0; i < len; i++)
            if (g_unichar_tolower (g_utf8_get_char (p + i))
             != g_unichar_tolower (g_utf8_get_char (needle + i)))
                goto next;

        g_free (nhaystack);
        return TRUE;

        next:
            p = g_utf8_next_char (p);
    }

    g_free (nhaystack);
    return FALSE;
    #else /* 0,000044 seconds */
    /* No unicode matching */
    const gchar *p = haystack;
    gsize len = strlen (needle);
    gsize i;

    while (*p)
    {
        for (i = 0; i < len; i++)
            if (g_ascii_tolower (p[i]) != g_ascii_tolower (needle[i]))
                goto next;

        return TRUE;

        next:
            p++;
    }

    return FALSE;
    #endif
}
