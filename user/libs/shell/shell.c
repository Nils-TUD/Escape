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
#include <esc/env.h>
#include <esc/dir.h>
#include <esc/fileio.h>
#include <esc/heap.h>
#include <esc/proc.h>
#include <esc/signals.h>
#include <esc/io.h>
#include <esc/keycodes.h>
#include <fsinterface.h>
#include <string.h>
#include <stdlib.h>
#include <esccodes.h>
#include <ctype.h>

#include "shell.h"
#include "history.h"
#include "completion.h"
#include "tokenizer.h"
#include "cmdbuilder.h"

static bool resetReadLine = false;
static u32 tabCount = 0;
static tPid waitingPid = INVALID_PID;

bool shell_prompt(void) {
	/* ensure that we start a new readline */
	char *path = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
	resetReadLine = true;
	if(path == NULL) {
		printf("ERROR: unable to get CWD\n");
		return false;
	}
	if(!getEnv(path,MAX_PATH_LEN + 1,"CWD")) {
		free(path);
		printf("ERROR: unable to get CWD\n");
		return false;
	}
	printf("\033[co;8]%s\033[co] # ",path);
	free(path);
	return true;
}

tPid shell_getWaitingPid(void) {
	return waitingPid;
}

s32 shell_executeCmd(char *line) {
	sCmdToken *tokens;
	sCommand *cmds,*cmd;
	sShellCmd **scmds;
	u32 i,cmdCount,tokCount;
	char path[MAX_CMD_LEN] = APPS_DIR;
	s32 res;
	s32 *pipes = NULL,*pipe;

	/* tokenize the line */
	tokens = tok_get(line,&tokCount);
	if(tokens == NULL)
		return -1;

	/* parse commands from the tokens */
	cmds = cmd_get(tokens,tokCount,&cmdCount);
	if(cmds == NULL) {
		tok_free(tokens,tokCount);
		return -1;
	}

	/* create pipe-fds */
	if(cmdCount > 1) {
		pipes = (s32*)malloc((cmdCount - 1) * sizeof(s32));
		if(pipes == NULL) {
			printe("Unable to allocate memory for pipes");
			tok_free(tokens,tokCount);
			cmd_free(cmds,cmdCount);
			return -1;
		}
	}

	pipe = pipes;
	cmd = cmds;
	for(i = 0; i < cmdCount; i++) {
		scmds = compl_get(cmd->arguments[0],strlen(cmd->arguments[0]),2,true,true);

		/* we need exactly one match and it has to be executable */
		if(scmds == NULL || scmds[0] == NULL || scmds[1] != NULL ||
				(scmds[0]->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC)) == 0) {
			printf("\033[co;4]%s: Command not found\033[co]\n",cmd->arguments[0]);
			/* close open pipe */
			if(cmd->dup & DUP_STDIN)
				close(*pipe);
			free(pipes);
			tok_free(tokens,tokCount);
			cmd_free(cmds,cmdCount);
			return -1;
		}

		/* open pipe or move it forward */
		if(cmd->dup & DUP_STDIN)
			pipe++;
		if(cmd->dup & DUP_STDOUT) {
			*pipe = open("/system/pipe",IO_READ | IO_WRITE);
			if(*pipe < 0) {
				/* close open pipe */
				if(cmd->dup & DUP_STDIN)
					close(*--pipe);
				free(pipes);
				compl_free(scmds);
				tok_free(tokens,tokCount);
				cmd_free(cmds,cmdCount);
				printe("Unable to open /system/pipe");
				return -1;
			}
		}

		/* execute command */
		if(scmds[0]->type == TYPE_BUILTIN) {
			/* redirect fds and make a copy of stdin and stdout because we want to keep them :) */
			/* (no fork here) */
			tFD fdout = -1,fdin = -1;
			if(cmd->dup & DUP_STDOUT) {
				fdout = dupFd(STDOUT_FILENO);
				redirFd(STDOUT_FILENO,*pipe);
			}
			if(cmd->dup & DUP_STDIN) {
				fdin = dupFd(STDIN_FILENO);
				redirFd(STDIN_FILENO,*(pipe - 1));
			}

			res = scmds[0]->func(cmd->argCount,cmd->arguments);

			/* restore stdin & stdout */
			if(cmd->dup & DUP_STDOUT) {
				redirFd(STDOUT_FILENO,fdout);
				/* we have to close fdout here because redirFd() will not do it for us */
				close(fdout);
			}
			if(cmd->dup & DUP_STDIN) {
				redirFd(STDIN_FILENO,fdin);
				close(fdin);
			}
		}
		else {
			s32 pid;
			if((pid = fork()) == 0) {
				/* redirect fds */
				if(cmd->dup & DUP_STDOUT)
					redirFd(STDOUT_FILENO,*pipe);
				if(cmd->dup & DUP_STDIN)
					redirFd(STDIN_FILENO,*(pipe - 1));

				/* exec */
				strcat(path,scmds[0]->name);
				exec(path,(const char**)cmd->arguments);

				/* if we're here, there is something wrong */
				printe("Exec of '%s' failed",path);
				exit(EXIT_FAILURE);
			}
			else if(pid < 0)
				printe("Fork of '%s%s' failed",path,scmds[0]->name);
			else if(!cmd->runInBG) {
				/* wait for child */
				waitingPid = pid;
				wait(EV_CHILD_DIED);
				waitingPid = INVALID_PID;
			}
		}

		/* if the process has read from the pipe, close it and walk to next */
		if(cmd->dup & DUP_STDIN)
			close(*(pipe - 1));

		compl_free(scmds);
		cmd++;
	}

	/* clean up */
	free(pipes);
	tok_free(tokens,tokCount);
	cmd_free(cmds,cmdCount);
	return res;
}

