/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <heap.h>
#include <io.h>
#include <bufio.h>

#include "cmdbuilder.h"

static bool cmd_isValid(eTokenType last,eTokenType current,bool isLast);

sCommand *cmd_get(sCmdToken *tokens,u32 tokenCount,u32 *cmdCount) {
	if(tokenCount == 0) {
		*cmdCount = 0;
		return NULL;
	}

	/* init command-array */
	u32 cmdPos = 0;
	u32 cmdLen = 5;
	sCommand *cmds = (sCommand*)malloc(cmdLen * sizeof(sCommand));
	if(cmds == NULL)
		return NULL;

	/* init some vars */
	eTokenType last = TOK_END;
	u32 cmdStart = 0;
	bool runInBG = 0;
	bool lastDup = 0;

	u32 i;
	for(i = 0;i < tokenCount;i++) {
		sCmdToken token = tokens[i];
		bool isLast = i == tokenCount - 1;

		/* check if the token-type is valid here */
		if(!cmd_isValid(last,token.type,isLast)) {
			printf("Invalid command!\n");
			free(cmds);
			*cmdCount = 0;
			return NULL;
		}

		last = token.type;

		/* store wether the command should be run in background */
		if(token.type == TOK_RUN_IN_BG)
			runInBG = true;

		/* command finished? */
		if(isLast || token.type == TOK_PIPE || token.type == TOK_END) {
			u32 argCount = token.type == TOK_ARGUMENT ? i - cmdStart + 1 : i - cmdStart;
			/* we don't want to add runInBG to the arguments */
			if(i > 0 && tokens[i - 1].type == TOK_RUN_IN_BG)
				argCount--;

			/* skip empty commands */
			if(argCount == 0) {
				cmdStart = i + 1;
				continue;
			}

			/* no more space? */
			if(cmdPos == cmdLen) {
				cmdLen *= 2;
				cmds = (sCommand*)realloc(cmds,cmdLen * sizeof(sCommand));
				if(cmds == NULL) {
					/* TODO we have to free the elements */
					return NULL;
				}
			}

			/* set arguments */
			cmds[cmdPos].argCount = argCount;
			cmds[cmdPos].arguments = (s8**)malloc((argCount + 1) * sizeof(s8*));
			if(cmds[cmdPos].arguments == NULL) {
				cmd_free(cmds,cmdPos);
				return NULL;
			}
			u32 x;
			for(x = 0;x < argCount;x++)
				cmds[cmdPos].arguments[x] = tokens[x + cmdStart].str;
			cmds[cmdPos].arguments[x] = (s8*)0;

			/* run in background if we've found RUN_IN_BG and no output-pipe is requested */
			cmds[cmdPos].runInBG = runInBG && token.type != TOK_PIPE;

			/* determine dup-type */
			cmds[cmdPos].dup = DUP_NONE;
			if(lastDup)
				cmds[cmdPos].dup |= DUP_STDIN;
			lastDup = false;
			if(token.type == TOK_PIPE) {
				cmds[cmdPos].dup |= DUP_STDOUT;
				lastDup = true;
			}

			/* set values for the next command */
			cmdStart = i + 1;
			cmdPos++;
			runInBG = false;
		}
	}

	/* no commands found? */
	if(cmdPos == 0) {
		free(cmds);
		*cmdCount = 0;
		return NULL;
	}

	/* correct size */
	if(cmdPos != cmdLen) {
		cmds = (sCommand*)realloc(cmds,(cmdPos + 1) * sizeof(sCommand));
		if(cmds == NULL) {
			/* TODO we have to free the elements */
			return NULL;
		}
	}

	*cmdCount = cmdPos;

	return cmds;
}

void cmd_free(sCommand *cmds,u32 cmdCount) {
	u32 i;
	for(i = 0;i < cmdCount;i++)
		free(cmds[i].arguments);
	free(cmds);
}

void cmd_printAll(sCommand *cmds,u32 count) {
	u32 i;
	for(i = 0; i < count; i++)
		cmd_print(cmds++);
}

void cmd_print(sCommand *cmd) {
	if(cmd == NULL)
		printf("cmd=NULL\n");
	else {
		printf("Arguments (%d): ",cmd->argCount);
		s8 **args = cmd->arguments;
		u32 i = 0;
		while(args[i] != NULL)
			printf("'%s' ",args[i++]);
		printf("\n");
		printf("RunInBG: %d\n",cmd->runInBG);
		printf("Dup: %d\n",cmd->dup);
	}
}

/* small helper to determine wether the given token is valid */
static bool cmd_isValid(eTokenType last,eTokenType current,bool isLast) {
	switch(last) {
		case TOK_PIPE:
		case TOK_END:
			return current == TOK_ARGUMENT;

		case TOK_ARGUMENT:
			if(isLast)
				return current != TOK_PIPE;
			return true;

		case TOK_RUN_IN_BG:
			return current == TOK_END;
	}

	return false;
}
