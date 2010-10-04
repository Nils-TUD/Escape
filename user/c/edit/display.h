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

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <esc/common.h>
#include <esc/sllist.h>

/* the movement-types for mvCurHor */
#define HOR_MOVE_HOME	0
#define HOR_MOVE_END	1
#define HOR_MOVE_LEFT	2
#define HOR_MOVE_RIGHT	3

/**
 * Inits the display with the given line-list
 *
 * @param lineList the line-list from the buffer-module
 */
void displ_init(sFileBuffer *buf);

/**
 * Finishes the display (restores the screen-content)
 */
void displ_finish(void);

/**
 * Retrieves the current cursor-position (in the buffer, not on the screen)
 *
 * @param col will be set to the column
 * @param row will be set to the row
 */
void displ_getCurPos(int *col,int *row);

/**
 * Moves the cursor horizontal. Maybe a vertical movement is done, too
 *
 * @param type the movement-type. See HOR_MOVE_*
 */
void displ_mvCurHor(uint type);

/**
 * Moves the cursor one page up or down
 *
 * @param up move up?
 */
void displ_mvCurVertPage(bool up);

/**
 * Moves the cursor down or up by <lineCount>
 *
 * @param lineCount if positive, move down, otherwise up
 */
void displ_mvCurVert(int lineCount);

/**
 * Marks the given region "dirty"
 *
 * @param start the first row
 * @param count the number of rows
 */
void displ_markDirty(size_t start,size_t count);

/**
 * Updates all dirty regions
 */
void displ_update(void);

/**
 * Lets the user enter the filename. It will be put to <file>.
 *
 * @param file the buffer to write to
 * @param bufSize the size of <file>
 * @return the result of scanl()
 */
int displ_getSaveFile(char *file,size_t bufSize);

/**
 * @param c the char
 * @return the display-length of the given character
 */
size_t displ_getCharLen(char c);

#endif /* DISPLAY_H_ */
