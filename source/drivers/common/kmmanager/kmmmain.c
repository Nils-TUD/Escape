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
#include <errno.h>
#include <stdlib.h>
#include "keymap.h"
#include "events.h"

#define KEYMAP_FILE					"/etc/keymap"
#define KB_DATA_BUF_SIZE			128
#define BUF_SIZE					128

static int kbClientThread(void *arg);
static int keymapThread(A_UNUSED void *arg);
static int keyeventsThread(A_UNUSED void *arg);

static sRingBuf *rbuf;
static sKeymapEntry *map;
static tULock lck;
static int ids[2];

int main(void) {
	char path[MAX_PATH_LEN];
	char *newline;
	FILE *f;

	events_init();

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

	if(startthread(keyeventsThread,NULL) < 0)
		error("Unable to start thread for keyevents-device");
	if(startthread(kbClientThread,NULL) < 0)
		error("Unable to start thread for reading from kb");
	return keymapThread(NULL);
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
		ssize_t count = IGNSIGS(read(kbFd,kbData,sizeof(kbData)));
		if(count < 0)
			printe("[KM] Unable to read");
		else {
			sKbData *kbd = kbData;
			bool wasReadable,readable;
			/* use the lock while we're using map, rbuf and the events */
			locku(&lck);
			wasReadable = rb_length(rbuf) > 0;
			readable = false;
			count /= sizeof(sKbData);
			while(count-- > 0) {
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
			if(!wasReadable && readable)
				fcntl(ids[0],F_SETDATA,true);
			unlocku(&lck);
		}
	}
	return EXIT_SUCCESS;
}

static int keymapThread(A_UNUSED void *arg) {
	ids[0] = createdev("/dev/kmmanager",DEV_TYPE_CHAR,DEV_READ);
	if(ids[0] < 0)
		error("Unable to register device 'kmmanager'");
	while(1) {
		sMsg msg;
		msgid_t mid;
		int fd = getwork(&ids[0],1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[KMM] Unable to get work");
		else {
			switch(mid) {
				case MSG_DEV_READ: {
					/* offset is ignored here */
					size_t count = msg.args.arg2 / sizeof(sKmData);
					sKmData *buffer = (sKmData*)malloc(count * sizeof(sKmData));
					msg.args.arg1 = 0;
					locku(&lck);
					if(buffer)
						msg.args.arg1 = rb_readn(rbuf,buffer,count) * sizeof(sKmData);
					/* the problem here is that send() doesn't set immediatly whether data is
					 * readable or not, because the message is queued and read any time later.
					 * but we have to make sure that this is done immediatly, i.e. before we unlock
					 * the lock. therefore we do it manually here with fcntl(). */
					msg.args.arg2 = READABLE_DONT_SET;
					fcntl(ids[0],F_SETDATA,rb_length(rbuf) > 0);
					unlocku(&lck);
					/* now send the response */
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(msg.args.arg1) {
						send(fd,MSG_DEV_READ_RESP,buffer,msg.args.arg1);
						free(buffer);
					}
				}
				break;

				case MSG_KM_SET: {
					sKeymapEntry *newMap;
					char *str = msg.str.s1;
					str[sizeof(msg.str.s1) - 1] = '\0';
					newMap = km_parse(str);
					if(!newMap)
						msg.str.arg1 = -EINVAL;
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
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}
	return 0;
}

static int keyeventsThread(A_UNUSED void *arg) {
	ids[1] = createdev("/dev/keyevents",DEV_TYPE_SERVICE,0);
	if(ids[1] < 0)
		error("Unable to register device 'keyevents'");
	while(1) {
		sMsg msg;
		msgid_t mid;
		int fd = getwork(&ids[1],1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[KMM] Unable to get work");
		else {
			switch(mid) {
				case MSG_KE_ADDLISTENER: {
					uchar flags = (uchar)msg.args.arg1;
					uchar key = (uchar)msg.args.arg2;
					uchar modifier = (uchar)msg.args.arg3;
					inode_t id = getclientid(fd);
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
					inode_t id = getclientid(fd);
					locku(&lck);
					events_remove(id,flags,key,modifier);
					unlocku(&lck);
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}
	return 0;
}
