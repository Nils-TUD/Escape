/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <string.h>
#include <heap.h>
#include <dir.h>
#include "completion.h"

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
static sShellCmd **compl_incrArray(sShellCmd **array,u32 pos,u32 *size);

/* our commands */
static sShellCmd externCmd;
static sShellCmd commands[] = {
	{TYPE_BUILTIN,	{"echo"	}, shell_cmdEcho	,-1},
	{TYPE_BUILTIN,	{"env"	}, shell_cmdEnv		,-1},
	{TYPE_BUILTIN,	{"help"	}, shell_cmdHelp	,-1},
	{TYPE_BUILTIN,	{"pwd"	}, shell_cmdPwd		,-1},
	{TYPE_BUILTIN,	{"cd"	}, shell_cmdCd		,-1},
};

sShellCmd **compl_get(s8 *str,u32 length,u32 max,bool searchPath) {
	u32 arraySize,arrayPos;
	u32 i,len,cmdlen,start;
	s32 dd;
	sDirEntry *entry;
	sShellCmd *cmd;
	sShellCmd **matches;
	s8 *paths[3] = {(s8*)APPS_DIR,NULL,NULL};

	/* create matches-array */
	arrayPos = 0;
	arraySize = MATCHES_ARRAY_INC;
	matches = malloc(arraySize * sizeof(sShellCmd*));
	if(matches == NULL)
		return NULL;

	/* look in builtin commands */
	if(searchPath) {
		for(i = 0; (max == 0 || arrayPos < max) && i < ARRAY_SIZE(commands); i++) {
			cmdlen = strlen(commands[i].name);
			/* beginning matches? */
			if(length <= cmdlen && strncmp(str,commands[i].name,length) == 0) {
				matches = compl_incrArray(matches,arrayPos,&arraySize);
				if(matches == NULL)
					return NULL;
				matches[arrayPos++] = commands + i;
			}
		}
	}

	/* calc the absolute path of the current line and search for matching entries in it */
	start = length;
	paths[2] = abspath(str);
	if(length > 0 && *(str + length - 1) != '/') {
		/* and try the upper directory, too */
		len = strlen(paths[2]);
		paths[1] = (s8*)malloc(len + 1);
		if(paths[1] == NULL) {
			matches[arrayPos] = NULL;
			compl_free(matches);
			return NULL;
		}
		strcpy(paths[1],paths[2]);
		dirname(paths[1]);
		/* it makes no sense to look in abspath(line) since line does not end with '/' */
		paths[2] = NULL;
	}

	/* we have to adjust line and length since we want to start at the last '/' */
	s8 *slash;
	slash = strrchr(str,'/');
	if(slash != NULL) {
		start = length - (slash - str) - 1;
		length -= slash - str + 1;
		str += slash - str + 1;
	}

	/* we don't want to look in a directory twice */
	if(paths[1] && strcmp(paths[0],paths[1]) == 0)
		paths[0] = NULL;
	else if(paths[2] && strcmp(paths[0],paths[2]) == 0)
		paths[0] = NULL;
	if(paths[1] && paths[2] && strcmp(paths[1],paths[2]) == 0)
		paths[2] = NULL;
	/* don't match stuff in PATH? */
	if(!searchPath)
		paths[0] = NULL;

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
			if(cmdlen < MAX_CMDNAME_LEN && length <= cmdlen && strncmp(str,entry->name,length) == 0) {
				matches = compl_incrArray(matches,arrayPos,&arraySize);
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
					compl_free(matches);
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
	matches = compl_incrArray(matches,arrayPos,&arraySize);
	if(matches == NULL) {
		if(paths[1] != NULL)
			free(paths[1]);
		return NULL;
	}
	matches[arrayPos] = NULL;

	return matches;
}

void compl_free(sShellCmd **matches) {
	sShellCmd **cmd = matches;
	while(*cmd != NULL) {
		if((*cmd)->type != TYPE_BUILTIN)
			free(*cmd);
		cmd++;
	}
	free(matches);
}

static sShellCmd **compl_incrArray(sShellCmd **array,u32 pos,u32 *size) {
	if(pos >= *size) {
		*size = *size + MATCHES_ARRAY_INC;
		array = (sShellCmd**)realloc(array,*size * sizeof(sShellCmd*));
		return array;
	}
	return array;
}
