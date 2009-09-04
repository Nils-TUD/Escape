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
#include <messages.h>
#include <esc/service.h>
#include <esc/io.h>
#include <esc/ports.h>
#include <esc/heap.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/fileio.h>
#include <stdlib.h>
#include <sllist.h>
#include <string.h>
#include <ringbuffer.h>
#include <errors.h>

#include "vterm.h"
#include "vtout.h"
#include "vtin.h"

#define KB_DATA_BUF_SIZE	128

/**
 * Determines the vterm for the given service-id
 *
 * @param sid the service-id
 * @return the vterm or NULL
 */
static sVTerm *getVTerm(tServ sid);

/* wether we should read from keyboard */
static bool readKeyboard = true;

static sMsg msg;
static sKbData kbData[KB_DATA_BUF_SIZE];
/* vterms */
static tServ servIds[VTERM_COUNT] = {-1};

int main(void) {
	u32 i;
	tFD kbFd;
	tServ client;
	tMsgId mid;
	char name[MAX_NAME_LEN + 1];

	/* reg services */
	for(i = 0; i < VTERM_COUNT; i++) {
		sprintf(name,"vterm%d",i);
		servIds[i] = regService(name,SERV_DRIVER);
		if(servIds[i] < 0) {
			printe("Unable to register service '%s'",name);
			return EXIT_FAILURE;
		}
	}

	/* init vterms */
	if(!vterm_initAll(servIds)) {
		fprintf(stderr,"Unable to init vterms");
		return EXIT_FAILURE;
	}

	/* open keyboard */
	kbFd = open("/drivers/keyboard",IO_READ | IO_CONNECT);
	if(kbFd < 0) {
		printe("Unable to open '/drivers/keyboard'");
		return EXIT_FAILURE;
	}

	/* request io-ports for qemu and bochs */
	requestIOPort(0xe9);
	requestIOPort(0x3f8);
	requestIOPort(0x3fd);

	/* select first vterm */
	vterm_selectVTerm(0);

	while(1) {
		tFD fd = getClient(servIds,VTERM_COUNT,&client);
		if(fd < 0) {
			if(readKeyboard) {
				/* read from keyboard */
				/* don't block here since there may be waiting clients.. */
				while(!eof(kbFd)) {
					sKbData *kbd = kbData;
					u32 count = read(kbFd,kbData,sizeof(kbData));
					count /= sizeof(sKbData);
					while(count-- > 0) {
						vterm_handleKeycode(kbd->isBreak,kbd->keycode);
						vterm_update(vterm_getActive());
						kbd++;
					}
				}
				wait(EV_CLIENT | EV_DATA_READABLE);
			}
			else
				wait(EV_CLIENT);
		}
		else {
			sVTerm *vt = getVTerm(client);
			if(vt != NULL) {
				u32 c;
				/* TODO this may cause trouble with escape-codes. maybe we should store the
				 * "escape-state" somehow... */
				while(receive(fd,&mid,&msg,sizeof(msg)) > 0) {
					switch(mid) {
						case MSG_DRV_OPEN:
							msg.args.arg1 = 0;
							send(fd,MSG_DRV_OPEN_RESP,&msg,sizeof(msg.args));
							break;
						case MSG_DRV_READ: {
							/* offset is ignored here */
							u32 count = msg.args.arg2;
							char *data = (char*)malloc(count);
							msg.args.arg1 = 0;
							if(data)
								msg.args.arg1 = rb_readn(vt->inbuf,data,count);
							msg.args.arg2 = rb_length(vt->inbuf) > 0;
							send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
							if(data) {
								send(fd,MSG_DRV_READ_RESP,data,count);
								free(data);
							}
						}
						break;
						case MSG_DRV_WRITE: {
							char *data;
							c = msg.args.arg2;
							data = (char*)malloc(c + 1);
							msg.args.arg1 = 0;
							if(data) {
								receive(fd,&mid,data,c + 1);
								data[c] = '\0';
								vterm_puts(vt,data,c,true);
								vterm_update(vt);
								free(data);
								msg.args.arg1 = c;
							}
							send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
						}
						break;
						case MSG_DRV_IOCTL: {
							msg.data.arg1 = vterm_ioctl(vt,msg.data.arg1,msg.data.d,&readKeyboard);
							send(fd,MSG_DRV_IOCTL_RESP,&msg,sizeof(msg.data));
						}
						break;
						case MSG_DRV_CLOSE:
							break;
					}
				}
			}
			close(fd);
		}
	}

	/* clean up */
	releaseIOPort(0xe9);
	releaseIOPort(0x3f8);
	releaseIOPort(0x3fd);
	close(kbFd);
	for(i = 0; i < VTERM_COUNT; i++) {
		unregService(servIds[i]);
		vterm_destroy(vterm_get(i));
	}
	return EXIT_SUCCESS;
}

static sVTerm *getVTerm(tServ sid) {
	u32 i;
	for(i = 0; i < VTERM_COUNT; i++) {
		if(servIds[i] == sid)
			return vterm_get(i);
	}
	return NULL;
}
