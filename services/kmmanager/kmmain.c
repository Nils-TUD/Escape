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
#include <esc/service.h>
#include <esc/proc.h>
#include <esc/fileio.h>
#include <esc/io.h>
#include <esc/heap.h>
#include <messages.h>
#include <errors.h>
#include <ringbuffer.h>
#include <stdlib.h>
#include "keymap.h"

#define KEYMAP_FILE					"/etc/keymap"
#define KB_DATA_BUF_SIZE			128
#define BUF_SIZE					128

/* file-descriptor for ourself */
static sMsg msg;
static sRingBuf *rbuf;
static sKeymapEntry *map;
static sKmData data;
static sKbData kbData[KB_DATA_BUF_SIZE];

int main(void) {
	char path[MAX_PATH_LEN];
	tServ id,client;
	tMsgId mid;
	tFD kbFd;
	tFile *f;

	id = regService("kmmanager",SERV_DRIVER);
	if(id < 0)
		error("Unable to register service 'kmmanager'");

	/* create buffers */
	rbuf = rb_create(sizeof(sKmData),BUF_SIZE,RB_OVERWRITE);
	if(rbuf == NULL)
		error("Unable to create the ring-buffer");

	/* open keyboard */
	kbFd = open("/drivers/keyboard",IO_READ);
	if(kbFd < 0)
		error("Unable to open '/drivers/keyboard'");

	/* determine default keymap */
	f = fopen(KEYMAP_FILE,"r");
	if(f == NULL)
		error("Unable to open %s",KEYMAP_FILE);
	fscanl(f,path,MAX_PATH_LEN);
	fclose(f);

	/* load default map */
	map = km_parse(path);
	if(!map)
		error("Unable to load default keymap");

    /* wait for commands */
	while(1) {
		tFD fd = getClient(&id,1,&client);
		if(fd < 0) {
			/* read from keyboard */
			/* don't block here since there may be waiting clients.. */
			while(!eof(kbFd)) {
				sKbData *kbd = kbData;
				u32 count = read(kbFd,kbData,sizeof(kbData));
				count /= sizeof(sKbData);
				while(count-- > 0) {
					data.isBreak = kbd->isBreak;
					data.keycode = kbd->keycode;
					data.character = km_translateKeycode(
							map,kbd->isBreak,kbd->keycode,&(data.modifier));
					rb_write(rbuf,&data);
					kbd++;
				}
				setDataReadable(id,true);
			}
			wait(EV_CLIENT | EV_DATA_READABLE);
		}
		else {
			while(receive(fd,&mid,&msg,sizeof(msg)) > 0) {
				switch(mid) {
					case MSG_DRV_OPEN:
						msg.args.arg1 = 0;
						send(fd,MSG_DRV_OPEN_RESP,&msg,sizeof(msg.args));
						break;
					case MSG_DRV_READ: {
						/* offset is ignored here */
						u32 count = msg.args.arg2 / sizeof(sKmData);
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
					case MSG_DRV_WRITE:
						msg.args.arg1 = ERR_UNSUPPORTED_OP;
						send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
						break;
					case MSG_DRV_IOCTL: {
						if(msg.data.arg1 == IOCTL_KM_SET && msg.data.arg2 > 0) {
							/* ensure that its null-terminated */
							msg.data.d[msg.data.arg2 - 1] = '\0';
							sKeymapEntry *newMap = km_parse((char*)msg.data.d);
							if(!newMap)
								msg.data.arg1 = ERR_INVALID_KEYMAP;
							else {
								msg.data.arg1 = 0;
								free(map);
								map = newMap;
							}
						}
						else
							msg.data.arg1 = ERR_UNSUPPORTED_OP;
						send(fd,MSG_DRV_IOCTL_RESP,&msg,sizeof(msg.data));
					}
					break;
					case MSG_DRV_CLOSE:
						break;
				}
			}
			close(fd);
		}
	}

	/* clean up */
	free(map);
	rb_destroy(rbuf);
	unregService(id);

	return EXIT_SUCCESS;
}
