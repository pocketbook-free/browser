/*
 Copyright (C) 2007-2009 Christian Dywan <christian@twotoasts.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
*/

#include <katze/katze.h>

#if HAVE_CONFIG_H
    #include <config.h>
#endif

#include <glib/gstdio.h>
#include <glib/gi18n.h>

#if HAVE_LIBXML
    #include <libxml/parser.h>
    #include <libxml/tree.h>
#endif

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#define katze_str_equal(str1, str2) !strcmp (str1, str2)

static void
katze_xbel_parse_info (KatzeItem* item,
                       xmlNodePtr cur);

static gchar*
katze_item_metadata_to_xbel (KatzeItem* item);

#if HAVE_LIBXML
static KatzeItem*
katze_item_from_xmlNodePtr (xmlNodePtr cur)
{
    KatzeItem* item;

    item = katze_item_new ();
    item->uri = (gchar*)xmlGetProp (cur, (xmlChar*)"href");

    cur = cur->xmlChildrenNode;
    while (cur)
    {
        if (katze_str_equal ((gchar*)cur->name, "title"))
            item->name = g_strstrip ((gchar*)xmlNodeGetContent (cur));
        else if (katze_str_equal ((gchar*)cur->name, "desc"))
            item->text = g_strstrip ((gchar*)xmlNodeGetContent (cur));
        else if (katze_str_equal ((gchar*)cur->name, "info"))
            katze_xbel_parse_info (item, cur);
        cur = cur->next;
    }
    return item;
}

/* Create an array from an xmlNodePtr */
static KatzeArray*
katze_array_from_xmlNodePtr (xmlNodePtr cur)
{
    KatzeArray* array;
    xmlChar* key;
    KatzeItem* item;

    array = katze_array_new (KATZE_TYPE_ARRAY);

    key = xmlGetProp (cur, (xmlChar*)"folded");
    if (key)
    {
        /* if (!g_ascii_strncasecmp ((gchar*)key, "yes", 3))
            folder->folded = TRUE;
        else if (!g_ascii_strncasecmp ((gchar*)key, "no", 2))
            folder->folded = FALSE;
        else
            g_warning ("XBEL: Unknown value for folded."); */
        xmlFree (key);
    }

    cur = cur->xmlChildrenNode;
    while (cur)
    {
        if (katze_str_equal ((gchar*)cur->name, "title"))
            ((KatzeItem*)array)->name = g_strstrip ((gchar*)xmlNodeGetContent (cur));
        else if (katze_str_equal ((gchar*)cur->name, "desc"))
            ((KatzeItem*)array)->text = g_strstrip ((gchar*)xmlNodeGetContent (cur));
        else if (katze_str_equal ((gchar*)cur->name, "info"))
            katze_xbel_parse_info ((KatzeItem*)array, cur);
        else if (katze_str_equal ((gchar*)cur->name, "folder"))
        {
            item = (KatzeItem*)katze_array_from_xmlNodePtr (cur);
            katze_array_add_item (array, item);
        }
        else if (katze_str_equal ((gchar*)cur->name, "bookmark"))
        {
            item = katze_item_from_xmlNodePtr (cur);
            katze_array_add_item (array, item);
        }
        else if (katze_str_equal ((gchar*)cur->name, "separator"))
        {
            item = katze_item_new ();
            katze_array_add_item (array, item);
        }
        cur = cur->next;
    }
    return array;
}

static void
katze_xbel_parse_info (KatzeItem* item,
                       xmlNodePtr cur)
{
    cur = cur->xmlChildrenNode;
    while (cur)
    {
        if (katze_str_equal ((gchar*)cur->name, "metadata"))
        {
            xmlChar* owner = xmlGetProp (cur, (xmlChar*)"owner");
            if (owner)
                g_strstrip ((gchar*)owner);
            else
                /* Albeit required, "owner" is not set by MicroB */
                owner = (xmlChar*)NULL;
            /* FIXME: Save metadata from unknown owners */
            if (!owner || katze_str_equal ((gchar*)owner, "http://www.twotoasts.de"))
            {
                xmlAttrPtr properties = cur->properties;
                xmlNodePtr children = cur->children;
                while (properties)
                {
                    xmlChar* value;

                    if (katze_str_equal ((gchar*)properties->name, "owner"))
                    {
                        properties = properties->next;
                        continue;
                    }
                    value = xmlGetProp (cur, properties->name);
                    if (properties->ns && properties->ns->prefix)
                    {
                        gchar* ns_value = g_strdup_printf ("%s:%s",
                            properties->ns->prefix, properties->name);
                        katze_item_set_meta_string (item,
                            (gchar*)ns_value, (gchar*)value);
                        g_free (ns_value);
                    }
                    else
                        katze_item_set_meta_string (item,
                            (gchar*)properties->name, (gchar*)value);
                    xmlFree (value);
                    properties = properties->next;
                }
                while (children)
                {
                    xmlNodePtr grand_children = children->children;
                    while (grand_children)
                    {
                        xmlChar* value = grand_children->content;
                        gchar* ns_value;
                        if (!owner)
                            ns_value = g_strdup_printf (":%s", children->name);
                        else if (katze_str_equal ((gchar*)owner, "http://www.twotoasts.de"))
                            ns_value = g_strdup_printf ("midori:%s", children->name);
                        else /* FIXME: Save metadata from unknown owners */
                            ns_value = g_strdup_printf (":%s", children->name);
                        katze_item_set_meta_string (item, ns_value, (gchar*)value);
                        g_free (ns_value);
                        grand_children = grand_children->next;
                    }

                    children = children->next;
                }
            }
            xmlFree (owner);
        }
        else if (!katze_str_equal ((gchar*)cur->name, "text"))
            g_critical ("Unexpected element <%s> in <metadata>.", cur->name);
        cur = cur->next;
    }
}

