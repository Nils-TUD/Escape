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
#include <esc/io.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/dir.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errors.h>
#include "../mem.h"
#include "../exec/running.h"
#include "../exec/value.h"
#include "../exec/env.h"
#include "../ast/redirfile.h"
#include "../ast/redirfd.h"
#include "../ast/functionstmt.h"
#include "node.h"
#include "subcmd.h"
#include "command.h"

#include "../completion.h"

#define OUTBUF_SIZE		128

/**
 * Opens the given file for input-redirection
 */
static tFD ast_redirFromFile(sEnv *e,sRedirFile *redir);
/**
 * Opens the given file for output-redirection
 */
static tFD ast_redirToFile(sEnv *e,sRedirFile *redir);
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
 * Terminates all processes of the current command
 */
static void ast_termProcsOfCmd(void);
/**
 * Signal-handler for processes in background
 */
static void ast_sigChildHndl(s32 sig);
/**
 * Removes the given proc (called in sigChildHndl)
 */
static void ast_removeProc(sRunningProc *p,s32 res);

static bool setSigHdl = false;
static s32 curResult = 0;
static volatile u32 curWaitCount = 0;
static tCmdId curCmd = 0;
static s32 diedProc = -1;
static s32 diedRes = -1;

sASTNode *ast_createCommand(void) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sCommand *expr = node->data = emalloc(sizeof(sCommand));
	expr->subList = esll_create();
	expr->runInBG = false;
	expr->retOutput = false;
	node->type = AST_COMMAND;
	return node;
}

