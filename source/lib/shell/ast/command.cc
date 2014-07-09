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
#include <esc/io.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/dir.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include "../mem.h"
#include "../exec/jobs.h"
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
static int ast_redirFromFile(sEnv *e,sRedirFile *redir);
/**
 * Opens the given file for output-redirection
 */
static int ast_redirToFile(sEnv *e,sRedirFile *redir);
/**
 * Free's the memory of the given command
 */
static void ast_destroyExecCmd(sExecSubCmd *cmd);
/**
 * Reads the output of the current command from given pipe and closes the pipe afterwards
 */
static sValue *ast_readCmdOutput(int *pipe);
/**
 * Waits for all processes of the current command
 */
static int ast_waitForJob(void);
/**
 * Signal-handler for processes in background
 */
static void ast_sigChildHndl(int sig);
/**
 * Removes the given proc (called in sigChildHndl)
 */
static void ast_removeProc(sJob *p,int res);
/**
 * Waits for a child and removes it
 */
static bool ast_catchZombie(void);

static bool setSigHdl = false;
static int curResult = 0;
static volatile size_t curWaitCount = 0;
static volatile size_t zombies = 0;
static tJobId curJob = 0;

sASTNode *ast_createCommand(void) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sCommand *expr = (sCommand*)emalloc(sizeof(sCommand));
	node->data = expr;
	expr->subList = esll_create();
	expr->runInBG = false;
	expr->retOutput = false;
	node->type = AST_COMMAND;
	return node;
}