/* Loads the contents from an xmlNodePtr into an array. */
static gboolean
katze_array_from_xmlDocPtr (KatzeArray* array,
                            xmlDocPtr   doc)
{
    xmlNodePtr cur;
    KatzeItem* item;

    cur = xmlDocGetRootElement (doc);

    if ((cur = xmlDocGetRootElement (doc)) == NULL)
    {
        /* Empty document */
        return FALSE;
    }
    if (katze_str_equal ((gchar*)cur->name, "xbel"))
    {
        /* XBEL 1.0 */
        gchar* value;

        value = (gchar*)xmlGetProp (cur, (xmlChar*)"version");
        if (!katze_str_equal (value, "1.0"))
            g_warning ("XBEL version is not 1.0.");
        g_free (value);

        value = (gchar*)xmlGetProp (cur, (xmlChar*)"title");
        katze_item_set_name (KATZE_ITEM (array), value);
        g_free (value);

        value = (gchar*)xmlGetProp (cur, (xmlChar*)"desc");
        katze_item_set_text (KATZE_ITEM (array), value);
        g_free (value);
    }
    else if (katze_str_equal ((gchar*)cur->name, "RDF"))
    {
        /* Minimal RSS 1.0 support, enough to read bookmarks */
        cur = cur->xmlChildrenNode;
        while (cur)
        {
            item = NULL;
            if (katze_str_equal ((gchar*)cur->name, "item"))
            {
                xmlNodePtr cur_item;

                item = katze_item_new ();

                cur_item = cur->xmlChildrenNode;
                while (cur_item)
                {
                    if (katze_str_equal ((gchar*)cur_item->name, "title"))
                        item->name = g_strstrip ((gchar*)xmlNodeGetContent (cur_item));
                    else if (katze_str_equal ((gchar*)cur_item->name, "link"))
                        item->uri = g_strstrip ((gchar*)xmlNodeGetContent (cur_item));
                    else if (katze_str_equal ((gchar*)cur_item->name, "subject"))
                    {
                        gchar* value = g_strstrip ((gchar*)xmlNodeGetContent (cur_item));
                        /* FIXME: Create a folder according to the tag */
                        g_free (value);
                    }
                    cur_item = cur_item->next;
                }
            }
            else if (katze_str_equal ((gchar*)cur->name, "channel"))
                /* Ignored */;
            else if (!katze_str_equal ((gchar*)cur->name, "text"))
                g_warning ("Unexpected attribute: %s", (gchar*)cur->name);
            if (item)
                katze_array_add_item (array, item);
            cur = cur->next;
        }
        return TRUE;
    }
    else
    {
        /* Wrong document kind */
        return FALSE;
    }

    cur = cur->xmlChildrenNode;
    while (cur)
    {
        item = NULL;
        if (katze_str_equal ((gchar*)cur->name, "folder"))
            item = (KatzeItem*)katze_array_from_xmlNodePtr (cur);
        else if (katze_str_equal ((gchar*)cur->name, "bookmark"))
            item = katze_item_from_xmlNodePtr (cur);
        else if (katze_str_equal ((gchar*)cur->name, "separator"))
            item = katze_item_new ();
        else if (katze_str_equal ((gchar*)cur->name, "info"))
            katze_xbel_parse_info (KATZE_ITEM (array), cur);
        else if (katze_str_equal ((gchar*)cur->name, "title"))
        {
            xmlNodePtr node = cur->xmlChildrenNode;
            katze_item_set_name (KATZE_ITEM (array), (gchar*)node->content);
        }
        else if (katze_str_equal ((gchar*)cur->name, "desc"))
        {
            xmlNodePtr node = cur->xmlChildrenNode;
            katze_item_set_text (KATZE_ITEM (array), (gchar*)node->content);
        }
        else if (!katze_str_equal ((gchar*)cur->name, "text"))
            g_warning ("Unexpected attribute: %s", (gchar*)cur->name);
        if (item)
            katze_array_add_item (array, item);
        cur = cur->next;
    }
    return TRUE;
}

