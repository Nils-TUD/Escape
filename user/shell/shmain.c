/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <proc.h>
#include <io.h>
#include <dir.h>
#include <string.h>
#include <service.h>
#include <mem.h>
#include <heap.h>
#include <keycodes.h>
#include <env.h>

#include "history.h"
#include "commands.h"

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
		c = readChar();

		if(handleDefaultEscapeCodes(buffer,&cursorPos,&i,c,&keycode,&modifier))
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
		putchar(c);
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
				putchar(buffer[x]);
			/* and walk backwards */
			putchar('\033');
			putchar(VK_HOME);
			putchar(i - cursorPos);
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
		putchar('\033');
		putchar(VK_END);
		putchar(*charcount - *cursorPos);
		/* delete chars */
		while(pos-- > 0)
			putchar('\b');

		/* replace in buffer and write string to vterm */
		memcpy(buffer,line,len + 1);
		for(i = 0; i < len; i++)
			putchar(buffer[i]);

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

		/* ensure that the line is terminated */
		line[ilength] = '\0';

		matches = shell_getMatches(line,ilength,0);
		if(matches == NULL || matches[0] == NULL)
			return;

		/* found one match? */
		if(matches[1] == NULL) {
			/* add chars */
			cmdlen = strlen(matches[0]->name);
			if(matches[0]->complStart == -1)
				i = ilength;
			else
				i = matches[0]->complStart;
			for(; i < cmdlen; i++) {
				s8 c = matches[0]->name[i];
				orgLine[ilength++] = c;
				putchar(c);
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
				putchar('\a');
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

		shell_freeMatches(matches);
	}
}

static s32 shell_executeCmd(s8 *line) {
	sShellCmd **cmds;
	sShellCmd *cmd;
	s8 path[MAX_CMD_LEN] = APPS_DIR;
	s8 c;
	s32 res;
	u32 i,len = strlen(line),npos = 0,pos = 0;

	/* match command to first space */
	pos = strchri(line,' ');
	c = line[pos];
	line[pos] = '\0';

	cmds = shell_getMatches(line,pos,2);
	/* check if there is another matching command */
	if(cmds == NULL || cmds[0] == NULL || cmds[1] != NULL || cmds[0]->type == TYPE_PATH) {
		printf("%s: Command not found\n",line);
		line[pos] = c;
		return -1;
	}

	/* parse line for arguments */
	line[pos] = c;
	pos = 0;
	for(i = 0; i < MAX_ARG_COUNT; ) {
		npos = strchri(line + pos,' ');
		s8 *buffer = (s8*)malloc((npos + 1) * sizeof(s8));
		if(buffer == NULL)
			break;
		strncpy(buffer,line + pos,npos);
		args[i++] = buffer;
		if(pos + npos == len)
			break;
		pos += npos + 1;
	}

	/* execute command */
	cmd = *cmds;
	if(cmd->type == TYPE_BUILTIN) {
		res = cmd->func(i,args);
	}
	else {
		if(fork() == 0) {
			strcat(path,cmd->name);
			exec(path,args);
			exit(0);
		}

		/* wait for child */
		sleep(EV_CHILD_DIED);
	}

	/* free args */
	while(i > 0) {
		i--;
		free(args[i]);
		args[i] = NULL;
	}

	return res;
}
