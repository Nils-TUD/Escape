/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <esc/dir.h>
#include <stdlib.h>
#include <string.h>

#include "completion.h"
#include "cmds.h"
#include "exec/env.h"
#include "exec/value.h"

#define MATCHES_ARRAY_INC	8
#define DIR_CACHE_SIZE		16

typedef struct {
	bool cached;
	dev_t devNo;
	inode_t inodeNo;
	time_t modified;
	size_t cmdCount;
	sShellCmd *cmds;
} sDirCache;

static sShellCmd **compl_incrArray(sShellCmd **array,size_t pos,size_t *size);
static sDirCache *compl_getCache(const char *path);
static void compl_freeCache(sDirCache *dc);

static sDirCache dirCache[DIR_CACHE_SIZE];
static sShellCmd commands[] = {
	{TYPE_BUILTIN,	(S_IFREG | S_IXOTH), "", {"."			}, SSTRLEN("."),		shell_cmdInclude,-1},
	{TYPE_BUILTIN,	(S_IFREG | S_IXOTH), "", {"clear"		}, SSTRLEN("clear"),	shell_cmdClear	,-1},
	{TYPE_BUILTIN,	(S_IFREG | S_IXOTH), "", {"echo"		}, SSTRLEN("echo"),		shell_cmdEcho	,-1},
	{TYPE_BUILTIN,	(S_IFREG | S_IXOTH), "", {"env"			}, SSTRLEN("env"),		shell_cmdEnv	,-1},
	{TYPE_BUILTIN,	(S_IFREG | S_IXOTH), "", {"pwd"			}, SSTRLEN("pwd"),		shell_cmdPwd	,-1},
	{TYPE_BUILTIN,	(S_IFREG | S_IXOTH), "", {"cd"			}, SSTRLEN("cd"),		shell_cmdCd		,-1},
	{TYPE_BUILTIN,	(S_IFREG | S_IXOTH), "", {"include"		}, SSTRLEN("include"),	shell_cmdInclude,-1},
	{TYPE_BUILTIN,	(S_IFREG | S_IXOTH), "", {"kill"		}, SSTRLEN("kill"),		shell_cmdKill	,-1},
	{TYPE_BUILTIN,	(S_IFREG | S_IXOTH), "", {"jobs"		}, SSTRLEN("jobs"),		shell_cmdJobs	,-1},
	{TYPE_BUILTIN,	(S_IFREG | S_IXOTH), "", {"help"		}, SSTRLEN("help"),		shell_cmdHelp	,-1},
};

