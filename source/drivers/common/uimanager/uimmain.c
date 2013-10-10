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
#include <esc/driver/screen.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/io.h>
#include <esc/ringbuffer.h>
#include <esc/messages.h>
#include <esc/keycodes.h>
#include <esc/esccodes.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "keymap.h"
#include "clients.h"
#include "keystrokes.h"
#include "jobs.h"

#define KEYMAP_FILE					"/etc/keymap"

static int mouseClientThread(void *arg);
static int kbClientThread(void *arg);
static int ctrlThread(A_UNUSED void *arg);
static int inputThread(A_UNUSED void *arg);

static sKeymap *defmap;
static tULock lck;
static sScreen *screens;
static size_t screenCount;

int main(int argc,char *argv[]) {
	char path[MAX_PATH_LEN];
	char *newline;
	FILE *f;

	if(argc < 2) {
		fprintf(stderr,"Usage: %s <driver>...\n",argv[0]);
		return EXIT_FAILURE;
	}

	if(crtlocku(&lck) < 0)
		error("Unable to create uimanager-lock");
	jobs_init();
	keys_init();
	srand(time(NULL));

	/* open screen-devices */
	screenCount = argc - 1;
	screens = (sScreen*)malloc(sizeof(sScreen) * screenCount);
	for(int i = 1; i < argc; i++) {
		sScreen *be = screens + i - 1;
		be->name = strdup(argv[i]);
		if(be->name == NULL) {
			printe("Unable to clone device name");
			return EXIT_FAILURE;
		}
		int fd = open(be->name,IO_READ | IO_WRITE | IO_MSGS);
		if(fd < 0) {
			printe("Unable to open '%s'",be->name);
			return EXIT_FAILURE;
		}

		/* get modes */
		be->modeCount = screen_getModeCount(fd);
		be->modes = (sScreenMode*)malloc(be->modeCount * sizeof(sScreenMode));
		if(!be->modes) {
			printe("Unable to allocate modes for '%s'",be->name);
			return EXIT_FAILURE;
		}
		if(screen_getModes(fd,be->modes,be->modeCount) < 0) {
			printe("Unable to get modes for '%s'",be->name);
			return EXIT_FAILURE;
		}
		close(fd);
	}

	/* determine default keymap */
	f = fopen(KEYMAP_FILE,"r");
	if(f == NULL)
		error("[UIM] Unable to open %s",KEYMAP_FILE);
	fgets(path,MAX_PATH_LEN,f);
	if((newline = strchr(path,'\n')))
		*newline = '\0';
	fclose(f);

	/* load default map */
	defmap = km_request(path);
	if(!defmap)
		error("[UIM] Unable to load default keymap");

	/* start helper threads */
	if(startthread(kbClientThread,NULL) < 0)
		error("[UIM] Unable to start thread for reading from kb");
	if(startthread(mouseClientThread,NULL) < 0)
		error("[UIM] Unable to start thread for reading from mouse");
	if(startthread(inputThread,NULL) < 0)
		error("[UIM] Unable to start thread for handling uim-input");
	if(startthread(ctrlThread,NULL) < 0)
		error("[UIM] Unable to start thread for handling uim-ctrl");

	/* now wait for terminated childs */
	jobs_wait();
	return 0;
}

static int mouseClientThread(A_UNUSED void *arg) {
	/* open mouse */
	int kbFd = open("/dev/mouse",IO_MSGS);
	if(kbFd < 0)
		error("[UIM] Unable to open '/dev/mouse'");

	while(1) {
		sMouseData mouseData;
		ssize_t res = IGNSIGS(receive(kbFd,NULL,&mouseData,sizeof(mouseData)));
		if(res < 0)
			printe("[UIM] receive from mouse failed");

		sUIMData data;
		data.type = KM_EV_MOUSE;
		memcpy(&data.d.mouse,&mouseData,sizeof(mouseData));
		locku(&lck);
		cli_send(&data,sizeof(data));
		unlocku(&lck);
	}
	return EXIT_SUCCESS;
}

static int kbClientThread(A_UNUSED void *arg) {
	/* open keyboard */
	int kbFd = open("/dev/keyboard",IO_MSGS);
	if(kbFd < 0)
		error("[UIM] Unable to open '/dev/keyboard'");

	while(1) {
		sKbData kbData;
		ssize_t res = IGNSIGS(receive(kbFd,NULL,&kbData,sizeof(kbData)));
		if(res < 0)
			printe("[UIM] receive from keyboard failed");

		/* translate keycode */
		sUIMData data;
		data.type = KM_EV_KEYBOARD;
		data.d.keyb.keycode = kbData.keycode;

		locku(&lck);
		sClient *active = cli_getActive();
		data.d.keyb.character = km_translateKeycode(
				active ? active->map : defmap,kbData.isBreak,kbData.keycode,&data.d.keyb.modifier);
		unlocku(&lck);

		/* we can't lock this because if we do a fork, the child might connect to us and we might
		 * wait until he has registered a device -> deadlock */
		if(!keys_handleKey(&data)) {
			locku(&lck);
			cli_send(&data,sizeof(data));
			unlocku(&lck);
		}
	}
	return EXIT_SUCCESS;
}

