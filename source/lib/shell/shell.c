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
#include <usergroup/user.h>
#include <esc/driver/vterm.h>
#include <esc/dir.h>
#include <esc/proc.h>
#include <esc/io.h>
#include <esc/keycodes.h>
#include <esc/fsinterface.h>
#include <esc/messages.h>
#include <esc/esccodes.h>
#include <esc/thread.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <shell/shell.h>
#include <shell/history.h>
#include "ast/command.h"
#include "exec/jobs.h"
#include "completion.h"
#include "parser.h"

#define USERS_FILE		"/etc/users"

static void shell_sigIntrpt(int sig);
static size_t shell_toNextWord(char *buffer,size_t icharcount,size_t *icursorPos);
static size_t shell_toPrevWord(char *buffer,size_t *icursorPos);
static char *shell_getComplToken(char *line,size_t length,size_t *start,bool *searchPath);
extern int yyparse(void);
extern int yylex_destroy(void);
extern int yydebug;
extern char *filename;

static bool resetReadLine = false;
static size_t tabCount = 0;
char *curLine = NULL;
FILE *curStream = NULL;
bool curIsStream = false;
sEnv *curEnv = NULL;

void shell_init(int argc,const char **argv) {
	jobs_init();
	curEnv = env_create(NULL);
	env_addArgs(curEnv,argc,argv);
	if(signal(SIG_INTRPT,shell_sigIntrpt) == SIG_ERR)
		error("Unable to announce sig-handler for %d",SIG_INTRPT);
}

bool shell_prompt(void) {
	uid_t uid = getuid();
	char path[MAX_PATH_LEN + 1];
	char username[MAX_USERNAME_LEN + 1];
	if(getenvto(username,sizeof(username),"USER") < 0) {
		printe("Unable to get USER");
		return false;
	}
	if(getenvto(path,MAX_PATH_LEN + 1,"CWD") < 0) {
		printe("Unable to get CWD");
		return false;
	}
	printf("\033[co;10]%s\033[co]:\033[co;8]%s\033[co] %c ",
			username,path,uid == ROOT_UID ? '#' : '$');
	return true;
}

static void shell_sigIntrpt(A_UNUSED int sig) {
	lang_setInterrupted();
	/* ensure that we start a new readline */
	resetReadLine = true;
}

int shell_executeCmd(char *line,bool isFile) {
	int res;
	curIsStream = isFile;
	if(isFile) {
		curStream = fopen(line,"r");
		if(curStream == NULL)
			return -errno;
		filename = line;
	}
	else
		filename = (char*)"<stdin>";
	curLine = line;
	lang_reset();
	res = yyparse();
	/* we need to reset the scanner if an error happened */
	if(res != 0)
		yylex_destroy();
	jobs_gc();
	if(isFile)
		fclose(curStream);
	ast_catchZombies();
	return res;
}

int shell_readLine(char *buffer,size_t max) {
	size_t cursorPos = 0;
	size_t i = 0;
	resetReadLine = false;

	/* disable "readline", enable "echo", enable "navi" (just to be sure) */
	if(vterm_setNavi(STDOUT_FILENO,true) < 0)
		error("Unable to enable navi");
	if(vterm_setReadline(STDOUT_FILENO,false) < 0)
		error("Unable to disable readline");
	if(vterm_setEcho(STDOUT_FILENO,true) < 0)
		error("Unable to enable echo");

	/* ensure that the line is empty */
	*buffer = '\0';
	while(i < max) {
		char c = 0;
		int cmd,n1,n2,n3;
		/* ensure that stdout is flushed and wait until input is available; wait gets interrupted
		 * for signals; fgetc() won't (more precisely: it is, but it repeats the call) */
		fflush(stdout);
		/* only wait if the buffer is empty; otherwise we might wait until the next keypress
		 * because if we have already read everything from vterm, we'll wait until there is
		 * more data to read. */
		if(favail(stdin) == 0)
			wait(EV_DATA_READABLE,STDIN_FILENO);
		/* maybe we've received a ^C. if so do a reset */
		if(resetReadLine) {
			i = 0;
			cursorPos = 0;
			resetReadLine = false;
			printf("\n");
			ast_catchZombies();
			shell_prompt();
			continue;
		}
		/* stop if we were unable to read from stdin */
		if((c = fgetc(stdin)) == EOF) {
			if(vterm_setReadline(STDOUT_FILENO,true) < 0)
				error("Unable to reenable readline");
			return ferror(stdin);
		}
		if(c != '\033')
			continue;
		cmd = freadesc(stdin,&n1,&n2,&n3);
		/* skip other escape-codes */
		if(cmd != ESCC_KEYCODE || (n3 & STATE_BREAK))
			continue;

		if(n3 != 0 || (n1 != '\t' && n1 != '\n' && !isprint(n1))) {
			if(shell_handleSpecialKey(buffer,n2,n3,&cursorPos,&i) || !isprint(n1))
				continue;
		}
		if(n1 == '\t') {
			shell_complete(buffer,&cursorPos,&i);
			continue;
		}
		/* if another key has been pressed, reset tab-counting */
		tabCount = 0;

		/* echo */
		putchar(n1);
		fflush(stdout);

		/* put the newline at the end */
		if(n1 == '\n') {
			buffer[i++] = n1;
			break;
		}
		/* not at the end */
		if(cursorPos < i) {
			size_t x;
			/* at first move all one char forward */
			memmove(buffer + cursorPos + 1,buffer + cursorPos,i - cursorPos);
			buffer[cursorPos] = n1;
			/* now write the chars to vterm */
			for(x = cursorPos + 1; x <= i; x++)
				putchar(buffer[x]);
			/* and walk backwards */
			printf("\033[ml;%d]",i - cursorPos);
		}
		/* we are at the end of the input */
		else {
			/* put in buffer */
			buffer[cursorPos] = n1;
		}

		cursorPos++;
		i++;
	}

	if(vterm_setReadline(STDOUT_FILENO,true) < 0)
		error("Unable to reenable readline");

	buffer[i] = '\0';
	return i;
}

