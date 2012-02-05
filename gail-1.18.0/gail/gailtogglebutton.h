/* GAIL - The GNOME Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GAIL_TOGGLE_BUTTON_H__
#define __GAIL_TOGGLE_BUTTON_H__

#include <gtk/gtkaccessible.h>
#include <gail/gailbutton.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GAIL_TYPE_TOGGLE_BUTTON              (gail_toggle_button_get_type ())
#define GAIL_TOGGLE_BUTTON(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAIL_TYPE_TOGGLE_BUTTON, GailToggleButton))
#define GAIL_TOGGLE_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GAIL_TYPE_TOGGLE_BUTTON, GailToggleButtonClass))
#define GAIL_IS_TOGGLE_BUTTON(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAIL_TYPE_TOGGLE_BUTTON))
#define GAIL_IS_TOGGLE_BUTTON_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GAIL_TYPE_TOGGLE_BUTTON))
#define GAIL_TOGGLE_BUTTON_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GAIL_TYPE_TOGGLE_BUTTON, GailToggleButtonClass))

typedef struct _GailToggleButton              GailToggleButton;
typedef struct _GailToggleButtonClass         GailToggleButtonClass;

struct _GailToggleButton
{
  GailButton parent;
};

GType gail_toggle_button_get_type (void);

struct _GailToggleButtonClass
{
  GailButtonClass parent_class;
};

AtkObject* gail_toggle_button_new( GtkWidget *widget);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GAIL_TOGGLE_BUTTON_H__ */