sValue *ast_execCommand(sEnv *e,sCommand *n) {
	s32 res = 0;
	sSLNode *sub;
	sExecSubCmd *cmd;
	sShellCmd **shcmd = NULL;
	u32 cmdNo,cmdCount;
	sRedirFile *redirOut,*redirIn,*redirErr;
	sRedirFd *redirFdesc;
	char path[MAX_CMD_LEN] = APPS_DIR;
	s32 pid,prevPid = -1;
	tFD pipeFds[2],prevPipe,errFd;
	curCmd = run_requestId();

	if(!setSigHdl) {
		setSigHdl = true;
		if(setSigHandler(SIG_CHILD_TERM,ast_sigChildHndl) < 0)
			error("Unable to set child-termination sig-handler");
	}

	prevPipe = -1;
	curWaitCount = 0;
	cmdNo = 0;
	cmdCount = sll_length(n->subList);
	for(sub = sll_begin(n->subList); sub != NULL; sub = sub->next, cmdNo++) {
		cmd = (sExecSubCmd*)ast_execute(e,(sASTNode*)sub->data);
		if(cmd == NULL || cmd->exprs[0] == NULL) {
			res = -1;
			goto error;
		}

		/* get the command */
		shcmd = compl_get(e,cmd->exprs[0],strlen(cmd->exprs[0]),2,true,true);

		/* we need exactly one match and it has to be executable */
		if(shcmd == NULL || shcmd[0] == NULL || shcmd[1] != NULL ||
				(shcmd[0]->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC)) == 0) {
			printf("\033[co;4]%s: Command not found\033[co]\n",cmd->exprs[0]);
			res = -1;
			goto error;
		}

		errFd = -1;
		pipeFds[0] = -1;
		pipeFds[1] = -1;
		redirOut = (sRedirFile*)cmd->redirOut->data;
		redirErr = (sRedirFile*)cmd->redirErr->data;
		redirIn = (sRedirFile*)cmd->redirIn->data;
		redirFdesc = (sRedirFd*)cmd->redirFd->data;
		if(redirOut->expr) {
			pipeFds[1] = ast_redirToFile(e,redirOut);
			if(pipeFds[1] < 0) {
				res = -1;
				goto error;
			}
		}
		if(redirErr->expr) {
			errFd = ast_redirToFile(e,redirErr);
			if(errFd < 0) {
				res = -1;
				goto error;
			}
		}
		if(redirIn->expr) {
			prevPipe = ast_redirFromFile(e,redirIn);
			if(prevPipe < 0) {
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
		if(shcmd[0]->type == TYPE_BUILTIN || shcmd[0]->type == TYPE_FUNCTION) {
			/* redirect fds and make a copy of stdin and stdout because we want to keep them :) */
			/* (no fork here) */
			pid = 0;
			tFD fdout = -1,fdin = -1,fderr = -1;
			if(redirFdesc->type == REDIR_OUT2ERR) {
				fdout = dupFd(STDOUT_FILENO);
				redirFd(STDOUT_FILENO,STDERR_FILENO);
			}
			else if(pipeFds[1] >= 0) {
				fdout = dupFd(STDOUT_FILENO);
				redirFd(STDOUT_FILENO,pipeFds[1]);
			}
			if(prevPipe >= 0) {
				fdin = dupFd(STDIN_FILENO);
				redirFd(STDIN_FILENO,prevPipe);
			}
			if(redirFdesc->type == REDIR_ERR2OUT) {
				fderr = dupFd(STDERR_FILENO);
				redirFd(STDERR_FILENO,STDOUT_FILENO);
			}
			else if(errFd >= 0) {
				fderr = dupFd(STDERR_FILENO);
				redirFd(STDERR_FILENO,errFd);
			}

			/* execute it */
			if(shcmd[0]->type == TYPE_BUILTIN)
				res = shcmd[0]->func(cmd->exprCount,cmd->exprs);
			else {
				sValue *func = env_get(e,shcmd[0]->name);
				assert(func);
				sFunctionStmt *stmt = (sFunctionStmt*)val_getFunc(func);
				res = ast_callFunction(stmt,cmd->exprCount,(const char**)cmd->exprs);
			}

			/* flush stdout just to be sure */
			fflush(stdout);

			/* restore stdin & stdout & stderr */
			if(fdout >= 0) {
				redirFd(STDOUT_FILENO,fdout);
				/* we have to close fdout here because redirFd() will not do it for us */
				close(fdout);
				/* close write-end */
				if(pipeFds[1] >= 0)
					close(pipeFds[1]);
			}
			if(fdin >= 0) {
				redirFd(STDIN_FILENO,fdin);
				close(fdin);
			}
			if(fderr >= 0) {
				redirFd(STDERR_FILENO,fderr);
				close(fderr);
				if(errFd >= 0)
					close(errFd);
			}

			/* close the previous pipe since we don't need it anymore */
			if(prevPipe >= 0)
				close(prevPipe);
			/* close read-end of the current if we don't want to read the command-output */
			/* otherwise we need it for the next process or ourself */
			if(!n->retOutput && cmdNo == cmdCount - 1)
				close(pipeFds[0]);
		}
		else {
			diedProc = -1;
			if((pid = fork()) == 0) {
				/* redirect fds */
				if(redirFdesc->type == REDIR_OUT2ERR)
					redirFd(STDOUT_FILENO,STDERR_FILENO);
				else if(pipeFds[1] >= 0)
					redirFd(STDOUT_FILENO,pipeFds[1]);
				if(prevPipe >= 0)
					redirFd(STDIN_FILENO,prevPipe);
				if(redirFdesc->type == REDIR_ERR2OUT)
					redirFd(STDERR_FILENO,STDOUT_FILENO);
				else if(errFd >= 0)
					redirFd(STDERR_FILENO,errFd);
				/* close our read-end */
				if(pipeFds[0] >= 0)
					close(pipeFds[0]);

				/* exec */
				strcat(path,shcmd[0]->name);
				exec(path,(const char**)cmd->exprs);

				/* if we're here, there is something wrong */
				printe("Exec of '%s' failed",path);
				fflush(stderr);
				exit(EXIT_FAILURE);
			}
			else if(pid < 0)
				printe("Fork of '%s%s' failed",path,shcmd[0]->name);
			else {
				curWaitCount++;
				run_addProc(curCmd,pid);
				/* always close write-end */
				close(pipeFds[1]);
				/* close error-redirection */
				if(errFd >= 0)
					close(errFd);
				/* close the previous pipe since we don't need it anymore */
				if(prevPipe >= 0)
					close(prevPipe);
				/* close read-end of the current if we don't want to read the command-output */
				/* otherwise we need it for the next process or ourself */
				if(!n->retOutput && cmdNo == cmdCount - 1)
					close(pipeFds[0]);

				/* if the process has died before run_addProc() was called, we didn't know him.
				 * Now we do, so remove him */
				if(diedProc != -1) {
					sRunningProc *rp = run_findProc(CMD_ID_ALL,diedProc);
					ast_removeProc(rp,diedRes);
				}
			}
		}

		compl_free(shcmd);
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
	return val_createInt(res);

error:
	compl_free(shcmd);
	ast_destroyExecCmd(cmd);
	return val_createInt(res);
}

static tFD ast_redirFromFile(sEnv *e,sRedirFile *redir) {
	tFD fd;
	char absFileName[MAX_PATH_LEN];
	/* redirection to file */
	u8 flags = IO_READ;
	sValue *fileExpr = ast_execute(e,redir->expr);
	char *filename = val_getStr(fileExpr);
	abspath(absFileName,MAX_PATH_LEN,filename);
	fd = open(absFileName,flags);
	efree(filename);
	val_destroy(fileExpr);
	if(fd < 0) {
		printe("Unable to open %s",filename);
		return -1;
	}
	return fd;
}

static tFD ast_redirToFile(sEnv *e,sRedirFile *redir) {
	char absFileName[MAX_PATH_LEN];
	tFD fd;
	/* redirection to file */
	u8 flags = IO_WRITE;
	sValue *fileExpr = ast_execute(e,redir->expr);
	char *filename = val_getStr(fileExpr);
	if(redir->type == REDIR_OUTCREATE)
		flags |= IO_CREATE | IO_TRUNCATE;
	else if(redir->type == REDIR_OUTAPPEND)
		flags |= IO_APPEND;
	abspath(absFileName,MAX_PATH_LEN,filename);
	fd = open(absFileName,flags);
	efree(filename);
	val_destroy(fileExpr);
	if(fd < 0) {
		printe("Unable to open %s",filename);
		return -1;
	}
	return fd;
}

static void ast_destroyExecCmd(sExecSubCmd *cmd) {
	u32 i;
	if(cmd) {
		for(i = 0; i < cmd->exprCount; i++)
			efree(cmd->exprs[i]);
		efree(cmd->exprs);
		efree(cmd);
	}
}

static sValue *ast_readCmdOutput(tFD *pipeFds) {
	u32 outSize = OUTBUF_SIZE;
	u32 outPos = 0;
	char *outBuf;
	s32 count;
	/* read from the pipe and return it as string */
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
	s32 res;
	if(lang_isInterrupted())
		ast_termProcsOfCmd();
	else {
		while(curWaitCount > 0) {
			res = wait(EV_NOEVENT);
			if(res == ERR_INTERRUPTED) {
				if(lang_isInterrupted()) {
					ast_termProcsOfCmd();
					break;
				}
			}
			else if(res < 0) {
				printe("Unable to wait");
				break;
			}
		}
	}
	run_gc();
	return curResult;
}

static void ast_termProcsOfCmd(void) {
	sRunningProc *p;
	s32 i = 0;
	p = run_getXProcOf(curCmd,i);
	while(p != NULL) {
		/* send SIG_INTRPT */
		if(sendSignalTo(p->pid,SIG_INTRPT) < 0)
			printe("Unable to send SIG_INTRPT to process %d",p->pid);
		run_remProc(p->pid);
		/* to next */
		i++;
		p = run_getXProcOf(curCmd,i);
	}
}

static void ast_sigChildHndl(s32 sig) {
	UNUSED(sig);
	sRunningProc *p;
	sExitState state;
	s32 res = waitChild(&state);
	if(res < 0) {
		printe("Unable to wait for child");
		return;
	}
	if(state.signal != SIG_COUNT)
		printf("\nProcess %d was terminated by signal %d\n",state.pid,state.signal);
	p = run_findProc(CMD_ID_ALL,state.pid);
	if(p)
		ast_removeProc(p,res);
	/* otherwise remember the pid */
	else {
		diedProc = state.pid;
		diedRes = res;
	}
}

static void ast_removeProc(sRunningProc *p,s32 res) {
	if(p->cmdId == curCmd && curWaitCount > 0) {
		curResult = res;
		curWaitCount--;
	}
	/*else
		printf("\n[%d] process %d finished with %d\n",p->cmdId,p->pid,state.exitCode);*/
	p->removable = true;
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
	esll_append(cmd->subList,sub);
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
