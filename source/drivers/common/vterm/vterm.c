/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <esc/mem.h>
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
#include <assert.h>

#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>

#define BUF_SIZE			4096
#define KB_DATA_BUF_SIZE	128
/* max number of requests handled in a row; we have to look sometimes for the keyboard.. */
#define MAX_SEQREQ			20

static int vtermThread(void *vterm);
static void uimInputThread(int uiminFd);
static int vtInit(int id,const char *name,uint cols,uint rows);
static int vtSetVideoMode(int mode);
static void vtUpdate(void);
static void vtSetCursor(sVTerm *vt);

static sScreenMode *modes = NULL;
static size_t modeCount = 0;
static sScreenMode *curMode = NULL;
static char *scrShm = NULL;
static sVTerm vterm;
static char buffer[BUF_SIZE];

int main(int argc,char **argv) {
	if(argc < 4) {
		fprintf(stderr,"Usage: %s <cols> <rows> <name>\n",argv[0]);
		return EXIT_FAILURE;
	}

	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/dev/%s",argv[3]);

	/* reg device */
	int drvId = createdev(path,0770,DEV_TYPE_CHAR,DEV_READ | DEV_WRITE | DEV_CLOSE);
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
	int modeid = vtInit(drvId,argv[3],atoi(argv[1]),atoi(argv[2]));
	if(modeid < 0)
		error("Unable to init vterms");

	/* open uimng's input device */
	int uiminFd = open("/dev/uim-input",IO_MSGS);
	if(uiminFd < 0)
		error("Unable to open '/dev/uim-input'");
	if(uimng_attach(uiminFd,vterm.uimngid) < 0)
		error("Unable to attach to uimanager");

	/* set video mode */
	int res = vtSetVideoMode(modeid);
	if(res < 0) {
		fprintf(stderr,"Unable to set mode: %s\n",strerror(-res));
		return res;
	}
	/* now we're the active client. update screen */
	vtctrl_markScrDirty(&vterm);
	vtUpdate();

	/* start thread to handle the vterm */
	if(startthread(vtermThread,&vterm) < 0)
		error("Unable to start thread for vterm %s",path);

	/* receive input-events from uimanager in this thread */
	uimInputThread(uiminFd);

	/* clean up */
	close(drvId);
	vtctrl_destroy(&vterm);
	return EXIT_SUCCESS;
}

static void uimInputThread(int uiminFd) {
	/* read from uimanager and handle the keys */
	while(1) {
		sUIMData kmData;
		ssize_t count = IGNSIGS(receive(uiminFd,NULL,&kmData,sizeof(kmData)));
		if(count > 0 && kmData.type == KM_EV_KEYBOARD) {
			vtin_handleKey(&vterm,kmData.d.keyb.keycode,kmData.d.keyb.modifier,kmData.d.keyb.character);
			vtUpdate();
		}
	}
	close(uiminFd);
}

