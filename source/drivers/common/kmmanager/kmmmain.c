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
#include <esc/driver.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/io.h>
#include <esc/ringbuffer.h>
#include <esc/messages.h>
#include <esc/keycodes.h>
#include <stdio.h>
#include <errors.h>
#include <stdlib.h>
#include "keymap.h"
#include "events.h"

#define KEYMAP_FILE					"/etc/keymap"
#define KB_DATA_BUF_SIZE			128
#define BUF_SIZE					128

static int kbClientThread(void *arg);
static void handleKeymap(msgid_t mid,int fd);
static void handleKeyevents(msgid_t mid,int fd);

/* file-descriptor for ourself */
static sMsg msg;
static sRingBuf *rbuf;
static sKeymapEntry *map;
static tULock lck;
static int ids[2];

int main(void) {
	char path[MAX_PATH_LEN];
	char *newline;
	sWaitObject waits[2];
	int drv;
	msgid_t mid;
	FILE *f;

	events_init();

	ids[0] = regDriver("kmmanager",DRV_READ);
	if(ids[0] < 0)
		error("Unable to register driver 'kmmanager'");

	/* create buffers */
	rbuf = rb_create(sizeof(sKmData),BUF_SIZE,RB_OVERWRITE);
	if(rbuf == NULL)
		error("Unable to create the ring-buffer");

	/* determine default keymap */
	f = fopen(KEYMAP_FILE,"r");
	if(f == NULL)
		error("Unable to open %s",KEYMAP_FILE);
	fgets(path,MAX_PATH_LEN,f);
	if((newline = strchr(path,'\n')))
		*newline = '\0';
	fclose(f);

	/* load default map */
	map = km_parse(path);
	if(!map)
		error("Unable to load default keymap");

	ids[1] = regDriver("keyevents",0);
	if(ids[1] < 0)
		error("Unable to register driver 'keyevents'");

	if(startThread(kbClientThread,NULL) < 0)
		error("Unable to start thread for reading from kb");

	waits[0].events = EV_CLIENT;
	waits[0].object = ids[0];
	waits[1].events = EV_CLIENT;
	waits[1].object = ids[1];

    /* wait for commands */
	while(1) {
		int fd = getWork(ids,2,&drv,&mid,&msg,sizeof(msg),GW_NOBLOCK);
		if(fd < 0) {
			if(fd != ERR_NO_CLIENT_WAITING)
				printe("[KM] Unable to get client");
			waitm(waits,ARRAY_SIZE(waits));
		}
		else {
			if(drv == ids[0])
				handleKeymap(mid,fd);
			else if(drv == ids[1])
				handleKeyevents(mid,fd);
			else
				printe("[KM] Unknown driver-id %d\n",drv);
			close(fd);
		}
	}
	return EXIT_SUCCESS;
}

static int kbClientThread(A_UNUSED void *arg) {
	static sKmData data;
	static sKbData kbData[KB_DATA_BUF_SIZE];
	int kbFd;

	/* open keyboard */
	kbFd = open("/dev/keyboard",IO_READ);
	if(kbFd < 0)
		error("Unable to open '/dev/keyboard'");

	while(1) {
		ssize_t count = RETRY(read(kbFd,kbData,sizeof(kbData)));
		if(count < 0)
			printe("[KM] Unable to read");
		else {
			sKbData *kbd = kbData;
			bool wasReadable = rb_length(rbuf) > 0;
			bool readable = false;
			count /= sizeof(sKbData);
			while(count-- > 0) {
				data.isBreak = kbd->isBreak;
				data.keycode = kbd->keycode;
				/* use the lock while we're using map, rbuf and the events */
				locku(&lck);
				data.character = km_translateKeycode(
						map,kbd->isBreak,kbd->keycode,&(data.modifier));
				/* if nobody has announced a global key-listener for it, add it to our rb */
				if(!events_send(ids[1],&data)) {
					rb_write(rbuf,&data);
					readable = true;
				}
				unlocku(&lck);
				kbd++;
			}
			if(!wasReadable && readable)
				fcntl(ids[0],F_SETDATA,true);
		}
	}
	return EXIT_SUCCESS;
}

static void handleKeymap(msgid_t mid,int fd) {
	switch(mid) {
		case MSG_DRV_READ: {
			/* offset is ignored here */
			size_t count = msg.args.arg2 / sizeof(sKmData);
			sKmData *buffer = (sKmData*)malloc(count * sizeof(sKmData));
			msg.args.arg1 = 0;
			locku(&lck);
			if(buffer)
				msg.args.arg1 = rb_readn(rbuf,buffer,count) * sizeof(sKmData);
			msg.args.arg2 = rb_length(rbuf) > 0;
			unlocku(&lck);
			send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
			if(buffer) {
				send(fd,MSG_DRV_READ_RESP,buffer,count * sizeof(sKmData));
				free(buffer);
			}
		}
		break;

		case MSG_KM_SET: {
			char *str = msg.str.s1;
			str[sizeof(msg.str.s1) - 1] = '\0';
			sKeymapEntry *newMap = km_parse(str);
			if(!newMap)
				msg.str.arg1 = ERR_INVALID_KEYMAP;
			else {
				msg.str.arg1 = 0;
				locku(&lck);
				free(map);
				map = newMap;
				unlocku(&lck);
			}
			send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.str));
		}
		break;

		default:
			msg.args.arg1 = ERR_UNSUPPORTED_OP;
			send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
			break;
	}
}

static void handleKeyevents(msgid_t mid,int fd) {
	switch(mid) {
		case MSG_KE_ADDLISTENER: {
			uchar flags = (uchar)msg.args.arg1;
			uchar key = (uchar)msg.args.arg2;
			uchar modifier = (uchar)msg.args.arg3;
			inode_t id = getClientId(fd);
			locku(&lck);
			msg.args.arg1 = events_add(id,flags,key,modifier);
			unlocku(&lck);
			send(fd,MSG_KE_ADDLISTENER,&msg,sizeof(msg.args));
		}
		break;

		case MSG_KE_REMLISTENER: {
			uchar flags = (uchar)msg.args.arg1;
			uchar key = (uchar)msg.args.arg2;
			uchar modifier = (uchar)msg.args.arg3;
			inode_t id = getClientId(fd);
			locku(&lck);
			events_remove(id,flags,key,modifier);
			unlocku(&lck);
		}
		break;

		default:
			msg.args.arg1 = ERR_UNSUPPORTED_OP;
			send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
			break;
	}
}
