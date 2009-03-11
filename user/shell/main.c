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

#include "cmd/echo.h"
#include "cmd/ps.h"

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
 * Determines the command for the given line
 *
 * @param line the entered line
 * @return the command or NULL
 */
static sShellCmd *getCmd(s8 *line);

/**
 * Executes the given line
 *
 * @param line the entered line
 * @return the result
 */
static s32 executeCmd(s8 *line);

/**
 * The "help" command
 */
static s32 cmdHelp(u32 argc,s8 **argv);

/* our commands */
static sShellCmd commands[] = {
	{"echo"		, cmdEcho	},
	{"ps"		, cmdPs		},
	{"help"		, cmdHelp	},
};

/* buffer for arguments */
static s8 *args[MAX_ARG_COUNT];

s32 main(void) {
	u32 i;
	s8 *buffer;

	printf("Welcome to Escape v0.1!\n");
	printf("\n");
	printf("Try 'help' to see the current features :)\n");
	printf("\n");

	/* create buffer */
	buffer = malloc((MAX_CMD_LEN + 1) * sizeof(s8));
	if(buffer == NULL) {
		printf("Not enough memory\n");
		return 1;
	}

	while(1) {
		/* read command */
		printf("# ");
		readLine(buffer,MAX_CMD_LEN);

		/* execute it */
		executeCmd(buffer);
	}

	free(buffer);

	return 0;
}

static sShellCmd *getCmd(s8 *line) {
	u32 i,pos;
	pos = strchri(line,' ');

	for(i = 0; i < ARRAY_SIZE(commands); i++) {
		if(strncmp(line,commands[i].name,MIN(strlen(commands[i].name),pos)) == 0)
			return commands + i;
	}

	line[pos] = '\0';
	printf("%s: Command not found\n",line);
	return NULL;
}

static s32 executeCmd(s8 *line) {
	sShellCmd *command = getCmd(line);
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

static s32 cmdHelp(u32 argc,s8 **argv) {
	u32 i;
	printf("Currently you can use the following commands:\n");
	for(i = 0; i < ARRAY_SIZE(commands); i++) {
		printf("\t%s\n",commands[i].name);
	}
	printf("\n");
	printf("You can scroll the screen with pageUp, pageDown, shift+arrowUp, shift+arrowDown\n");
	printf("\n");
	return 0;
}
