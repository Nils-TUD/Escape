/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/dbg/cmd/mem.h>
#include <sys/dbg/cmd/panic.h>
#include <sys/dbg/cmd/dump.h>
#include <sys/dbg/kb.h>
#include <sys/mem/cache.h>
#include <sys/task/smp.h>
#include <sys/video.h>
#include <esc/keycodes.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define HISTORY_SIZE	32
#define MAX_ARG_COUNT	5
#define MAX_ARG_LEN		32
#define MAX_SEARCH_LEN	16

typedef int (*fCommand)(size_t argc,char **argv);
typedef struct {
	char name[MAX_ARG_LEN];
	fCommand exec;
} sCommand;

static uintptr_t cons_getMaxAddr(uintptr_t end);
static uintptr_t cons_incrAddr(uintptr_t end,uintptr_t addr,size_t amount);
static uintptr_t cons_decrAddr(uintptr_t addr,size_t amount);
static uint8_t *cons_loadLine(void *data,uintptr_t addr);
static const char *cons_getLineInfo(void *data,uintptr_t addr);
static bool cons_lineMatches(void *data,uintptr_t addr,const char *search,size_t searchlen);
static void cons_displayLine(void *data,uintptr_t addr,uint8_t *bytes);
static uintptr_t cons_gotoAddr(void *data,const char *gotoAddr);
static void cons_display(const sNaviBackend *backend,void *data,
                         const char *searchInfo,const char *search,int searchMode,uintptr_t *addr);
static uint8_t cons_charToInt(char c);
static void cons_convSearch(const char *src,char *dst,size_t len);
static char **cons_readCommand(size_t *argc);
static char *cons_readLine(void);
static sCommand *cons_getCommand(const char *name);

static size_t histWritePos = 0;
static size_t histReadPos = 0;
static size_t histSize = 0;
static char *history[HISTORY_SIZE];
static char emptyLine[VID_COLS];
static sScreenBackup backup;
static sCommand commands[] = {
	{"help",NULL},
	{"exit",NULL},
	{"file",cons_cmd_file},
	{"dump",cons_cmd_dump},
	{"view",cons_cmd_view},
	{"log",cons_cmd_log},
	{"ls",cons_cmd_ls},
	{"mem",cons_cmd_mem},
	{"panic",cons_cmd_panic},
};

void cons_start(void) {
	size_t i,argc;
	int res;
	sCommand *cmd;
	char **argv;

	/* first, pause all other CPUs to ensure that we're alone */
	smp_pauseOthers();

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
			vid_printf("\n");
			vid_printf("All commands use a viewer, that supports the following key-strokes:\n");
			vid_printf(" - up/down/pageup/pagedown/home/end: navigate through the data\n");
			vid_printf(" - left/right: to previous/next search result\n");
			vid_printf(" - esc: quit\n");
		}
		else {
			cmd = cons_getCommand(argv[0]);
			if(cmd) {
				res = cmd->exec(argc,argv);
				if(res != 0)
					vid_printf("Executing command '%s' failed: %s (%d)\n",argv[0],strerror(-res),res);
			}
			else
				vid_printf("Unknown command '%s'\n",argv[0]);
		}
	}

	for(i = 0; i < HISTORY_SIZE; i++)
		cache_free(history[i]);
	vid_restore(backup.screen,backup.row,backup.col);
	vid_setTargets(TARGET_LOG);

	/* now let the other CPUs continue */
	smp_resumeOthers();
}

void cons_setLogEnabled(bool enabled) {
	if(enabled)
		vid_setTargets(TARGET_SCREEN | TARGET_LOG);
	else
		vid_setTargets(TARGET_SCREEN);
}

void cons_dumpLine(uintptr_t addr,uint8_t *bytes) {
	size_t i;
	vid_printf("%p: ",addr);
	if(bytes) {
		for(i = 0; i < BYTES_PER_LINE; i++)
			vid_printf("%02X ",bytes[i]);
		vid_printf("| ");
		for(i = 0; i < BYTES_PER_LINE; i++) {
			if(isprint(bytes[i]) && bytes[i] < 0x80 && !isspace(bytes[i]))
				vid_printf("%c",bytes[i]);
			else
				vid_printf(".");
		}
	}
	else {
		for(i = 0; i < BYTES_PER_LINE; i++)
			vid_printf("?? ");
		vid_printf("| ");
		for(i = 0; i < BYTES_PER_LINE; i++)
			vid_printf(".");
	}
	vid_printf("\n");
}

