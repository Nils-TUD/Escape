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

#ifndef SHELL_H_
#define SHELL_H_

#include <esc/common.h>

#define MAX_ARG_COUNT		10
#define ERR_CMD_NOT_FOUND	-100

#define MAX_CMDNAME_LEN		30
#define MAX_CMD_LEN			70

#define STATE_SHIFT			(1 << 0)
#define STATE_CTRL			(1 << 1)
#define STATE_ALT			(1 << 2)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prints the shell-prompt
 *
 * @return true if successfull
 */
bool shell_prompt(void);

/**
 * @return the pid we're waiting for or INVALID_PID
 */
tPid shell_getWaitingPid(void);

/**
 * Executes the given line
 *
 * @param line the entered line
 * @return the result
 */
s32 shell_executeCmd(char *line);

/**
 * Reads a line
 *
 * @param buffer the buffer in which the characters should be put
 * @param max the maximum number of chars
 * @return the number of read chars
 */
u32 shell_readLine(char *buffer,u32 max);

/**
 * Handles the escape-codes for the shell
 *
 * @param buffer the buffer with read characters
 * @param c the read character
 * @param cursorPos the current cursor-position in the buffer (may be changed)
 * @param charcount the number of read characters so far (may be changed)
 * @return true if the escape-code was handled
 */
bool shell_handleEscapeCodes(char *buffer,char c,u32 *cursorPos,u32 *charcount);

/**
 * Completes the current input, if possible
 *
 * @param line the input
 * @param cursorPos the cursor-position (may be changed)
 * @param length the number of entered characters yet (may be changed)
 */
void shell_complete(char *line,u32 *cursorPos,u32 *length);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_H_ */
