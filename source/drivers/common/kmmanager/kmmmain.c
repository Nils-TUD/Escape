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

static void handleKeymap(tMsgId mid,tFD fd);
static void handleKeyevents(tMsgId mid,tFD fd);

/* file-descriptor for ourself */
static sMsg msg;
static sRingBuf *rbuf;
static sKeymapEntry *map;
static sKmData data;
static sKbData kbData[KB_DATA_BUF_SIZE];

int main(void) {
	char path[MAX_PATH_LEN];
	char *newline;
	sWaitObject waits[3];
	tFD ids[2];
	tFD drv;
	tMsgId mid;
	tFD kbFd;
	FILE *f;

	events_init();

	ids[0] = regDriver("kmmanager",DRV_READ);
	if(ids[0] < 0)
		error("Unable to register driver 'kmmanager'");

	/* create buffers */
	rbuf = rb_create(sizeof(sKmData),BUF_SIZE,RB_OVERWRITE);
	if(rbuf == NULL)
		error("Unable to create the ring-buffer");

	/* open keyboard */
	kbFd = open("/dev/keyboard",IO_READ | IO_NOBLOCK);
	if(kbFd < 0)
		error("Unable to open '/dev/keyboard'");

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

	waits[0].events = EV_CLIENT;
	waits[0].object = ids[0];
	waits[1].events = EV_CLIENT;
	waits[1].object = ids[1];
	waits[2].events = EV_DATA_READABLE;
	waits[2].object = kbFd;

    /* wait for commands */
	while(1) {
		tFD fd = getWork(ids,2,&drv,&mid,&msg,sizeof(msg),GW_NOBLOCK);
		if(fd < 0) {
			/* read from keyboard */
			sKbData *kbd = kbData;
			ssize_t count;
			bool readable = false;
			/* don't block here since there may be waiting clients.. */
			/*debugf("** rc **\n");*/
			while((count = RETRY(read(kbFd,kbData,sizeof(kbData)))) > 0) {
				count /= sizeof(sKbData);
				while(count-- > 0) {
					data.isBreak = kbd->isBreak;
					data.keycode = kbd->keycode;
					data.character = km_translateKeycode(
							map,kbd->isBreak,kbd->keycode,&(data.modifier));
					/* if nobody has announced a global key-listener for it, add it to our rb */
					if(!events_send(ids[1],&data)) {
						rb_write(rbuf,&data);
						readable = true;
					}
					kbd++;
				}
			}
			if(count < 0 && count != ERR_WOULD_BLOCK)
				printe("Unable to read");
			if(readable)
				fcntl(ids[0],F_SETDATA,true);
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

	/* clean up */
	close(ids[1]);
	free(map);
	rb_destroy(rbuf);
	close(ids[0]);

	return EXIT_SUCCESS;
}

static void handleKeymap(tMsgId mid,tFD fd) {
	switch(mid) {
		case MSG_DRV_READ: {
			/* offset is ignored here */
			size_t count = msg.args.arg2 / sizeof(sKmData);
			sKmData *buffer = (sKmData*)malloc(count * sizeof(sKmData));
			msg.args.arg1 = 0;
			if(buffer)
				msg.args.arg1 = rb_readn(rbuf,buffer,count) * sizeof(sKmData);
			msg.args.arg2 = rb_length(rbuf) > 0;
			send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
			if(buffer) {
				send(fd,MSG_DRV_READ_RESP,buffer,count * sizeof(sKmData));
				free(buffer);
			}
		}
		break;
		case MSG_KM_SET: {
			char *str = msg.data.d;
			str[sizeof(msg.data.d) - 1] = '\0';
			sKeymapEntry *newMap = km_parse(str);
			if(!newMap)
				msg.data.arg1 = ERR_INVALID_KEYMAP;
			else {
				msg.data.arg1 = 0;
				free(map);
				map = newMap;
			}
			send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
		}
		break;
	}
}

static void handleKeyevents(tMsgId mid,tFD fd) {
	switch(mid) {
		case MSG_KE_ADDLISTENER: {
			uchar flags = (uchar)msg.args.arg1;
			uchar key = (uchar)msg.args.arg2;
			uchar modifier = (uchar)msg.args.arg3;
			msg.args.arg1 = events_add(getClientId(fd),flags,key,modifier);
			send(fd,MSG_KE_ADDLISTENER,&msg,sizeof(msg.args));
		}
		break;

		case MSG_KE_REMLISTENER: {
			uchar flags = (uchar)msg.args.arg1;
			uchar key = (uchar)msg.args.arg2;
			uchar modifier = (uchar)msg.args.arg3;
			events_remove(getClientId(fd),flags,key,modifier);
		}
		break;
	}
}