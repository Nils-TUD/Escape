/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <string.h>
#include <heap.h>
#include <dir.h>
#include "commands.h"

#include "cmd/echo.h"
#include "cmd/env.h"
#include "cmd/help.h"
#include "cmd/pwd.h"
#include "cmd/cd.h"

#define MATCHES_ARRAY_INC	8

/**
 * Increase the size of the given shell-commands-array
 *
 * @param array the commands
 * @param pos the current number of entries
 * @param size the current size
 * @return the commands
 */
static sShellCmd **shell_incrArray(sShellCmd **array,u32 pos,u32 *size);

/* our commands */
static sShellCmd externCmd;
static sShellCmd commands[] = {
	{TYPE_BUILTIN,	{"echo"	}, shell_cmdEcho	,-1},
	{TYPE_BUILTIN,	{"env"	}, shell_cmdEnv		,-1},
	{TYPE_BUILTIN,	{"help"	}, shell_cmdHelp	,-1},
	{TYPE_BUILTIN,	{"pwd"	}, shell_cmdPwd		,-1},
	{TYPE_BUILTIN,	{"cd"	}, shell_cmdCd		,-1},
};

sShellCmd **shell_getMatches(s8 *line,u32 length,u32 max) {
	u32 arraySize,arrayPos;
	u32 i,len,rem,cmdlen,start;
	s32 dd;
	sDirEntry *entry;
	sShellCmd *cmd;
	sShellCmd **matches;
	s8 *tmp;
	s8 *paths[3] = {(s8*)APPS_DIR,NULL,NULL};

	/* walk to the last space */
	rem = length;
	line += length;
	while(rem > 0 && *(line - 1) != ' ') {
		line--;
		rem--;
	}
	length -= rem;

	/* create matches-array */
	arrayPos = 0;
	arraySize = MATCHES_ARRAY_INC;
	matches = malloc(arraySize * sizeof(sShellCmd*));
	if(matches == NULL)
		return NULL;

	/* we want to look in the builtin commands if there is no space yet */
	if(rem == 0) {
		/* look in builtin commands */
		for(i = 0; (max == 0 || arrayPos < max) && i < ARRAY_SIZE(commands); i++) {
			cmdlen = strlen(commands[i].name);
			/* beginning matches? */
			if(length <= cmdlen && strncmp(line,commands[i].name,length) == 0) {
				matches = shell_incrArray(matches,arrayPos,&arraySize);
				if(matches == NULL)
					return NULL;
				matches[arrayPos++] = commands + i;
			}
		}
	}

	/* calc the absolute path of the current line and search for matching entries in it */
	start = length;
	paths[2] = abspath(line);
	if(length > 0 && *(line + length - 1) != '/') {
		/* and try the upper directory, too */
		len = strlen(paths[2]);
		paths[1] = (s8*)malloc(len + 1);
		if(paths[1] == NULL) {
			matches[arrayPos] = NULL;
			shell_freeMatches(matches);
			return NULL;
		}
		strcpy(paths[1],paths[2]);
		dirname(paths[1]);
		/* it makes no sense to look in abspath(line) since line does not end with '/' */
		paths[2] = NULL;
	}

	/* we have to adjust line and length since we want to start at the last '/' */
	s8 *slash;
	slash = strrchr(line,'/');
	if(slash != NULL) {
		start = length - (slash - line) - 1;
		length -= slash - line + 1;
		line += slash - line + 1;
	}

	/* we don't want to look in a directory twice */
	if(paths[1] && strcmp(paths[0],paths[1]) == 0)
		paths[0] = NULL;
	else if(paths[2] && strcmp(paths[0],paths[2]) == 0)
		paths[0] = NULL;
	if(paths[1] && paths[2] && strcmp(paths[1],paths[2]) == 0)
		paths[2] = NULL;
	/* don't match stuff in PATH if there is already a space */
	if(rem > 0)
		paths[0] = NULL;
/*
	for(i = 0; i < ARRAY_SIZE(paths); i++)
		printf("PATH[%d]='%s'\n",i,paths[i] ? paths[i] : "<NULL>");
	printf("line='%s' (%d)\n",line,length);
	printf("start=%d\n",start);
*/
	for(i = 0; (max == 0 || arrayPos < max) && i < ARRAY_SIZE(paths); i++) {
		/* we want to look with an empty line for paths[2] */
		if(i == 2)
			length = 0;
		if(paths[i] == NULL)
			continue;
		if((dd = opendir(paths[i])) < 0)
			continue;

		while((entry = readdir(dd)) != NULL) {
			/* skip . and .. */
			if(strcmp(entry->name,".") == 0 || strcmp(entry->name,"..") == 0)
				continue;

			cmdlen = strlen(entry->name);
			if(cmdlen < MAX_CMDNAME_LEN && length <= cmdlen && strncmp(line,entry->name,length) == 0) {
				matches = shell_incrArray(matches,arrayPos,&arraySize);
				if(matches == NULL) {
					if(paths[1] != NULL)
						free(paths[1]);
					closedir(dd);
					return NULL;
				}

				cmd = malloc(sizeof(sShellCmd));
				if(cmd == NULL) {
					if(paths[1] != NULL)
						free(paths[1]);
					matches[arrayPos] = NULL;
					shell_freeMatches(matches);
					closedir(dd);
					return NULL;
				}

				if(i == 0)
					cmd->type = TYPE_EXTERN;
				else
					cmd->type = TYPE_PATH;
				cmd->func = NULL;
				memcpy(cmd->name,entry->name,cmdlen + 1);
				if(i == 0)
					cmd->complStart = -1;
				else
					cmd->complStart = start;
				matches[arrayPos++] = cmd;

				if(max > 0 && arrayPos >= max)
					break;
			}
		}

		closedir(dd);
	}

	/* terminate */
	matches = shell_incrArray(matches,arrayPos,&arraySize);
	if(matches == NULL) {
		if(paths[1] != NULL)
			free(paths[1]);
		return NULL;
	}
	matches[arrayPos] = NULL;

	return matches;
}

void shell_freeMatches(sShellCmd **matches) {
	sShellCmd **cmd = matches;
	while(*cmd != NULL) {
		if((*cmd)->type != TYPE_BUILTIN)
			free(*cmd);
		cmd++;
	}
	free(matches);
}

static sShellCmd **shell_incrArray(sShellCmd **array,u32 pos,u32 *size) {
	if(pos >= *size) {
		*size = *size + MATCHES_ARRAY_INC;
		array = (sShellCmd**)realloc(array,*size * sizeof(sShellCmd*));
		return array;
	}
	return array;
}
