/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <proc.h>
#include <io.h>
#include <bufio.h>
#include <dir.h>
#include <string.h>
#include <service.h>
#include <mem.h>
#include <heap.h>
#include <keycodes.h>
#include <env.h>

#include "history.h"
#include "completion.h"
#include "tokenizer.h"
#include "cmdbuilder.h"

#define MAX_ARG_COUNT		10

#define ERR_CMD_NOT_FOUND	-100

/**
 * Prints the shell-prompt
 *
 * @return true if successfull
 */
static bool shell_prompt(void);

/**
 * Reads a line
 *
 * @param buffer the buffer in which the characters should be put
 * @param max the maximum number of chars
 * @return the number of read chars
 */
static u16 shell_readLine(s8 *buffer,u16 max);

/**
 * Handles the default-escape-codes

 * @param line the buffer with read characters
 * @param cursorPos the current cursor-position in the buffer (may be changed)
 * @param charcount the number of read characters so far (may be changed)
 * @param c the character
 * @param keycode the keycode of the escape-code
 * @param modifier the modifier of the escape-code
 * @return true if the escape-code was handled
 */
static bool handleDefEscapes(s8 *line,u32 *cursorPos,u32 *charcount,s8 c,u8 *keycode,u8 *modifier);

/**
 * Handles the escape-codes for the shell
 *
 * @param buffer the buffer with read characters
 * @param cursorPos the current cursor-position in the buffer (may be changed)
 * @param charcount the number of read characters so far (may be changed)
 * @param keycode the keycode of the escape-code
 * @param modifier the modifier of the escape-code
 * @return true if the escape-code was handled
 */
static bool shell_handleEscapeCodes(s8 *buffer,u16 *cursorPos,u16 *charcount,u8 keycode,u8 modifier);

/**
 * Completes the current input, if possible
 *
 * @param line the input
 * @param cursorPos the cursor-position (may be changed)
 * @param length the number of entered characters yet (may be changed)
 */
static void shell_complete(s8 *line,u16 *cursorPos,u16 *length);

/**
 * Executes the given line
 *
 * @param line the entered line
 * @return the result
 */
static s32 shell_executeCmd(s8 *line);

/* buffer for arguments */
static u32 tabCount = 0;
static s8 *args[MAX_ARG_COUNT];

s32 main(u32 argc,s8 **argv) {
	s8 *buffer;

	printf("\033f\011Welcome to Escape v0.1!\033r\011\n");
	printf("\n");
	printf("Try 'help' to see the current features :)\n");
	printf("\n");

	while(1) {
		/* create buffer (history will free it) */
		buffer = malloc((MAX_CMD_LEN + 1) * sizeof(s8));
		if(buffer == NULL) {
			printf("Not enough memory\n");
			return 1;
		}

		if(!shell_prompt())
			return 1;

		/* read command */
		shell_readLine(buffer,MAX_CMD_LEN);

		/* execute it */
		shell_executeCmd(buffer);
		shell_addToHistory(buffer);
	}

	return 0;
}

static bool shell_prompt(void) {
	s8 *path = getEnv("CWD");
	if(path == NULL) {
		printf("ERROR: unable to get CWD\n");
		return false;
	}
	printf("%s # ",path);
	return true;
}

static u16 shell_readLine(s8 *buffer,u16 max) {
	s8 c;
	u8 keycode;
	u8 modifier;
	u16 cursorPos = 0;
	u16 i = 0;

	/* ensure that the line is empty */
	*buffer = '\0';
	while(i < max) {
		c = scanc();

		if(handleDefEscapes(buffer,&cursorPos,&i,c,&keycode,&modifier))
			continue;
		if(c == '\033') {
			shell_handleEscapeCodes(buffer,&cursorPos,&i,keycode,modifier);
			continue;
		}
		if(c == '\t') {
			shell_complete(buffer,&cursorPos,&i);
			continue;
		}

		/* echo */
		printc(c);
		flush();

		if(c == '\n')
			break;

		/* not at the end */
		if(cursorPos < i) {
			u32 x;
			/* at first move all one char forward */
			memmove(buffer + cursorPos + 1,buffer + cursorPos,i - cursorPos);
			buffer[cursorPos] = c;
			/* now write the chars to vterm */
			for(x = cursorPos + 1; x <= i; x++)
				printc(buffer[x]);
			/* and walk backwards */
			printc('\033');
			printc(VK_HOME);
			printc(i - cursorPos);
			/* we want to do that immediatly */
			flush();
			/* we've added a char */
			cursorPos++;
			i++;
		}
		/* we are at the end of the input */
		else {
			/* put in buffer */
			buffer[cursorPos++] = c;
			i++;
		}
	}

	buffer[i] = '\0';
	return i;
}

