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

#ifndef HISTORY_H_
#define HISTORY_H_

#include <esc/common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Adds the given line to history. It will be assumed that line has been allocated on the heap!
 *
 * @param line the line
 */
void shell_addToHistory(char *line);

/**
 * Moves one step up in the history
 *
 * @return the current history-entry (NULL if no available)
 */
char *shell_histUp(void);

/**
 * Moves one step down in the history
 *
 * @return the current history-entry (NULL if no available)
 */
char *shell_histDown(void);

/**
 * Prints the history
 */
void shell_histPrint(void);

#ifdef __cplusplus
}
#endif

#endif /* HISTORY_H_ */
