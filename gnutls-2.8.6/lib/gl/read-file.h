/* read-file.h -- read file contents into a string
   Copyright (C) 2006 Free Software Foundation, Inc.
   Written by Simon Josefsson.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef READ_FILE_H
#define READ_FILE_H

/* Get size_t.  */
#include <stddef.h>

/* Get FILE.  */
#include <stdio.h>

extern char *fread_file (FILE * stream, size_t * length);

extern char *read_file (const char *filename, size_t * length);

extern char *read_binary_file (const char *filename, size_t * length);

#endif /* READ_FILE_H */