static gboolean
katze_array_from_opera_file (KatzeArray* array,
                             FILE*       file)
{
    gchar line[200];
    gchar* partial_line = NULL;
    KatzeArray* folder = array;
    KatzeItem* item = NULL;

    while (fgets (line, 200, file))
    {
        gboolean incomplete_line = (strlen (line) == 199);
        g_strstrip (line);
        if (line[0] == '\0')
        {
            item = NULL;
            continue;
        }
        else if (line[0] == '-')
        {
            item = NULL;
            if (folder != array)
                folder = katze_item_get_parent ((KatzeItem*)folder);
            else
                g_warning ("A level-up although we are at the top level");
            continue;
        }

        if (line[0] == '#')
        {
            const gchar* element = &line[1];
            if (!g_ascii_strncasecmp (element, "FOLDER", 6))
            {
                item = (KatzeItem*)katze_array_new (KATZE_TYPE_ARRAY);
                katze_array_add_item (folder, item);
                folder = (KatzeArray*)item;
            }
            else if (!g_ascii_strncasecmp (element, "URL", 3))
            {
                item = katze_item_new ();
                katze_array_add_item (folder, item);
            }
            else
                g_warning ("Unexpected element: %s", element);
        }
        else if (item)
        {
            gchar** parts;

            /* Handle lines longer than 200 characters */
            if (incomplete_line)
            {
                if (partial_line)
                {
                    gchar* chunk = g_strconcat (partial_line, line, NULL);
                    katze_assign (partial_line, chunk);
                }
                else
                    partial_line = g_strdup (line);
                continue;
            }

            if (partial_line)
            {
                gchar* full_line = g_strconcat (partial_line, line, NULL);
                katze_assign (partial_line, NULL);
                parts = g_strsplit (full_line, "=", 2);
                g_free (full_line);
            }
            else
                parts = g_strsplit (line, "=", 2);

            if (parts && parts[0] && parts[1])
            {
                if (katze_str_equal (parts[0], "NAME"))
                    item->name = g_strdup (parts[1]);
                else if (katze_str_equal (parts[0], "URL"))
                    item->uri = g_strdup (parts[1]);
                else if (katze_str_equal (parts[0], "DESCRIPTION"))
                    item->text = g_strdup (parts[1]);
                else if (katze_str_equal (parts[0], "CREATED"))
                    item->added = g_ascii_strtoull (parts[1], NULL, 10);
                /* FIXME: Implement visited time
                else if (katze_str_equal (parts[0], "VISITED"))
                    item->visited = g_ascii_strtoull (parts[1], NULL, 10); */
                else if (katze_str_equal (parts[0], "ON PERSONALBAR"))
                    katze_item_set_meta_integer (item, "toolbar",
                        katze_str_equal (parts[1], "YES") ? 1 : -1);
                /* FIXME: Implement websites as panels
                else if (katze_str_equal (parts[0], "IN PANEL"))
                    ; */
            }
            else
                g_warning ("Broken property: %s", line);
            g_strfreev (parts);
        }
        else
            g_warning ("Unexpected property outside of element: %s", line);
    }
    return TRUE;
}

/**
 * midori_array_from_file:
 * @array: a #KatzeArray
 * @filename: a filename to load from
 * @format: "xbel", "opera", or %NULL
 * @error: a #GError or %NULL
 *
 * Loads the contents of a file in the specified format.
 *
 * Since 0.2.2 @format can be %NULL to indicate that the
 *   file should be loaded if it's any supported format.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.1.6
 **/
