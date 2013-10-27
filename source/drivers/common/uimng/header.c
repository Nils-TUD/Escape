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

#include <esc/common.h>
#include <esc/conf.h>
#include <vbetext/vbetext.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "header.h"
#include "clients.h"

#define TITLE_BAR_COLOR		0x1F
#define CPU_USAGE_COLOR		0x70
#define ACTIVE_CLI_COLOR	0x4F
#define INACTIVE_CLI_COLOR	0x70

#define OS_TITLE			"Escape v" ESCAPE_VERSION

typedef void (*header_func)(sScreenMode *mode,char *header,uint *col,char c,char color);

static void header_readCPUUsage(void);
static void header_buildTitle(sClient *cli,gsize_t *width,gsize_t *height,bool force);

typedef struct {
	int usage;
	char str[3];
} sCPUUsage;

static sCPUUsage *cpuUsage = NULL;
static sCPUUsage *lastUsage = NULL;
static size_t cpuCount;

void header_init(void) {
	cpuCount = sysconf(CONF_CPU_COUNT);
	cpuUsage = (sCPUUsage*)calloc(cpuCount,sizeof(sCPUUsage));
	lastUsage = (sCPUUsage*)calloc(cpuCount,sizeof(sCPUUsage));
	if(!cpuUsage || !lastUsage)
		error("Not enough memory for cpu array");
	/* until the first update... */
	for(size_t i = 0; i < cpuCount; ++i)
		snprintf(cpuUsage[i].str,sizeof(cpuUsage[i].str),"%2d",0);
}

size_t header_getSize(sScreenMode *mode,int type,gpos_t x) {
	if(type == VID_MODE_TYPE_TUI)
		return x * 2;
	/* always update the whole width because it simplifies the copy it shouldn't be much slower
	 * than doing a loop with 1 memcpy per line */
	return x * (mode->bitsPerPixel / 8) * FONT_HEIGHT;
}

size_t header_getHeight(int type) {
	if(type == VID_MODE_TYPE_TUI)
		return 1;
	return FONT_HEIGHT;
}

static void header_doUpdate(sClient *cli,gsize_t width,gsize_t height) {
	if(cli->type == VID_MODE_TYPE_TUI || width == cli->screenMode->width)
		memcpy(cli->screenShm,cli->header,header_getSize(cli->screenMode,cli->type,width));
	else {
		size_t off = 0;
		size_t len = width * (cli->screenMode->bitsPerPixel / 8);
		size_t inc = cli->screenMode->width * (cli->screenMode->bitsPerPixel / 8);
		for(gsize_t y = 0; y < height; y++) {
			memcpy(cli->screenShm + off,cli->header + off,len);
			off += inc;
		}
	}
}

bool header_update(sClient *cli,gsize_t *width,gsize_t *height) {
	*width = *height = 0;
	header_buildTitle(cli,width,height,true);
	if(*width > 0)
		header_doUpdate(cli,*width,*height);
	return *width > 0;
}

bool header_rebuild(sClient *cli,gsize_t *width,gsize_t *height) {
	header_readCPUUsage();
	*width = *height = 0;
	header_buildTitle(cli,width,height,false);
	if(*width > 0)
		header_doUpdate(cli,*width,*height);
	return *width > 0;
}

static void header_readCPUUsage(void) {
	FILE *f = fopen("/system/cpu","r");
	if(f) {
		while(!feof(f)) {
			int no;
			uint64_t total,used;
			/* read CPU number */
			ssize_t res = fscanf(f,"%*s %d:",&no);
			assert(res == 1);
			/* read total cycles and used cycles */
			res = fscanf(f,"%*s%Lu%*s",&total);
			assert(res == 1);
			res = fscanf(f,"%*s%Lu%*s",&used);
			assert(res == 1);

			cpuUsage[no].usage = 100 * (used / (double)total);
			cpuUsage[no].usage = MAX(0,MIN(cpuUsage[no].usage,99));
			snprintf(cpuUsage[no].str,sizeof(cpuUsage[no].str),"%2d",cpuUsage[no].usage);

			/* read until the next cpu */
			assert(fgetc(f) == '\n');
			while(!feof(f) && fgetc(f) == '\t') {
				while(!feof(f) && fgetc(f) != '\n')
					;
			}
		}
		fclose(f);
	}
}

