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

#include <sys/common.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/virtmem.h>
#include <sys/dbg/kb.h>
#include <sys/boot.h>
#include <sys/log.h>
#include <sys/video.h>
#include <sys/config.h>

#define MAX_ARG_COUNT	8
#define MAX_ARG_LEN		64
#define KERNEL_PERCENT	40
#define BAR_HEIGHT		1
#define BAR_WIDTH		60
#define BAR_PAD			10
#define BAR_TEXT_PAD	1
#define BAR_PADX		((VID_COLS - BAR_WIDTH) / 2)
#define BAR_PADY		((VID_ROWS / 2) - ((BAR_HEIGHT + 2) / 2) - 1)

extern void (*CTORS_BEGIN)();
extern void (*CTORS_END)();

static size_t finished = 0;

void Boot::start(BootInfo *info) {
	for(void (**func)() = &CTORS_BEGIN; func != &CTORS_END; func++)
		(*func)();

	archStart(info);
	drawProgressBar();
	for(size_t i = 0; i < taskList.count; i++) {
		const BootTask *task = taskList.tasks + i;
		Log::get().writef("%s\n",task->name);
		taskStarted(task->name);
		task->execute();
		taskFinished();
	}

	Log::get().writef("%d free frames (%d KiB)\n",PhysMem::getFreeFrames(PhysMem::CONT | PhysMem::DEF),
			PhysMem::getFreeFrames(PhysMem::CONT | PhysMem::DEF) * PAGE_SIZE / K);

	if(DISABLE_DEMLOAD)
		Log::get().writef("Warning: demand loading is disabled!\n");
}

void Boot::taskStarted(const char *text) {
	if(!Config::get(Config::LOG_TO_VGA)) {
		Video::get().goTo(BAR_PADY + BAR_HEIGHT + 2 + BAR_TEXT_PAD,BAR_PADX);
		Video::get().writef("%-*s",VID_COLS - BAR_PADX * 2,text);
	}
}

void Boot::taskFinished() {
	if(!Config::get(Config::LOG_TO_VGA)) {
		const uint width = BAR_WIDTH - 3;
		finished++;
		size_t total = taskList.count + taskList.moduleCount;
		uint percent = (KERNEL_PERCENT * finished) / total;
		uint filled = (width * percent) / 100;
		Video::get().goTo(BAR_PADY + 1,BAR_PADX + 1);
		if(filled)
			Video::get().writef("\033[co;0;7]%*s\033[co]",filled," ");
		if(width - filled)
			Video::get().writef("%*s",width - filled," ");
	}
}

const char **Boot::parseArgs(const char *line,int *argc) {
	static char argvals[MAX_ARG_COUNT][MAX_ARG_LEN];
	static char *args[MAX_ARG_COUNT];
	size_t i = 0,j = 0;
	args[0] = argvals[0];
	while(*line) {
		if(*line == ' ') {
			if(args[j][0]) {
				if(j + 2 >= MAX_ARG_COUNT)
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
	*argc = j + 1;
	args[j][i] = '\0';
	args[j + 1] = NULL;
	return (const char**)args;
}

void Boot::drawProgressBar() {
	if(!Config::get(Config::LOG_TO_VGA)) {
		Video &vid = Video::get();
		/* top */
		vid.goTo(BAR_PADY,BAR_PADX);
		vid.writef("\xC9");
		for(ushort x = 1; x < BAR_WIDTH - 2; x++)
			vid.writef("\xCD");
		vid.writef("\xBB");
		/* left and right */
		for(ushort y = 0; y < BAR_HEIGHT; y++) {
			vid.goTo(BAR_PADY + 1 + y,BAR_PADX);
			vid.writef("\xBA");
			vid.goTo(BAR_PADY + 1 + y,VID_COLS - (BAR_PADX + 2));
			vid.writef("\xBA");
		}
		/* bottom */
		vid.goTo(BAR_PADY + BAR_HEIGHT + 1,BAR_PADX);
		vid.writef("\xC8");
		for(ushort x = 1; x < VID_COLS - (BAR_PADX * 2 + 2); x++)
			vid.writef("\xCD");
		vid.writef("\xBC");
	}
}