sValue *ast_execCommand(sEnv *e,sCommand *n) {
	int res = 0;
	sSLNode *sub;
	sExecSubCmd *cmd;
	sShellCmd **shcmd = NULL;
	size_t cmdNo,cmdCount;
	sRedirFile *redirOut,*redirIn,*redirErr;
	sRedirFd *redirFdesc;
	char path[MAX_CMD_LEN];
	int pid,pipeFds[2],prevPipe,errFd;
	curJob = jobs_requestId();

	if(!setSigHdl) {
		setSigHdl = true;
		if(signal(SIG_CHILD_TERM,ast_sigChildHndl) == SIG_ERR)
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

		/* we need at least one match and it has to be executable */
		if(shcmd == NULL || shcmd[0] == NULL ||
				(shcmd[0]->mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) {
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
			int fdout = -1,fdin = -1,fderr = -1;
			pid = 0;
			if(redirFdesc->type == REDIR_OUT2ERR) {
				fdout = dup(STDOUT_FILENO);
				redirect(STDOUT_FILENO,STDERR_FILENO);
			}
			else if(pipeFds[1] >= 0) {
				fdout = dup(STDOUT_FILENO);
				redirect(STDOUT_FILENO,pipeFds[1]);
			}
			if(prevPipe >= 0) {
				fdin = dup(STDIN_FILENO);
				redirect(STDIN_FILENO,prevPipe);
			}
			if(redirFdesc->type == REDIR_ERR2OUT) {
				fderr = dup(STDERR_FILENO);
				redirect(STDERR_FILENO,STDOUT_FILENO);
			}
			else if(errFd >= 0) {
				fderr = dup(STDERR_FILENO);
				redirect(STDERR_FILENO,errFd);
			}

			/* execute it */
			if(shcmd[0]->type == TYPE_BUILTIN)
				res = shcmd[0]->func(cmd->exprCount,cmd->exprs);
			else {
				sFunctionStmt *stmt;
				sValue *func = env_get(e,shcmd[0]->name);
				assert(func);
				stmt = (sFunctionStmt*)val_getFunc(func);
				res = ast_callFunction(stmt,cmd->exprCount,(const char**)cmd->exprs);
			}

			/* flush stdout just to be sure */
			fflush(stdout);

			/* restore stdin & stdout & stderr */
			if(fdout >= 0) {
				redirect(STDOUT_FILENO,fdout);
				/* we have to close fdout here because redirect() will not do it for us */
				close(fdout);
				/* close write-end */
				if(pipeFds[1] >= 0)
					close(pipeFds[1]);
			}
			if(fdin >= 0) {
				redirect(STDIN_FILENO,fdin);
				close(fdin);
			}
			if(fderr >= 0) {
				redirect(STDERR_FILENO,fderr);
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
			if((pid = fork()) == 0) {
				/* redirect fds */
				if(redirFdesc->type == REDIR_OUT2ERR)
					redirect(STDOUT_FILENO,STDERR_FILENO);
				else if(pipeFds[1] >= 0)
					redirect(STDOUT_FILENO,pipeFds[1]);
				if(prevPipe >= 0)
					redirect(STDIN_FILENO,prevPipe);
				if(redirFdesc->type == REDIR_ERR2OUT)
					redirect(STDERR_FILENO,STDOUT_FILENO);
				else if(errFd >= 0)
					redirect(STDERR_FILENO,errFd);
				/* close our read-end */
				if(pipeFds[0] >= 0)
					close(pipeFds[0]);

				/* exec */
				snprintf(path,sizeof(path),"%s/%s",shcmd[0]->path,shcmd[0]->name);
				execv(path,(const char**)cmd->exprs);

				/* if we're here, there is something wrong */
				printe("Exec of '%s' failed",path);
				fflush(stderr);
				exit(EXIT_FAILURE);
			}
			else if(pid < 0)
				printe("Fork of '%s/%s' failed",shcmd[0]->path,shcmd[0]->name);
			else {
				curWaitCount++;
				jobs_addProc(curJob,pid,cmd->exprCount,cmd->exprs,n->runInBG);
				if(n->runInBG)
					printf("%%%d: job started with pid %d\n",curJob,pid);
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
			}
		}

		compl_free(shcmd);
		ast_destroyExecCmd(cmd);
		prevPipe = pipeFds[0];
	}

	/* read the output of the last command from the pipe */
	if(n->retOutput)
		return ast_readCmdOutput(pipeFds);
	/* wait for childs if we should not run this in background */
	if(!n->runInBG)
		return val_createInt(ast_waitForJob());
	return val_createInt(res);

error:
	compl_free(shcmd);
	ast_destroyExecCmd(cmd);
	return val_createInt(res);
}

void ast_catchZombies(void) {
	while(zombies > 0) {
		if(!ast_catchZombie())
			break;
	}
}

void ast_termProcsOfJob(tJobId cmd) {
	sJob *p;
	int i = 0;
	p = jobs_getXProcOf(cmd,i);
	while(p != NULL) {
		/* send SIG_INTRPT */
		if(kill(p->pid,SIG_INTRPT) < 0)
			printe("Unable to send SIG_INTRPT to process %d",p->pid);
		/* only use the next i if we couldn't remove the job */
		if(!jobs_remProc(p->pid))
			i++;
		p = jobs_getXProcOf(cmd,i);
	}
}

static int ast_redirFromFile(sEnv *e,sRedirFile *redir) {
	int fd;
	/* redirection to file */
	sValue *fileExpr = ast_execute(e,redir->expr);
	char *filename = val_getStr(fileExpr);
	fd = open(filename,O_RDONLY);
	efree(filename);
	val_destroy(fileExpr);
	if(fd < 0) {
		printe("Unable to open %s",filename);
		return -1;
	}
	return fd;
}

static int ast_redirToFile(sEnv *e,sRedirFile *redir) {
	int fd;
	/* redirection to file */
	uint flags = O_WRITE;
	sValue *fileExpr = ast_execute(e,redir->expr);
	char *filename = val_getStr(fileExpr);
	if(redir->type == REDIR_OUTCREATE)
		flags |= O_CREAT | O_TRUNC;
	else if(redir->type == REDIR_OUTAPPEND)
		flags |= O_APPEND;
	fd = open(filename,flags);
	efree(filename);
	val_destroy(fileExpr);
	if(fd < 0) {
		printe("Unable to open %s",filename);
		return -1;
	}
	return fd;
}

static void ast_destroyExecCmd(sExecSubCmd *cmd) {
	if(cmd) {
		size_t i;
		for(i = 0; i < cmd->exprCount; i++)
			efree(cmd->exprs[i]);
		efree(cmd->exprs);
		efree(cmd);
	}
}

static sValue *ast_readCmdOutput(int *pipeFds) {
	size_t outSize = OUTBUF_SIZE;
	size_t outPos = 0;
	char *outBuf;
	ssize_t count;
	/* read from the pipe and return it as string */
	outBuf = (char*)malloc(OUTBUF_SIZE + 1);
	while(outBuf && (count = read(pipeFds[0],outBuf + outPos,OUTBUF_SIZE)) > 0) {
		outPos += count;
		if(outPos + OUTBUF_SIZE >= outSize) {
			outSize += OUTBUF_SIZE;
			outBuf = (char*)realloc(outBuf,outSize + 1);
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

static int ast_waitForJob() {
	if(lang_isInterrupted())
		ast_termProcsOfJob(curJob);
	else {
		while(curWaitCount > 0 && ast_catchZombie())
			;
	}
	jobs_gc();
	return curResult;
}

static bool ast_catchZombie(void) {
	sJob *p;
	sExitState state;
	int res = -1;
	while(res != 0) {
		res = waitchild(&state);
		if(res == -EINTR) {
			if(lang_isInterrupted()) {
				ast_termProcsOfJob(curJob);
				return false;
			}
		}
		else if(res < 0) {
			if(res != -ECHILD)
				printe("Unable to wait");
			else
				zombies = 0;
			return false;
		}
	}

	zombies--;
	p = jobs_findProc(CMD_ID_ALL,state.pid);
	if(state.signal != SIG_COUNT)
		printf("Process %d was terminated by signal %d\n",state.pid,state.signal);
	else if(p && p->background) {
		printf("%%%d: '%s' with pid %d exited with status %d\n",
			p->jobId,p->command,p->pid,state.exitCode);
	}

	if(p)
		ast_removeProc(p,res);
	return true;
}

static void ast_sigChildHndl(A_UNUSED int sig) {
	zombies++;
	signal(SIG_CHILD_TERM,ast_sigChildHndl);
}

static void ast_removeProc(sJob *p,int res) {
	if(p->jobId == curJob && curWaitCount > 0) {
		curResult = res;
		curWaitCount--;
	}
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

void ast_printCommand(sCommand *s,uint layer) {
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
