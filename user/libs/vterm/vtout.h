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

#ifndef VTOUT_H_
#define VTOUT_H_

#include <esc/common.h>
#include "vtctrl.h"

/**
 * Prints the given string
 *
 * @param vt the vterm
 * @param str the string
 * @param len the string-length
 * @param resetRead wether readline-stuff should be reset
 */
void vterm_puts(sVTerm *vt,char *str,u32 len,bool resetRead);

/**
 * Prints the given character to screen
 *
 * @param vt the vterm
 * @param c the character
 */
void vterm_putchar(sVTerm *vt,char c);

#endif /* VTOUT_H_ */
