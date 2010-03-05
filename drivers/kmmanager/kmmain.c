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
#include <esc/fileio.h>
#include <esc/io.h>
#include <esc/heap.h>
#include <messages.h>
#include <errors.h>
#include <ringbuffer.h>
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
	tDrvId id;
	tMsgId mid;
	tFD kbFd;
	tFile *f;

	id = regDriver("kmmanager",DRV_READ);
	if(id < 0)
		error("Unable to register driver 'kmmanager'");

	/* create buffers */
	rbuf = rb_create(sizeof(sKmData),BUF_SIZE,RB_OVERWRITE);
	if(rbuf == NULL)
		error("Unable to create the ring-buffer");

	/* open keyboard */
	kbFd = open("/dev/keyboard",IO_READ);
	if(kbFd < 0)
		error("Unable to open '/dev/keyboard'");

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
		tFD fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),GW_NOBLOCK);
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
			switch(mid) {
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
			close(fd);
		}
	}

	/* clean up */
	free(map);
	rb_destroy(rbuf);
	unregDriver(id);

	return EXIT_SUCCESS;
}
