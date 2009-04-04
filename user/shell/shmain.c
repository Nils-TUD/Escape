/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/dir.h>
#include <esc/service.h>
#include <esc/mem.h>
#include <esc/heap.h>
#include <esc/keycodes.h>
#include <esc/env.h>
#include <esc/signals.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"
#include "completion.h"
#include "tokenizer.h"
#include "cmdbuilder.h"

#define MAX_ARG_COUNT		10
#define MAX_VTERM_NAME_LEN	10

#define ERR_CMD_NOT_FOUND	-100

/**
 * Prints the shell-prompt
 *
 * @return true if successfull
 */
static bool shell_prompt(void);

/**
 * Executes the given line
 *
 * @param line the entered line
 * @return the result
 */
static s32 shell_executeCmd(char *line);

/**
 * Handles SIG_INTRPT
 */
static void shell_sigIntrpt(tSig sig,u32 data);

/**
 * Reads a line
 *
 * @param buffer the buffer in which the characters should be put
 * @param max the maximum number of chars
 * @return the number of read chars
 */
static u32 shell_readLine(char *buffer,u32 max);

/**
 * Handles the escape-codes for the shell
 *
 * @param buffer the buffer with read characters
 * @param c the read character
 * @param cursorPos the current cursor-position in the buffer (may be changed)
 * @param charcount the number of read characters so far (may be changed)
 * @return true if the escape-code was handled
 */
static bool shell_handleEscapeCodes(char *buffer,char c,u32 *cursorPos,u32 *charcount);

/**
 * Completes the current input, if possible
 *
 * @param line the input
 * @param cursorPos the cursor-position (may be changed)
 * @param length the number of entered characters yet (may be changed)
 */
static void shell_complete(char *line,u32 *cursorPos,u32 *length);

static u32 vterm;
static u32 tabCount = 0;
static tPid waitingPid = INVALID_PID;

int main(int argc,char **argv) {
	tFD fd;
	char *buffer;
	char servPath[10 + MAX_VTERM_NAME_LEN + 1] = "services:/";

	/* we need either the vterm as argument or "-e <cmd>" */
	if(argc < 2) {
		fprintf(stderr,"Unable to run a shell with no arguments\n");
		return EXIT_FAILURE;
	}

	/* none-interactive-mode */
	if(argc == 3) {
		/* in this case we already have stdin, stdout and stderr */
		if(strcmp(argv[1],"-e") != 0) {
			fprintf(stderr,"Invalid shell-usage\n");
			return EXIT_FAILURE;
		}

		return shell_executeCmd(argv[2]);
	}

	/* interactive mode */

	/* open stdin */
	strcat(servPath,argv[1]);
	/* parse vterm-number from "vtermX" */
	vterm = atoi(argv[1] + 5);
	if(open(servPath,IO_READ) < 0) {
		printe("Unable to open '%s' for STDIN\n",servPath);
		return EXIT_FAILURE;
	}

	/* open stdout */
	if((fd = open(servPath,IO_WRITE)) < 0) {
		printe("Unable to open '%s' for STDOUT\n",servPath);
		return EXIT_FAILURE;
	}

	/* dup stdout to stderr */
	if(dupFd(fd) < 0) {
		printe("Unable to duplicate STDOUT to STDERR\n");
		return EXIT_FAILURE;
	}

	if(setSigHandler(SIG_INTRPT,shell_sigIntrpt) < 0) {
		printe("Unable to announce sig-handler for %d",SIG_INTRPT);
		return EXIT_FAILURE;
	}

	printf("\033f\011Welcome to Escape v0.1!\033r\011\n");
	printf("\n");
	printf("Try 'help' to see the current features :)\n");
	printf("\n");

	while(1) {
		/* create buffer (history will free it) */
		buffer = malloc((MAX_CMD_LEN + 1) * sizeof(char));
		if(buffer == NULL) {
			printf("Not enough memory\n");
			return EXIT_FAILURE;
		}

		if(!shell_prompt())
			return EXIT_FAILURE;

		/* read command */
		shell_readLine(buffer,MAX_CMD_LEN);

		/* execute it */
		shell_executeCmd(buffer);
		shell_addToHistory(buffer);
	}

	return EXIT_SUCCESS;
}

