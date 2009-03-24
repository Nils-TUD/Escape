/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
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
} sCommand;

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

#endif /* CMDBUILDER_H_ */