static int vtermThread(A_UNUSED void *arg) {
	sMsg msg;
	msgid_t mid;
	while(1) {
		int fd = getwork(vterm.sid,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("Unable to get work");
		else {
			switch(mid) {
				case MSG_DEV_READ: {
					/* offset is ignored here */
					int avail;
					size_t count = msg.args.arg2;
					char *data = count <= BUF_SIZE ? buffer : (char*)malloc(count);
					msg.args.arg1 = 0;
					msg.args.arg2 = READABLE_DONT_SET;
					if(data)
						msg.args.arg1 = vtin_gets(&vterm,data,count,&avail);
					else
						printe("Not enough memory to read %zu bytes",count);
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(msg.args.arg1)
						send(fd,MSG_DEV_READ_RESP,data,msg.args.arg1);
					if(count > BUF_SIZE)
						free(data);
				}
				break;
				case MSG_DEV_WRITE: {
					size_t count = msg.args.arg2;
					char *data = count <= BUF_SIZE ? buffer : (char*)malloc(count + 1);
					msg.args.arg1 = 0;
					if(data) {
						if(IGNSIGS(receive(fd,&mid,data,count + 1)) >= 0) {
							data[count] = '\0';
							vtout_puts(&vterm,data,count,true);
							vtUpdate();
							msg.args.arg1 = count;
						}
						if(count > BUF_SIZE)
							free(data);
					}
					else
						printe("Not enough memory to write %zu bytes",count + 1);
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_SCR_GETMODES: {
					if(msg.args.arg1)
						send(fd,MSG_DEF_RESPONSE,modes,sizeof(sScreenMode) * modeCount);
					else {
						msg.args.arg1 = modeCount;
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					}
				}
				break;

				case MSG_SCR_GETMODE: {
					msg.data.arg1 = screen_getMode(vterm.uimng,(sScreenMode*)msg.data.d);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;

				case MSG_UIM_GETKEYMAP: {
					msg.str.arg1 = uimng_getKeymap(vterm.uimng,msg.str.s1,sizeof(msg.str.s1));
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.str));
				}
				break;

				case MSG_UIM_SETKEYMAP: {
					msg.args.arg1 = uimng_setKeymap(vterm.uimng,msg.str.s1);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VT_SETMODE: {
					msg.data.arg1 = vtSetVideoMode(msg.data.arg1);
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
					msg.data.arg1 = vtctrl_control(&vterm,mid,msg.data.d);
					vtUpdate();
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
					break;

				case MSG_DEV_CLOSE:
					close(fd);
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

static int vtInit(int id,const char *name,uint cols,uint rows) {
	sScreenMode mode;
	int res;

	/* connect to ui-manager */
	vterm.uimng = open("/dev/uim-ctrl",IO_MSGS);
	if(vterm.uimng < 0)
		error("Unable to open '/dev/uim-ctrl'");
	vterm.uimngid = uimng_getId(vterm.uimng);

	/* get all modes */
	modeCount = screen_getModeCount(vterm.uimng);
	/* TODO actually, we need to filter out the only-gui-modes here */
	modes = (sScreenMode*)malloc(modeCount * sizeof(sScreenMode));
	if(!modes)
		error("Unable to create modes array");
	if((res = screen_getModes(vterm.uimng,modes,modeCount)) < 0)
		error("Unable to get modes");

	/* find a suitable mode */
	res = screen_findTextMode(vterm.uimng,cols,rows,&mode);
	if(res < 0) {
		fprintf(stderr,"Unable to find a suitable mode: %s\n",strerror(-res));
		return res;
	}

	/* open speaker */
	vterm.speaker = open("/dev/speaker",IO_MSGS);
	/* ignore errors here. in this case we simply don't use it */

	vterm.index = 0;
	vterm.sid = id;
	vterm.defForeground = LIGHTGRAY;
	vterm.defBackground = BLACK;
	vterm.setCursor = vtSetCursor;
	memcpy(vterm.name,name,MAX_VT_NAME_LEN + 1);
	if(!vtctrl_init(&vterm,&mode))
		return -ENOMEM;
	return mode.id;
}

static void vtUpdate(void) {
	usemdown(&vterm.usem);
	/* if we should scroll, mark the whole screen (without title) as dirty */
	if(vterm.upScroll != 0) {
		vterm.upCol = 0;
		vterm.upRow = MIN(vterm.upRow,0);
		vterm.upHeight = vterm.rows - vterm.upRow;
		vterm.upWidth = vterm.cols;
	}

	if(vterm.upWidth > 0) {
		/* update content */
		assert(vterm.upCol + vterm.upWidth <= vterm.cols);
		assert(vterm.upRow + vterm.upHeight <= vterm.rows);
		size_t offset = vterm.upRow * vterm.cols * 2 + vterm.upCol * 2;
		char **lines = vterm.lines + vterm.firstVisLine + vterm.upRow;
		for(size_t i = 0; i < vterm.upHeight; ++i) {
			memcpy(scrShm + offset + i * vterm.cols * 2,
				lines[i] + vterm.upCol * 2,vterm.upWidth * 2);
		}

		if(screen_update(vterm.uimng,vterm.upCol,vterm.upRow,vterm.upWidth,vterm.upHeight) < 0)
			printe("Unable to update screen");
	}
	vtSetCursor(&vterm);

	/* all synchronized now */
	vterm.upCol = vterm.cols;
	vterm.upRow = vterm.rows;
	vterm.upWidth = 0;
	vterm.upHeight = 0;
	vterm.upScroll = 0;
	usemup(&vterm.usem);
}

static int vt_doSetMode(const char *shmname,sScreenMode *mode) {
	int res = screen_createShm(mode,&scrShm,shmname,VID_MODE_TYPE_TUI,0644);
	if(res < 0)
		return res;
	res = screen_setMode(vterm.uimng,VID_MODE_TYPE_TUI,mode->id,shmname,true);
	if(res < 0) {
		screen_destroyShm(mode,scrShm,shmname,VID_MODE_TYPE_TUI);
		return res;
	}
	return 0;
}

static int vtSetVideoMode(int mode) {
	ssize_t res;
	for(size_t i = 0; i < modeCount; i++) {
		if(modes[i].id == mode) {
			/* rename old shm as a backup */
			char *scrShmTmp = scrShm;
			char tmpname[32];
			if(scrShm) {
				snprintf(tmpname,sizeof(tmpname),"%s-tmp",vterm.name);
				if((res = shm_rename(vterm.name,tmpname)) < 0)
					return res;
			}

			/* try to set new mode */
			res = vt_doSetMode(vterm.name,modes + i);
			if(res < 0) {
				scrShm = scrShmTmp;
				if(scrShm) {
					if(shm_rename(tmpname,vterm.name) < 0)
						error("Unable to restore old mode by renaming; exiting");
				}
				return res;
			}

			if(scrShmTmp) {
				/* the new mode is set, so destroy the old stuff. we have to re-set the mode anyway if
				 * something fails below */
				screen_destroyShm(curMode,scrShmTmp,tmpname,VID_MODE_TYPE_TUI);
			}

			/* resize vterm if necessary */
			if(vterm.cols != modes[i].cols || vterm.rows != modes[i].rows) {
				if(!vtctrl_resize(&vterm,modes[i].cols,modes[i].rows)) {
					res = vt_doSetMode(vterm.name,curMode);
					if(res < 0)
						error("Unable to restore old mode");
					return -ENOMEM;
				}
			}
			curMode = modes + i;
			return 0;
		}
	}
	return -EINVAL;
}

static void vtSetCursor(sVTerm *vt) {
	gpos_t x,y;
	if(vt->upScroll != 0 || vt->col != vt->lastCol || vt->row != vt->lastRow) {
		/* draw no cursor if it's not visible by setting it to an invalid location */
		if(vt->firstVisLine + vt->rows <= vt->currLine + vt->row) {
			x = vt->cols;
			y = vt->rows;
		}
		else {
			x = vt->col;
			y = vt->row;
		}
		screen_setCursor(vt->uimng,x,y,CURSOR_DEFAULT);
		vt->lastCol = x;
		vt->lastRow = y;
	}
}