sShellCmd **compl_get(sEnv *e,char *str,size_t length,size_t max,bool searchCmd,bool searchPath) {
	size_t arraySize,arrayPos;
	size_t i,j,len,start,matchLen,pathLen;
	sShellCmd *ncmd,*cmd;
	sShellCmd **matches;
	sFileInfo info;
	char *slash;
	char *filePath = NULL;
	char *paths[] = {(char*)APPS_DIR,NULL,NULL};

	/* create matches-array */
	arrayPos = 0;
	arraySize = MATCHES_ARRAY_INC;
	matches = (sShellCmd**)malloc(arraySize * sizeof(sShellCmd*));
	if(matches == NULL)
		goto failed;

	/* look in builtin commands */
	if(searchPath) {
		for(i = 0; (max == 0 || arrayPos < max) && i < ARRAY_SIZE(commands); i++) {
			matchLen = searchCmd ? commands[i].nameLen : length;
			/* beginning matches? */
			if(length <= commands[i].nameLen && strncmp(str,commands[i].name,matchLen) == 0) {
				matches = compl_incrArray(matches,arrayPos,&arraySize);
				if(matches == NULL)
					goto failed;
				matches[arrayPos++] = commands + i;
			}
		}
	}

	/* look for matching variables */
	{
		sSLNode *n;
		sSLList *vars = env_getMatching(e,str,length,searchCmd);
		for(n = sll_begin(vars); n != NULL; n = n->next) {
			const char *vname = (const char*)n->data;
			cmd = (sShellCmd*)malloc(sizeof(sShellCmd));
			cmd->func = NULL;
			cmd->mode = S_IFREG | S_IXOTH;
			strnzcpy(cmd->name,vname,sizeof(cmd->name));
			cmd->nameLen = strlen(cmd->name);
			cmd->type = cmd->name[0] == '$' ? TYPE_VARIABLE : TYPE_FUNCTION;
			cmd->complStart = -1;
			matches = compl_incrArray(matches,arrayPos,&arraySize);
			if(matches == NULL)
				goto failed;
			matches[arrayPos++] = cmd;
		}
		sll_destroy(vars,false);
	}

	/* calc the absolute path of the current line and search for matching entries in it */
	start = length;
	paths[2] = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
	if(paths[2] == NULL)
		goto failed;
	{
		size_t p2count = cleanpath(paths[2],MAX_PATH_LEN + 1,str);
		if(p2count > 1) {
			paths[2][p2count] = '/';
			paths[2][p2count + 1] = '\0';
		}
	}

	if(length > 0 && *(str + length - 1) != '/') {
		/* and try the upper directory, too */
		len = strlen(paths[2]);
		/* len + 2 to leave one space for '/' */
		paths[1] = (char*)malloc(len + 2);
		if(paths[1] == NULL)
			goto failed;
		strcpy(paths[1],paths[2]);
		dirname(paths[1]);
		if(strcmp(paths[1],"/") != 0)
			strcat(paths[1],"/");
		/* it makes no sense to look in cleanpath(line) since line does not end with '/' */
		free(paths[2]);
		paths[2] = NULL;
	}

	/* we have to adjust line and length since we want to start at the last '/' */
	slash = strrchr(str,'/');
	if(slash != NULL) {
		start = length - (slash - str) - 1;
		length -= slash - str + 1;
		str += slash - str + 1;
		/* if there is a slash in the path, we don't want to search in PATH */
		paths[0] = NULL;
	}

	/* we don't want to look in a directory twice */
	if(paths[1] && paths[0] && searchPath && strcmp(paths[0],paths[1]) == 0)
		paths[1] = NULL;
	else if(paths[2] && paths[0] && searchPath && strcmp(paths[0],paths[2]) == 0) {
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
		sDirCache *dc;
		/* we want to look with an empty line for paths[2] */
		if(i == 2)
			length = 0;
		if(paths[i] == NULL)
			continue;

		dc = compl_getCache(paths[i]);
		if(!dc)
			continue;

		/* copy path to the beginning of the file-path */
		strcpy(filePath,paths[i]);
		pathLen = strlen(paths[i]);

		for(j = 0; j < dc->cmdCount; j++) {
			cmd = dc->cmds + j;
			matchLen = searchCmd ? cmd->nameLen : length;
			if(length <= cmd->nameLen && strncmp(str,cmd->name,matchLen) == 0) {
				matches = compl_incrArray(matches,arrayPos,&arraySize);
				if(matches == NULL)
					goto failed;

				/* append filename and get fileinfo */
				strcpy(filePath + pathLen,cmd->name);
				if(stat(filePath,&info) < 0)
					goto failed;

				/* alloc mem for cmd */
				ncmd = (sShellCmd*)malloc(sizeof(sShellCmd));
				if(ncmd == NULL)
					goto failed;
				memcpy(ncmd,cmd,sizeof(sShellCmd));

				/* fill cmd */
				if(i == 0)
					ncmd->type = TYPE_EXTERN;
				else
					ncmd->type = TYPE_PATH;
				ncmd->mode = info.mode;
				if(i == 0)
					ncmd->complStart = -1;
				else
					ncmd->complStart = start;
				matches[arrayPos++] = ncmd;

				if(max > 0 && arrayPos >= max)
					break;
			}
		}

		compl_freeCache(dc);
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

static sShellCmd **compl_incrArray(sShellCmd **array,size_t pos,size_t *size) {
	if(pos >= *size) {
		*size = *size + MATCHES_ARRAY_INC;
		array = (sShellCmd**)realloc(array,*size * sizeof(sShellCmd*));
		return array;
	}
	return array;
}

static sDirCache *compl_getCache(const char *path) {
	sDirCache *dc;
	DIR *d;
	sDirEntry e;
	size_t i,cmdsSize,freeIndex = DIR_CACHE_SIZE;
	sFileInfo info;
	int res = stat(path,&info);
	if(res < 0)
		return NULL;

	/* first check whether we already have this dir in the cache */
	for(i = 0; i < DIR_CACHE_SIZE; i++) {
		if(dirCache[i].inodeNo == 0)
			freeIndex = i;
		else if(dirCache[i].inodeNo == info.inodeNo && dirCache[i].devNo == info.device) {
			/* has it been modified? */
			if(dirCache[i].modified < info.modifytime) {
				freeIndex = i;
				break;
			}
			return dirCache + i;
		}
	}

	/* if we have no free slot, allocate a new dircache, that is used temporary and destroyed
	 * when compl_get is finished */
	if(freeIndex == DIR_CACHE_SIZE) {
		dc = (sDirCache*)malloc(sizeof(sDirCache));
		if(!dc)
			return NULL;
		dc->cached = false;
	}
	/* otherwise use one of the free slots */
	else {
		dc = dirCache + freeIndex;
		dc->cached = true;
		if(dc->inodeNo != 0)
			free(dc->cmds);
	}

	d = opendir(path);
	if(!d)
		return NULL;
	/* setup cache-entry */
	dc->inodeNo = info.inodeNo;
	dc->devNo = info.device;
	dc->modified = info.modifytime;
	cmdsSize = 8;
	dc->cmdCount = 0;
	dc->cmds = (sShellCmd*)malloc(cmdsSize * sizeof(sShellCmd));
	if(!dc->cmds)
		goto failed;
	while(readdir(d,&e)) {
		sShellCmd *cmd;
		/* skip . and .. */
		if(strcmp(e.name,".") == 0 || strcmp(e.name,"..") == 0)
			continue;

		/* increase array-size? */
		if(dc->cmdCount >= cmdsSize) {
			sShellCmd *old = dc->cmds;
			cmdsSize *= 2;
			dc->cmds = (sShellCmd*)realloc(dc->cmds,cmdsSize * sizeof(sShellCmd));
			if(!dc->cmds) {
				dc->cmds = old;
				goto failed;
			}
		}

		/* build shell-command */
		cmd = dc->cmds + dc->cmdCount++;
		cmd->type = TYPE_PATH;
		/* mode and complStart will be set later */
		cmd->mode = 0;
		cmd->complStart = 0;
		cmd->func = NULL;
		strnzcpy(cmd->path,path,sizeof(cmd->path));
		strnzcpy(cmd->name,e.name,sizeof(cmd->name));
		cmd->nameLen = strlen(cmd->name);
	}
	closedir(d);
	return dc;

failed:
	closedir(d);
	compl_freeCache(dc);
	return NULL;
}

static void compl_freeCache(sDirCache *dc) {
	if(!dc->cached) {
		free(dc->cmds);
		free(dc);
	}
}