static bool shell_prompt(void) {
	char *path = getEnv("CWD");
	if(path == NULL) {
		printf("ERROR: unable to get CWD\n");
		return false;
	}
	printf("\033f\x8%s\033r\x0 # ",path);
	return true;
}

static s32 shell_executeCmd(char *line) {
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
			printf("%s: Command not found\n",cmd->arguments[0]);
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
			*pipe = open("system:/pipe",IO_READ | IO_WRITE);
			if(*pipe < 0) {
				/* close open pipe */
				if(cmd->dup & DUP_STDIN)
					close(*--pipe);
				free(pipes);
				compl_free(scmds);
				tok_free(tokens,tokCount);
				cmd_free(cmds,cmdCount);
				printe("Unable to open system:/pipe");
				return -1;
			}
		}

		/* execute command */
		if(scmds[0]->type == TYPE_BUILTIN) {
			/* redirect fds and make a copy of stdin and stdout because we want to keep them :) */
			/* (no fork here) */
			tFD fdout,fdin;
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
			tPid pid;
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
				printe("Fork failed");
			else {
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

static void shell_sigIntrpt(tSig sig,u32 data) {
	UNUSED(sig);
	/* was this interrupt intended for our vterm? */
	if(vterm == data) {
		printf("\n");
		if(waitingPid != INVALID_PID)
			sendSignalTo(waitingPid,SIG_KILL,0);
		else
			shell_prompt();
	}
}

static u32 shell_readLine(char *buffer,u32 max) {
	char c;
	u32 cursorPos = 0;
	u32 i = 0;

	/* disable "readline", enable "echo" */
	printf("\033e\x1\033l\x0");

	/* ensure that the line is empty */
	*buffer = '\0';
	while(i < max) {
		c = scanc();

		if(shell_handleEscapeCodes(buffer,c,&cursorPos,&i))
			continue;
		if(c == '\t') {
			shell_complete(buffer,&cursorPos,&i);
			continue;
		}

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
			printc('\033');
			printc(VK_HOME);
			printc(i - cursorPos);
			/* we want to do that immediatly */
			flush();
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
	printf("\033l\x1");

	buffer[i] = '\0';
	return i;
}

static bool shell_handleEscapeCodes(char *buffer,char c,u32 *cursorPos,u32 *charcount) {
	bool res = false;
	char *line = NULL;
	u32 icursorPos = *cursorPos;
	u32 icharcount = *charcount;
	switch(c) {
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
			bool writeBack = false;
			u8 keycode = scanc();
			u8 modifier = scanc();
			switch(keycode) {
				/* write escape-code back */
				case VK_RIGHT:
					if(icursorPos < icharcount) {
						writeBack = true;
						icursorPos++;
					}
					break;
				case VK_HOME:
					if(icursorPos > 0) {
						writeBack = true;
						/* send the number of chars to move */
						modifier = icursorPos;
						icursorPos = 0;
					}
					break;
				case VK_END:
					if(icursorPos < icharcount) {
						writeBack = true;
						/* send the number of chars to move */
						modifier = icharcount - icursorPos;
						icursorPos = icharcount;
					}
					break;
				case VK_LEFT:
					if(icursorPos > 0) {
						writeBack = true;
						icursorPos--;
					}
					break;
				case VK_UP:
					line = shell_histUp();
					break;
				case VK_DOWN:
					line = shell_histDown();
					break;
			}

			/* write escape-code back */
			if(writeBack) {
				printc(c);
				printc(keycode);
				printc(modifier);
				flush();
			}
			res = true;
		}
		break;
	}

	if(line != NULL) {
		u32 pos = *charcount;
		u32 i,len = strlen(line);

		/* go to line end */
		printc('\033');
		printc(VK_END);
		printc(*charcount - *cursorPos);
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

static void shell_complete(char *line,u32 *cursorPos,u32 *length) {
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
			last = (matches[0]->mode & MODE_TYPE_DIR) ? '/' : ' ';
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
			/* show all on second tab */
			if(tabCount == 0) {
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
					printf("%s ",(*cmd)->name);
					cmd++;
				}

				/* print the prompt + entered chars again */
				printf("\n");
				shell_prompt();
				printf("%s",line);
			}
		}

		compl_free(matches);
	}
}
