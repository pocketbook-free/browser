/* GAIL - The GNOME Accessibility Implementation Library
 * Copyright 2004 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include "gailcombobox.h"

#if GTK_MINOR_VERSION > 4
#define GAIL_COMBOX_BOX_A11y_COMPLETE
#endif

static void         gail_combo_box_class_init              (GailComboBoxClass *klass);
static void         gail_combo_box_object_init             (GailComboBox      *combo_box);
static void         gail_combo_box_real_initialize         (AtkObject      *obj,
                                                            gpointer       data);

static void         gail_combo_box_changed_gtk             (GtkWidget      *widget);

static G_CONST_RETURN gchar* gail_combo_box_get_name       (AtkObject      *obj);
static gint         gail_combo_box_get_n_children          (AtkObject      *obj);
static AtkObject*   gail_combo_box_ref_child               (AtkObject      *obj,
                                                            gint           i);
static void         gail_combo_box_finalize                (GObject        *object);
#ifdef GAIL_COMBOX_BOX_A11y_COMPLETE
static void         atk_action_interface_init              (AtkActionIface *iface);

static gboolean     gail_combo_box_do_action               (AtkAction      *action,
                                                            gint           i);
static gboolean     idle_do_action                         (gpointer       data);
static gint         gail_combo_box_get_n_actions           (AtkAction      *action)
;
static G_CONST_RETURN gchar* gail_combo_box_get_description(AtkAction      *action,
                                                            gint           i);
static G_CONST_RETURN gchar* gail_combo_box_action_get_name(AtkAction      *action,
                                                            gint           i);
static gboolean              gail_combo_box_set_description(AtkAction      *action,
                                                            gint           i,
                                                            const gchar    *desc);
#endif

static void         atk_selection_interface_init           (AtkSelectionIface *iface);
static gboolean     gail_combo_box_add_selection           (AtkSelection   *selection,
                                                            gint           i);
static gboolean     gail_combo_box_clear_selection         (AtkSelection   *selection);
static AtkObject*   gail_combo_box_ref_selection           (AtkSelection   *selection,
                                                            gint           i);
static gint         gail_combo_box_get_selection_count     (AtkSelection   *selection);
static gboolean     gail_combo_box_is_child_selected       (AtkSelection   *selection,
                                                            gint           i);
static gboolean     gail_combo_box_remove_selection        (AtkSelection   *selection,
                                                            gint           i);

static gpointer parent_class = NULL;

GType
gail_combo_box_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo tinfo =
      {
        sizeof (GailComboBoxClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) gail_combo_box_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        sizeof (GailComboBox), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) gail_combo_box_object_init, /* instance init */
        NULL /* value table */
      };

#ifdef GAIL_COMBOX_BOX_A11y_COMPLETE
      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) atk_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
#endif
      static const GInterfaceInfo atk_selection_info =
      {
        (GInterfaceInitFunc) atk_selection_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      type = g_type_register_static (GAIL_TYPE_CONTAINER,
                                     "GailComboBox", &tinfo, 0);

#ifdef GAIL_COMBOX_BOX_A11y_COMPLETE
      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);
#endif
      g_type_add_interface_static (type, ATK_TYPE_SELECTION,
                                   &atk_selection_info);
    }

  return type;
}

static void
gail_combo_box_class_init (GailComboBoxClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = gail_combo_box_finalize;

  parent_class = g_type_class_peek_parent (klass);

  class->get_name = gail_combo_box_get_name;
  class->get_n_children = gail_combo_box_get_n_children;
  class->ref_child = gail_combo_box_ref_child;
  class->initialize = gail_combo_box_real_initialize;
}

static void
gail_combo_box_object_init (GailComboBox      *combo_box)
{
  combo_box->press_description = NULL;
  combo_box->old_selection = -1;
  combo_box->name = NULL;
  combo_box->popup_set = FALSE;
}

