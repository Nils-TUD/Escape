/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <esc/common.h>
#include <esc/heap.h>
#include <esc/dir.h>
#include <string.h>

#include "shell.h"
#include "completion.h"
#include "cmd/echo.h"
#include "cmd/env.h"
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
static sShellCmd commands[] = {
	{TYPE_BUILTIN,	(MODE_TYPE_FILE | MODE_OTHER_EXEC),	{"echo"	}, shell_cmdEcho	,-1},
	{TYPE_BUILTIN,	(MODE_TYPE_FILE | MODE_OTHER_EXEC),	{"env"	}, shell_cmdEnv		,-1},
	{TYPE_BUILTIN,	(MODE_TYPE_FILE | MODE_OTHER_EXEC),	{"pwd"	}, shell_cmdPwd		,-1},
	{TYPE_BUILTIN,	(MODE_TYPE_FILE | MODE_OTHER_EXEC),	{"cd"	}, shell_cmdCd		,-1},
};

sShellCmd **compl_get(char *str,u32 length,u32 max,bool searchCmd,bool searchPath) {
	u32 arraySize,arrayPos;
	u32 i,len,cmdlen,start,matchLen,pathLen;
	tFD dd;
	sDirEntry entry;
	sShellCmd *cmd;
	sShellCmd **matches;
	sFileInfo info;
	char *slash;
	char *filePath = NULL;
	char *paths[] = {(char*)APPS_DIR,NULL,NULL};

	/* create matches-array */
	arrayPos = 0;
	arraySize = MATCHES_ARRAY_INC;
	matches = malloc(arraySize * sizeof(sShellCmd*));
	if(matches == NULL)
		goto failed;

	/* look in builtin commands */
	if(searchPath) {
		for(i = 0; (max == 0 || arrayPos < max) && i < ARRAY_SIZE(commands); i++) {
			cmdlen = strlen(commands[i].name);
			matchLen = searchCmd ? cmdlen : length;
			/* beginning matches? */
			if(length <= cmdlen && strncmp(str,commands[i].name,matchLen) == 0) {
				matches = compl_incrArray(matches,arrayPos,&arraySize);
				if(matches == NULL)
					goto failed;
				matches[arrayPos++] = commands + i;
			}
		}
	}

	/* calc the absolute path of the current line and search for matching entries in it */
	start = length;
	paths[2] = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
	if(paths[2] == NULL)
		goto failed;
	abspath(paths[2],MAX_PATH_LEN + 1,str);
	if(length > 0 && *(str + length - 1) != '/') {
		/* and try the upper directory, too */
		len = strlen(paths[2]);
		paths[1] = (char*)malloc(len + 1);
		if(paths[1] == NULL)
			goto failed;
		strcpy(paths[1],paths[2]);
		dirname(paths[1]);
		/* it makes no sense to look in abspath(line) since line does not end with '/' */
		free(paths[2]);
		paths[2] = NULL;
	}

	/* we have to adjust line and length since we want to start at the last '/' */
	slash = strrchr(str,'/');
	if(slash != NULL) {
		start = length - (slash - str) - 1;
		length -= slash - str + 1;
		str += slash - str + 1;
	}

	/* we don't want to look in a directory twice */
	if(paths[1] && strcmp(paths[0],paths[1]) == 0 && searchPath)
		paths[1] = NULL;
	else if(paths[2] && strcmp(paths[0],paths[2]) == 0 && searchPath) {
		free(paths[2]);
		paths[2] = NULL;
	}
	if(paths[1] && paths[2] && strcmp(paths[1],paths[2]) == 0) {
		free(paths[2]);
		paths[2] = NULL;
	}
	/* don't match stuff in PATH? */
	if(!searchPath)
		paths[0] = NULL;

	/* reserve mem for file-path */
	filePath = (char*)malloc(MAX_PATH_LEN + 1);
	if(filePath == NULL)
		goto failed;

	for(i = 0; (max == 0 || arrayPos < max) && i < ARRAY_SIZE(paths); i++) {
		/* we want to look with an empty line for paths[2] */
		if(i == 2)
			length = 0;
		if(paths[i] == NULL)
			continue;
		if((dd = opendir(paths[i])) < 0)
			continue;

		/* copy path to the beginning of the file-path */
		strcpy(filePath,paths[i]);
		pathLen = strlen(paths[i]);

		while(readdir(&entry,dd)) {
			/* skip . and .. */
			if(strcmp(entry.name,".") == 0 || strcmp(entry.name,"..") == 0)
				continue;

			cmdlen = strlen(entry.name);
			matchLen = searchCmd ? cmdlen : length;
			if(cmdlen < MAX_CMDNAME_LEN && length <= cmdlen && strncmp(str,entry.name,matchLen) == 0) {
				matches = compl_incrArray(matches,arrayPos,&arraySize);
				if(matches == NULL) {
					closedir(dd);
					goto failed;
				}

				/* append filename and get fileinfo */
				strcpy(filePath + pathLen,entry.name);
				if(stat(filePath,&info) < 0) {
					closedir(dd);
					goto failed;
				}

				/* alloc mem for cmd */
				cmd = malloc(sizeof(sShellCmd));
				if(cmd == NULL) {
					closedir(dd);
					goto failed;
				}

				/* fill cmd */
				if(i == 0)
					cmd->type = TYPE_EXTERN;
				else
					cmd->type = TYPE_PATH;
				cmd->mode = info.mode;
				cmd->func = NULL;
				memcpy(cmd->name,entry.name,cmdlen + 1);
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

	free(filePath);
	free(paths[1]);
	free(paths[2]);

	/* terminate */
	matches = compl_incrArray(matches,arrayPos,&arraySize);
	if(matches == NULL)
		return NULL;
	matches[arrayPos] = NULL;

	return matches;

failed:
	free(paths[1]);
	free(paths[2]);
	free(filePath);
	if(matches) {
		matches[arrayPos] = NULL;
		compl_free(matches);
	}
	return NULL;
}

void compl_free(sShellCmd **matches) {
	sShellCmd **cmd = matches;
	if(cmd) {
		while(*cmd != NULL) {
			if((*cmd)->type != TYPE_BUILTIN)
				free(*cmd);
			cmd++;
		}
		free(matches);
	}
}

static sShellCmd **compl_incrArray(sShellCmd **array,u32 pos,u32 *size) {
	if(pos >= *size) {
		*size = *size + MATCHES_ARRAY_INC;
		array = (sShellCmd**)realloc(array,*size * sizeof(sShellCmd*));
		return array;
	}
	return array;
}