u32 shell_readLine(char *buffer,u32 max) {
	char c;
	u32 cursorPos = 0;
	u32 i = 0;
	resetReadLine = false;

	/* disable "readline", enable "echo" */
	ioctl(STDOUT_FILENO,IOCTL_VT_DIS_RDLINE,NULL,0);
	ioctl(STDOUT_FILENO,IOCTL_VT_EN_ECHO,NULL,0);

	/* ensure that the line is empty */
	*buffer = '\0';
	while(i < max) {
		c = scanc();

		/* maybe we've received a ^C. if so do a reset */
		if(resetReadLine) {
			i = 0;
			cursorPos = 0;
			resetReadLine = false;
		}

		if(shell_handleEscapeCodes(buffer,c,&cursorPos,&i))
			continue;
		if(c == '\t') {
			shell_complete(buffer,&cursorPos,&i);
			continue;
		}
		/* if another key has been pressed, reset tab-counting */
		tabCount = 0;

		/* echo */
		printc(c);
		flush();

		if(c == '\n')
			break;

		/* not at the end */
		if(cursorPos < i) {
			u32 x;
			/* at first move all one char forward */
			memmove(buffer + cursorPos + 1,buffer + cursorPos,i - cursorPos);
			buffer[cursorPos] = c;
			/* now write the chars to vterm */
			for(x = cursorPos + 1; x <= i; x++)
				printc(buffer[x]);
			/* and walk backwards */
			printf("\033[ml;%d]",i - cursorPos);
		}
		/* we are at the end of the input */
		else {
			/* put in buffer */
			buffer[cursorPos] = c;
		}

		cursorPos++;
		i++;
	}

	/* enable "readline" */
	ioctl(STDOUT_FILENO,IOCTL_VT_EN_RDLINE,NULL,0);

	buffer[i] = '\0';
	return i;
}

bool shell_handleEscapeCodes(char *buffer,char c,u32 *cursorPos,u32 *charcount) {
	bool res = false;
	char *line = NULL;
	u32 icursorPos = *cursorPos;
	u32 icharcount = *charcount;
	switch(c) {
		/* TODO backspace should be NPRINT, right? */
		case '\b':
			if(icursorPos > 0) {
				/* remove last char */
				if(icursorPos < icharcount)
					memmove(buffer + icursorPos - 1,buffer + icursorPos,icharcount - icursorPos);
				icharcount--;
				icursorPos--;
				buffer[icharcount] = '\0';
				/* send backspace */
				printc(c);
				flush();
			}
			res = true;
			break;

		case '\033': {
			s32 cmd,n1,n2;
			cmd = freadesc(stdin,&n1,&n2);
			if(cmd != ESCC_KEYCODE)
				break;
			switch(n1) {
				/* write escape-code back */
				case VK_DELETE:
					/* delete is the same as moving cursor one step right and pressing backspace */
					if(icursorPos < icharcount) {
						printf("\033[mr]");
						icursorPos++;
						shell_handleEscapeCodes(buffer,'\b',&icursorPos,&icharcount);
					}
					break;
				case VK_HOME:
					if(icursorPos > 0) {
						printf("\033[ml;%d]",icursorPos);
						icursorPos = 0;
					}
					break;
				case VK_END:
					if(icursorPos < icharcount) {
						printf("\033[mr;%d]",icharcount - icursorPos);
						icursorPos = icharcount;
					}
					break;
				case VK_RIGHT:
					if(icursorPos < icharcount) {
						/* to next word */
						if(n2 & STATE_CTRL) {
							u16 count = 0;
							/* skip first whitespace */
							while(icursorPos < icharcount && isspace(buffer[icursorPos])) {
								count++;
								icursorPos++;
							}
							/* walk to last whitespace */
							while(icursorPos < icharcount && !isspace(buffer[icursorPos])) {
								count++;
								icursorPos++;
							}
							if(count > 0)
								printf("\033[mr;%d]",count);
						}
						else {
							icursorPos++;
							printf("\033[mr]");
						}
					}
					break;
				case VK_LEFT:
					if(icursorPos > 0) {
						/* to prev word */
						if(n2 & STATE_CTRL) {
							u16 count = 0;
							/* skip first whitespace */
							while(icursorPos > 0 && isspace(buffer[icursorPos - 1])) {
								count++;
								icursorPos--;
							}
							/* walk to last whitespace */
							while(icursorPos > 0 && !isspace(buffer[icursorPos - 1])) {
								count++;
								icursorPos--;
							}
							if(count > 0)
								printf("\033[ml;%d]",count);
						}
						else {
							icursorPos--;
							printf("\033[ml]");
						}
					}
					break;
				case VK_UP:
					line = shell_histUp();
					break;
				case VK_DOWN:
					line = shell_histDown();
					break;
			}

			res = true;
		}
		break;
	}

	if(line != NULL) {
		u32 pos = *charcount;
		u32 i,len = strlen(line);

		/* go to line end */
		printf("\033[mr;%d]",*charcount - *cursorPos);
		/* delete chars */
		while(pos-- > 0)
			printc('\b');

		/* replace in buffer and write string to vterm */
		memcpy(buffer,line,len + 1);
		for(i = 0; i < len; i++)
			printc(buffer[i]);

		/* now send the commands */
		flush();

		/* set the cursor to the end */
		*cursorPos = len;
		*charcount = len;
	}
	else {
		*cursorPos = icursorPos;
		*charcount = icharcount;
	}

	return res;
}