gboolean
midori_array_from_file (KatzeArray*  array,
                        const gchar* filename,
                        const gchar* format,
                        GError**     error)
{
    g_return_val_if_fail (katze_array_is_a (array, KATZE_TYPE_ITEM), FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);
    g_return_val_if_fail (!error || !*error, FALSE);

    if (g_access (filename, F_OK) != 0)
    {
        /* File doesn't exist */
        if (error)
            *error = g_error_new_literal (G_FILE_ERROR, G_FILE_ERROR_NOENT,
                                          _("File not found."));
        return FALSE;
    }

    if (!format)
        format = "";

    /* Opera6 */
    if (katze_str_equal (format, "opera")
    || (!*format && g_str_has_suffix (filename, ".adr")))
    {
        FILE* file;
        if ((file = g_fopen (filename, "r")))
        {
            guint verify;
            gchar line[50];

            verify = 0;
            while (fgets (line, 50, file))
            {
                g_strstrip (line);
                if (verify == 0 && katze_str_equal (line, "Opera Hotlist version 2.0"))
                    verify++;
                else if (verify == 1
                    && katze_str_equal (line, "Options: encoding = utf8, version=3"))
                    verify++;
                else if (verify == 2)
                {
                    if (!katze_array_from_opera_file (array, file))
                    {
                        /* Parsing failed */
                        fclose (file);
                        if (error)
                            *error = g_error_new_literal (G_FILE_ERROR,
                                G_FILE_ERROR_FAILED, _("Malformed document."));
                        return FALSE;
                    }
                    return TRUE;
                }
                else
                    break;
            }
            fclose (file);
        }
    }

    /* XBEL */
    if (katze_str_equal (format, "xbel")
     || !*format)
    {
        xmlDocPtr doc;

        if ((doc = xmlParseFile (filename)) == NULL)
        {
            /* No valid xml or broken encoding */
            if (error)
                *error = g_error_new_literal (G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                              _("Malformed document."));
            return FALSE;
        }
        if (!katze_array_from_xmlDocPtr (array, doc))
        {
            /* Parsing failed */
            xmlFreeDoc (doc);
            if (error)
                *error = g_error_new_literal (G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                              _("Malformed document."));
            return FALSE;
        }
        xmlFreeDoc (doc);
        return TRUE;
    }

    if (error)
        *error = g_error_new_literal (G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                      _("Unrecognized bookmark format."));
    return FALSE;
}
#endif

/* Inspired by append_escaped_text() from gmarkup.c in Glib.
   The main difference is that we filter out control characters. */
static void
string_append_escaped (GString     *str,
                       const gchar *text)
{
    gssize length;
    const gchar* p;
    const gchar* end;
    gunichar c;

    length = strlen (text);
    p = text;
    end = text + length;

    while (p != end)
    {
        const gchar *next;
        next = g_utf8_next_char (p);

        switch (*p)
        {
        case '&':
            g_string_append (str, "&amp;");
            break;
        case '<':
            g_string_append (str, "&lt;");
            break;
        case '>':
            g_string_append (str, "&gt;");
            break;
        case '\'':
            g_string_append (str, "&apos;");
            break;
        case '"':
            g_string_append (str, "&quot;");
            break;
        default:
            c = g_utf8_get_char (p);
            if (g_unichar_iscntrl (c))
                g_string_append_c (str, ' ');
            else if ((0x1 <= c && c <= 0x8)
             || (0xb <= c && c  <= 0xc)
             || (0xe <= c && c <= 0x1f)
             || (0x7f <= c && c <= 0x84)
             || (0x86 <= c && c <= 0x9f))
                g_string_append_printf (str, "&#x%x;", c);
            else
                g_string_append_len (str, p, next - p);
            break;
        }

        p = next;
    }
}

static void
string_append_xml_element (GString*     string,
                           const gchar* name,
                           const gchar* value)
{
    if (value)
    {
        g_string_append_printf (string, "<%s>", name);
        string_append_escaped (string, value);
        g_string_append_printf (string, "</%s>\n", name);
    }
}

static void
string_append_item (GString*   string,
                    KatzeItem* item)
{
    gchar* markup;
    gchar* metadata;

    g_return_if_fail (KATZE_IS_ITEM (item));

    markup = NULL;
    metadata = katze_item_metadata_to_xbel (item);
    if (KATZE_IS_ARRAY (item))
    {
        guint i = 0;
        KatzeItem* _item;
        KatzeArray* array = KATZE_ARRAY (item);

        g_string_append (string, "<folder>\n");
        /* FIXME: " folded=\"no\" */
        string_append_xml_element (string, "title", katze_item_get_name (item));
        string_append_xml_element (string, "desc", katze_item_get_text (item));
        while ((_item = katze_array_get_nth_item (array, i++)))
            string_append_item (string, _item);
        g_string_append (string, metadata);
        g_string_append (string, "</folder>\n");
    }
    else if (katze_item_get_uri (item))
    {
        g_string_append (string, "<bookmark href=\"");
        string_append_escaped (string, katze_item_get_uri (item));
        g_string_append (string, "\">\n");
        string_append_xml_element (string, "title", katze_item_get_name (item));
        string_append_xml_element (string, "desc", katze_item_get_text (item));
        g_string_append (string, metadata);
        g_string_append (string, "</bookmark>\n");
    }
    else
        g_string_append (string, "<separator/>\n");
    g_free (metadata);
}

