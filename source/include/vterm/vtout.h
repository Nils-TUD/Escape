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
 * Prints the given string
 *
 * @param vt the vterm
 * @param str the string
 * @param len the string-length
 * @param resetRead whether readline-stuff should be reset
 */
void vtout_puts(sVTerm *vt,char *str,size_t len,bool resetRead);

/**
 * Prints the given character to screen (unlocked)
 *
 * @param vt the vterm
 * @param c the character
 */
void vtout_putchar(sVTerm *vt,char c);