static bool shell_handleEscapeCodes(s8 *buffer,u16 *cursorPos,u16 *charcount,u8 keycode,u8 modifier) {
	bool res = false;
	s8 *line = NULL;

	UNUSED(modifier);

	switch(keycode) {
		case VK_UP:
			line = shell_histUp();
			res = true;
			break;
		case VK_DOWN:
			line = shell_histDown();
			res = true;
			break;
	}

	if(line != NULL) {
		u32 pos = *charcount;
		u32 i,len = strlen(line);

		/* go to line end */
		printc('\033');
		printc(VK_END);
		printc(*charcount - *cursorPos);
		/* delete chars */
		while(pos-- > 0)
			printc('\b');

		/* replace in buffer and write string to vterm */
		memcpy(buffer,line,len + 1);
		for(i = 0; i < len; i++)
			printc(buffer[i]);

		/* now send the commands */
		flush();

		/* set the cursor to the end */
		*cursorPos = len;
		*charcount = len;
	}

	return res;
}

static void shell_complete(s8 *line,u16 *cursorPos,u16 *length) {
	u16 icursorPos = *cursorPos;
	u32 i,cmdlen,ilength = *length;
	s8 *orgLine = line;

	/* ignore tabs when the cursor is not at the end of the input */
	if(icursorPos == ilength) {
		sShellCmd **matches;
		sShellCmd **cmd;
		sCmdToken *tokens;
		s8 *token;
		u32 tokLen,tokCount;

		/* get tokens */
		line[ilength] = '\0';
		tokens = tok_get(line,&tokCount);
		if(tokens == NULL)
			return;

		/* get matches for last token */
		if(tokCount > 0)
			token = tokens[tokCount - 1].str;
		else
			token = (s8*)"";
		tokLen = strlen(token);
		matches = compl_get(token,tokLen,0,false,tokCount <= 1);
		if(matches == NULL || matches[0] == NULL)
			return;

		/* found one match? */
		if(matches[1] == NULL) {
			tabCount = 0;
			/* add chars */
			cmdlen = strlen(matches[0]->name);
			if(matches[0]->complStart == -1)
				i = ilength;
			else
				i = matches[0]->complStart;
			for(; i < cmdlen; i++) {
				s8 c = matches[0]->name[i];
				orgLine[ilength++] = c;
				printc(c);
			}
			/* set length and cursor */
			*length = ilength;
			*cursorPos = ilength;
			flush();
		}
		else {
			/* show all on second tab */
			if(tabCount == 0) {
				tabCount++;
				/* beep */
				printc('\a');
				flush();
			}
			else {
				tabCount = 0;

				/* print all matching commands */
				printf("\n");
				cmd = matches;
				while(*cmd != NULL) {
					printf("%s ",(*cmd)->name);
					cmd++;
				}

				/* print the prompt + entered chars again */
				printf("\n");
				shell_prompt();
				printf("%s",line);
			}
		}

		compl_free(matches);
	}
}

