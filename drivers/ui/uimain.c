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

#include <esc/common.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <esc/proc.h>
#include <stdio.h>
#include <esc/keycodes.h>
#include <string.h>
#include <esc/messages.h>

#define GUI_INDEX			6
#define MAX_RETRY_COUNT		500

static void switchTo(u32 index);
static bool startGUI(void);
static bool startDriver(const char *name,const char *waitDev);
static void addListener(tFD fd,u8 flags,u8 key,u8 modifiers);

static sMsg msg;
static u32 curTerm = 0;
static bool guiStarted = false;
static u8 keys[] = {
	VK_1,VK_2,VK_3,VK_4,VK_5,VK_6,VK_7
};

int main(void) {
	tMsgId mid;
	u32 i;
	tFD fd = open("/dev/keyevents",IO_READ | IO_WRITE);

	for(i = 0; i < ARRAY_SIZE(keys); i++)
		addListener(fd,KE_EV_KEYCODE | KE_EV_PRESSED,keys[i],CTRL_MASK);

	while(1) {
		sKmData *km = (sKmData*)&msg.data.d;
		if(receive(fd,&mid,&msg,sizeof(msg.data)) < 0)
			printe("[UI] Unable to receive event from keyevents");
		else
			switchTo(km->keycode - VK_1);
	}

	close(fd);
	return EXIT_SUCCESS;
}

static void switchTo(u32 index) {
	if(index != GUI_INDEX && curTerm == GUI_INDEX) {
		/* disable winmanager */
		tFD fd = open("/dev/winmanager",IO_WRITE);
		if(fd >= 0) {
			send(fd,MSG_WIN_DISABLE,NULL,0);
			close(fd);
			/* enable vterm */
			fd = open("/dev/vterm0",IO_WRITE);
			if(fd >= 0) {
				send(fd,MSG_VT_ENABLE,NULL,0);
				msg.args.arg1 = index;
				send(fd,MSG_VT_SELECT,&msg,sizeof(msg.args));
				close(fd);
				curTerm = index;
			}
		}
	}
	else if(index == GUI_INDEX && curTerm != GUI_INDEX) {
		if(startGUI()) {
			tFD fd = open("/dev/vterm0",IO_WRITE);
			if(fd >= 0) {
				/* diable vterm */
				send(fd,MSG_VT_DISABLE,NULL,0);
				close(fd);
				/* enable winmanager */
				fd = open("/dev/winmanager",IO_WRITE);
				if(fd >= 0) {
					send(fd,MSG_WIN_ENABLE,NULL,0);
					close(fd);
					curTerm = index;
				}
			}
		}
	}
	else if(index != GUI_INDEX) {
		/* select vterm */
		tFD fd = open("/dev/vterm0",IO_WRITE);
		if(fd >= 0) {
			msg.args.arg1 = index;
			send(fd,MSG_VT_SELECT,&msg,sizeof(msg.args));
			close(fd);
			curTerm = index;
		}
	}
}

static bool startGUI(void) {
	if(guiStarted)
		return true;
	if(!startDriver("vesa","/dev/vesa"))
		return false;
	if(!startDriver("mouse","/dev/mouse"))
		return false;
	if(!startDriver("winmanager","/dev/winmanager"))
		return false;
	/* TODO temporary */
	if(fork() == 0) {
		exec("/bin/gtest",NULL);
		error("Unable to start gui-test");
	}
	guiStarted = true;
	return true;
}

static bool startDriver(const char *name,const char *waitDev) {
	tFD fd;
	u32 i;
	char path[MAX_PATH_LEN + 1] = "/sbin/";

	/* already started? fine */
	if((fd = open(waitDev,IO_READ)) >= 0)
		return true;

	/* start */
	strcat(path,name);
	if(fork() == 0) {
		exec(path,NULL);
		error("Exec with '%s' failed",path);
	}

	/* wait for it */
	i = 0;
	do {
		fd = open(waitDev,IO_READ);
		if(fd < 0)
			sleep(20);
		i++;
	}
	while(fd < 0 && i < MAX_RETRY_COUNT);
	if(fd < 0) {
		printe("Haven't found '%s' after %d retries",waitDev,i);
		return false;
	}
	return true;
}

static void addListener(tFD fd,u8 flags,u8 key,u8 modifiers) {
	tMsgId mid;
	msg.args.arg1 = gettid();
	msg.args.arg2 = flags;
	msg.args.arg3 = key;
	msg.args.arg4 = modifiers;
	if(send(fd,MSG_KE_ADDLISTENER,&msg,sizeof(msg.args)) < 0)
		error("Unable to send msg to keyevents");
	if(receive(fd,&mid,&msg,sizeof(msg.args)) < 0 || (s32)msg.args.arg1 < 0)
		error("Unable to receive reply or invalid reply from keyevents");
}
