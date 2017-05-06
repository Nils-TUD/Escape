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

#include <esc/proto/vterm.h>
#include <shell/history.h>
#include <shell/shell.h>
#include <sys/common.h>
#include <sys/esccodes.h>
#include <sys/io.h>
#include <sys/keycodes.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/thread.h>
#include <usergroup/usergroup.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast/command.h"
#include "exec/jobs.h"
#include "completion.h"

extern "C" {
#include "parser.h"
}

#define USERS_FILE		"/etc/users"
#define INBUF_SIZE		128

static void shell_sigIntrpt(int sig);
static size_t shell_toNextWord(char *buffer,size_t icharcount,size_t *icursorPos);
static size_t shell_toPrevWord(char *buffer,size_t *icursorPos);
static char *shell_getComplToken(char *line,size_t length,size_t *start,bool *searchPath);
extern "C" int yyparse(void);
extern "C" int yylex_destroy(void);
extern int yydebug;
extern const char *filename;

static esc::VTerm vterm(STDOUT_FILENO);
static bool resetReadLine = false;
static size_t tabCount = 0;
const char *curLine = NULL;
FILE *curStream = NULL;
bool curIsStream = false;
sEnv *curEnv = NULL;

void shell_init(int argc,const char **argv) {
	jobs_init();
	curEnv = env_create(NULL);
	env_addArgs(curEnv,argc,argv);
	if(signal(SIGINT,shell_sigIntrpt) == SIG_ERR)
		error("Unable to announce sig-handler for %d",SIGINT);
}

ssize_t shell_prompt(void) {
	uid_t uid = getuid();
	char path[MAX_PATH_LEN + 1];
	char username[MAX_NAME_LEN + 1];
	if(getenvto(username,sizeof(username),"USER") < 0) {
		printe("Unable to get USER");
		return -1;
	}
	if(getenvto(path,MAX_PATH_LEN + 1,"CWD") < 0) {
		printe("Unable to get CWD");
		return -1;
	}
	size_t count = printf("\033[co;10]%s\033[co]:\033[co;8]%s\033[co] %c ",
			username,path,uid == ROOT_UID ? '#' : '$');
	return count - SSTRLEN("\033[co;10]\033[co]\033[co;8]\033[co]");
}

static void shell_sigIntrpt(A_UNUSED int sig) {
	lang_setInterrupted();
	/* ensure that we start a new readline */
	resetReadLine = true;
	signal(SIGINT,shell_sigIntrpt);
}

int shell_executeCmd(const char *line,bool isFile) {
	int res;
	curIsStream = isFile;
	if(isFile) {
		curStream = fopen(line,"r");
		if(curStream == NULL)
			return errno;
		filename = line;
	}
	else
		filename = "<stdin>";
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
	char inbuf[INBUF_SIZE];
	char escbuf[MAX_ESCC_LENGTH];
	ssize_t ibpos = 0;
	ssize_t iblen = 0;
	size_t cursorPos = 0;
	size_t i = 0, o = 0;
	resetReadLine = false;

	/* disable "readline", enable "echo", enable "navi" (just to be sure) */
	vterm.setFlag(esc::VTerm::FL_NAVI,true);
	vterm.setFlag(esc::VTerm::FL_READLINE,false);
	vterm.setFlag(esc::VTerm::FL_ECHO,true);

	/* ensure that the line is empty */
	*buffer = '\0';
	while(o < max) {
		int n1,n2,n3;
		/* ensure that stdout is flushed because we might block */
		fflush(stdout);
		/* no more chars left? so read more from stdin */
		if(ibpos >= iblen) {
			ibpos = 0;
			iblen = read(STDIN_FILENO,inbuf,sizeof(inbuf));
			/* received signal? */
			if(iblen == -EINTR) {
				/* do a reset on ^C */
				if(resetReadLine) {
					i = 0;
					o = 0;
					cursorPos = 0;
					resetReadLine = false;
					printf("\n");
					ast_catchZombies();
					shell_prompt();
				}
				continue;
			}
			/* EOF? */
			if(iblen <= 0) {
				vterm.setFlag(esc::VTerm::FL_READLINE,true);
				return iblen;
			}
		}

		// support pastes
		if(i == 0 && inbuf[ibpos] != '\033') {
			n1 = inbuf[ibpos++];
			n2 = 0;
			n3 = 0;
		}
		else {
			/* read from inbuf to escbuf */
			if(inbuf[ibpos] == '\033')
				ibpos++;
			while(i < MAX_ESCC_LENGTH && ibpos < iblen) {
				escbuf[i++] = inbuf[ibpos];
				if(inbuf[ibpos++] == ']')
					break;
			}
			escbuf[i] = '\0';

			/* parse escape code */
			const char *escbufptr = escbuf;
			int cmd = escc_get(&escbufptr,&n1,&n2,&n3);
			if(cmd == ESCC_INCOMPLETE) {
				/* invalid escape code? */
				if(ibpos < iblen) {
					ibpos = iblen;
					i = 0;
				}
				continue;
			}

			/* esc code is complete, so start at the beginning next time */
			i = 0;
			/* skip other escape-codes or incomplete ones */
			if(cmd != ESCC_KEYCODE || (n3 & STATE_BREAK))
				continue;
		}

		/* handle special keycodes */
		if(n3 != 0 || (n1 != '\t' && n1 != '\n' && !isprint(n1))) {
			if(shell_handleSpecialKey(buffer,n2,n3,&cursorPos,&o) || (!isprint(n1) && !isspace(n1)))
				continue;
		}
		if(n1 == '\t') {
			shell_complete(buffer,&cursorPos,&o);
			continue;
		}

		/* if another key has been pressed, reset tab-counting */
		tabCount = 0;

		/* echo */
		putchar(n1);
		fflush(stdout);

		/* put the newline at the end */
		if(n1 == '\n') {
			buffer[o++] = n1;
			break;
		}
		/* not at the end */
		if(cursorPos < o) {
			size_t x;
			/* at first move all one char forward */
			memmove(buffer + cursorPos + 1,buffer + cursorPos,o - cursorPos);
			buffer[cursorPos] = n1;
			/* now write the chars to vterm */
			for(x = cursorPos + 1; x <= o; x++)
				putchar(buffer[x]);
			/* and walk backwards */
			printf("\033[ml;%zu]",o - cursorPos);
		}
		/* we are at the end of the input */
		else {
			/* put in buffer */
			buffer[cursorPos] = n1;
		}

		cursorPos++;
		o++;
	}

	vterm.setFlag(esc::VTerm::FL_READLINE,true);

	buffer[o] = '\0';
	return o;
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
				printf("\033[ml;%zu]",icursorPos);
				icursorPos = 0;
			}
			res = true;
			break;
		case VK_END:
			if(icursorPos < icharcount) {
				printf("\033[mr;%zu]",icharcount - icursorPos);
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
						printf("\033[mr;%zu]",count);
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
						printf("\033[ml;%zu]",count);
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
		case VK_P:
		case VK_N:
			if(modifier & STATE_CTRL) {
				line = keycode == VK_P ? shell_histUp() : shell_histDown();
				res = true;
			}
			break;
	}

	if(line != NULL) {
		size_t pos = *charcount;
		size_t i,len = strlen(line);

		/* go to line end */
		printf("\033[mr;%zu]",*charcount - *cursorPos);
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