static gchar*
katze_item_metadata_to_xbel (KatzeItem* item)
{
    GList* keys = katze_item_get_meta_keys (item);
    GString* markup;
    GString* markdown;
    /* FIXME: Allow specifying an alternative default namespace */
    /* FIXME: Support foreign namespaces with their own URI */
    gchar* namespace = NULL;
    const gchar* namespace_uri;
    gsize i;
    const gchar* key;
    const gchar* value;

    if (!keys)
        return g_strdup ("");

    markup = g_string_new ("<info>\n<metadata");
    markdown = g_string_new (NULL);
    i = 0;
    while ((key = g_list_nth_data (keys, i++)))
        if ((value = katze_item_get_meta_string (item, key)))
        {
            namespace = strchr (key, ':');
            if (key[0] == ':') /* MicroB uses un-namespaced children */
            {
                key = &key[1];
                g_string_append_printf (markdown, "<%s>", key);
                string_append_escaped (markdown, value);
                g_string_append_printf (markdown, "</%s>\n", key);
            }
            else if (namespace)
            {
                g_string_append_printf (markup, " %s=\"", key);
                string_append_escaped (markup, value);
                g_string_append_c (markup, '\"');
            }
            else
            {
                g_string_append_printf (markup, " midori:%s=\"", key);
                string_append_escaped (markup, value);
                g_string_append_c (markup, '\"');
            }
        }
    if (!namespace)
    {
        namespace_uri = "http://www.twotoasts.de";
        g_string_append_printf (markup, " owner=\"%s\"", namespace_uri);
    }
    if (markdown->len)
        g_string_append_printf (markup, ">\n%s</metadata>\n</info>\n", markdown->str);
    else
        g_string_append_printf (markup, "/>\n</info>\n");
    g_string_free (markdown, TRUE);
    return g_string_free (markup, FALSE);
}

static gchar*
katze_array_to_xbel (KatzeArray* array,
                     GError**    error)
{
    gchar* metadata = katze_item_metadata_to_xbel (KATZE_ITEM (array));
    guint i;
    KatzeItem* item;

    GString* markup = g_string_new (
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE xbel PUBLIC \"+//IDN python.org//DTD "
        "XML Bookmark Exchange Language 1.0//EN//XML\" "
        "\"http://www.python.org/topics/xml/dtds/xbel-1.0.dtd\">\n"
        "<xbel version=\"1.0\""
        " xmlns:midori=\"http://www.twotoasts.de\""
        ">\n");
    string_append_xml_element (markup, "title", katze_item_get_name (KATZE_ITEM (array)));
    string_append_xml_element (markup, "desc", katze_item_get_text (KATZE_ITEM (array)));
    g_string_append (markup, metadata);
    i = 0;
    while ((item = katze_array_get_nth_item (array, i++)))
        string_append_item (markup, item);
    g_string_append (markup, "</xbel>\n");

    g_free (metadata);

    return g_string_free (markup, FALSE);
}

static gboolean
midori_array_to_file_xbel (KatzeArray*  array,
                           const gchar* filename,
                           GError**     error)
{
    gchar* data;
    FILE* fp;

    if (!(data = katze_array_to_xbel (array, error)))
        return FALSE;
    if (!(fp = fopen (filename, "w")))
    {
        *error = g_error_new_literal (G_FILE_ERROR, G_FILE_ERROR_ACCES,
                                      _("Writing failed."));
        return FALSE;
    }
    fputs (data, fp);
    fclose (fp);
    g_free (data);
    return TRUE;
}

/**
 * midori_array_to_file:
 * @array: a #KatzeArray
 * @filename: a filename to load from
 * @format: the desired format
 * @error: a #GError or %NULL
 *
 * Saves the contents to a file in the specified format.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.1.6
 **/
gboolean
midori_array_to_file (KatzeArray*  array,
                      const gchar* filename,
                      const gchar* format,
                      GError**     error)
{
    g_return_val_if_fail (katze_array_is_a (array, KATZE_TYPE_ITEM), FALSE);
    g_return_val_if_fail (filename, FALSE);
    g_return_val_if_fail (!error || !*error, FALSE);

    if (!g_strcmp0 (format, "xbel"))
        return midori_array_to_file_xbel (array, filename, error);
    g_critical ("Cannot write KatzeArray to unknown format '%s'.", format);
    return FALSE;
}
