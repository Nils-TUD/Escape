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

#ifndef LINES_H_
#define LINES_H_

#include <sys/common.h>

typedef struct {
	char **lines;
	s32 lineCount;
	s32 linePos;
	s32 lineSize;
} sLines;

/**
 * Initializes the given lines-struct
 *
 * @param l the lines
 * @return 0 on success
 */
s32 lines_create(sLines *l);

/**
 * Appends the character to the current line, if possible
 *
 * @param l the lines
 * @param c the character
 */
void lines_append(sLines *l,char c);

/**
 * Adds a new line
 *
 * @param l the lines
 * @return 0 on success
 */
s32 lines_newline(sLines *l);

/**
 * Ends the current line. This is intended for finalizing, i.e. you should call it when everything
 * has been added. It will finish the current line and increase the l->lineCount by 1, so that its
 * really the number of lines and not the index of the current line.
 *
 * @param l the lines
 */
void lines_end(sLines *l);

/**
 * Free's the memory of the given lines
 *
 * @param l the lines
 */
void lines_destroy(sLines *l);

#endif /* LINES_H_ */
