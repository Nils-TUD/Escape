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
#include <esc/proc.h>
#include <esc/dir.h>
#include <stdlib.h>
#include <errors.h>
#include "../mem.h"
#include "../exec/running.h"
#include "../exec/value.h"
#include "../exec/env.h"
#include "../ast/redirfile.h"
#include "node.h"
#include "subcmd.h"
#include "command.h"

#include <shell/shell.h>
#include <shell/completion.h>

#define OUTBUF_SIZE		128

/**
 * Free's the memory of the given command
 */
static void ast_destroyExecCmd(sExecSubCmd *cmd);
/**
 * Reads the output of the current command from given pipe and closes the pipe afterwards
 */
static sValue *ast_readCmdOutput(tFD *pipe);
/**
 * Waits for all processes of the current command
 */
static s32 ast_waitForCmd(void);
/**
 * Signal-handler for processes in background
 */
static void ast_sigChildHndl(tSig sig,u32 data);

static bool setSigHdl = false;
static s32 curResult = 0;
static volatile u32 curWaitCount = 0;
static tCmdId curCmd = 0;
static tFD closePipe[2] = {-1,-1};

sASTNode *ast_createCommand(void) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sCommand *expr = node->data = emalloc(sizeof(sCommand));
	expr->subList = sll_create();
	/* TODO errorhandling */
	expr->runInBG = false;
	expr->retOutput = false;
	node->type = AST_COMMAND;
	return node;
}

sValue *ast_execCommand(sEnv *e,sCommand *n) {
	s32 res = 0;
	sSLNode *sub;
	sExecSubCmd *cmd;
	sShellCmd **shcmd;
	u32 cmdNo,cmdCount;
	sRedirFile *redirOut,*redirIn;
	char path[MAX_CMD_LEN] = APPS_DIR;
	s32 pid,prevPid;
	tFD pipeFds[2],prevPipe;
	curCmd = run_requestId();

	if(!setSigHdl) {
		setSigHdl = true;
		setSigHandler(SIG_CHILD_TERM,ast_sigChildHndl);
	}

	prevPipe = -1;
	curWaitCount = 0;
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

		pipeFds[0] = -1;
		pipeFds[1] = -1;
		redirOut = (sRedirFile*)cmd->redirOut->data;
		redirIn = (sRedirFile*)cmd->redirIn->data;
		if(redirOut->expr) {
			char absFileName[MAX_PATH_LEN];
			/* redirection to file */
			u8 flags = IO_WRITE;
			sValue *fileExpr = ast_execute(e,redirOut->expr);
			char *filename = val_getStr(fileExpr);
			if(redirOut->type == REDIR_OUTCREATE)
				flags |= IO_CREATE | IO_TRUNCATE;
			else if(redirOut->type == REDIR_OUTAPPEND)
				flags |= IO_APPEND;
			abspath(absFileName,MAX_PATH_LEN,filename);
			pipeFds[1] = open(absFileName,flags);
			efree(filename);
			val_destroy(fileExpr);
			if(pipeFds[1] < 0) {
				printe("Unable to open %s",filename);
				res = -1;
				goto error;
			}
		}
		if(redirIn->expr) {
			char absFileName[MAX_PATH_LEN];
			/* redirection to file */
			u8 flags = IO_READ;
			sValue *fileExpr = ast_execute(e,redirIn->expr);
			char *filename = val_getStr(fileExpr);
			abspath(absFileName,MAX_PATH_LEN,filename);
			prevPipe = open(absFileName,flags);
			efree(filename);
			val_destroy(fileExpr);
			if(prevPipe < 0) {
				printe("Unable to open %s",filename);
				res = -1;
				goto error;
			}
		}
		if(pipeFds[1] == -1 && ((cmdNo == cmdCount - 1 && n->retOutput) || cmdNo < cmdCount - 1)) {
			/* create pipe */
			res = pipe(pipeFds + 0,pipeFds + 1);
			if(res < 0) {
				printe("Unable to open pipe");
				goto error;
			}
		}

		/* execute command */
		if(shcmd[0]->type == TYPE_BUILTIN) {
			/* redirect fds and make a copy of stdin and stdout because we want to keep them :) */
			/* (no fork here) */
			pid = 0;
			tFD fdout = -1,fdin = -1;
			if(pipeFds[1] >= 0) {
				fdout = dupFd(STDOUT_FILENO);
				redirFd(STDOUT_FILENO,pipeFds[1]);
			}
			if(prevPipe >= 0) {
				fdin = dupFd(STDIN_FILENO);
				redirFd(STDIN_FILENO,prevPipe);
			}

			res = shcmd[0]->func(cmd->exprCount,cmd->exprs);

			/* restore stdin & stdout */
			if(fdout >= 0) {
				redirFd(STDOUT_FILENO,fdout);
				/* we have to close fdout here because redirFd() will not do it for us */
				close(fdout);
				/* close write-end */
				close(pipeFds[1]);
			}
			if(fdin >= 0) {
				redirFd(STDIN_FILENO,fdin);
				close(fdin);
			}
			/* close pipe (to file) if there is no next process */
			if(pipeFds[1] >= 0 && cmdNo >= cmdCount - 1)
				close(pipeFds[1]);
		}
		else {
			if((pid = fork()) == 0) {
				/* redirect fds */
				if(pipeFds[1] >= 0)
					redirFd(STDOUT_FILENO,pipeFds[1]);
				if(prevPipe >= 0)
					redirFd(STDIN_FILENO,prevPipe);

				/* exec */
				strcat(path,shcmd[0]->name);
				exec(path,(const char**)cmd->exprs);

				/* if we're here, there is something wrong */
				printe("Exec of '%s' failed",path);
				exit(EXIT_FAILURE);
			}
			else if(pid < 0)
				printe("Fork of '%s%s' failed",path,shcmd[0]->name);
			else {
				curWaitCount++;
				run_addProc(curCmd,pid,prevPipe,pipeFds[1],cmdNo < cmdCount - 1);
				if(prevPid > 0) {
					/* find the prev process */
					sRunningProc *prevcmd = run_findProc(curCmd,prevPid);
					/* if there is any, tell the prev one that we're running now */
					if(prevcmd) {
						prevcmd->next = CMD_NEXT_RUNNING;
						/* if the prev-process is already terminated, close the pipe and remove
						 * the command because we haven't done that yet */
						if(prevcmd->terminated) {
							if(prevcmd->pipe[1] >= 0)
								close(prevcmd->pipe[1]);
							run_remProc(prevcmd->pid);
						}
					}
				}
			}
		}

		ast_destroyExecCmd(cmd);
		prevPid = pid;
		prevPipe = pipeFds[0];
	}

	/* read the output of the last command from the pipe */
	if(n->retOutput)
		return ast_readCmdOutput(pipeFds);
	/* wait for childs if we should not run this in background */
	if(!n->runInBG)
		return val_createInt(ast_waitForCmd());

