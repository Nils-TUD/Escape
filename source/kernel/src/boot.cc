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
#include <sys/mem/paging.h>
#include <sys/dbg/kb.h>
#include <sys/boot.h>
#include <sys/log.h>
#include <sys/video.h>

#define MAX_ARG_COUNT	8
#define MAX_ARG_LEN		64
#define KERNEL_PERCENT	40
#define BAR_HEIGHT		1
#define BAR_WIDTH		60
#define BAR_PAD			10
#define BAR_TEXT_PAD	1
#define BAR_PADX		((VID_COLS - BAR_WIDTH) / 2)
#define BAR_PADY		((VID_ROWS / 2) - ((BAR_HEIGHT + 2) / 2) - 1)

static void drawProgressBar(void);
extern void (*CTORS_BEGIN)();
extern void (*CTORS_END)();

static size_t finished = 0;

void boot_start(sBootInfo *info) {
	size_t i;
	for(void (**func)() = &CTORS_BEGIN; func != &CTORS_END; func++)
		(*func)();

	boot_arch_start(info);
	Video::setTargets(Video::SCREEN);
	drawProgressBar();
	for(i = 0; i < bootTaskList.count; i++) {
		const sBootTask *task = bootTaskList.tasks + i;
		Log::printf("%s",task->name);
		boot_taskStarted(task->name);
		task->execute();
		Log::printf("%|s","done");
		boot_taskFinished();
	}

	Log::printf("%d free frames (%d KiB)\n",PhysMem::getFreeFrames(PhysMem::CONT | PhysMem::DEF),
			PhysMem::getFreeFrames(PhysMem::CONT | PhysMem::DEF) * PAGE_SIZE / K);
}

void boot_taskStarted(const char *text) {
	Video::goTo(BAR_PADY + BAR_HEIGHT + 2 + BAR_TEXT_PAD,BAR_PADX);
	Video::printf("%-*s",VID_COLS - BAR_PADX * 2,text);
}

void boot_taskFinished(void) {
	const uint width = BAR_WIDTH - 3;
	size_t total;
	uint percent,filled;
	finished++;
	total = bootTaskList.count + bootTaskList.moduleCount;
	percent = KERNEL_PERCENT * ((float)finished / total);
	filled = percent == 0 ? 0 : (uint)(width / (100.0 / percent));
	Video::goTo(BAR_PADY + 1,BAR_PADX + 1);
	if(filled)
		Video::printf("\033[co;0;7]%*s\033[co]",filled," ");
	if(width - filled)
		Video::printf("%*s",width - filled," ");
}

const char **boot_parseArgs(const char *line,int *argc) {
	static char argvals[MAX_ARG_COUNT][MAX_ARG_LEN];
	static char *args[MAX_ARG_COUNT];
	size_t i = 0,j = 0;
	args[0] = argvals[0];
	while(*line) {
		if(*line == ' ') {
			if(args[j][0]) {
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
	*argc = j + 1;
	args[j][i] = '\0';
	return (const char**)args;
}

static void drawProgressBar(void) {
	ushort x,y;
	/* top */
	Video::goTo(BAR_PADY,BAR_PADX);
	Video::printf("\xC9");
	for(x = 1; x < BAR_WIDTH - 2; x++)
		Video::printf("\xCD");
	Video::printf("\xBB");
	/* left and right */
	for(y = 0; y < BAR_HEIGHT; y++) {
		Video::goTo(BAR_PADY + 1 + y,BAR_PADX);
		Video::printf("\xBA");
		Video::goTo(BAR_PADY + 1 + y,VID_COLS - (BAR_PADX + 2));
		Video::printf("\xBA");
	}
	/* bottom */
	Video::goTo(BAR_PADY + BAR_HEIGHT + 1,BAR_PADX);
	Video::printf("\xC8");
	for(x = 1; x < VID_COLS - (BAR_PADX * 2 + 2); x++)
		Video::printf("\xCD");
	Video::printf("\xBC");
}
