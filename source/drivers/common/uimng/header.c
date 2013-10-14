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
static void header_buildTitle(sClient *cli);

typedef struct {
	int usage;
	char str[3];
} sCPUUsage;

static sCPUUsage *cpuUsage = NULL;
static size_t cpuCount;

void header_init(void) {
	cpuCount = sysconf(CONF_CPU_COUNT);
	cpuUsage = (sCPUUsage*)calloc(cpuCount,sizeof(sCPUUsage));
	if(!cpuUsage)
		error("Not enough memory for cpu array");
	/* until the first update... */
	for(size_t i = 0; i < cpuCount; ++i)
		snprintf(cpuUsage[i].str,sizeof(cpuUsage[i].str),"%2d",0);
}

size_t header_getSize(sScreenMode *mode,int type) {
	if(type == VID_MODE_TYPE_TUI)
		return mode->cols * 2;
	return mode->width * (mode->bitsPerPixel / 8) * FONT_HEIGHT;
}

size_t header_getHeight(int type) {
	if(type == VID_MODE_TYPE_TUI)
		return 1;
	return FONT_HEIGHT;
}

static bool header_setSize(sClient *cli,gsize_t *width,gsize_t *height) {
	if(!cli->screenMode)
		return false;
	if(width && height) {
		if(cli->type == VID_MODE_TYPE_TUI) {
			*width = cli->screenMode->cols;
			*height = 1;
		}
		else {
			*width = cli->screenMode->width;
			*height = FONT_HEIGHT;
		}
	}
	return true;
}

bool header_update(sClient *cli,gsize_t *width,gsize_t *height) {
	if(!header_setSize(cli,width,height))
		return false;

	header_buildTitle(cli);
	memcpy(cli->screenShm,cli->header,header_getSize(cli->screenMode,cli->type));
	return true;
}

bool header_rebuild(sClient *cli,gsize_t *width,gsize_t *height) {
	if(!header_setSize(cli,width,height))
		return false;

	header_readCPUUsage();
	header_buildTitle(cli);
	memcpy(cli->screenShm,cli->header,header_getSize(cli->screenMode,cli->type));
	return true;
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

static void header_buildTitle(sClient *cli) {
	uint col = 0;
	header_func func = cli->type == VID_MODE_TYPE_GUI ? header_putcGUI : header_putcTUI;

	/* print CPU usage */
	size_t maxCpus = MIN(cpuCount,8);
	for(size_t i = 0; i < maxCpus; i++) {
		func(cli->screenMode,cli->header,&col,' ',CPU_USAGE_COLOR);
		func(cli->screenMode,cli->header,&col,cpuUsage[i].str[0],CPU_USAGE_COLOR);
		func(cli->screenMode,cli->header,&col,cpuUsage[i].str[1],CPU_USAGE_COLOR);
	}
	if(cpuCount > maxCpus) {
		func(cli->screenMode,cli->header,&col,' ',CPU_USAGE_COLOR);
		func(cli->screenMode,cli->header,&col,'.',CPU_USAGE_COLOR);
		func(cli->screenMode,cli->header,&col,'.',CPU_USAGE_COLOR);
	}

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
}
