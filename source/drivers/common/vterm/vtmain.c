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
#include <esc/driver/video.h>
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <esc/ringbuffer.h>
#include <stdio.h>
#include <string.h>
#include <errors.h>
#include <stdlib.h>

#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include "vterm.h"

#define KB_DATA_BUF_SIZE	128
/* max number of requests handled in a row; we have to look sometimes for the keyboard.. */
#define MAX_SEQREQ			20

static sVTerm *getVTerm(int sid);

static sMsg msg;
static sVTermCfg cfg;
static sKmData kmData[KB_DATA_BUF_SIZE];
/* vterms */
static int drvIds[VTERM_COUNT] = {-1};

int main(void) {
	size_t i,reqc;
	int kbFd;
	int client;
	msgid_t mid;
	sWaitObject waits[VTERM_COUNT + 1];
	char name[MAX_VT_NAME_LEN + 1];

	cfg.readKb = true;
	cfg.enabled = false;

	/* reg drivers */
	for(i = 0; i < VTERM_COUNT; i++) {
		snprintf(name,sizeof(name),"vterm%d",i);
		drvIds[i] = regDriver(name,DRV_READ | DRV_WRITE);
		if(drvIds[i] < 0)
			error("Unable to register driver '%s'",name);
		waits[i].events = EV_CLIENT;
		waits[i].object = drvIds[i];
	}

	/* init vterms */
	if(!vt_initAll(drvIds,&cfg))
		error("Unable to init vterms");

	/* open keyboard */
	kbFd = open("/dev/kmmanager",IO_READ | IO_NOBLOCK);
	if(kbFd < 0)
		error("Unable to open '/dev/kmmanager'");

	waits[VTERM_COUNT].events = EV_DATA_READABLE;
	waits[VTERM_COUNT].object = kbFd;

	reqc = 0;
	while(1) {
		int fd = getWork(drvIds,VTERM_COUNT,&client,&mid,&msg,sizeof(msg),GW_NOBLOCK);
		if((cfg.enabled && cfg.readKb && reqc >= MAX_SEQREQ) || fd < 0) {
			if(fd < 0 && fd != ERR_NO_CLIENT_WAITING)
				printe("[VTERM] Unable to get work");
			reqc = 0;
			if(cfg.enabled && cfg.readKb) {
				/* read from keyboard */
				ssize_t count = RETRY(read(kbFd,kmData,sizeof(kmData)));
				if(count < 0) {
					if(count != ERR_WOULD_BLOCK)
						printe("[VTERM] Unable to read from km-manager");
				}
				else {
					sKmData *kmsg = kmData;
					count /= sizeof(sKmData);
					while(count-- > 0) {
						if(!kmsg->isBreak) {
							sVTerm *vt = vt_getActive();
							vtin_handleKey(vt,kmsg->keycode,kmsg->modifier,kmsg->character);
							vt_update(vt);
						}
						kmsg++;
					}
				}
			}
			/* if we've received a msg (and just interrupted because reqc >= MAX_SEQREQ), we
			 * HAVE TO handle it of course */
			if(fd < 0) {
				if(cfg.enabled && cfg.readKb)
					waitm(waits,VTERM_COUNT + 1);
				else
					waitm(waits,VTERM_COUNT);
				continue;
			}
		}

		sVTerm *vt = getVTerm(client);
		if(vt != NULL) {
			reqc++;
			switch(mid) {
				case MSG_DRV_READ: {
					/* offset is ignored here */
					size_t count = msg.args.arg2;
					char *data = (char*)malloc(count);
					msg.args.arg1 = 0;
					if(data)
						msg.args.arg1 = rb_readn(vt->inbuf,data,count);
					if(rb_length(vt->inbuf) == 0)
						vt->inbufEOF = false;
					msg.args.arg2 = vt->inbufEOF || rb_length(vt->inbuf) > 0;
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
					size_t c = msg.args.arg2;
					data = (char*)malloc(c + 1);
					msg.args.arg1 = 0;
					if(data) {
						if(RETRY(receive(fd,&mid,data,c + 1)) >= 0) {
							if(cfg.enabled) {
								data[c] = '\0';
								vtout_puts(vt,data,c,true);
								vt_update(vt);
							}
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
					size_t index = msg.args.arg1;
					if(index < VTERM_COUNT) {
						if(!vt_getActive() || vt_getActive()->index != index) {
							vt_selectVTerm(index);
							vt_update(vt_getActive());
						}
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
					msg.data.arg1 = vtctrl_control(vt,&cfg,mid,msg.data.d);
					/* reenable mode, if necessary */
					if(mid == MSG_VT_ENABLE) {
						/* always use the active one here */
						vt = vt_getActive();
						if(vt) {
							video_setMode(vt->video);
							vtctrl_markScrDirty(vt);
						}
					}
					vt_update(vt);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
					break;

				default:
					msg.args.arg1 = ERR_UNSUPPORTED_OP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
		close(fd);
	}

	/* clean up */
	close(kbFd);
	for(i = 0; i < VTERM_COUNT; i++) {
		close(drvIds[i]);
		vtctrl_destroy(vt_get(i));
	}
	return EXIT_SUCCESS;
}

static sVTerm *getVTerm(int sid) {
	size_t i;
	for(i = 0; i < VTERM_COUNT; i++) {
		if(drvIds[i] == sid)
			return vt_get(i);
	}
	return NULL;
}
