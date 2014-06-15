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
#include <stdio.h>

#define MAX_ARG_COUNT		10

#define MAX_CMDNAME_LEN		30
#define MAX_CMD_LEN			256

#if defined(__cplusplus)
extern "C" {
#endif

extern FILE *curStream;
extern bool curIsStream;
extern const char *curLine;

/**
 * Inits the shell; announces the signal-handler for SIG_INTRPT and creates the environment with
 * given arguments
 *
 * @param argc the number of args
 * @param argv the arguments
 */
void shell_init(int argc,const char **argv);

/**
 * Prints the shell-prompt
 *
 * @return the number of printed characters if successfull
 */
ssize_t shell_prompt(void);

/**
 * Executes the given line or file
 *
 * @param line the entered line
 * @param isFile whether <line> is a file
 * @return the result
 */
int shell_executeCmd(const char *line,bool isFile);

/**
 * Reads a line
 *
 * @param buffer the buffer in which the characters should be put
 * @param max the maximum number of chars
 * @return the number of read chars, < 0 if an error occurred
 */
int shell_readLine(char *buffer,size_t max);

/**
 * Handles the given keycode for the shell
 *
 * @param buffer the buffer with read characters
 * @param keycode the keycode
 * @param modifier the modifier (shift, ctrl, ...)
 * @param cursorPos the current cursor-position in the buffer (may be changed)
 * @param charcount the number of read characters so far (may be changed)
 */
bool shell_handleSpecialKey(char *buffer,int keycode,int modifier,size_t *cursorPos,
		size_t *charcount);

/**
 * Completes the current input, if possible
 *
 * @param line the input
 * @param cursorPos the cursor-position (may be changed)
 * @param length the number of entered characters yet (may be changed)
 */
void shell_complete(char *line,size_t *cursorPos,size_t *length);

#if defined(__cplusplus)
}
#endif
