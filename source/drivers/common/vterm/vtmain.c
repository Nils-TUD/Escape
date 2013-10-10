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
#include <esc/driver/screen.h>
#include <esc/driver/uimng.h>
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <esc/ringbuffer.h>
#include <usergroup/group.h>
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
static void uimInputThread(void);

static sVTerm vterm;

int main(int argc,char **argv) {
	if(argc < 4) {
		fprintf(stderr,"Usage: %s <cols> <rows> <name>\n",argv[0]);
		return EXIT_FAILURE;
	}

	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/dev/%s",argv[3]);

	/* reg device */
	int drvId = createdev(path,DEV_TYPE_CHAR,DEV_READ | DEV_WRITE);
	if(drvId < 0)
		error("Unable to register device '%s'",path);

	/* we want to give only users that are in the ui-group access to this vterm */
	size_t gcount;
	sGroup *groups = group_parseFromFile(GROUPS_PATH,&gcount);
	sGroup *uigrp = group_getByName(groups,argv[3]);
	if(uigrp != NULL) {
		if(chown(path,ROOT_UID,uigrp->gid) < 0)
			printe("Unable to add ui-group to group-list");
	}
	else
		printe("Unable to find ui-group '%s'",argv[3]);
	group_free(groups);

	/* init vterms */
	if(!vt_init(drvId,&vterm,argv[3],atoi(argv[1]),atoi(argv[2])))
		error("Unable to init vterms");

	/* start thread to handle the vterm */
	if(startthread(vtermThread,&vterm) < 0)
		error("Unable to start thread for vterm %s",path);

	/* receive input-events from uimanager in this thread */
	uimInputThread();

	/* clean up */
	close(drvId);
	vtctrl_destroy(&vterm);
	return EXIT_SUCCESS;
}

static void uimInputThread(void) {
	int uiminFd = open("/dev/uim-input",IO_MSGS);
	if(uiminFd < 0)
		error("Unable to open '/dev/uim-input'");
	if(uimng_attach(uiminFd,vterm.uimngid) < 0)
		error("Unable to attach to uimanager");

	/* now we're the active client. update screen */
	vtctrl_markScrDirty(&vterm);
	vt_update(&vterm);

	/* read from uimanager and handle the keys */
	while(1) {
		sUIMData kmData;
		ssize_t count = IGNSIGS(receive(uiminFd,NULL,&kmData,sizeof(kmData)));
		if(count > 0 && kmData.type == KM_EV_KEYBOARD) {
			vtin_handleKey(&vterm,kmData.d.keyb.keycode,kmData.d.keyb.modifier,kmData.d.keyb.character);
			vt_update(&vterm);
		}
	}
	close(uiminFd);
}

static int vtermThread(void *vtptr) {
	sVTerm *vt = (sVTerm*)vtptr;
	sMsg msg;
	msgid_t mid;
	while(1) {
		int fd = getwork(vt->sid,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[VTERM] Unable to get work");
		else {
			switch(mid) {
				case MSG_DEV_READ: {
					/* offset is ignored here */
					int avail;
					size_t count = msg.args.arg2;
					char *data = (char*)malloc(count);
					msg.args.arg1 = 0;
					msg.args.arg2 = READABLE_DONT_SET;
					if(data)
						msg.args.arg1 = vtin_gets(vt,data,count,&avail);
					else
						printe("[VTERM] Not enough memory to read %zu bytes",count);
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(msg.args.arg1)
						send(fd,MSG_DEV_READ_RESP,data,msg.args.arg1);
					free(data);
				}
				break;
				case MSG_DEV_WRITE: {
					char *data;
					size_t c = msg.args.arg2;
					data = (char*)malloc(c + 1);
					msg.args.arg1 = 0;
					if(data) {
						if(IGNSIGS(receive(fd,&mid,data,c + 1)) >= 0) {
							data[c] = '\0';
							vtout_puts(vt,data,c,true);
							vt_update(vt);
							msg.args.arg1 = c;
						}
						free(data);
					}
					else
						printe("[VTERM] Not enough memory to write %zu bytes",c + 1);
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_SCR_GETMODES: {
					size_t count;
					sScreenMode *modes = vt_getModes(vt,msg.args.arg1,&count);
					if(modes) {
						send(fd,MSG_DEF_RESPONSE,modes,sizeof(sScreenMode) * count);
						free(modes);
					}
					else {
						msg.args.arg1 = count;
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					}
				}
				break;

				case MSG_SCR_GETMODE: {
					msg.data.arg1 = screen_getMode(vt->uimng,(sScreenMode*)msg.data.d);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;

				case MSG_UIM_SETKEYMAP: {
					msg.args.arg1 = uimng_setKeymap(vt->uimng,msg.str.s1);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VT_SETMODE: {
					msg.data.arg1 = vt_setVideoMode(vt,msg.data.arg1);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;

				case MSG_VT_SHELLPID:
				case MSG_VT_EN_ECHO:
				case MSG_VT_DIS_ECHO:
				case MSG_VT_EN_RDLINE:
				case MSG_VT_DIS_RDLINE:
				case MSG_VT_EN_NAVI:
				case MSG_VT_DIS_NAVI:
				case MSG_VT_BACKUP:
				case MSG_VT_RESTORE:
				case MSG_VT_ISVTERM:
					msg.data.arg1 = vtctrl_control(vt,mid,msg.data.d);
					vt_update(vt);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
					break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
	}
	return 0;
}
