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
#include <esc/driver.h>
#include <esc/thread.h>
#include <esc/sync.h>
#include <esc/io.h>
#include <esc/ringbuffer.h>
#include <esc/messages.h>
#include <esc/keycodes.h>
#include <esc/esccodes.h>
#include <ipc/proto/input.h>
#include <ipc/proto/ui.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <mutex>

#include "keymap.h"
#include "clients.h"
#include "keystrokes.h"
#include "jobmng.h"
#include "screens.h"
#include "header.h"
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
	sFileInfo info;
	if(stat("/dev/mouse",&info) < 0)
		return EXIT_FAILURE;

	/* open mouse */
	ipc::IPCStream ms("/dev/mouse");
	ipc::Mouse::Event ev;
	while(1) {
		ms >> ipc::ReceiveData(&ev,sizeof(ev));

		ipc::UIEvents::Event uiev;
		uiev.type = ipc::UIEvents::Event::TYPE_MOUSE;
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

static bool handleKey(ipc::UIEvents::Event *data) {
	if(data->d.keyb.modifier & STATE_BREAK)
		return false;

	if(data->d.keyb.keycode == VK_F12) {
		std::lock_guard<std::mutex> guard(mutex);
		Keystrokes::enterDebugger();
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
			std::lock_guard<std::mutex> guard(mutex);
			UIClient::switchTo(data->d.keyb.keycode - VK_0);
			return true;
		}
		/* we can't lock this because if we do a fork, the child might connect to us and we might
		 * wait until he has registered a device -> deadlock */
		case VK_T:
			Keystrokes::createTextConsole();
			return true;
		case VK_G:
			Keystrokes::createGUIConsole();
			return true;
		case VK_LEFT: {
			std::lock_guard<std::mutex> guard(mutex);
			UIClient::prev();
			return true;
		}
		case VK_RIGHT: {
			std::lock_guard<std::mutex> guard(mutex);
			UIClient::next();
			return true;
		}
	}
	return false;
}

static int kbClientThread(A_UNUSED void *arg) {
	/* open keyboard */
	ipc::IPCStream kb("/dev/keyb");
	ipc::Keyb::Event ev;
	while(1) {
		kb >> ipc::ReceiveData(&ev,sizeof(ev));

		/* translate keycode */
		ipc::UIEvents::Event uiev;
		uiev.type = ipc::UIEvents::Event::TYPE_KEYBOARD;
		uiev.d.keyb.keycode = ev.keycode;

		{
			std::lock_guard<std::mutex> guard(mutex);
			UIClient *active = UIClient::getActive();
			const Keymap *map = active && active->keymap() ? active->keymap() : Keymap::getDefault();
			uiev.d.keyb.character = map->translateKeycode(ev.isBreak,ev.keycode,&uiev.d.keyb.modifier);
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
