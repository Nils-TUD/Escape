/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include <esc/common.h>
#include <sllist.h>

typedef struct {
	sSLList *lines;
	char *filename;
	bool modified;
} sFileBuffer;

/* one line in our linked lists of lines */
typedef struct {
	char *str;
	u32 size;		/* size of <str> */
	u32 length;		/* length of the line */
	u32 displLen;	/* length for displaying the line (tabs expanded,...) */
} sLine;

/**
 * Initializes the buffer with the given file
 *
 * @param file the filename (maybe NULL to create a new one)
 */
void buf_open(const char *file);

/**
 * @return the number of lines
 */
u32 buf_getLineCount(void);

/**
 * @return the buffer
 */
sFileBuffer *buf_get(void);

/**
 * Inserts the given character at the given position
 *
 * @param col the column
 * @param row the row
 * @param c the character
 */
void buf_insertAt(s32 col,s32 row,char c);

/**
 * Creates a new line behind the given row
 *
 * @param row the row
 */
void buf_newLine(s32 row);

/**
 * Removes the current char
 *
 * @param col the column
 * @param row the row
 */
void buf_removeCur(s32 col,s32 row);

/**
 * Stores the buffer to the given file
 *
 * @param file the file
 */
void buf_store(const char *file);

#endif /* BUFFER_H_ */