void cons_navigation(const sNaviBackend *backend,void *data) {
	/* the maximum has to be BYTES_PER_LINE because cons_display assumes it atm */
	char search[BYTES_PER_LINE + 1] = "";
	char searchClone[BYTES_PER_LINE + 1] = "";
	int searchMode = SEARCH_NONE;
	uintptr_t addr = backend->startPos;
	size_t searchPos = 0;
	sKeyEvent ev;
	bool run = true;
	assert((backend->maxPos & (BYTES_PER_LINE - 1)) == 0);
	while(run) {
		assert((addr & (BYTES_PER_LINE - 1)) == 0);
		cons_convSearch(search,searchClone,searchPos);
		cons_display(backend,data,search,searchClone,searchMode,&addr);
		searchMode = SEARCH_NONE;
		kb_get(&ev,KEV_PRESS,true);
		switch(ev.keycode) {
			case VK_UP:
				addr = cons_decrAddr(addr,BYTES_PER_LINE);
				break;
			case VK_DOWN:
				addr = cons_incrAddr(backend->maxPos,addr,BYTES_PER_LINE);
				break;
			case VK_PGUP:
				addr = cons_decrAddr(addr,BYTES_PER_LINE * SCROLL_LINES);
				break;
			case VK_PGDOWN:
				addr = cons_incrAddr(backend->maxPos,addr,BYTES_PER_LINE * SCROLL_LINES);
				break;
			case VK_HOME:
				addr = backend->startPos;
				break;
			case VK_END:
				if(backend->maxPos < BYTES_PER_LINE * SCROLL_LINES)
					addr = 0;
				else
					addr = backend->maxPos - BYTES_PER_LINE * SCROLL_LINES;
				break;
			case VK_BACKSP:
				if(searchPos > 0)
					search[--searchPos] = '\0';
				break;
			case VK_ENTER:
				if(searchPos > 0) {
					addr = backend->gotoAddr(data,search);
					searchPos = 0;
					search[0] = '\0';
				}
				break;
			case VK_LEFT:
				if(searchPos > 0)
					searchMode = SEARCH_BACKWARDS;
				break;
			case VK_RIGHT:
				if(searchPos > 0)
					searchMode = SEARCH_FORWARD;
				break;
			case VK_ESC:
			case VK_Q:
				run = false;
				break;
			default:
				if(isprint(ev.character)) {
					if(searchPos < sizeof(search) - 1) {
						search[searchPos++] = tolower(ev.character);
						search[searchPos] = '\0';
					}
				}
				break;
		}
	}
}

void cons_viewLines(const sLines *l) {
	sNaviBackend backend;
	backend.startPos = 0;
	backend.maxPos = l->lineCount * BYTES_PER_LINE;
	backend.loadLine = cons_loadLine;
	backend.getInfo = cons_getLineInfo;
	backend.lineMatches = cons_lineMatches;
	backend.displayLine = cons_displayLine;
	backend.gotoAddr = cons_gotoAddr;
	cons_navigation(&backend,(void*)l);
}

bool cons_multiLineMatches(const sNaviBackend *backend,void *data,uintptr_t addr,
                           const char *search,size_t searchlen) {
	uint8_t *bytes = backend->loadLine(data,addr);
	if(bytes && searchlen > 0) {
		size_t i;
		for(i = 0; i < BYTES_PER_LINE; i++) {
			size_t len = MIN(searchlen,BYTES_PER_LINE - i);
			if(strncasecmp(search,(char*)bytes + i,len) == 0) {
				if(len < searchlen) {
					uint8_t *nextBytes = backend->loadLine(data,addr + BYTES_PER_LINE);
					if(nextBytes && strncasecmp(search + len,(char*)nextBytes,searchlen - len) == 0)
						return true;
					bytes = backend->loadLine(data,addr);
				}
				else
					return true;
			}
		}
	}
	return false;
}

static uintptr_t cons_getMaxAddr(uintptr_t end) {
	uintptr_t max = end - BYTES_PER_LINE * SCROLL_LINES;
	return max > end ? 0 : max;
}

static uintptr_t cons_incrAddr(uintptr_t end,uintptr_t addr,size_t amount) {
	uintptr_t max = cons_getMaxAddr(end);
	if(addr + amount < addr || addr + amount > max)
		return max;
	return addr + amount;
}

static uintptr_t cons_decrAddr(uintptr_t addr,size_t amount) {
	if(addr - amount > addr)
		return 0;
	return addr - amount;
}

static uint8_t *cons_loadLine(void *data,uintptr_t addr) {
	const sLines *l = (const sLines*)data;
	if(addr / BYTES_PER_LINE < l->lineCount)
		return (uint8_t*)l->lines[addr / BYTES_PER_LINE];
	return NULL;
}

