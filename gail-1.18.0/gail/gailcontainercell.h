/* GAIL - The GNOME Accessibility Enabling Library
 * Copyright 2001 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GAIL_CONTAINER_CELL_H__
#define __GAIL_CONTAINER_CELL_H__

#include <atk/atk.h>
#include <gail/gailcell.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GAIL_TYPE_CONTAINER_CELL            (gail_container_cell_get_type ())
#define GAIL_CONTAINER_CELL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAIL_TYPE_CONTAINER_CELL, GailContainerCell))
#define GAIL_CONTAINER_CELL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAIL_TYPE_CONTAINER_CELL, GailContainerCellClass))
#define GAIL_IS_CONTAINER_CELL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAIL_TYPE_CONTAINER_CELL))
#define GAIL_IS_CONTAINER_CELL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAIL_TYPE_CONTAINER_CELL))
#define GAIL_CONTAINER_CELL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GAIL_TYPE_CONTAINER_CELL, GailContainerCellClass))

typedef struct _GailContainerCell                  GailContainerCell;
typedef struct _GailContainerCellClass             GailContainerCellClass;

struct _GailContainerCell
{
  GailCell parent;
  GList *children;
  gint NChildren;
};

GType gail_container_cell_get_type (void);

struct _GailContainerCellClass
{
  GailCellClass parent_class;
};

GailContainerCell *
gail_container_cell_new (void);

void
gail_container_cell_add_child (GailContainerCell *container,
			       GailCell *child);

void
gail_container_cell_remove_child (GailContainerCell *container,
				  GailCell *child);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GAIL_TREE_VIEW_TEXT_CELL_H__ */
