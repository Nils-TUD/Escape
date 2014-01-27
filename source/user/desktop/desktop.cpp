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
#include <gui/application.h>
#include <esc/thread.h>
#include <esc/proc.h>
#include "desktopwin.h"

using namespace gui;
using namespace std;

static int childWaitThread(void *arg);

int main() {
	Shortcut sc1("/etc/guishell.bmp","/bin/guishell");
	Shortcut sc2("/etc/calc.bmp","/bin/gcalc");
	Shortcut sc3("/etc/fileman.bmp","/bin/fileman");
	Shortcut sc4("/etc/gtest.bmp","/bin/gtest");
	Shortcut sc5("/etc/settings.bmp","/bin/gsettings");
	Application *app = Application::create();
	shared_ptr<DesktopWin> win = make_control<DesktopWin>(app->getScreenSize());
	win->addShortcut(&sc1);
	win->addShortcut(&sc2);
	win->addShortcut(&sc3);
	win->addShortcut(&sc4);
	win->addShortcut(&sc5);
	win->show();
	if(startthread(childWaitThread,nullptr) < 0)
		error("Unable to start thread");
	app->addWindow(win);
	return app->run();
}

static int childWaitThread(A_UNUSED void *arg) {
	while(1)
		waitchild(nullptr);
	return 0;
}
