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

#include <gui/application.h>
#include <sys/common.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/wait.h>

#include "desktopwin.h"

using namespace gui;
using namespace std;

static int childWaitThread(void *arg);

static int childsm;

static const struct {
	const char *icon;
	const char *args[4];
} shortcuts[] = {
	{"/etc/guishell.png",	{"/bin/guishell",	NULL,	NULL,	NULL}},
	{"/etc/calc.png",		{"/bin/gcalc",		NULL,	NULL,	NULL}},
	{"/etc/fileman.png",	{"/bin/fileman",	NULL,	NULL,	NULL}},
	{"/etc/gtest.png",		{"/bin/gtest",		NULL,	NULL,	NULL}},
	{"/etc/settings.png",	{"/bin/gsettings",	NULL,	NULL,	NULL}},
	{"/etc/cpugraph.png",	{"/bin/cpugraph",	NULL,	NULL,	NULL}},
	{"/etc/plasma.png",		{"/bin/plasma",		"-m",	"win",	NULL}},
};

int main() {
	if((childsm = semcrt(0)) < 0)
		error("Unable to create semaphore");

	Application *app = Application::create();
	shared_ptr<DesktopWin> win = make_control<DesktopWin>(app->getScreenSize(),childsm);
	for(size_t i = 0; i < ARRAY_SIZE(shortcuts); ++i)
		win->addShortcut(new Shortcut(shortcuts[i].icon,const_cast<const char**>(shortcuts[i].args)));
	win->show();
	if(startthread(childWaitThread,nullptr) < 0)
		error("Unable to start thread");
	app->addWindow(win);
	return app->run();
}

static int childWaitThread(A_UNUSED void *arg) {
	while(1) {
		semdown(childsm);
		sExitState state;
		if(waitchild(&state,-1) < 0)
			printe("waitchild");
	}
	return 0;
}
