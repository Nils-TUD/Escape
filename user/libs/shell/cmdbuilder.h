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

#ifndef CMDBUILDER_H_
#define CMDBUILDER_H_

#include "tokenizer.h"

/* the types for command.dup */
#define DUP_NONE 0
#define DUP_STDIN 1
#define DUP_STDOUT 2

/**
 * Represents a command
 */
typedef struct {
	u32 argCount;
	char **arguments;	/* terminated by (char*)0 */
	bool runInBG;		/* 0 if dup != DUP_NONE */
	u8 dup;				/* may be DUP_NONE, DUP_STDIN, DUP_STDOUT or both */
	/* needed by the shell */
	tFD pipe[2];
	s32 pid;
} sCommand;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prints all given commands
 *
 * @param cmds the commands
 * @param count the number of commands
 */
void cmd_printAll(sCommand *cmds,u32 count);

/**
 * Prints the given command
 *
 * @param cmd the command
 */
void cmd_print(sCommand *cmd);

/**
 * Builds commands from the given token-array. If any error occurres the function prints it and returns NULL.
 * The function ensures that all commands are valid (also in combination with each other).
 * Note that you have to call freeCommands() if the return-value is NOT NULL!
 *
 * @param tokens the token-array
 * @param tokenCount the number of tokens
 * @param cmdCount a pointer on the number of found commands (will be set by the function)
 * @return the found commands or NULL
 */
sCommand *cmd_get(sCmdToken *tokens,u32 tokenCount,u32 *cmdCount);

/**
 * Frees the given commands
 *
 * @param cmds the commands
 * @param cmdCount the command-count
 */
void cmd_free(sCommand *cmds,u32 cmdCount);

#ifdef __cplusplus
}
#endif

#endif /* CMDBUILDER_H_ */
