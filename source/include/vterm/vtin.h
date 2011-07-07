/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#ifndef VTIN_H_
#define VTIN_H_

#include <esc/common.h>
#include "vtctrl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handles the given keycode with modifiers and character
 *
 * @param vt the vterm
 * @param keycode the keycode
 * @param modifier the modifiers
 * @param c the character
 */
void vterm_handleKey(sVTerm *vt,uchar keycode,uchar modifier,char c);

/**
 * Flushes the readline-buffer
 */
void vterm_rlFlushBuf(sVTerm *vt);

/**
 * Puts the given charactern into the readline-buffer and handles everything necessary
 *
 * @param vt the vterm
 * @param c the character
 */
void vterm_rlPutchar(sVTerm *vt,char c);

#ifdef __cplusplus
}
#endif

#endif /* VTIN_H_ */