AtkObject* 
gail_combo_box_new (GtkWidget *widget)
{
  GObject *object;
  AtkObject *accessible;

  g_return_val_if_fail (GTK_IS_COMBO_BOX (widget), NULL);

  object = g_object_new (GAIL_TYPE_COMBO_BOX, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

static void
gail_combo_box_real_initialize (AtkObject *obj,
                                gpointer  data)
{
  GtkComboBox *combo_box;
  GailComboBox *gail_combo_box;
#ifdef GAIL_COMBOX_BOX_A11y_COMPLETE
  AtkObject *popup;
#endif

  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  combo_box = GTK_COMBO_BOX (data);

  gail_combo_box = GAIL_COMBO_BOX (obj);

  g_signal_connect (combo_box,
                    "changed",
                    G_CALLBACK (gail_combo_box_changed_gtk),
                    NULL);
  gail_combo_box->old_selection = gtk_combo_box_get_active (combo_box);

#ifdef GAIL_COMBOX_BOX_A11y_COMPLETE
  popup = gtk_combo_box_get_popup_accessible (combo_box);
  if (popup)
    {
      atk_object_set_parent (popup, obj);
      gail_combo_box->popup_set = TRUE;
    }
  if (GTK_IS_COMBO_BOX_ENTRY (combo_box))
    atk_object_set_parent (gtk_widget_get_accessible (gtk_bin_get_child (GTK_BIN (combo_box))), obj);
#endif

  obj->role = ATK_ROLE_COMBO_BOX;
}

static void
gail_combo_box_changed_gtk (GtkWidget *widget)
{
  GtkComboBox *combo_box;
  AtkObject *obj;
  GailComboBox *gail_combo_box;
  gint index;

  combo_box = GTK_COMBO_BOX (widget);

  index = gtk_combo_box_get_active (combo_box);
  obj = gtk_widget_get_accessible (widget);
  gail_combo_box = GAIL_COMBO_BOX (obj);
  if (gail_combo_box->old_selection != index)
    {
      gail_combo_box->old_selection = index;
      g_signal_emit_by_name (obj, "selection_changed");
    }
}

static G_CONST_RETURN gchar* 
gail_combo_box_get_name (AtkObject *obj)
{
  GtkWidget *widget;
  GtkComboBox *combo_box;
  GailComboBox *gail_combo_box;
  GtkTreeIter iter;
  G_CONST_RETURN gchar *name;
  GtkTreeModel *model;
  gint n_columns;
  gint i;

  g_return_val_if_fail (GAIL_IS_COMBO_BOX (obj), NULL);

  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);
  if (name)
    return name;

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  combo_box = GTK_COMBO_BOX (widget);
  gail_combo_box = GAIL_COMBO_BOX (obj);
  if (gtk_combo_box_get_active_iter (combo_box, &iter))
    {
      model = gtk_combo_box_get_model (combo_box);
      n_columns = gtk_tree_model_get_n_columns (model);
      for (i = 0; i < n_columns; i++)
        {
          GValue value = { 0, };

          gtk_tree_model_get_value (model, &iter, i, &value);
          if (G_VALUE_HOLDS_STRING (&value))
            {
	      if (gail_combo_box->name) g_free (gail_combo_box->name);
              gail_combo_box->name =  g_strdup ((gchar *) 
						g_value_get_string (&value));
	      g_value_unset (&value);
              break;
            }
	  else
	    g_value_unset (&value);
        }
    }
  return gail_combo_box->name;
}

/*
 * The children of a GailComboBox are the list of items and the entry field
 * if it is editable.
 */
static gint
gail_combo_box_get_n_children (AtkObject* obj)
{
  gint n_children = 0;
  GtkWidget *widget;

  g_return_val_if_fail (GAIL_IS_COMBO_BOX (obj), 0);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

#ifdef GAIL_COMBOX_BOX_A11y_COMPLETE
  n_children++;
  if (GTK_IS_COMBO_BOX_ENTRY (widget))
    n_children ++;
#endif

  return n_children;
}

static AtkObject*
gail_combo_box_ref_child (AtkObject *obj,
                          gint      i)
{
  GtkWidget *widget;
#ifdef GAIL_COMBOX_BOX_A11y_COMPLETE
  AtkObject *child;
  GailComboBox *box;
#endif

  g_return_val_if_fail (GAIL_IS_COMBO_BOX (obj), NULL);

  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

#ifdef GAIL_COMBOX_BOX_A11y_COMPLETE
  if (i == 0)
    {
      child = gtk_combo_box_get_popup_accessible (GTK_COMBO_BOX (widget));
      box = GAIL_COMBO_BOX (obj);
      if (box->popup_set == FALSE)
        {
          atk_object_set_parent (child, obj);
          box->popup_set = TRUE;
        }
    }
  else if (i == 1 && GTK_IS_COMBO_BOX_ENTRY (widget))
    {
      child = gtk_widget_get_accessible (gtk_bin_get_child (GTK_BIN (widget)));
    }
  else
    {
      return NULL;
    }
  return g_object_ref (child);
#else
  return NULL;
#endif
}

#ifdef GAIL_COMBOX_BOX_A11y_COMPLETE
static void
atk_action_interface_init (AtkActionIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->do_action = gail_combo_box_do_action;
  iface->get_n_actions = gail_combo_box_get_n_actions;
  iface->get_description = gail_combo_box_get_description;
  iface->get_name = gail_combo_box_action_get_name;
  iface->set_description = gail_combo_box_set_description;
}

static gboolean
gail_combo_box_do_action (AtkAction *action,
                          gint      i)
{
  GailComboBox *combo_box;
  GtkWidget *widget;

  widget = GTK_ACCESSIBLE (action)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!GTK_WIDGET_SENSITIVE (widget) || !GTK_WIDGET_VISIBLE (widget))
    return FALSE;

  combo_box = GAIL_COMBO_BOX (action);
  if (i == 0)
    {
      if (combo_box->action_idle_handler)
        return FALSE;

      combo_box->action_idle_handler = g_idle_add (idle_do_action, combo_box);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
idle_do_action (gpointer data)
{
  GtkComboBox *combo_box;
  GtkWidget *widget;
  GailComboBox *gail_combo_box;
  AtkObject *popup;
  gboolean do_popup;

  GDK_THREADS_ENTER ();

  gail_combo_box = GAIL_COMBO_BOX (data);
  gail_combo_box->action_idle_handler = 0;
  widget = GTK_ACCESSIBLE (gail_combo_box)->widget;
  if (widget == NULL || /* State is defunct */
      !GTK_WIDGET_SENSITIVE (widget) || !GTK_WIDGET_VISIBLE (widget))
    {
      GDK_THREADS_LEAVE ();
      return FALSE;
    }

  combo_box = GTK_COMBO_BOX (widget);

  popup = gtk_combo_box_get_popup_accessible (combo_box);
  do_popup = !GTK_WIDGET_MAPPED (GTK_ACCESSIBLE (popup)->widget);
  if (do_popup)
      gtk_combo_box_popup (combo_box);
  else
      gtk_combo_box_popdown (combo_box);

  GDK_THREADS_LEAVE ();

  return FALSE;
}

static gint
gail_combo_box_get_n_actions (AtkAction *action)
{
  /*
   * The default behavior of a combo_box box is to have one action -
   */
  return 1;
}

static G_CONST_RETURN gchar*
gail_combo_box_get_description (AtkAction *action,
                           gint      i)
{
  if (i == 0)
    {
      GailComboBox *combo_box;

      combo_box = GAIL_COMBO_BOX (action);
      return combo_box->press_description;
    }
  else
    return NULL;
}

static G_CONST_RETURN gchar*
gail_combo_box_action_get_name (AtkAction *action,
                                gint      i)
{
  if (i == 0)
    return "press";
  else
    return NULL;
}

static gboolean
gail_combo_box_set_description (AtkAction   *action,
                                gint        i,
                                const gchar *desc)
{
  if (i == 0)
    {
      GailComboBox *combo_box;

      combo_box = GAIL_COMBO_BOX (action);
      g_free (combo_box->press_description);
      combo_box->press_description = g_strdup (desc);
      return TRUE;
    }
  else
    return FALSE;
}
#endif

static void
atk_selection_interface_init (AtkSelectionIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->add_selection = gail_combo_box_add_selection;
  iface->clear_selection = gail_combo_box_clear_selection;
  iface->ref_selection = gail_combo_box_ref_selection;
  iface->get_selection_count = gail_combo_box_get_selection_count;
  iface->is_child_selected = gail_combo_box_is_child_selected;
  iface->remove_selection = gail_combo_box_remove_selection;
  /*
   * select_all_selection does not make sense for a combo_box
   * so no implementation is provided.
   */
}

static gboolean
gail_combo_box_add_selection (AtkSelection *selection,
                              gint         i)
{
  GtkComboBox *combo_box;
  GtkWidget *widget;

  widget = GTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  combo_box = GTK_COMBO_BOX (widget);

  gtk_combo_box_set_active (combo_box, i);
  return TRUE;
}

static gboolean 
gail_combo_box_clear_selection (AtkSelection *selection)
{
  GtkComboBox *combo_box;
  GtkWidget *widget;

  widget = GTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  combo_box = GTK_COMBO_BOX (widget);

  gtk_combo_box_set_active (combo_box, -1);
  return TRUE;
}

static AtkObject*
gail_combo_box_ref_selection (AtkSelection *selection,
                              gint         i)
{
  GtkComboBox *combo_box;
  GtkWidget *widget;
#ifdef GAIL_COMBOX_BOX_A11y_COMPLETE
  AtkObject *obj;
  gint index;
#endif

  widget = GTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  combo_box = GTK_COMBO_BOX (widget);

  /*
   * A combo_box box can have only one selection.
   */
  if (i != 0)
    return NULL;

#ifdef GAIL_COMBOX_BOX_A11y_COMPLETE
  obj = gtk_combo_box_get_popup_accessible (combo_box);
  index = gtk_combo_box_get_active (combo_box);
  return atk_object_ref_accessible_child (obj, index);
#else
  return NULL;
#endif
}

static gint
gail_combo_box_get_selection_count (AtkSelection *selection)
{
  GtkComboBox *combo_box;
  GtkWidget *widget;

  widget = GTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  combo_box = GTK_COMBO_BOX (widget);

  return (gtk_combo_box_get_active (combo_box) == -1) ? 0 : 1;
}

static gboolean
gail_combo_box_is_child_selected (AtkSelection *selection,
                                  gint         i)
{
  GtkComboBox *combo_box;
  gint j;
  GtkWidget *widget;

  widget = GTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  combo_box = GTK_COMBO_BOX (widget);

  j = gtk_combo_box_get_active (combo_box);

  return (j == i);
}

static gboolean
gail_combo_box_remove_selection (AtkSelection *selection,
                                 gint         i)
{
  if (atk_selection_is_child_selected (selection, i))
    atk_selection_clear_selection (selection);

  return TRUE;
}

static void
gail_combo_box_finalize (GObject *object)
{
  GailComboBox *combo_box = GAIL_COMBO_BOX (object);

  g_free (combo_box->press_description);
  g_free (combo_box->name);
  if (combo_box->action_idle_handler)
    {
      g_source_remove (combo_box->action_idle_handler);
      combo_box->action_idle_handler = 0;
    }
  G_OBJECT_CLASS (parent_class)->finalize (object);
}