static const char *cons_getLineInfo(void *data,uintptr_t addr) {
	static char tmp[64];
	sStringBuffer buf;
	const sLines *l = (const sLines*)data;
	size_t start = addr / BYTES_PER_LINE;
	size_t end = MIN(l->lineCount,start + VID_ROWS - 1);

	buf.len = 0;
	buf.dynamic = false;
	buf.size = sizeof(tmp);
	buf.str = tmp;
	prf_sprintf(&buf,"Lines %zu..%zu of %zu",start + 1,end,l->lineCount);
	if(l->lineSize == (size_t)-1)
		prf_sprintf(&buf," (incomplete)");
	return buf.str;
}

static bool cons_lineMatches(void *data,uintptr_t addr,const char *search,A_UNUSED size_t searchlen) {
	uint8_t *bytes = cons_loadLine(data,addr);
	return bytes != NULL && strcasestr((char*)bytes,search) != NULL;
}

static void cons_displayLine(A_UNUSED void *data,A_UNUSED uintptr_t addr,uint8_t *bytes) {
	if(bytes)
		vid_printf("%s\r",bytes);
	else
		vid_printf("%*s\n",VID_COLS - 1,"");
}

static uintptr_t cons_gotoAddr(A_UNUSED void *data,const char *gotoAddr) {
	return (strtoul(gotoAddr,NULL,10) - 1) * BYTES_PER_LINE;
}

static void cons_display(const sNaviBackend *backend,void *data,
                         const char *searchInfo,const char *search,int searchMode,uintptr_t *addr) {
	static char states[] = {'|','/','-','\\','|','/','-'};
	const char *info;
	uint y;
	bool found = true;

	uintptr_t startAddr = *addr;
	if(searchMode != SEARCH_NONE) {
		long count = 0;
		sKeyEvent ev;
		int state = 0;
		size_t searchlen = strlen(search);
		found = false;
		for(; !found && ((searchMode == SEARCH_FORWARD && startAddr < cons_getMaxAddr(backend->maxPos)) ||
			  (searchMode == SEARCH_BACKWARDS && startAddr >= BYTES_PER_LINE)); count++) {
			if(count % 100 == 0) {
				vid_goto(VID_ROWS - 1,0);
				if(kb_get(&ev,KEV_PRESS,false) && ev.keycode == VK_S) {
					*addr = startAddr;
					break;
				}
				vid_printf("\033[co;0;7]%c\033[co]",states[state++ % ARRAY_SIZE(states)]);
			}

			startAddr += searchMode == SEARCH_FORWARD ? BYTES_PER_LINE : -BYTES_PER_LINE;
			if(backend->lineMatches(data,startAddr,search,searchlen)) {
				found = true;
				break;
			}
		}
		if(!found)
			startAddr = *addr;
	}

	if(startAddr > cons_getMaxAddr(backend->maxPos))
		startAddr = cons_getMaxAddr(backend->maxPos) & ~(BYTES_PER_LINE - 1);
	if(found)
		*addr = startAddr;

	info = backend->getInfo(data,startAddr);

	vid_goto(0,0);
	for(y = 0; y < VID_ROWS - 1; y++) {
		uint8_t *bytes = backend->loadLine(data,startAddr);
		bool matches = backend->lineMatches(data,startAddr,search,strlen(search));
		if(matches)
			vid_printf("\033[co;0;2]");
		backend->displayLine(data,startAddr,bytes);
		if(matches)
			vid_printf("\033[co]");
		startAddr += BYTES_PER_LINE;
	}

	if(found)
		vid_printf("\033[co;0;7]- Search/Goto: %s%|s\033[co]",searchInfo,info);
	else
		vid_printf("\033[co;0;7]- Search/Goto: \033[co;4;7]%s\033[co;0;7]%|s\033[co]",searchInfo,info);
}

static uint8_t cons_charToInt(char c) {
	if(c >= 'a' && c <= 'f')
		return 10 + c - 'a';
	if(c >= 'A' && c <= 'F')
		return 10 + c - 'A';
	if(c >= '0' && c <= '9')
		return c - '0';
	return 0;
}

static void cons_convSearch(const char *src,char *dst,size_t len) {
	size_t i,j;
	for(i = 0, j = 0; i < len; ++i) {
		if(src[i] == '\\' && len > i + 2) {
			dst[j++] = (cons_charToInt(src[i + 1]) << 4) | cons_charToInt(src[i + 2]);
			i += 2;
		}
		else if(src[i] != '\\')
			dst[j++] = src[i];
	}
	dst[j] = '\0';
}

static char **cons_readCommand(size_t *argc) {
	static char argvals[MAX_ARG_COUNT][MAX_ARG_LEN];
	static char *args[MAX_ARG_COUNT];
	size_t i = 0,j = 0;
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
	size_t i = 0;
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
	size_t i;
	for(i = 0; i < ARRAY_SIZE(commands); i++) {
		if(strcmp(commands[i].name,name) == 0)
			return commands + i;
	}
	return NULL;
}
