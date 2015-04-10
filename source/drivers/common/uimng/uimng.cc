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

#include <esc/proto/input.h>
#include <esc/proto/ui.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/esccodes.h>
#include <sys/io.h>
#include <sys/keycodes.h>
#include <sys/messages.h>
#include <sys/sync.h>
#include <sys/thread.h>
#include <usergroup/group.h>
#include <errno.h>
#include <mutex>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "clients.h"
#include "header.h"
#include "jobmng.h"
#include "keymap.h"
#include "keystrokes.h"
#include "screens.h"
#include "uimngdev.h"

static int mouseClientThread(void *arg);
static int kbClientThread(void *arg);
static int ctrlThread(A_UNUSED void *arg);
static int headerThread(A_UNUSED void *arg);

static std::mutex mutex;

int main(int argc,char *argv[]) {
	if(argc < 2) {
		fprintf(stderr,"Usage: %s <driver>...\n",argv[0]);
		return EXIT_FAILURE;
	}

	Header::init();
	ScreenMng::init(argc - 1,argv + 1);
	srand(time(NULL));

	size_t gcount;
	sGroup *groups = group_parseFromFile(GROUPS_PATH,&gcount);
	sGroup *uigrp = group_getByName(groups,"output");
	if(!uigrp)
		error("Unable to find group 'output'");

	/* create clipboard */
	int fd = create(esc::Clipboard::PATH,O_WRONLY | O_CREAT | O_TRUNC,0660);
	if(fd < 0)
		error("Unable to create clipboard");
	// TODO a fchown would be nice
	if(chown(esc::Clipboard::PATH,-1,uigrp->gid) < 0)
		error("Unable to change clipboard owner");
	close(fd);

	/* start helper threads */
	if(startthread(kbClientThread,NULL) < 0)
		error("Unable to start thread for reading from kb");
	if(startthread(mouseClientThread,NULL) < 0)
		error("Unable to start thread for reading from mouse");
	if(startthread(ctrlThread,NULL) < 0)
		error("Unable to start thread for handling uimng");
	if(startthread(headerThread,NULL) < 0)
		error("Unable to start thread for drawing the header");

	/* now wait for terminated childs */
	JobMng::wait();
	return 0;
}

static int mouseClientThread(A_UNUSED void *arg) {
	struct stat info;
	if(stat("/dev/mouse",&info) < 0)
		return EXIT_FAILURE;

	/* open mouse */
	esc::IPCStream ms("/dev/mouse");
	esc::Mouse::Event ev;
	while(1) {
		ms >> esc::ReceiveData(&ev,sizeof(ev));

		esc::UIEvents::Event uiev;
		uiev.type = esc::UIEvents::Event::TYPE_MOUSE;
		uiev.d.mouse = ev;

		try {
			std::lock_guard<std::mutex> guard(mutex);
			UIClient::send(&uiev,sizeof(uiev));
		}
		catch(const std::exception &e) {
			printe("mouseClientThread: %s",e.what());
		}
	}
	return EXIT_SUCCESS;
}

static bool handleKey(esc::UIEvents::Event *data) {
	if(data->d.keyb.keycode == VK_F12) {
		if(~data->d.keyb.modifier & STATE_BREAK) {
			std::lock_guard<std::mutex> guard(mutex);
			Keystrokes::enterDebugger();
		}
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
		case VK_7: {
			if(~data->d.keyb.modifier & STATE_BREAK) {
				std::lock_guard<std::mutex> guard(mutex);
				UIClient::switchTo(data->d.keyb.keycode - VK_0);
			}
			return true;
		}
		/* we can't lock this because if we do a fork, the child might connect to us and we might
		 * wait until he has registered a device -> deadlock */
		case VK_T:
			if(~data->d.keyb.modifier & STATE_BREAK)
				Keystrokes::createTextConsole();
			return true;
		case VK_G:
			if(~data->d.keyb.modifier & STATE_BREAK)
				Keystrokes::createGUIConsole();
			return true;
		case VK_LEFT: {
			if(~data->d.keyb.modifier & STATE_BREAK) {
				std::lock_guard<std::mutex> guard(mutex);
				UIClient::prev();
			}
			return true;
		}
		case VK_RIGHT: {
			if(~data->d.keyb.modifier & STATE_BREAK) {
				std::lock_guard<std::mutex> guard(mutex);
				UIClient::next();
			}
			return true;
		}
	}
	return false;
}

static int kbClientThread(A_UNUSED void *arg) {
	/* open keyboard */
	esc::IPCStream kb("/dev/keyb");
	esc::Keyb::Event ev;
	while(1) {
		kb >> esc::ReceiveData(&ev,sizeof(ev));

		/* translate keycode */
		esc::UIEvents::Event uiev;
		uiev.type = esc::UIEvents::Event::TYPE_KEYBOARD;
		uiev.d.keyb.keycode = ev.keycode;

		{
			std::lock_guard<std::mutex> guard(mutex);
			UIClient *active = UIClient::getActive();
			const Keymap *map = active && active->keymap() ? active->keymap() : Keymap::getDefault();
			uiev.d.keyb.character = map->translateKeycode(ev.flags,ev.keycode,&uiev.d.keyb.modifier);
		}

		/* mode switching etc. might fail */
		try {
			/* the create-console commands can't be locked */
			if(!handleKey(&uiev)) {
				std::lock_guard<std::mutex> guard(mutex);
				UIClient::send(&uiev,sizeof(uiev));
			}
		}
		catch(const std::exception &e) {
			printe("kbClientThread: %s",e.what());
		}
	}
	return EXIT_SUCCESS;
}

static int ctrlThread(A_UNUSED void *arg) {
	UIMngDevice dev("/dev/uimng",0110,mutex);

	/* create first client */
	Keystrokes::createTextConsole();

	dev.loop();
	return 0;
}

static int headerThread(A_UNUSED void *arg) {
	while(1) {
		{
			std::lock_guard<std::mutex> guard(mutex);
			UIClient *active = UIClient::getActive();
			if(active && active->fb()) {
				gsize_t width,height;
				try {
					if(Header::rebuild(active,&width,&height))
						active->screen()->update(0,0,width,height);
				}
				catch(const std::exception &e) {
					printe("headerThread: %s",e.what());
				}
			}
		}

		sleep(1000);
	}
	return 0;
}
