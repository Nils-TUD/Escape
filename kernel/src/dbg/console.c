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

#include <sys/common.h>
#include <sys/dbg/console.h>
#include <sys/dbg/cmd/file.h>
#include <sys/dbg/cmd/view.h>
#include <sys/dbg/cmd/log.h>
#include <sys/dbg/cmd/ls.h>
#include <sys/dbg/kb.h>
#include <sys/mem/kheap.h>
#include <sys/video.h>
#include <esc/keycodes.h>
#include <string.h>
#include <ctype.h>

#define HISTORY_SIZE	32
#define MAX_ARG_COUNT	5
#define MAX_ARG_LEN		32
#define MAX_CMD_LEN		16

typedef s32 (*fCommand)(s32 argc,char **argv);
typedef struct {
	char name[MAX_ARG_LEN];
	fCommand exec;
} sCommand;

static char **cons_readCommand(s32 *argc);
static char *cons_readLine(void);
static sCommand *cons_getCommand(const char *name);

static u32 histWritePos = 0;
static u32 histReadPos = 0;
static u32 histSize = 0;
static char *history[HISTORY_SIZE];
static char emptyLine[VID_COLS];
static sScreenBackup backup;
static sCommand commands[] = {
	{"help",NULL},
	{"exit",NULL},
	{"file",cons_cmd_file},
	{"view",cons_cmd_view},
	{"log",cons_cmd_log},
	{"ls",cons_cmd_ls},
};

void cons_start(void) {
	u32 i;
	s32 res,argc;
	sCommand *cmd;
	char **argv;

	vid_setTargets(TARGET_SCREEN);
	vid_backup(backup.screen,&backup.row,&backup.col);

	vid_clearScreen();
	for(i = 0; i < VID_ROWS - 3; i++)
		vid_printf("\n");
	vid_printf("Welcome to the debugging console!\nType 'help' to get a list of commands!\n\n");

	histWritePos = histReadPos = histSize = 0;
	memclear(history,sizeof(char*) * HISTORY_SIZE);

	memset(emptyLine,' ',VID_COLS - 1);
	emptyLine[VID_COLS - 1] = '\0';

	while(true) {
		vid_printf("# ");

		argv = cons_readCommand(&argc);
		vid_printf("\n");

		if(argc == 0)
			continue;
		if(strcmp(argv[0],"exit") == 0)
			break;
		if(strcmp(argv[0],"help") == 0) {
			vid_printf("Available commands:\n");
			for(i = 0; i < ARRAY_SIZE(commands); i++)
				vid_printf("	%s\n",commands[i].name);
		}
		else {
			cmd = cons_getCommand(argv[0]);
			if(cmd) {
				res = cmd->exec(argc,argv);
				if(res != 0)
					vid_printf("Executing command '%s' failed: %s (%d)\n",argv[0],strerror(res),res);
			}
			else
				vid_printf("Unknown command '%s'\n",argv[0]);
		}
	}

	for(i = 0; i < HISTORY_SIZE; i++)
		kheap_free(history[i]);
	vid_restore(backup.screen,backup.row,backup.col);
	vid_setTargets(TARGET_SCREEN | TARGET_LOG);
}

void cons_setLogEnabled(bool enabled) {
	if(enabled)
		vid_setTargets(TARGET_SCREEN | TARGET_LOG);
	else
		vid_setTargets(TARGET_SCREEN);
}

void cons_viewLines(const sLines *l) {
	sKeyEvent ev;
	s32 i,end,start = 0;
	while(true) {
		/* print lines */
		vid_clearScreen();
		for(i = start, end = MIN(l->lineCount,start + VID_ROWS - 1); i < end; i++)
			vid_printf("%s\r",l->lines[i]);
		for(; (i - start) < VID_ROWS - 1; i++)
			vid_printf("\n");
		/* print info-line */
		vid_printf("\033[co;0;7]Lines %d..%d of %d%|s\033[co]",start + 1,end,l->lineCount,
				"Navigation: up/down, pageup/-down, home/end, q=quit");
		/* wait for key */
		kb_get(&ev,KEV_PRESS,true);
		if(ev.keycode == VK_UP && start > 0)
			start--;
		else if(ev.keycode == VK_DOWN && start < l->lineCount - VID_ROWS + 1)
			start++;
		else if(ev.keycode == VK_PGUP)
			start = MAX(start - (VID_ROWS - 1),0);
		else if(ev.keycode == VK_PGDOWN)
			start = MAX(0,MIN(start + (VID_ROWS - 1),l->lineCount - VID_ROWS + 1));
		else if(ev.keycode == VK_HOME)
			start = 0;
		else if(ev.keycode == VK_END)
			start = MAX(0,l->lineCount - VID_ROWS + 1);
		else if(ev.keycode == VK_Q)
			break;
	}
}

static char **cons_readCommand(s32 *argc) {
	static char argvals[MAX_ARG_COUNT][MAX_ARG_LEN];
	static char *args[MAX_ARG_COUNT];
	s32 i = 0,j = 0;
	char *line = cons_readLine();
	args[0] = argvals[0];
	while(*line) {
		if(*line == ' ') {
			if(i > 0) {
				if(j + 1 >= MAX_ARG_COUNT)
					break;
				args[j][i] = '\0';
				j++;
				i = 0;
				args[j] = argvals[j];
			}
		}
		else if(i < MAX_ARG_LEN)
			args[j][i++] = *line;
		line++;
	}
	if(i > 0) {
		*argc = j + 1;
		args[j][i] = '\0';
	}
	else
		*argc = j;
	return args;
}

static char *cons_readLine(void) {
	static char line[VID_COLS + 1];
	u32 i = 0;
	sKeyEvent ev;
	while(true) {
		kb_get(&ev,KEV_PRESS,true);
		if(i >= sizeof(line) - 1 || ev.keycode == VK_ENTER)
			break;
		if((ev.keycode == VK_UP || ev.keycode == VK_DOWN) && histSize > 0) {
			if(ev.keycode == VK_UP) {
				if(histReadPos == 0)
					histReadPos = histSize - 1;
				else
					histReadPos = (histReadPos - 1) % histSize;
			}
			else
				histReadPos = (histReadPos + 1) % histSize;
			if(history[histReadPos]) {
				strcpy(line,history[histReadPos]);
				i = strlen(line);
				vid_printf("\r%s\r# %s",emptyLine,line);
			}
		}
		else if(ev.keycode == VK_BACKSP) {
			if(i > 0) {
				line[--i] = '\0';
				vid_printf("\r%s\r# %s",emptyLine,line);
			}
		}
		else if(isprint(ev.character)) {
			vid_printf("%c",ev.character);
			line[i++] = ev.character;
		}
	}
	line[i] = '\0';
	history[histWritePos] = strdup(line);
	histWritePos = (histWritePos + 1) % HISTORY_SIZE;
	histReadPos = histWritePos;
	histSize = MIN(histSize + 1,HISTORY_SIZE);
	return line;
}

static sCommand *cons_getCommand(const char *name) {
	u32 i;
	for(i = 0; i < ARRAY_SIZE(commands); i++) {
		if(strcmp(commands[i].name,name) == 0)
			return commands + i;
	}
	return NULL;
}
