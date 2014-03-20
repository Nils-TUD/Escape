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
 * Reads <count> characters from the given vterm into the given buffer.
 *
 * @param vt the vterm
 * @param buffer the buffer to write to (may be NULL)
 * @param count the number of chars to read
 * @param avail will be set to true if there is more input available
 * @return the number of read characters
 */
size_t vtin_gets(sVTerm *vt,char *buffer,size_t count,int *avail);

/**
 * Puts the given charactern into the readline-buffer and handles everything necessary (unlocked)
 *
 * @param vt the vterm
 * @param c the character
 */
void vtin_rlPutchar(sVTerm *vt,char c);
