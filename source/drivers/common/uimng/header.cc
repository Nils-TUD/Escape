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
#include <esc/conf.h>
#include <vbetext/vbetext.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "header.h"
#include "clients.h"

const char *Header::OS_TITLE = "Escape v" ESCAPE_VERSION;
Header::CPUUsage *Header::_cpuUsage = NULL;
Header::CPUUsage *Header::_lastUsage = NULL;
size_t Header::_lastClientCount = 0;
size_t Header::_cpuCount;

void Header::init() {
	_cpuCount = sysconf(CONF_CPU_COUNT);
	_cpuUsage = new CPUUsage[_cpuCount]();
	_lastUsage = new CPUUsage[_cpuCount]();
	/* until the first update... */
	for(size_t i = 0; i < _cpuCount; ++i)
		snprintf(_cpuUsage[i].str,sizeof(_cpuUsage[i].str),"%2d",0);
}

void Header::doUpdate(UIClient *cli,gsize_t width,gsize_t height) {
	if(cli->type() == ipc::Screen::MODE_TYPE_TUI || width == cli->fb()->mode().width)
		memcpy(cli->fb()->addr(),cli->header(),getSize(cli->fb()->mode(),cli->type(),width));
	else {
		size_t off = 0;
		size_t len = width * (cli->fb()->mode().bitsPerPixel / 8);
		size_t inc = cli->fb()->mode().width * (cli->fb()->mode().bitsPerPixel / 8);
		for(gsize_t y = 0; y < height; y++) {
			memcpy(cli->fb()->addr() + off,cli->header() + off,len);
			off += inc;
		}
	}
}

bool Header::update(UIClient *cli,gsize_t *width,gsize_t *height) {
	*width = *height = 0;
	buildTitle(cli,width,height,true);
	if(*width > 0)
		doUpdate(cli,*width,*height);
	return *width > 0;
}

bool Header::rebuild(UIClient *cli,gsize_t *width,gsize_t *height) {
	readCPUUsage();
	*width = *height = 0;
	buildTitle(cli,width,height,false);
	if(*width > 0)
		doUpdate(cli,*width,*height);
	return *width > 0;
}

void Header::readCPUUsage(void) {
	FILE *f = fopen("/system/cpu","r");
	if(f) {
		while(!feof(f)) {
			int no;
			uint64_t total,used;
			/* read CPU number */
			if(fscanf(f,"%*s %d:",&no) == 1 &&
				fscanf(f,"%*s%Lu%*s",&total) == 1 &&
				fscanf(f,"%*s%Lu%*s",&used) == 1) {
				_cpuUsage[no].usage = 100 * (used / (double)total);
				_cpuUsage[no].usage = MAX(0,MIN(_cpuUsage[no].usage,99));
				snprintf(_cpuUsage[no].str,sizeof(_cpuUsage[no].str),"%2d",_cpuUsage[no].usage);
			}

			/* read until the next cpu */
			if(fgetc(f) != '\n')
				break;
			while(!feof(f) && fgetc(f) == '\t') {
				while(!feof(f) && fgetc(f) != '\n')
					;
			}
		}
		fclose(f);
	}
}

void Header::putcTUI(const ipc::Screen::Mode &,char *header,uint *col,char c,char color) {
	header[*col * 2] = c;
	header[*col * 2 + 1] = color;
	(*col)++;
}

void Header::putcGUI(const ipc::Screen::Mode &mode,char *header,uint *col,char c,char color) {
#ifdef __i386__
	vbet_drawChar(mode,(uint8_t*)header,*col,0,c,color);
#endif
	(*col)++;
}

void Header::makeDirty(UIClient *cli,size_t cols,gsize_t *width,gsize_t *height) {
	if(cli->type() == ipc::Screen::MODE_TYPE_TUI) {
		*width = cols;
		*height = 1;
	}
	else {
		*width = cols * FONT_WIDTH;
		*height = FONT_HEIGHT;
	}
}

void Header::buildTitle(UIClient *cli,gsize_t *width,gsize_t *height,bool force) {
	if(!cli->fb())
		return;

	uint col = 0;
	putc_func func = cli->type() == ipc::Screen::MODE_TYPE_GUI ? putcGUI : putcTUI;
	const ipc::Screen::Mode &mode = cli->fb()->mode();

	/* print CPU usage */
	bool changed = false;
	size_t maxCpus = MIN(_cpuCount,8);
	for(size_t i = 0; i < maxCpus; i++) {
		if(force || _cpuUsage[i].usage != _lastUsage[i].usage) {
			func(mode,cli->header(),&col,' ',CPU_USAGE_COLOR);
			func(mode,cli->header(),&col,_cpuUsage[i].str[0],CPU_USAGE_COLOR);
			func(mode,cli->header(),&col,_cpuUsage[i].str[1],CPU_USAGE_COLOR);
			changed = true;
		}
		else
			col += 3;
	}
	memcpy(_lastUsage,_cpuUsage,_cpuCount * sizeof(CPUUsage));
	if(force || changed) {
		if(_cpuCount > maxCpus) {
			func(mode,cli->header(),&col,' ',CPU_USAGE_COLOR);
			func(mode,cli->header(),&col,'.',CPU_USAGE_COLOR);
			func(mode,cli->header(),&col,'.',CPU_USAGE_COLOR);
		}
		makeDirty(cli,col,width,height);
	}

	size_t clients = UIClient::count();
	if(force || clients != _lastClientCount) {
		/* print sep */
		func(mode,cli->header(),&col,0xC7,TITLE_BAR_COLOR);

		/* fill with '-' */
		while(col < mode.cols)
			func(mode,cli->header(),&col,0xC4,TITLE_BAR_COLOR);

		/* print OS name */
		size_t len = strlen(OS_TITLE);
		col = (mode.cols - len) / 2;
		for(size_t i = 0; i < len; i++)
			func(mode,cli->header(),&col,OS_TITLE[i],TITLE_BAR_COLOR);

		/* print clients */
		col = mode.cols - clients * 3 - 1;
		func(mode,cli->header(),&col,0xB6,TITLE_BAR_COLOR);
		for(size_t i = 0; i < UIClient::MAX_CLIENTS; ++i) {
			if(!UIClient::exists(i))
				continue;

			char color = UIClient::isActive(i) ? ACTIVE_CLI_COLOR : INACTIVE_CLI_COLOR;
			func(mode,cli->header(),&col,' ',color);
			func(mode,cli->header(),&col,' ',color);
			func(mode,cli->header(),&col,'0' + i,color);
		}

		makeDirty(cli,mode.cols,width,height);
		_lastClientCount = clients;
	}
}