static bool findBackend(int mid,sScreenMode **mode,sScreen **be) {
	for(size_t i = 0; i < screenCount; ++i) {
		for(size_t j = 0; j < screens[i].modeCount; ++j) {
			if(screens[i].modes[j].id == mid) {
				*mode = screens[i].modes + j;
				*be = screens + i;
				return true;
			}
		}
	}
	return false;
}

static int setMode(sClient *cli,int type,int mid,const char *shm) {
	if(cli->screen) {
		close(cli->screenFd);
		free(cli->screenShmName);
		cli->screen = NULL;
	}

	sScreenMode *mode;
	sScreen *be;
	if(findBackend(mid,&mode,&be)) {
		cli->type = type;
		cli->screenFd = open(be->name,IO_MSGS);
		if(cli->screenFd < 0)
			return cli->screenFd;
		int res;
		if((res = screen_setMode(cli->screenFd,cli->type,mid,shm,true)) < 0) {
			close(cli->screenFd);
			return res;
		}
		cli->screen = be;
		cli->screenMode = mode;
		cli->screenShmName = strdup(shm);
		return 0;
	}
	return -EINVAL;
}

static int ctrlThread(A_UNUSED void *arg) {
	int id = createdev("/dev/uim-ctrl",DEV_TYPE_CHAR,DEV_OPEN | DEV_CLOSE);
	if(id < 0)
		error("[UIM] Unable to register device 'uim-ctrl'");

	/* create first client */
	keys_createTextConsole();

	while(1) {
		sMsg msg;
		msgid_t mid;
		int fd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[UIM] Unable to get work");
		else {
			switch(mid) {
				case MSG_DEV_OPEN: {
					locku(&lck);
					msg.args.arg1 = cli_add(fd,defmap);;
					unlocku(&lck);
					send(fd,MSG_DEV_OPEN_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_CLOSE: {
					locku(&lck);
					cli_remove(fd);
					close(fd);
					unlocku(&lck);
				}
				break;

				case MSG_UIM_GETID: {
					msg.args.arg1 = cli_get(fd)->randid;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_UIM_SETKEYMAP: {
					char *str = msg.str.s1;
					str[sizeof(msg.str.s1) - 1] = '\0';
					msg.args.arg1 = -EINVAL;
					sKeymap *newMap = km_request(str);
					if(newMap) {
						sClient *client = cli_getActive();
						if(client) {
							km_release(client->map);
							client->map = newMap;
							msg.args.arg1 = 0;
						}
					}
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_SCR_GETMODES: {
					size_t count = 0;
					for(size_t i = 0; i < screenCount; ++i)
						count += screens[i].modeCount;
					if(msg.args.arg1 == 0) {
						msg.args.arg1 = count;
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					}
					else {
						sScreenMode *modes = (sScreenMode*)malloc(count * sizeof(sScreenMode));
						size_t pos = 0;
						if(modes) {
							for(size_t i = 0; i < screenCount; ++i) {
								memcpy(modes + pos,screens[i].modes,
									screens[i].modeCount * sizeof(sScreenMode));
								pos += screens[i].modeCount;
							}
						}
						send(fd,MSG_DEF_RESPONSE,modes,pos * sizeof(sScreenMode));
						free(modes);
					}
				}
				break;

				case MSG_SCR_GETMODE: {
					sClient *cli = cli_get(fd);
					msg.data.arg1 = -EINVAL;
					if(cli->screen) {
						msg.data.arg1 = 0;
						memcpy(msg.data.d,cli->screenMode,sizeof(*cli->screenMode));
					}
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;

				case MSG_SCR_SETMODE: {
					int modeid = (int)msg.str.arg1;
					int type = msg.str.arg2;
					sClient *cli = cli_get(fd);
					msg.args.arg1 = setMode(cli,type,modeid,msg.str.s1);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_SCR_SETCURSOR: {
					sClient *cli = cli_get(fd);
					if(cli == cli_getActive()) {
						cli->cursor.x = msg.args.arg1;
						cli->cursor.y = msg.args.arg2;
						cli->cursor.cursor = msg.args.arg3;
						screen_setCursor(cli->screenFd,cli->cursor.x,cli->cursor.y,cli->cursor.cursor);
					}
				}
				break;

				case MSG_SCR_UPDATE: {
					gpos_t x = (gpos_t)msg.args.arg1;
					gpos_t y = (gpos_t)msg.args.arg2;
					gsize_t width = (gsize_t)msg.args.arg3;
					gsize_t height = (gsize_t)msg.args.arg4;
					sClient *cli = cli_get(fd);
					msg.args.arg1 = 0;
					if(cli == cli_getActive())
						msg.args.arg1 = screen_update(cli->screenFd,x,y,width,height);
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;
			}
		}
	}
	return 0;
}

static int inputThread(A_UNUSED void *arg) {
	int id = createdev("/dev/uim-input",DEV_TYPE_CHAR,DEV_CLOSE);
	if(id < 0)
		error("[UIM] Unable to register device 'uim-input'");
	while(1) {
		sMsg msg;
		msgid_t mid;
		int fd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[UIM] Unable to get work");
		else {
			switch(mid) {
				case MSG_UIM_ATTACH: {
					locku(&lck);
					msg.args.arg1 = cli_attach(fd,msg.args.arg1);
					unlocku(&lck);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_CLOSE: {
					locku(&lck);
					cli_detach(fd);
					close(fd);
					unlocku(&lck);
				}
				break;
			}
		}
	}
	return 0;
}
