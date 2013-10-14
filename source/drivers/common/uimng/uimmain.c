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
#include "screens.h"
#include "header.h"

#define KEYMAP_FILE					"/etc/keymap"

static int mouseClientThread(void *arg);
static int kbClientThread(void *arg);
static int ctrlThread(A_UNUSED void *arg);
static int header_thread(A_UNUSED void *arg);
static int inputThread(A_UNUSED void *arg);

static char defKeymapPath[MAX_PATH_LEN];
static sKeymap *defmap;
static tULock lck;

int main(int argc,char *argv[]) {
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
	header_init();
	screens_init(argc - 1,argv + 1);
	srand(time(NULL));

	/* determine default keymap */
	f = fopen(KEYMAP_FILE,"r");
	if(f == NULL)
		error("Unable to open %s",KEYMAP_FILE);
	fgets(defKeymapPath,MAX_PATH_LEN,f);
	if((newline = strchr(defKeymapPath,'\n')))
		*newline = '\0';
	fclose(f);

	/* load default map */
	defmap = km_request(defKeymapPath);
	if(!defmap)
		error("Unable to load default keymap");

	/* start helper threads */
	if(startthread(kbClientThread,NULL) < 0)
		error("Unable to start thread for reading from kb");
	if(startthread(mouseClientThread,NULL) < 0)
		error("Unable to start thread for reading from mouse");
	if(startthread(inputThread,NULL) < 0)
		error("Unable to start thread for handling uim-input");
	if(startthread(ctrlThread,NULL) < 0)
		error("Unable to start thread for handling uim-ctrl");
	if(startthread(header_thread,NULL) < 0)
		error("Unable to start thread for drawing the header");

	/* now wait for terminated childs */
	jobs_wait();
	return 0;
}

static int mouseClientThread(A_UNUSED void *arg) {
	/* open mouse */
	int kbFd = open("/dev/mouse",IO_MSGS);
	if(kbFd < 0)
		error("Unable to open '/dev/mouse'");

	while(1) {
		sMouseData mouseData;
		ssize_t res = IGNSIGS(receive(kbFd,NULL,&mouseData,sizeof(mouseData)));
		if(res < 0)
			printe("receive from mouse failed");

		sUIMData data;
		data.type = KM_EV_MOUSE;
		memcpy(&data.d.mouse,&mouseData,sizeof(mouseData));
		locku(&lck);
		cli_send(&data,sizeof(data));
		unlocku(&lck);
	}
	return EXIT_SUCCESS;
}

static bool handleKey(sUIMData *data) {
	if(data->d.keyb.modifier & STATE_BREAK)
		return false;

	if(data->d.keyb.keycode == VK_F12) {
		locku(&lck);
		keys_enterDebugger();
		unlocku(&lck);
		return true;
	}

	if(!(data->d.keyb.modifier & STATE_CTRL))
		return false;

	switch(data->d.keyb.keycode) {
		case VK_0:
		case VK_1:
		case VK_2:
		case VK_3:
		case VK_4:
		case VK_5:
		case VK_6:
		case VK_7:
			locku(&lck);
			cli_switchTo(data->d.keyb.keycode - VK_0);
			unlocku(&lck);
			return true;
		/* we can't lock this because if we do a fork, the child might connect to us and we might
		 * wait until he has registered a device -> deadlock */
		case VK_T:
			keys_createTextConsole();
			return true;
		case VK_G:
			keys_createGUIConsole();
			return true;
		case VK_LEFT:
			locku(&lck);
			cli_prev();
			unlocku(&lck);
			return true;
		case VK_RIGHT:
			locku(&lck);
			cli_next();
			unlocku(&lck);
			return true;
	}
	return false;
}

static int kbClientThread(A_UNUSED void *arg) {
	/* open keyboard */
	int kbFd = open("/dev/keyb",IO_MSGS);
	if(kbFd < 0)
		error("Unable to open '/dev/keyb'");

	while(1) {
		sKbData kbData;
		ssize_t res = IGNSIGS(receive(kbFd,NULL,&kbData,sizeof(kbData)));
		if(res < 0)
			printe("receive from keyboard failed");

		/* translate keycode */
		sUIMData data;
		data.type = KM_EV_KEYBOARD;
		data.d.keyb.keycode = kbData.keycode;

		locku(&lck);
		sClient *active = cli_getActive();
		data.d.keyb.character = km_translateKeycode(
				active ? active->map : defmap,kbData.isBreak,kbData.keycode,&data.d.keyb.modifier);
		unlocku(&lck);

		/* the create-console commands can't be locked */
		if(!handleKey(&data)) {
			locku(&lck);
			cli_send(&data,sizeof(data));
			unlocku(&lck);
		}
	}
	return EXIT_SUCCESS;
}