static s32 shell_executeCmd(s8 *line) {
	sCmdToken *tokens;
	sCommand *cmds,*cmd;
	sShellCmd **scmds;
	u32 i,cmdCount,tokCount;
	s8 path[MAX_CMD_LEN] = APPS_DIR;
	s32 res;
	s32 *pipes = NULL,*pipe;

	/* tokenize the line */
	tokens = tok_get(line,&tokCount);
	if(tokens == NULL)
		return -1;

	/* parse commands from the tokens */
	cmds = cmd_get(tokens,tokCount,&cmdCount);
	if(cmds == NULL) {
		tok_free(tokens,tokCount);
		return -1;
	}

	/* create pipe-fds */
	if(cmdCount > 1) {
		pipes = (s32*)malloc((cmdCount - 1) * sizeof(s32));
		if(pipes == NULL) {
			printLastError();
			tok_free(tokens,tokCount);
			cmd_free(cmds,cmdCount);
			return -1;
		}
	}

	pipe = pipes;
	cmd = cmds;
	for(i = 0; i < cmdCount; i++) {
		scmds = compl_get(cmd->arguments[0],strlen(cmd->arguments[0]),2,true,true);

		/* we need exactly one match and it has to be TYPE_EXTERN or TYPE_BUILTIN */
		if(scmds == NULL || scmds[0] == NULL || scmds[1] != NULL || scmds[0]->type == TYPE_PATH) {
			printf("%s: Command not found\n",cmd->arguments[0]);
			free(pipes);
			tok_free(tokens,tokCount);
			cmd_free(cmds,cmdCount);
			return -1;
		}

		/* open pipe or move it forward */
		if(cmd->dup & DUP_STDIN)
			pipe++;
		if(cmd->dup & DUP_STDOUT) {
			*pipe = open("system:/pipe",IO_READ | IO_WRITE);
			if(*pipe < 0) {
				free(pipes);
				compl_free(scmds);
				tok_free(tokens,tokCount);
				cmd_free(cmds,cmdCount);
				printLastError();
				return -1;
			}
		}

		/* execute command */
		if(scmds[0]->type == TYPE_BUILTIN) {
			/* redirect fds and make a copy of stdin and stdout because we want to keep them :) */
			/* (no fork here) */
			s32 fdout,fdin;
			if(cmd->dup & DUP_STDOUT) {
				fdout = dupFd(STDOUT_FILENO);
				redirFd(STDOUT_FILENO,*pipe);
			}
			if(cmd->dup & DUP_STDIN) {
				fdin = dupFd(STDIN_FILENO);
				redirFd(STDIN_FILENO,*(pipe - 1));
			}

			res = scmds[0]->func(cmd->argCount,cmd->arguments);

			/* restore stdin & stdout */
			if(cmd->dup & DUP_STDOUT) {
				redirFd(STDOUT_FILENO,fdout);
				/* we have to close fdout here because redirFd() will not do it for us */
				close(fdout);
			}
			if(cmd->dup & DUP_STDIN) {
				redirFd(STDIN_FILENO,fdin);
				close(fdin);
			}
		}
		else {
			if(fork() == 0) {
				/* redirect fds */
				if(cmd->dup & DUP_STDOUT)
					redirFd(STDOUT_FILENO,*pipe);
				if(cmd->dup & DUP_STDIN)
					redirFd(STDIN_FILENO,*(pipe - 1));

				/* exec */
				strcat(path,scmds[0]->name);
				exec(path,cmd->arguments);
				exit(0);
			}

			/* wait for child */
			sleep(EV_CHILD_DIED);
		}

		/* if the process has read from the pipe, close it and walk to next */
		if(cmd->dup & DUP_STDIN)
			close(*(pipe - 1));

		compl_free(scmds);
		cmd++;
	}

	/* clean up */
	free(pipes);
	tok_free(tokens,tokCount);
	cmd_free(cmds,cmdCount);
	return res;
}

static bool handleDefEscapes(s8 *line,u32 *cursorPos,u32 *charcount,s8 c,u8 *keycode,u8 *modifier) {
	bool res = false;
	u32 icursorPos = *cursorPos;
	u32 icharcount = *charcount;
	switch(c) {
		case '\b':
			if(icursorPos > 0) {
				/* remove last char */
				if(icursorPos < icharcount)
					memmove(line + icursorPos - 1,line + icursorPos,icharcount - icursorPos);
				icharcount--;
				icursorPos--;
				line[icharcount] = '\0';
				/* send backspace */
				printc(c);
				flush();
			}
			res = true;
			break;

		case '\033': {
			bool writeBack = false;
			*keycode = scanc();
			*modifier = scanc();
			switch(*keycode) {
				/* write escape-code back */
				case VK_RIGHT:
					if(icursorPos < icharcount) {
						writeBack = true;
						icursorPos++;
					}
					res = true;
					break;
				case VK_HOME:
					if(icursorPos > 0) {
						writeBack = true;
						/* send the number of chars to move */
						*modifier = icursorPos;
						icursorPos = 0;
					}
					res = true;
					break;
				case VK_END:
					if(icursorPos < icharcount) {
						writeBack = true;
						/* send the number of chars to move */
						*modifier = icharcount - icursorPos;
						icursorPos = icharcount;
					}
					res = true;
					break;
				case VK_LEFT:
					if(icursorPos > 0) {
						writeBack = true;
						icursorPos--;
					}
					res = true;
					break;
			}

			/* write escape-code back */
			if(writeBack) {
				printc(c);
				printc(*keycode);
				printc(*modifier);
				flush();
			}
		}
		break;
	}

	*cursorPos = icursorPos;
	*charcount = icharcount;

	return res;
}