static void header_putcTUI(A_UNUSED sScreenMode *mode,char *header,uint *col,char c,char color) {
	header[*col * 2] = c;
	header[*col * 2 + 1] = color;
	(*col)++;
}

static void header_putcGUI(sScreenMode *mode,char *header,uint *col,char c,char color) {
#ifdef __i386__
	vbet_drawChar(mode,(uint8_t*)header,*col,0,c,color);
#endif
	(*col)++;
}

static void header_makeDirty(sClient *cli,size_t cols,gsize_t *width,gsize_t *height) {
	if(cli->type == VID_MODE_TYPE_TUI) {
		*width = cols;
		*height = 1;
	}
	else {
		*width = cols * FONT_WIDTH;
		*height = FONT_HEIGHT;
	}
}

static void header_buildTitle(sClient *cli,gsize_t *width,gsize_t *height,bool force) {
	if(!cli->screenMode)
		return;

	uint col = 0;
	header_func func = cli->type == VID_MODE_TYPE_GUI ? header_putcGUI : header_putcTUI;

	/* print CPU usage */
	bool changed = false;
	size_t maxCpus = MIN(cpuCount,8);
	for(size_t i = 0; i < maxCpus; i++) {
		if(force || cpuUsage[i].usage != lastUsage[i].usage) {
			func(cli->screenMode,cli->header,&col,' ',CPU_USAGE_COLOR);
			func(cli->screenMode,cli->header,&col,cpuUsage[i].str[0],CPU_USAGE_COLOR);
			func(cli->screenMode,cli->header,&col,cpuUsage[i].str[1],CPU_USAGE_COLOR);
			changed = true;
		}
		else
			col += 3;
	}
	memcpy(lastUsage,cpuUsage,cpuCount * sizeof(sCPUUsage));
	if(force || changed) {
		if(cpuCount > maxCpus) {
			func(cli->screenMode,cli->header,&col,' ',CPU_USAGE_COLOR);
			func(cli->screenMode,cli->header,&col,'.',CPU_USAGE_COLOR);
			func(cli->screenMode,cli->header,&col,'.',CPU_USAGE_COLOR);
		}
		header_makeDirty(cli,col,width,height);
	}

	if(force) {
		/* print sep */
		func(cli->screenMode,cli->header,&col,0xC7,TITLE_BAR_COLOR);

		/* fill with '-' */
		while(col < cli->screenMode->cols)
			func(cli->screenMode,cli->header,&col,0xC4,TITLE_BAR_COLOR);

		/* print OS name */
		size_t len = strlen(OS_TITLE);
		col = (cli->screenMode->cols - len) / 2;
		for(size_t i = 0; i < len; i++)
			func(cli->screenMode,cli->header,&col,OS_TITLE[i],TITLE_BAR_COLOR);

		/* print clients */
		size_t clients = cli_getCount();
		col = cli->screenMode->cols - clients * 3 - 1;
		func(cli->screenMode,cli->header,&col,0xB6,TITLE_BAR_COLOR);
		for(size_t i = 0; i < MAX_CLIENTS; ++i) {
			if(!cli_exists(i))
				continue;

			char color = cli_isActive(i) ? ACTIVE_CLI_COLOR : INACTIVE_CLI_COLOR;
			func(cli->screenMode,cli->header,&col,' ',color);
			func(cli->screenMode,cli->header,&col,' ',color);
			func(cli->screenMode,cli->header,&col,'0' + i,color);
		}

		header_makeDirty(cli,cli->screenMode->cols,width,height);
	}
}
