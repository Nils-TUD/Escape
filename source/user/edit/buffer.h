/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#pragma once

#include <esc/common.h>
#include <esc/sllist.h>

typedef struct {
	sSLList *lines;
	char *filename;
	bool modified;
} sFileBuffer;

/* one line in our linked lists of lines */
typedef struct {
	char *str;
	size_t size;		/* size of <str> */
	size_t length;		/* length of the line */
	size_t displLen;	/* length for displaying the line (tabs expanded,...) */
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
size_t buf_getLineCount(void);

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
void buf_insertAt(int col,int row,char c);

/**
 * Creates a new line behind the given row and moves all stuff behind <col> from the current
 * one to the next.
 *
 * @param col the column
 * @param row the row
 */
void buf_newLine(int col,int row);

/**
 * Moves the line <row> to the end of the previous one
 *
 * @param row the current row
 */
void buf_moveToPrevLine(int row);

/**
 * Removes the current char
 *
 * @param col the column
 * @param row the row
 */
void buf_removeCur(int col,int row);

/**
 * Stores the buffer to the given file
 *
 * @param file the file
 */
void buf_store(const char *file);