void shell_complete(char *line,u32 *cursorPos,u32 *length) {
	u32 icursorPos = *cursorPos;
	u32 i,cmdlen,ilength = *length;
	char *orgLine = line;

	/* ignore tabs when the cursor is not at the end of the input */
	if(icursorPos == ilength) {
		sShellCmd **matches;
		sShellCmd **cmd;
		sCmdToken *tokens;
		char *token;
		u32 tokLen,tokCount;
		bool searchPath;

		/* get tokens */
		line[ilength] = '\0';
		tokens = tok_get(line,&tokCount);
		if(tokens == NULL)
			return;

		/* get matches for last token */
		if(tokCount > 0)
			token = tokens[tokCount - 1].str;
		else
			token = (char*)"";
		tokLen = strlen(token);
		searchPath = tokCount <= 1 && (tokCount == 0 || strchr(tokens[0].str,'/') == NULL);
		matches = compl_get(token,tokLen,0,false,searchPath);
		if(matches == NULL || matches[0] == NULL)
			return;

		/* found one match? */
		if(matches[1] == NULL) {
			char last;
			tabCount = 0;

			/* add chars */
			cmdlen = strlen(matches[0]->name);
			if(matches[0]->complStart == -1)
				i = ilength;
			else
				i = matches[0]->complStart;
			for(; i < cmdlen; i++) {
				char c = matches[0]->name[i];
				orgLine[ilength++] = c;
				printc(c);
			}

			/* append '/' or ' ' depending on wether its a dir or not */
			last = MODE_IS_DIR(matches[0]->mode) ? '/' : ' ';
			if(orgLine[ilength - 1] != last) {
				orgLine[ilength++] = last;
				printc(last);
			}

			/* set length and cursor */
			*length = ilength;
			*cursorPos = ilength;
			flush();
		}
		else {
			char last;
			u32 oldLen = ilength;
			bool allEqual;
			/* note that we can assume here that its the same for all matches because it just makes
			 * a difference if we're completing in a subdir. If we are in a subdir all matches
			 * are in that subdir (otherwise they are no matches ;)). So complStart is the last '/'
			 * for all commands in this case */
			if(matches[0]->complStart == -1)
				i = ilength;
			else
				i = matches[0]->complStart;
			for(; i < MAX_CMDNAME_LEN; i++) {
				/* check if the char is equal in all commands */
				allEqual = true;
				cmd = matches;
				last = (*cmd)->name[i];
				cmd++;
				while(*cmd != NULL) {
					if((*cmd)->name[i] != last) {
						allEqual = false;
						break;
					}
					cmd++;
				}

				/* if so, add it */
				if(allEqual) {
					orgLine[ilength++] = last;
					printc(last);
				}
				else
					break;
			}

			/* have we added something? */
			if(oldLen != ilength) {
				*length = ilength;
				*cursorPos = ilength;
				/* beep */
				printc('\a');
				flush();
			}
			/* show all on second tab */
			else if(tabCount == 0) {
				tabCount++;
				/* beep */
				printc('\a');
				flush();
			}
			else {
				tabCount = 0;

				/* print all matching commands */
				printf("\n");
				cmd = matches;
				while(*cmd != NULL) {
					printf("%s%s",(*cmd)->name,MODE_IS_DIR((*cmd)->mode) ? "/ " : " ");
					cmd++;
				}

				/* print the prompt + entered chars again */
				printf("\n");
				shell_prompt();
				/* we don't want to reset the read here */
				resetReadLine = false;
				printf("%s",line);
			}
		}

		compl_free(matches);
	}
}
