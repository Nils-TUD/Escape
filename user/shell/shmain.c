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

#include "history.h"

#define APPS_DIR			"file:/apps/"

#define MAX_CMD_LEN			40
#define MAX_ARG_COUNT		10

#define ERR_CMD_NOT_FOUND	-100

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
 * Determines the command for the given line
 *
 * @param line the entered line
 * @return the command or NULL
 */
static s8 *shell_getCmd(s8 *line);

/**
 * Executes the given line
 *
 * @param line the entered line
 * @return the result
 */
static s32 shell_executeCmd(s8 *line);

/**
 * The "help" command
 */
static s32 shell_cmdHelp(u32 argc,s8 **argv);

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

		/* read command */
		printf("# ");
		shell_readLine(buffer,MAX_CMD_LEN);

		/* execute it */
		shell_executeCmd(buffer);
		shell_addToHistory(buffer);
	}

	return 0;
}

static u16 shell_readLine(s8 *buffer,u16 max) {
	s8 c;
	u8 keycode;
	u8 modifier;
	u16 cursorPos = 0;
	u16 i = 0;
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
	u32 cmdlen,ilength = *length;

	/* ignore tabs when the cursor is not at the end of the input */
	if(icursorPos == ilength) {
		s32 dd;
		/* search in file:/apps/ */
		u32 count = 0;
		if((dd = opendir(APPS_DIR)) < 0)
			return;

		sDirEntry *entry;
		s8 *first = NULL;
		while((entry = readdir(dd)) != NULL) {
			cmdlen = strlen(entry->name);
			/* beginning matches? */
			if(ilength < cmdlen && strncmp(line,entry->name,ilength) == 0) {
				count++;
				first = (s8*)malloc(cmdlen + 1);
				if(first == NULL) {
					closedir(dd);
					return;
				}
				memcpy(first,entry->name,cmdlen + 1);
			}
		}
		closedir(dd);

		/* found one match? */
		if(count == 1) {
			/* add chars */
			cmdlen = strlen(first);
			for(; ilength < cmdlen; ilength++) {
				s8 c = first[ilength];
				line[ilength] = c;
				putchar(c);
			}
			/* set length and cursor */
			*length = ilength;
			*cursorPos = ilength;
			flush();
		}
		else if(count > 1) {
			/* show all on second tab */
			if(tabCount == 0) {
				tabCount++;
				/* beep */
				putchar('\a');
				flush();
			}
			else {
				if((dd = opendir(APPS_DIR)) < 0)
					return;

				tabCount = 0;
				printf("\n");
				while((entry = readdir(dd)) != NULL) {
					cmdlen = strlen(entry->name);
					/* beginning matches? */
					if(ilength < cmdlen && strncmp(line,entry->name,ilength) == 0)
						printf("%s ",entry->name);
				}
				closedir(dd);

				/* print the prompt + entered chars again */
				printf("\n# %s",line);
			}
		}
	}
}

static s8 *shell_getCmd(s8 *line) {
	u32 pos,len;
	sDirEntry *entry;
	s8 *path;
	s32 dd;
	s8 c;
	pos = strchri(line,' ');

	if((dd = opendir(APPS_DIR)) < 0)
		return NULL;

	while((entry = readdir(dd)) != NULL) {
		len = strlen(entry->name);
		/* beginning matches? */
		if(pos == len && strncmp(line,entry->name,len) == 0) {
			closedir(dd);
			pos = strlen(APPS_DIR);
			if(pos + len >= MAX_CMD_LEN)
				return NULL;
			path = (s8*)malloc(pos + len + 1);
			if(path == NULL)
				return NULL;
			strcpy(path,APPS_DIR);
			strncat(path,line,len);
			return path;
		}
	}
	closedir(dd);

	c = line[pos];
	line[pos] = '\0';
	printf("%s: Command not found\n",line);
	line[pos] = c;
	return NULL;
}

static s32 shell_executeCmd(s8 *line) {
	s8 *command = shell_getCmd(line);
	if(command == NULL)
		return -1;

	/* parse line for arguments */
	u32 i,len = strlen(line),npos = 0,pos = 0;
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

	/* fork & exec */
	if(fork() == 0) {
		exec(command,args);
		exit(0);
	}

	/* wait for child */
	sleep(EV_CHILD_DIED);

	/* free args */
	do {
		free(args[i]);
		args[i] = NULL;
		i--;
	}
	while(i > 0);
	free(command);

	return 0;
}
