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

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <sys/common.h>
#include <sys/dbg/lines.h>
#include <sys/video.h>

/* to make a screen-backup */
typedef struct {
	char screen[VID_COLS * VID_ROWS * 2];
	ushort row;
	ushort col;
} sScreenBackup;

/**
 * Starts the debugging-console
 */
void cons_start(void);

/**
 * Enables/disables writing to log
 *
 * @param enabled the new value
 */
void cons_setLogEnabled(bool enabled);

/**
 * Displays the given lines and provides a navigation through them
 *
 * @param l the lines
 */
void cons_viewLines(const sLines *l);

#endif /* CONSOLE_H_ */
