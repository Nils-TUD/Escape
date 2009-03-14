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
#include "cmd/echo.h"
#include "cmd/ps.h"
#include "cmd/memusage.h"
#include "cmd/test.h"
#include "cmd/ata.h"

#define MAX_CMD_LEN			40
#define MAX_ARG_COUNT		10

#define ERR_CMD_NOT_FOUND	-100

/* for shell commands */
typedef s32 (*cmdFunc)(u32 argc,s8 **argv);
typedef struct {
	cstring name;
	cmdFunc func;
} sShellCmd;

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
 * @param c the character
 * @param keycode the keycode of the escape-code
 * @param modifier the modifier of the escape-code
 * @return true if the escape-code was handled
 */
static bool shell_handleEscapeCodes(s8 *buffer,u16 *cursorPos,u16 *charcount,s8 c,u8 keycode,u8 modifier);

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
static sShellCmd *shell_getCmd(s8 *line);

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

/* our commands */
static sShellCmd commands[] = {
	{"echo"		, shell_cmdEcho		},
	{"ps"		, shell_cmdPs		},
	{"help"		, shell_cmdHelp		},
	{"memusage"	, shell_cmdMemUsage	},
	{"test"		, shell_cmdTest		},
	{"ata"		, shell_cmdAta		},
};

/* buffer for arguments */
static u32 tabCount = 0;
static s8 *args[MAX_ARG_COUNT];

s32 main(void) {
	s8 *buffer;

	printf("\033f\011Welcome to Escape v0.1!\033r\00\n");
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
		shell_addToHistory(buffer);
		shell_executeCmd(buffer);
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
			shell_handleEscapeCodes(buffer,&cursorPos,&i,c,keycode,modifier);
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

		/* put in buffer */
		buffer[cursorPos++] = c;
		if(cursorPos > i)
			i++;
	}

	buffer[i] = '\0';
	return i;
}

static bool shell_handleEscapeCodes(s8 *buffer,u16 *cursorPos,u16 *charcount,s8 c,u8 keycode,u8 modifier) {
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
	s32 i;
	u16 icursorPos = *cursorPos;
	u32 cmdlen,ilength = *length;

	/* ignore tabs when the cursor is not at the end of the input */
	if(icursorPos == ilength) {
		/* search in commands */
		s32 index = -1;
		u32 count = 0;
		for(i = 0; i < ARRAY_SIZE(commands); i++) {
			cmdlen = strlen(commands[i].name);
			/* beginning matches? */
			if(ilength < cmdlen && strncmp(line,commands[i].name,ilength) == 0) {
				count++;
				index = i;
			}
		}

		/* found one match? */
		if(count == 1) {
			/* add chars */
			cmdlen = strlen(commands[index].name);
			for(; ilength < cmdlen; ilength++) {
				s8 c = commands[index].name[ilength];
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
				tabCount = 0;
				printf("\n");
				for(i = 0; i < ARRAY_SIZE(commands); i++) {
					cmdlen = strlen(commands[i].name);
					/* beginning matches? */
					if(ilength < cmdlen && strncmp(line,commands[i].name,ilength) == 0) {
						printf("%s ",commands[i].name);
					}
				}
				/* print the prompt + entered chars again */
				printf("\n# %s",line);
			}
		}
	}
}

static sShellCmd *shell_getCmd(s8 *line) {
	u32 i,pos,len;
	s8 c;
	pos = strchri(line,' ');

	for(i = 0; i < ARRAY_SIZE(commands); i++) {
		len = strlen(commands[i].name);
		if(pos == len && strncmp(line,commands[i].name,len) == 0)
			return commands + i;
	}

	c = line[pos];
	line[pos] = '\0';
	printf("%s: Command not found\n",line);
	line[pos] = c;
	return NULL;
}

static s32 shell_executeCmd(s8 *line) {
	sShellCmd *command = shell_getCmd(line);
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

	/* execute command */
	s32 res = command->func(i,args);

	/* free args */
	do {
		free(args[i]);
		args[i] = NULL;
		i--;
	}
	while(i > 0);

	return res;
}

static s32 shell_cmdHelp(u32 argc,s8 **argv) {
	u32 i;

	UNUSED(argc);
	UNUSED(argv);

	printf("Currently you can use the following commands:\n");
	for(i = 0; i < ARRAY_SIZE(commands); i++) {
		printf("\t%s\n",commands[i].name);
	}
	printf("\n");
	printf("You can scroll the screen with pageUp, pageDown, shift+arrowUp, shift+arrowDown\n");
	printf("\n");
	return 0;
}