static int ctrlThread(A_UNUSED void *arg) {
	int id = createdev("/dev/uim-ctrl",DEV_TYPE_CHAR,DEV_OPEN | DEV_CLOSE);
	if(id < 0)
		error("Unable to register device 'uim-ctrl'");

	/* create first client */
	keys_createTextConsole();

	while(1) {
		sMsg msg;
		msgid_t mid;
		int fd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("Unable to get work");
		else {
			switch(mid) {
				case MSG_DEV_OPEN: {
					locku(&lck);
					msg.args.arg1 = cli_add(fd,defKeymapPath);
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

				case MSG_UIM_GETKEYMAP: {
					sClient *cli = cli_get(fd);
					msg.str.arg1 = cli->map != NULL;
					strnzcpy(msg.str.s1,cli->map->path,sizeof(msg.str.s1));
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.str));
				}
				break;

				case MSG_UIM_SETKEYMAP: {
					char *str = msg.str.s1;
					str[sizeof(msg.str.s1) - 1] = '\0';
					msg.args.arg1 = -EINVAL;
					sKeymap *newMap = km_request(str);
					if(newMap) {
						/* we don't need to lock this, because the client is only removed if this
						 * device is closed. since this device is only handled by one thread, it
						 * can't happen now */
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
					size_t n = msg.args.arg1;
					sScreenMode *modes = n == 0 ? NULL : (sScreenMode*)malloc(n* sizeof(sScreenMode));
					ssize_t res = screens_getModes(modes,n);
					if(n == 0) {
						msg.args.arg1 = res;
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					}
					else {
						send(fd,MSG_DEF_RESPONSE,modes,res > 0 ? msg.args.arg1 * sizeof(sScreenMode) : 0);
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
						screens_adjustMode((sScreenMode*)msg.data.d);
					}
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;

				case MSG_SCR_SETMODE: {
					int modeid = (int)msg.str.arg1;
					int type = msg.str.arg2;
					/* lock that to prevent that we interfere with e.g. the debugger keystroke */
					locku(&lck);
					sClient *cli = cli_get(fd);
					msg.args.arg1 = screens_setMode(cli,type,modeid,msg.str.s1);
					/* update header */
					if(msg.args.arg1 == 0) {
						gsize_t width,height;
						if(header_update(cli,&width,&height))
							screen_update(cli->screenFd,0,0,width,height);
					}
					unlocku(&lck);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_SCR_SETCURSOR: {
					sClient *cli = cli_get(fd);
					if(cli == cli_getActive()) {
						cli->cursor.x = msg.args.arg1;
						cli->cursor.y = msg.args.arg2;
						cli->cursor.cursor = msg.args.arg3;
						cli->cursor.y += header_getHeight(cli->type);
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
					if(cli == cli_getActive()) {
						y += header_getHeight(cli->type);
						msg.args.arg1 = screen_update(cli->screenFd,x,y,width,height);
					}
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;
			}
		}
	}
	return 0;
}

static int header_thread(A_UNUSED void *arg) {
	while(1) {
		locku(&lck);
		sClient *active = cli_getActive();
		if(active && active->screenMode) {
			gsize_t width,height;
			if(header_rebuild(active,&width,&height))
				screen_update(active->screenFd,0,0,width,height);
		}
		unlocku(&lck);
		sleep(1000);
	}
	return 0;
}

static int inputThread(A_UNUSED void *arg) {
	int id = createdev("/dev/uim-input",DEV_TYPE_CHAR,DEV_CLOSE);
	if(id < 0)
		error("Unable to register device 'uim-input'");
	while(1) {
		sMsg msg;
		msgid_t mid;
		int fd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("Unable to get work");
		else {
			switch(mid) {
				case MSG_UIM_ATTACH: {
					locku(&lck);
					msg.args.arg1 = cli_attach(fd,msg.args.arg1);
					/* update header */
					if(msg.args.arg1 == 0) {
						gsize_t width,height;
						sClient *cli = cli_get(fd);
						if(header_update(cli,&width,&height))
							screen_update(cli->screenFd,0,0,width,height);
					}
					/* TODO actually, we should remove the entire client if this failed to make it
					 * harder to hijack a foreign session via brute-force. but this would destroy
					 * our lock-strategy because we assume currently that only the other device
					 * destroys the session on close. */
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
