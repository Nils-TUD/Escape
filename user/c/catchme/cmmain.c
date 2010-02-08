/**
 * $Id: ctmain.c 271 2009-08-29 15:08:30Z nasmussen $
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

#include <esc/common.h>
#include <esc/proc.h>
#include <esc/fileio.h>
#include <esc/io.h>
#include <esc/keycodes.h>
#include <esc/conf.h>
#include <esc/signals.h>
#include <messages.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errors.h>

#include "display.h"
#include "bar.h"
#include "objlist.h"
#include "object.h"

#define KEYCODE_COUNT		128
#define TICK_SLICE			10
#define UPDATE_INTERVAL		(TICK_SLICE / (1000 / (timerFreq)))
#define KEYPRESS_INTERVAL	(UPDATE_INTERVAL * 2)
#define FIRE_INTERVAL		(UPDATE_INTERVAL * 8)

static void performAction(u8 keycode);
static void tick(void);
static void sigTimer(tSig sig,u32 data);
static void qerror(const char *msg,...);
static void quit(void);

static s32 timerFreq;
static bool run = true;
static bool lastLastBreak = true;
static bool lastBreak = true;
static tFD keymap = -1;
static sKmData kmdata;
static u32 time = 0;
static u8 pressed[KEYCODE_COUNT];

int main(void) {
	/* backup screen and stop vterm to read from keyboard */
	ioctl(STDOUT_FILENO,IOCTL_VT_BACKUP,NULL,0);
	ioctl(STDOUT_FILENO,IOCTL_VT_DIS_RDKB,NULL,0);

	keymap = open("/drivers/kmmanager",IO_READ | IO_WRITE);
	if(keymap < 0)
		qerror("Unable to open keymap-driver");

	timerFreq = getConf(CONF_TIMER_FREQ);
	if(timerFreq < 0)
		qerror("Unable to get timer-frequency");

	if(!displ_init()) {
		quit();
		exit(EXIT_FAILURE);
	}
	bar_init();
	objlist_create();

	if(setSigHandler(SIG_INTRPT_TIMER,sigTimer) < 0)
		qerror("Unable to set sig-handler");

	tick();
	while(run) {
		if(wait(EV_DATA_READABLE) != ERR_INTERRUPTED) {
			while(!eof(keymap)) {
				if(read(keymap,&kmdata,sizeof(kmdata)) < 0)
					qerror("Unable to read from keymap");
				pressed[kmdata.keycode] = !kmdata.isBreak;
			}
			/*lastLastBreak = lastBreak;
			lastBreak = kmdata.isBreak;*/
		}

		if((time % UPDATE_INTERVAL) == 0)
			tick();
	}

	quit();
	return EXIT_SUCCESS;
}

static void performAction(u8 keycode) {
	switch(keycode) {
		case VK_LEFT:
			if(!lastLastBreak)
				bar_moveLeft();
			if(!lastBreak)
				bar_moveLeft();
			bar_moveLeft();
			break;
		case VK_RIGHT:
			if(!lastLastBreak)
				bar_moveRight();
			if(!lastBreak)
				bar_moveRight();
			bar_moveRight();
			break;
		case VK_SPACE:
			if((time % FIRE_INTERVAL) == 0) {
				u32 start,end;
				sObject *o;
				bar_getDim(&start,&end);
				o = obj_create(TYPE_BULLET,start + (end - start) / 2,GHEIGHT - 2,1,1,DIR_UP,4);
				objlist_add(o);
			}
			break;
		case VK_Q:
			run = false;
			break;
	}
}

static void tick(void) {
	if((time % KEYPRESS_INTERVAL) == 0) {
		u32 i;
		for(i = 0; i < KEYCODE_COUNT; i++) {
			if(pressed[i])
				performAction(i);
		}
	}
	objlist_tick();
	displ_update();
}

static void sigTimer(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	time++;
}

static void qerror(const char *msg,...) {
	va_list ap;
	quit();
	va_start(ap,msg);
	vprinte(msg,ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

static void quit(void) {
	displ_destroy();
	close(keymap);
	ioctl(STDOUT_FILENO,IOCTL_VT_RESTORE,NULL,0);
	ioctl(STDOUT_FILENO,IOCTL_VT_EN_RDKB,NULL,0);
}
