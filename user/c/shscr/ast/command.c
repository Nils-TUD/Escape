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
#include <esc/fileio.h>
#include <esc/io.h>
#include <esc/signals.h>
#include <stdlib.h>
#include <errors.h>
#include "../mem.h"
#include "node.h"
#include "subcmd.h"
#include "command.h"

#include <shell/shell.h>
#include <shell/completion.h>

sASTNode *ast_createCommand(void) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sCommand *expr = node->data = emalloc(sizeof(sCommand));
	expr->subList = sll_create();
	/* TODO errorhandling */
	expr->runInBG = false;
	node->type = AST_COMMAND;
	return node;
}

sValue *ast_execCommand(sEnv *e,sCommand *n) {
	sSLNode *sub;
	sExecSubCmd *cmd,*prevCmd = NULL;
	sShellCmd **shcmd;
	u32 i,j,cmdNo,cmdCount;
	char path[MAX_CMD_LEN] = APPS_DIR;
	s32 res = 0;
	u32 waitingCount = 0;

	cmdNo = 0;
	cmdCount = sll_length(n->subList);
	for(sub = sll_begin(n->subList); sub != NULL; sub = sub->next, cmdNo++) {
		cmd = (sExecSubCmd*)ast_execute(e,(sASTNode*)sub->data);

		/* get the command */
		shcmd = compl_get(cmd->exprs[0],strlen(cmd->exprs[0]),2,true,true);

		/* we need exactly one match and it has to be executable */
		if(shcmd == NULL || shcmd[0] == NULL || shcmd[1] != NULL ||
				(shcmd[0]->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC)) == 0) {
			printf("\033[co;4]%s: Command not found\033[co]\n",cmd->exprs[0]);
			res = -1;
			goto error;
		}

		/* create pipe */
		if(cmdNo < cmdCount - 1) {
			cmd->pipe = open("/system/pipe",IO_READ | IO_WRITE);
			if(cmd->pipe < 0) {
				printe("Unable to open /system/pipe");
				res = -1;
				goto error;
			}
		}

		/* execute command */
		if(shcmd[0]->type == TYPE_BUILTIN) {
			/* redirect fds and make a copy of stdin and stdout because we want to keep them :) */
			/* (no fork here) */
			tFD fdout = -1,fdin = -1;
			if(cmdNo < cmdCount - 1) {
				fdout = dupFd(STDOUT_FILENO);
				redirFd(STDOUT_FILENO,cmd->pipe);
			}
			if(cmdNo > 0) {
				fdin = dupFd(STDIN_FILENO);
				redirFd(STDIN_FILENO,prevCmd->pipe);
			}

			res = shcmd[0]->func(cmd->exprCount,cmd->exprs);

			/* restore stdin & stdout */
			if(cmdNo < cmdCount - 1) {
				redirFd(STDOUT_FILENO,fdout);
				/* we have to close fdout here because redirFd() will not do it for us */
				close(fdout);
			}
			if(cmdNo > 0) {
				redirFd(STDIN_FILENO,fdin);
				close(fdin);
			}
		}
		else {
			if((cmd->pid = fork()) == 0) {
				/* redirect fds */
				if(cmdNo < cmdCount - 1)
					redirFd(STDOUT_FILENO,cmd->pipe);
				if(cmdNo > 0)
					redirFd(STDIN_FILENO,prevCmd->pipe);

				/* exec */
				strcat(path,shcmd[0]->name);
				exec(path,(const char**)cmd->exprs);

				/* if we're here, there is something wrong */
				printe("Exec of '%s' failed",path);
				exit(EXIT_FAILURE);
			}
			else if(cmd->pid < 0)
				printe("Fork of '%s%s' failed",path,shcmd[0]->name);
			else {
				/* if the last command was a builtin one, we have to close the pipe here. We
				 * can't do it earlier because we would remove the pipe. Here it is ok because
				 * fork() has duplicated the file-descriptors and increased the references on
				 * the node (not just the file!). */
				/* This way we send EOF to the pipe */
				if(cmdNo > 0 && prevCmd->pid == 0) {
					close(prevCmd->pipe);
					prevCmd->pipe = -1;
				}
				waitingCount++;
				if(cmdNo >= cmdCount - 1/* && !cmd->runInBG*/) {
					sExitState state;
					/* wait for child */
					while(waitingCount > 0) {
						while(1) {
							res = waitChild(&state);
							if(res != ERR_INTERRUPTED)
								break;
						}
						/* if we've terminated a child via signal, we don't get it here anymore */
						if(res == ERR_NO_CHILD)
							break;
						else if(res < 0)
							printe("Unable to wait for child");
						else if(res == 0) {
							sSLNode *ssub;
							res = state.exitCode;
							if(state.signal != SIG_COUNT) {
								printf("\nProcess %d (%s%s) was terminated by signal %d\n",state.pid,
										path,shcmd[0]->name,state.signal);
							}
							/*
							for(j = 0; j < cmdCount; j++) {
								if(cmds[j].pid == state.pid) {
									if(cmds[j].pipe != -1) {
										close(cmds[j].pipe);
										cmds[j].pipe = -1;
									}
									cmds[j].pid = 0;
									waitingCount--;
									break;
								}
							}*/
						}
					}
				}
			}
		}

		prevCmd = cmd;
	}

	/* clean up */
error:
	/* TODO free mem */
	/* TODO we have to return the output sometimes... */
	return val_createInt(res);
}

void ast_setRunInBG(sASTNode *c,bool runInBG) {
	sCommand *cmd = (sCommand*)c->data;
	cmd->runInBG = runInBG;
}

void ast_addSubCmd(sASTNode *c,sASTNode *sub) {
	sCommand *cmd = (sCommand*)c->data;
	sll_append(cmd->subList,sub);
	/* TODO errorhandling */
}

void ast_printCommand(sCommand *s,u32 layer) {
	sSLNode *n;
	printf("%*s",layer,"");
	for(n = sll_begin(s->subList); n != NULL; n = n->next) {
		ast_printTree((sASTNode*)n->data,layer);
		if(n->next)
			printf(" | ");
	}
	if(s->runInBG)
		printf(" &");
	printf("\n");
}

void ast_destroyCommand(sCommand *c) {
	sSLNode *n;
	for(n = sll_begin(c->subList); n != NULL; n = n->next)
		ast_destroy((sASTNode*)n->data);
	sll_destroy(c->subList,false);
}