bool shell_handleSpecialKey(char *buffer,int keycode,int modifier,size_t *cursorPos,
		size_t *charcount) {
	bool res = false;
	char *line = NULL;
	size_t icursorPos = *cursorPos;
	size_t icharcount = *charcount;
	switch(keycode) {
		case VK_W:
			if(modifier & STATE_CTRL) {
				size_t oldPos = icursorPos;
				size_t count = shell_toPrevWord(buffer,&icursorPos);
				if(count > 0) {
					memmove(buffer + icursorPos,buffer + oldPos,icharcount - oldPos);
					icharcount -= count;
					buffer[icharcount] = '\0';
					/* send backspaces */
					while(count-- > 0)
						putchar('\b');
					fflush(stdout);
				}
				res = true;
			}
			break;

		case VK_BACKSP:
			if(icursorPos > 0) {
				/* remove last char */
				if(icursorPos < icharcount)
					memmove(buffer + icursorPos - 1,buffer + icursorPos,icharcount - icursorPos);
				icharcount--;
				icursorPos--;
				buffer[icharcount] = '\0';
				/* send backspace */
				putchar('\b');
				fflush(stdout);
			}
			res = true;
			break;
		/* write escape-code back */
		case VK_DELETE:
			/* delete is the same as moving cursor one step right and pressing backspace */
			if(icursorPos < icharcount) {
				printf("\033[mr]");
				icursorPos++;
				shell_handleSpecialKey(buffer,VK_BACKSP,0,&icursorPos,&icharcount);
			}
			res = true;
			break;
		case VK_HOME:
			if(icursorPos > 0) {
				printf("\033[ml;%d]",icursorPos);
				icursorPos = 0;
			}
			res = true;
			break;
		case VK_END:
			if(icursorPos < icharcount) {
				printf("\033[mr;%d]",icharcount - icursorPos);
				icursorPos = icharcount;
			}
			res = true;
			break;
		case VK_RIGHT:
			if(icursorPos < icharcount) {
				/* to next word */
				if(modifier & STATE_CTRL) {
					size_t count = shell_toNextWord(buffer,icharcount,&icursorPos);
					if(count > 0)
						printf("\033[mr;%d]",count);
				}
				else {
					icursorPos++;
					printf("\033[mr]");
				}
			}
			res = true;
			break;
		case VK_LEFT:
			if(icursorPos > 0) {
				/* to prev word */
				if(modifier & STATE_CTRL) {
					size_t count = shell_toPrevWord(buffer,&icursorPos);
					if(count > 0)
						printf("\033[ml;%d]",count);
				}
				else {
					icursorPos--;
					printf("\033[ml]");
				}
			}
			res = true;
			break;
		case VK_UP:
			line = shell_histUp();
			res = true;
			break;
		case VK_DOWN:
			line = shell_histDown();
			res = true;
			break;
	}

	if(line != NULL) {
		size_t pos = *charcount;
		size_t i,len = strlen(line);

		/* go to line end */
		printf("\033[mr;%d]",*charcount - *cursorPos);
		/* delete chars */
		while(pos-- > 0)
			putchar('\b');

		/* replace in buffer and write string to vterm */
		memcpy(buffer,line,len + 1);
		for(i = 0; i < len; i++)
			putchar(buffer[i]);

		/* now send the commands */
		fflush(stdout);

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

static size_t shell_toNextWord(char *buffer,size_t icharcount,size_t *icursorPos) {
	size_t count = 0;
	/* skip first whitespace */
	while(*icursorPos < icharcount && isspace(buffer[*icursorPos])) {
		count++;
		(*icursorPos)++;
	}
	/* walk to last whitespace */
	while(*icursorPos < icharcount && !isspace(buffer[*icursorPos])) {
		count++;
		(*icursorPos)++;
	}
	return count;
}

static size_t shell_toPrevWord(char *buffer,size_t *icursorPos) {
	size_t count = 0;
	/* skip first whitespace */
	while(*icursorPos > 0 && isspace(buffer[*icursorPos - 1])) {
		count++;
		(*icursorPos)--;
	}
	/* walk to last whitespace */
	while(*icursorPos > 0 && !isspace(buffer[*icursorPos - 1])) {
		count++;
		(*icursorPos)--;
	}
	return count;
}

static char *shell_getComplToken(char *line,size_t length,size_t *start,bool *searchPath) {
	char c;
	char *res;
	bool inSStr = false;
	bool inDStr = false;
	bool inWord = false;
	bool inSubCmd = false;
	size_t startPos = 0;
	size_t i;
	*searchPath = true;
	for(i = 0; i < length; i++) {
		c = line[i];
		switch(c) {
			case ',':
			case '|':
			case ';':
				startPos = i + 1;
				*searchPath = true;
				inWord = false;
				break;
			case ' ':
			case '\t':
			case '=':
			case '(':
			case ')':
				if((c != '(' && c != ')') || !inSStr) {
					startPos = i + 1;
					if(inWord) {
						*searchPath = false;
						inWord = false;
					}
				}
				break;
			case '`':
				if(!inSStr) {
					inSubCmd = !inSubCmd;
					startPos = i + 1;
					*searchPath = inSubCmd;
					inWord = false;
				}
				break;
			case '\'':
				if(!inDStr) {
					inSStr = !inSStr;
					startPos = i + 1;
					*searchPath = true;
					inWord = false;
				}
				break;
			case '"':
				if(!inSStr) {
					inDStr = !inDStr;
					startPos = i + 1;
					*searchPath = true;
					inWord = false;
				}
				break;
			case '/':
				*searchPath = false;
				inWord = true;
				break;
			default:
				inWord = true;
				break;
		}
	}

	res = (char*)malloc((length - startPos) + 1);
	if(res) {
		memcpy(res,line + startPos,length - startPos);
		res[length - startPos] = '\0';
	}
	*start = startPos;
	return res;
}

void shell_complete(char *line,size_t *cursorPos,size_t *length) {
	size_t icursorPos = *cursorPos;
	size_t i,cmdlen,ilength = *length;
	char *orgLine = line;

	/* ignore tabs when the cursor is not at the end of the input */
	if(icursorPos == ilength) {
		sShellCmd **matches;
		sShellCmd **cmd;
		bool searchPath = false;
		size_t partLen;
		size_t startPos;
		char *part;
		/* extract part to complete */
		line[ilength] = '\0';
		part = shell_getComplToken(line,ilength,&startPos,&searchPath);
		if(part == NULL)
			return;

		/* get matches for last token */
		partLen = strlen(part);
		matches = compl_get(curEnv,part,partLen,0,false,searchPath);
		if(matches == NULL || matches[0] == NULL) {
			/* beep because we have found no match */
			putchar('\a');
			fflush(stdout);
			free(part);
			compl_free(matches);
			return;
		}

		/* found one match? */
		if(matches[1] == NULL) {
			char last;
			tabCount = 0;

			/* add chars */
			cmdlen = strlen(matches[0]->name);
			if(matches[0]->complStart == -1)
				i = ilength - startPos;
			else
				i = matches[0]->complStart;
			for(; i < cmdlen; i++) {
				char c = matches[0]->name[i];
				orgLine[ilength++] = c;
				putchar(c);
			}

			/* append '/' or ' ' depending on whether its a dir or not */
			last = S_ISDIR(matches[0]->mode) ? '/' : ' ';
			if(orgLine[ilength - 1] != last) {
				orgLine[ilength++] = last;
				putchar(last);
			}

			/* set length and cursor */
			*length = ilength;
			*cursorPos = ilength;
			fflush(stdout);
		}
		else {
			char last;
			size_t oldLen = ilength;
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
				if(allEqual && last != '\0') {
					orgLine[ilength++] = last;
					putchar(last);
				}
				else
					break;
			}

			/* have we added something? */
			if(oldLen != ilength) {
				*length = ilength;
				*cursorPos = ilength;
				/* beep */
				putchar('\a');
				fflush(stdout);
			}
			/* show all on second tab */
			else if(tabCount == 0) {
				tabCount++;
				/* beep */
				putchar('\a');
				fflush(stdout);
			}
			else {
				tabCount = 0;

				/* print all matching commands */
				printf("\n");
				cmd = matches;
				while(*cmd != NULL) {
					printf("%s%s",(*cmd)->name,S_ISDIR((*cmd)->mode) ? "/ " : " ");
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

		free(part);
		compl_free(matches);
	}
}
