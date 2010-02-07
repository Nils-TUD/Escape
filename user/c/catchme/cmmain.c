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
#include <esc/signals.h>
#include <messages.h>
#include <stdlib.h>
#include <stdarg.h>

#define WIDTH				(size.width)
#define HEIGHT				(size.height)
#define VIDEO_DRIVER		"/drivers/video"

#define TIMER_INTERVAL		20
#define UPDATE_INTERVAL		20

static void tick(void);
static void sigTimer(tSig sig,u32 data);
static void qerror(const char *msg,...);
static void quit(void);

static tFD video = -1;
static tFD keymap = -1;
static char *buffer = NULL;
static sKmData kmdata;
static sIoCtlSize size;

static u32 time = 0;
static u32 xpos = 1, ypos = 1;

int main(void) {
	u32 x,y;
	/* backup screen and stop vterm to read from keyboard */
	ioctl(STDOUT_FILENO,IOCTL_VT_BACKUP,NULL,0);
	ioctl(STDOUT_FILENO,IOCTL_VT_DIS_RDKB,NULL,0);

	video = open(VIDEO_DRIVER,IO_READ | IO_WRITE);
	if(video < 0)
		qerror("Unable to open video-driver");

	keymap = open("/drivers/kmmanager",IO_READ | IO_WRITE);
	if(keymap < 0)
		qerror("Unable to open keymap-driver");

	/* get screen size */
	if(ioctl(video,IOCTL_VID_GETSIZE,&size,sizeof(sIoCtlSize)) < 0)
		qerror("Unable to get screensize");

	/* first line is the title */
	HEIGHT--;
	buffer = (char*)malloc(WIDTH * HEIGHT * 2);
	if(!buffer)
		qerror("Unable to alloc mem for buffer");

	if(setSigHandler(SIG_INTRPT_TIMER,sigTimer) < 0)
		qerror("Unable to set sig-handler");

	for(y = 1; y < HEIGHT - 1; y++) {
		for(x = 1; x < WIDTH - 1; x++) {
			buffer[y * WIDTH * 2 + x * 2] = ' ';
			buffer[y * WIDTH * 2 + x * 2 + 1] = 0x07;
		}
	}

	for(x = 1; x < WIDTH - 1; x++) {
		buffer[0 * WIDTH * 2 + x * 2] = 0xCD;
		buffer[0 * WIDTH * 2 + x * 2 + 1] = 0x07;
		buffer[(HEIGHT - 1) * WIDTH * 2 + x * 2] = 0xCD;
		buffer[(HEIGHT - 1) * WIDTH * 2 + x * 2 + 1] = 0x07;
	}
	for(y = 1; y < HEIGHT - 1; y++) {
		buffer[y * WIDTH * 2 + 0 * 2] = 0xBA;
		buffer[y * WIDTH * 2 + 0 * 2 + 1] = 0x07;
		buffer[y * WIDTH * 2 + (WIDTH - 1) * 2] = 0xBA;
		buffer[y * WIDTH * 2 + (WIDTH - 1) * 2 + 1] = 0x07;
	}
	buffer[0 * WIDTH * 2 + 0 * 2] = 0xC9;
	buffer[0 * WIDTH * 2 + 0 * 2 + 1] = 0x07;
	buffer[0 * WIDTH * 2 + (WIDTH - 1)  * 2] = 0xBB;
	buffer[0 * WIDTH * 2 + (WIDTH - 1)  * 2 + 1] = 0x07;
	buffer[(HEIGHT - 1) * WIDTH * 2 + 0 * 2] = 0xC8;
	buffer[(HEIGHT - 1) * WIDTH * 2 + 0 * 2 + 1] = 0x07;
	buffer[(HEIGHT - 1) * WIDTH * 2 + (WIDTH - 1) * 2] = 0xBC;
	buffer[(HEIGHT - 1) * WIDTH * 2 + (WIDTH - 1) * 2 + 1] = 0x07;
	tick();

	while(1) {
		if(!eof(keymap)) {
			if(read(keymap,&kmdata,sizeof(kmdata)) < 0)
				qerror("Unable to read from keymap");
			if(kmdata.keycode == VK_Q)
				break;
		}

		if(time >= UPDATE_INTERVAL) {
			time = 0;
			tick();
		}

		wait(EV_DATA_READABLE);
	}

	quit();
	return EXIT_SUCCESS;
}

static void tick(void) {
	u32 lastX = xpos;
	u32 lastY = ypos;
	xpos++;
	if(xpos >= WIDTH - 1) {
		xpos = 1;
		ypos++;
		if(ypos >= HEIGHT - 1)
			ypos = 1;
	}

	buffer[lastY * WIDTH * 2 + lastX * 2] = ' ';
	buffer[ypos * WIDTH * 2 + xpos * 2] = 'X';

	seek(video,WIDTH * 2,SEEK_SET);
	write(video,buffer,WIDTH * HEIGHT * 2);
}

static void sigTimer(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	time += TIMER_INTERVAL;
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
	close(video);
	close(keymap);
	free(buffer);
	ioctl(STDOUT_FILENO,IOCTL_VT_RESTORE,NULL,0);
	ioctl(STDOUT_FILENO,IOCTL_VT_EN_RDKB,NULL,0);
}
