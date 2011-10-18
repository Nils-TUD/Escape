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
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include "vterm.h"

#define KB_DATA_BUF_SIZE	128
/* max number of requests handled in a row; we have to look sometimes for the keyboard.. */
#define MAX_SEQREQ			20

static int vtermThread(void *vterm);
static void sigUsr1(int sig);
static void kmMngThread(void);

static sVTermCfg cfg;

int main(void) {
	size_t i;
	int drvIds[VTERM_COUNT] = {-1};
	char name[MAX_VT_NAME_LEN + 5 + 1];
	cfg.readKb = true;
	cfg.enabled = false;

	/* reg devices */
	for(i = 0; i < VTERM_COUNT; i++) {
		snprintf(name,sizeof(name),"/dev/vterm%d",i);
		drvIds[i] = createdev(name,DEV_TYPE_CHAR,DEV_READ | DEV_WRITE);
		if(drvIds[i] < 0)
			error("Unable to register device '%s'",name);
	}

	/* init vterms */
	if(!vt_initAll(drvIds,&cfg))
		error("Unable to init vterms");

	/* start threads to handle them */
	for(i = 0; i < VTERM_COUNT; i++) {
		if(startThread(vtermThread,vt_get(i)) < 0)
			error("Unable to start thread for vterm %d",i);
	}
	kmMngThread();

	/* clean up */
	for(i = 0; i < VTERM_COUNT; i++) {
		close(drvIds[i]);
		vtctrl_destroy(vt_get(i));
	}
	return EXIT_SUCCESS;
}

static int vtermThread(void *vterm) {
	sVTerm *vt = (sVTerm*)vterm;
	sMsg msg;
	msgid_t mid;
	while(1) {
		int fd = getWork(&vt->sid,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[VTERM] Unable to get work");
		else {
			switch(mid) {
				case MSG_DEV_READ: {
					/* offset is ignored here */
					bool avail;
					size_t count = msg.args.arg2;
					char *data = (char*)malloc(count);
					msg.args.arg1 = vtin_gets(vt,data,count,&avail);
					msg.args.arg2 = avail;
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(data) {
						send(fd,MSG_DEV_READ_RESP,data,count);
						free(data);
					}
					else
						printe("[VTERM] Not enough memory");
				}
				break;
				case MSG_DEV_WRITE: {
					char *data;
					size_t c = msg.args.arg2;
					data = (char*)malloc(c + 1);
					msg.args.arg1 = 0;
					if(data) {
						if(RETRY(receive(fd,&mid,data,c + 1)) >= 0) {
							data[c] = '\0';
							vtout_puts(vt,data,c,true);
							if(cfg.enabled)
								vt_update(vt);
							msg.args.arg1 = c;
						}
						free(data);
					}
					else
						printe("[VTERM] Not enough memory");
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VT_SELECT: {
					size_t index = msg.args.arg1;
					if(index < VTERM_COUNT)
						vt_selectVTerm(index);
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
					/* reenable us, if necessary */
					if(mid == MSG_VT_ENABLE)
						vt_enable();
					/* wakeup thread to start reading from keyboard again */
					if(mid == MSG_VT_ENABLE || mid == MSG_VT_EN_RDKB)
						sendSignalTo(getpid(),SIG_USR1);
					vt_update(vt);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
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

static void sigUsr1(A_UNUSED int sig) {
	/* ignore */
}

static void kmMngThread(void) {
	ssize_t count;
	sKmData kmData[KB_DATA_BUF_SIZE];
	int kbFd;

	if(setSigHandler(SIG_USR1,sigUsr1) < 0)
		error("Unable to announce SIG_USR1-handler");

	kbFd = open("/dev/kmmanager",IO_READ | IO_NOBLOCK);
	if(kbFd < 0)
		error("Unable to open '/dev/kmmanager'");

	while(1) {
		/* if we shouldn't read from keyboard, wait for ever; we'll get waked up by a signal */
		if(!cfg.enabled || !cfg.readKb) {
			wait(EV_NOEVENT,0);
			continue;
		}

		/* read from keyboard and handle the keys */
		count = RETRY(read(kbFd,kmData,sizeof(kmData)));
		if(count <= 0)
			wait(EV_DATA_READABLE,kbFd);
		else {
			sVTerm *vt = vt_getActive();
			sKmData *kmsg = kmData;
			count /= sizeof(sKmData);
			while(count-- > 0) {
				vtin_handleKey(vt,kmsg->keycode,kmsg->modifier,kmsg->character);
				kmsg++;
			}
			vt_update(vt);
		}
	}
	close(kbFd);
}
