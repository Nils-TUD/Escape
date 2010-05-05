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
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/ports.h>
#include <esc/heap.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <stdio.h>
#include <sllist.h>
#include <string.h>
#include <ringbuffer.h>
#include <errors.h>

#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include "vterm.h"

#define KB_DATA_BUF_SIZE	128
/* max number of requests handled in a row; we have to look sometimes for the keyboard.. */
#define MAX_SEQREQ			20

/**
 * Determines the vterm for the given driver-id
 *
 * @param sid the driver-id
 * @return the vterm or NULL
 */
static sVTerm *getVTerm(tDrvId sid);

static sMsg msg;
static sVTermCfg cfg;
static sKmData kmData[KB_DATA_BUF_SIZE];
/* vterms */
static tDrvId drvIds[VTERM_COUNT] = {-1};

int main(void) {
	u32 i,reqc;
	tFD kbFd;
	tDrvId client;
	tMsgId mid;
	char name[MAX_VT_NAME_LEN + 1];

	cfg.readKb = true;
	cfg.enabled = true;

	/* reg drivers */
	for(i = 0; i < VTERM_COUNT; i++) {
		snprintf(name,sizeof(name),"vterm%d",i);
		drvIds[i] = regDriver(name,DRV_READ | DRV_WRITE | DRV_TERM);
		if(drvIds[i] < 0)
			error("Unable to register driver '%s'",name);
	}

	/* init vterms */
	if(!vterm_initAll(drvIds,&cfg))
		error("Unable to init vterms");

	/* open keyboard */
	kbFd = open("/dev/kmmanager",IO_READ);
	if(kbFd < 0)
		error("Unable to open '/dev/kmmanager'");

	/* request io-ports for qemu and bochs */
	if(requestIOPort(0xe9) < 0 || requestIOPort(0x3f8) < 0 || requestIOPort(0x3fd) < 0)
		error("Unable to request ports for qemu/bochs");

	/* select first vterm */
	vterm_selectVTerm(0);

	reqc = 0;
	while(1) {
		tFD fd = getWork(drvIds,VTERM_COUNT,&client,&mid,&msg,sizeof(msg),GW_NOBLOCK);
		if((cfg.enabled && cfg.readKb && reqc >= MAX_SEQREQ) || fd < 0) {
			if(fd < 0 && fd != ERR_NO_CLIENT_WAITING)
				printe("[VTERM] Unable to get work");
			reqc = 0;
			if(cfg.enabled && cfg.readKb) {
				/* read from keyboard */
				/* don't block here since there may be waiting clients.. */
				if(!eof(kbFd)) {
					sKmData *kmsg = kmData;
					s32 count = read(kbFd,kmData,sizeof(kmData));
					if(count < 0)
						printe("[VTERM] Unable to read from km-manager");
					else {
						count /= sizeof(sKmData);
						while(count-- > 0) {
							if(!kmsg->isBreak) {
								vterm_handleKey(vterm_getActive(),kmsg->keycode,kmsg->modifier,kmsg->character);
								vterm_update(vterm_getActive());
							}
							kmsg++;
						}
					}
				}
			}
			/* if we've received a msg (and just interrupted because reqc >= MAX_SEQREQ), we
			 * HAVE TO handle it of course */
			if(fd < 0) {
				if(cfg.enabled && cfg.readKb)
					wait(EV_CLIENT | EV_DATA_READABLE);
				else
					wait(EV_CLIENT);
				continue;
			}
		}

		sVTerm *vt = getVTerm(client);
		if(vt != NULL) {
			reqc++;
			switch(mid) {
				case MSG_DRV_READ: {
					/* offset is ignored here */
					u32 count = msg.args.arg2;
					char *data = (char*)malloc(count);
					msg.args.arg1 = 0;
					if(data)
						msg.args.arg1 = rb_readn(vt->inbuf,data,count);
					msg.args.arg2 = vt->inbufEOF || rb_length(vt->inbuf) > 0;
					if(rb_length(vt->inbuf) == 0)
						vt->inbufEOF = false;
					send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
					if(data) {
						send(fd,MSG_DRV_READ_RESP,data,count);
						free(data);
					}
					else
						printe("[VTERM] Not enough memory");
				}
				break;
				case MSG_DRV_WRITE: {
					char *data;
					u32 c;
					c = msg.args.arg2;
					data = (char*)malloc(c + 1);
					msg.args.arg1 = 0;
					if(data) {
						if(receive(fd,&mid,data,c + 1) >= 0) {
							data[c] = '\0';
							vterm_puts(vt,data,c,true);
							vterm_update(vt);
							msg.args.arg1 = c;
						}
						free(data);
					}
					else
						printe("[VTERM] Not enough memory");
					send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VT_SELECT: {
					u32 index = msg.args.arg1;
					if(index < VTERM_COUNT && vterm_getActive()->index != index) {
						vterm_selectVTerm(index);
						vterm_update(vterm_getActive());
					}
				}
				break;

				case MSG_VT_SHELLPID:
				case MSG_VT_ENABLE:
				case MSG_VT_DISABLE:
				case MSG_VT_EN_ECHO:
				case MSG_VT_DIS_ECHO:
				case MSG_VT_EN_RDLINE:
				case MSG_VT_DIS_RDLINE:
				case MSG_VT_EN_RDKB:
				case MSG_VT_DIS_RDKB:
				case MSG_VT_EN_NAVI:
				case MSG_VT_DIS_NAVI:
				case MSG_VT_BACKUP:
				case MSG_VT_RESTORE:
				case MSG_VT_GETSIZE:
					msg.data.arg1 = vterm_ctl(vt,&cfg,mid,msg.data.d);
					/* reenable mode, if necessary */
					if(mid == MSG_VT_ENABLE) {
						send(vt->video,MSG_VID_SETMODE,NULL,0);
						vterm_markScrDirty(vt);
					}
					vterm_update(vt);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
					break;

				default:
					printe("Unable to handle msg %d\n",mid);
					break;
			}
		}
		close(fd);
	}

	/* clean up */
	releaseIOPort(0xe9);
	releaseIOPort(0x3f8);
	releaseIOPort(0x3fd);
	close(kbFd);
	for(i = 0; i < VTERM_COUNT; i++) {
		unregDriver(drvIds[i]);
		vterm_destroy(vterm_get(i));
	}
	return EXIT_SUCCESS;
}

static sVTerm *getVTerm(tDrvId sid) {
	u32 i;
	for(i = 0; i < VTERM_COUNT; i++) {
		if(drvIds[i] == sid)
			return vterm_get(i);
	}
	return NULL;
}