error:
	return val_createInt(res);
}

static void ast_destroyExecCmd(sExecSubCmd *cmd) {
	u32 i;
	for(i = 0; i < cmd->exprCount; i++)
		efree(cmd->exprs[i]);
	efree(cmd->exprs);
	efree(cmd);
}

static sValue *ast_readCmdOutput(tFD *pipeFds) {
	u32 outSize = OUTBUF_SIZE;
	u32 outPos = 0;
	char *outBuf;
	s32 count;
	/* first wait for all processes to terminated. atm we won't get interrupted by a signal
	 * during read()... */
	closePipe[0] = pipeFds[0];
	closePipe[1] = pipeFds[1];
	ast_waitForCmd();
	closePipe[0] = -1;
	closePipe[1] = -1;
	/* first send EOF by closing the write-end */
	close(pipeFds[1]);
	/* now read from the pipe and return it as string */
	outBuf = (char*)malloc(OUTBUF_SIZE + 1);
	while(outBuf && (count = read(pipeFds[0],outBuf + outPos,OUTBUF_SIZE)) > 0) {
		outPos += count;
		if(outPos + OUTBUF_SIZE >= outSize) {
			outSize += OUTBUF_SIZE;
			outBuf = realloc(outBuf,outSize + 1);
		}
	}
	/* close read-end */
	close(pipeFds[0]);
	if(outBuf) {
		outBuf[outPos] = '\0';
		return val_createStr(outBuf);
	}
	return val_createInt(1);
}

static s32 ast_waitForCmd() {
	while(curWaitCount > 0) {
		if(wait(EV_NOEVENT) < 0) {
			printe("Unable to wait");
			break;
		}
	}
	run_gc();
	return curResult;
}

static void ast_sigChildHndl(tSig sig,u32 data) {
	UNUSED(sig);
	sRunningProc *p;
	sExitState state;
	s32 res = waitChild(&state);
	if(res < 0)
		return;
	p = run_findProc(CMD_ID_ALL,(tPid)data);
	if(p) {
		if(state.signal != SIG_COUNT)
			printe("\nProcess %d was terminated by signal %d\n",state.pid,state.signal);
		if(p->cmdId == curCmd && curWaitCount > 0) {
			curResult = res;
			curWaitCount--;
		}
		/*else
			printf("\n[%d] process %d finished with %d\n",p->cmdId,p->pid,state.exitCode);*/

		/* if we can close this pipe (not needed for reading output)... */
		if(p->pipe[1] >= 0 && closePipe[1] != p->pipe[1]) {
			if(p->next != CMD_NEXT_AWAIT) {
				/* if the next command already exists (or there is no next),
				 * everything is fine and we can close the write-end of the pipe */
				close(p->pipe[1]);
				p->removable = true;
			}
			else {
				/* if we have no next proc yet we have to take care that run_findProc doesn't find us
				 * again. because our pid may be reused since we're already terminated. */
				p->terminated = true;
			}
		}
		/* otherwise simply remove the process */
		else
			p->removable = true;
		/* close read-end if we don't want to read the output ourself */
		if(p->pipe[0] >= 0 && closePipe[0] != p->pipe[0])
			close(p->pipe[0]);
	}
}

void ast_setRetOutput(sASTNode *c,bool retOutput) {
	sCommand *cmd = (sCommand*)c->data;
	cmd->retOutput = retOutput;
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
	if(s->retOutput)
		printf("`");
	else
		printf("%*s",layer,"");
	for(n = sll_begin(s->subList); n != NULL; n = n->next) {
		ast_printTree((sASTNode*)n->data,layer);
		if(n->next)
			printf(" | ");
	}
	if(s->runInBG)
		printf(" &");
	if(s->retOutput)
		printf("`");
	else
		printf("\n");
}

void ast_destroyCommand(sCommand *c) {
	sSLNode *n;
	for(n = sll_begin(c->subList); n != NULL; n = n->next)
		ast_destroy((sASTNode*)n->data);
	sll_destroy(c->subList,false);
}
