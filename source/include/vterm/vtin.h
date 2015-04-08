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

#include <sys/common.h>

#include "vtctrl.h"

/**
 * Handles the given keycode with modifiers and character
 *
 * @param vt the vterm
 * @param keycode the keycode
 * @param modifier the modifiers
 * @param c the character
 */
void vtin_handleKey(sVTerm *vt,uchar keycode,uchar modifier,char c);

/**
 * Sets the mouse cursor to give position.
 *
 * @param vt the vterm
 * @param x the x-position (on the screen in columns)
 * @param y the y-position (on the screen in rows)
 * @param z the movement in z-direction (in rows)
 * @param select whether the selection should be changed
 */
void vtin_handleMouse(sVTerm *vt,size_t x,size_t y,int z,bool select);

/**
 * Flips the fore- and background color at given position.
 *
 * @param vt the vterm
 * @param x the x-position in the buffer
 * @param y the y-position in the buffer
 */
void vtin_changeColor(sVTerm *vt,int x,int y);

/**
 * Flips the fore- and background color of given range.
 *
 * @param vt the vterm
 * @param start the start-position
 * @param end the end-position
 */
void vtin_changeColorRange(sVTerm *vt,size_t start,size_t end);

/**
 * Removes the cursor, if necessary
 *
 * @param vt the vterm
 */
void vtin_removeCursor(sVTerm *vt);

/**
 * Removes the selection, if necessary
 *
 * @param vt the vterm
 */
void vtin_removeSelection(sVTerm *vt);

/**
 * @param vt the vterm
 * @return true if there is data to read
 */
bool vtin_hasData(sVTerm *vt);

/**
 * Reads <count> characters from the given vterm into the given buffer.
 *
 * @param vt the vterm
 * @param buffer the buffer to write to (may be NULL)
 * @param count the number of chars to read
 * @return the number of read characters
 */
size_t vtin_gets(sVTerm *vt,char *buffer,size_t count);

/**
 * Puts the given charactern into the readline-buffer and handles everything necessary (unlocked)
 *
 * @param vt the vterm
 * @param c the character
 */
void vtin_rlPutchar(sVTerm *vt,char c);
