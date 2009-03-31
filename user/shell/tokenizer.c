/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/heap.h>
#include <string.h>
#include "tokenizer.h"

static bool tok_addCommand(sCmdToken **tokens,u32 *argIndex,u32 *argCount,char **last,char *current,
		eTokenType type);

sCmdToken *tok_get(char *str,u32 *tokenCount) {
	/* allocate memory for the result-array */
	u32 currentArgPos = 0;
	u32 currentArgLen = ARG_BUFFER_SIZE;
	sCmdToken *res = (sCmdToken*)malloc(currentArgLen * sizeof(sCmdToken));
	if(res == NULL)
		return NULL;

	/* init some vars */
	char *c = str;
	u32 foundQuotes = 0;
	char *lastPos = c;
	const char *delims = "\";&| \t\r\n";
	bool finished = 0;

	while(!finished) {
		c = strpbrk(c,delims);

		/* if there is no other delimited we set c to the end of the string to add everything */
		if(c == NULL)
			c = str + strlen(str);

		switch(*c) {
			case ';':
			case '|':
			case '&':
				if(!foundQuotes) {
					/* at first we add all stuff in front of the current char */
					if(!tok_addCommand(&res,&currentArgPos,&currentArgLen,&lastPos,c,TOK_ARGUMENT))
						return NULL;

					/* determine token-type */
					eTokenType type;
					switch(*c) {
						case ';':
							type = TOK_END;
							break;
						case '|':
							type = TOK_PIPE;
							break;
						case '&':
							type = TOK_RUN_IN_BG;
							break;
						default:
							free(res);
							return NULL;
					}
					/* lastPos points behind *c, so decrement it by 1 */
					lastPos--;
					/* use c + 1 because we want to get *c :) */
					if(!tok_addCommand(&res,&currentArgPos,&currentArgLen,&lastPos,c + 1,type))
						return NULL;
					/* to start with the next token directly behind *c, decrement lastPos again (it has been set to c + 2 by addCommand) */
					lastPos--;
				}
				break;

			case '"':
				/* toggle quotes */
				foundQuotes = !foundQuotes;
				/* fall through */

			default:
				/* are we finished? */
				if(*c == '\n' || *c == '\r' || *c == '\0')
					finished = true;

				/* don't break in strings and ensure that we add everything before we're done */
				if(!foundQuotes || *c == '"' || finished) {
					if(!tok_addCommand(&res,&currentArgPos,&currentArgLen,&lastPos,c,TOK_ARGUMENT))
						return NULL;
				}
				break;
		}

		/* go to the next char; don't advance if we've already found the \0 */
		if(*c != '\0')
			c++;
	}

	/* if we've no parts found, free the memory */
	if(currentArgPos == 0)
		free(res);
	/* otherwise correct the size */
	else if(currentArgLen != currentArgPos) {
		res = (sCmdToken*)realloc(res,(currentArgPos + 1) * sizeof(sCmdToken));
		if(res == NULL)
			return NULL;
	}

	*tokenCount = currentArgPos;

	return res;
}

void tok_free(sCmdToken *argv,u32 argc) {
	u32 i;
	for(i = 0;i < argc;i++)
		free(argv[i].str);
	free(argv);
}

void tok_printAll(sCmdToken *token,u32 count) {
	u32 i;
	for(i = 0; i < count; i++)
		tok_print(token++);
}

void tok_print(sCmdToken *token) {
	switch(token->type) {
		case TOK_ARGUMENT:
			printf("%s: '%s'\n","Argument",token->str);
			break;
		case TOK_PIPE:
			printf("%s: '%s'\n","Pipe",token->str);
			break;
		case TOK_END:
			printf("%s: '%s'\n","End",token->str);
			break;
		case TOK_RUN_IN_BG:
			printf("%s: '%s'\n","RunInBG",token->str);
			break;
	}
}

/* little helper to add a command */
static bool tok_addCommand(sCmdToken **tokens,u32 *argIndex,u32 *argCount,char **last,char *current,
		eTokenType type) {
	u32 pos = *argIndex;
	char *lastPos = *last;

	if(current - lastPos > 0) {
		/* no more space? */
		if(pos == *argCount) {
			*argCount *= 2;
			*tokens = (sCmdToken*)realloc(*tokens,*argCount * sizeof(sCmdToken));
			if(*tokens == NULL)
				return false;
		}

		/* copy substring to res */
		(*tokens)[pos].str = (char*)malloc(((current - lastPos) + 1) * sizeof(char));
		if((*tokens)[pos].str == NULL) {
			free(*tokens);
			return false;
		}
		(*tokens)[pos].str = strncpy((*tokens)[pos].str,lastPos,current - lastPos);
		(*tokens)[pos].str[current - lastPos] = '\0';
		(*tokens)[pos].type = type;
		(*argIndex)++;
	}

	/* store current position */
	*last = current + 1;
	return true;
}
