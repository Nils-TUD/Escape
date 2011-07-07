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
#include <esc/io.h>
#include <esc/thread.h>
#include <esc/proc.h>
#include <esc/messages.h>
#include <esc/keycodes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define GUI_INDEX			6
#define MAX_RETRY_COUNT		500

static void switchTo(size_t index);
static bool startGUI(void);
static bool exists(const char *name);
static bool startDriver(const char *name,const char *waitDev);
static void addListener(int fd,uchar flags,uchar key,uchar modifiers);

static sMsg msg;
static size_t curTerm = 0;
static bool guiStarted = false;
static uchar keys[] = {
	VK_1,VK_2,VK_3,VK_4,VK_5,VK_6,VK_7
};

static void childTermHandler(int sig) {
	UNUSED(sig);
	RETRY(waitChild(NULL));
}

int main(void) {
	msgid_t mid;
	size_t i;
	if(setSigHandler(SIG_CHILD_TERM,&childTermHandler) < 0)
		error("Unable to set SIG_CHILD_TERM-handler");

	/* announce listener */
	int fd = open("/dev/keyevents",IO_MSGS);
	if(fd < 0)
		error("[UI] Unable to open '/dev/keyevents'");
	for(i = 0; i < ARRAY_SIZE(keys); i++)
		addListener(fd,KE_EV_KEYCODE | KE_EV_PRESSED,keys[i],CTRL_MASK);

	while(1) {
		sKmData *km = (sKmData*)&msg.data.d;
		if(RETRY(receive(fd,&mid,&msg,sizeof(msg.data))) < 0)
			printe("[UI] Unable to receive event from keyevents");
		else
			switchTo(km->keycode - VK_1);
	}

	close(fd);
	return EXIT_SUCCESS;
}

static void switchTo(size_t index) {
	if(index != GUI_INDEX && curTerm == GUI_INDEX) {
		/* disable winmanager */
		int fd = open("/dev/winmanager",IO_MSGS);
		if(fd >= 0) {
			send(fd,MSG_WIN_DISABLE,NULL,0);
			close(fd);
			/* enable vterm */
			fd = open("/dev/vterm0",IO_MSGS);
			if(fd >= 0) {
				sendRecvMsgData(fd,MSG_VT_ENABLE,NULL,0);
				msg.args.arg1 = index;
				send(fd,MSG_VT_SELECT,&msg,sizeof(msg.args));
				close(fd);
				curTerm = index;
			}
		}
	}
	else if(index == GUI_INDEX && curTerm != GUI_INDEX) {
		if(startGUI()) {
			int fd = open("/dev/vterm0",IO_MSGS);
			if(fd >= 0) {
				/* diable vterm */
				sendRecvMsgData(fd,MSG_VT_DISABLE,NULL,0);
				close(fd);
				/* enable winmanager */
				fd = open("/dev/winmanager",IO_MSGS);
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
		int fd = open("/dev/vterm0",IO_MSGS);
		if(fd >= 0) {
			msg.args.arg1 = index;
			send(fd,MSG_VT_SELECT,&msg,sizeof(msg.args));
			close(fd);
			curTerm = index;
		}
	}
}

static bool startGUI(void) {
	if(guiStarted) {
		/* check if its still running */
		if(exists("/dev/vesa") && exists("/dev/mouse") && exists("/dev/winmanager"))
			return true;
		/* if not restart the missing ones now */
	}
	if(!startDriver("vesa","/dev/vesa"))
		return false;
	if(!startDriver("mouse","/dev/mouse"))
		return false;
	if(!startDriver("winmanager","/dev/winmanager"))
		return false;
	if(fork() == 0) {
		exec("/bin/desktop",NULL);
		error("Unable to start desktop");
	}
	/* TODO temporary */
	if(fork() == 0) {
		exec("/bin/gtest",NULL);
		error("Unable to start gui-test");
	}
	guiStarted = true;
	return true;
}

static bool exists(const char *name) {
	int fd;
	if((fd = open(name,IO_READ)) >= 0) {
		close(fd);
		return true;
	}
	return false;
}

static bool startDriver(const char *name,const char *waitDev) {
	int fd;
	size_t i;
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
	close(fd);
	return true;
}

static void addListener(int fd,uchar flags,uchar key,uchar modifiers) {
	msgid_t mid;
	msg.args.arg1 = flags;
	msg.args.arg2 = key;
	msg.args.arg3 = modifiers;
	if(send(fd,MSG_KE_ADDLISTENER,&msg,sizeof(msg.args)) < 0)
		error("Unable to send msg to keyevents");
	if(RETRY(receive(fd,&mid,&msg,sizeof(msg.args))) < 0 || (int)msg.args.arg1 < 0)
		error("Unable to receive reply or invalid reply from keyevents");
}
