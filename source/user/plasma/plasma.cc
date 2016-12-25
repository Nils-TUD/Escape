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

#include <esc/proto/ui.h>
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <esc/util.h>
#include <sys/common.h>
#include <sys/esccodes.h>
#include <sys/keycodes.h>
#include <sys/thread.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>

#include "painter/gui.h"
#include "painter/tui.h"
#include "painter/win.h"

using namespace esc;

enum Mode {
	TUI,
	GUI,
	WIN,
};

static const int INTERVAL 	= 50000; /* us */

static volatile bool run = true;
static volatile float speed = 0.04f;
static volatile size_t raster = 1;

static RGB add = {
	0,
	2 * PI / 3,
	4 * PI / 3
};

bool handleKey(int keycode) {
	switch(keycode) {
		case VK_Q:
			run = false;
			return false;
		case VK_UP:
			if(raster > 1)
				raster /= 2;
			break;
		case VK_DOWN:
			if(raster < 16)
				raster *= 2;
			break;
		case VK_LEFT:
			if(speed > 0)
				speed -= 0.01f;
			break;
		case VK_RIGHT:
			speed += 0.01f;
			break;
		case VK_PGUP:
			add.r += 0.05f;
			add.g += 0.1f;
			add.b += 0.2f;
			break;
		case VK_PGDOWN:
			add.r -= 0.05f;
			add.g -= 0.1f;
			add.b -= 0.2f;
			break;
	}
	return true;
}

static void usage(const char *name) {
	serr << "Usage: " << name << " [-m (gui|tui|win)] [-s <cols>x<rows>]\n";
	serr << "    (gui|tui|win) selects the mode\n";
	serr << "    <cols>x<rows> selects the plasma size\n";
	serr << "\n";
	serr << "Key-combinations:\n";
	serr << "    left/right: decrease/increase speed\n";
	serr << "    up/down:    higher/lower resolution\n";
	serr << "    pgup/pgdn:  change colors\n";
	serr << "    q:          quit\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	std::string modestr;
	std::string scrsize;

	esc::cmdargs args(argc,argv,esc::cmdargs::NO_FREE);
	try {
		args.parse("m=s s=s",&modestr,&scrsize);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const esc::cmdargs_error& e) {
		errmsg("Invalid arguments: " << e.what());
		usage(argv[0]);
	}

	Mode mode = TUI;
	gsize_t width = 100;
	gsize_t height = 37;
	if(!modestr.empty()) {
		if(modestr == "gui") {
			width = 1024;
			height = 700;
			mode = GUI;
		}
		else if(modestr == "win") {
			width = 400;
			height = 300;
			mode = WIN;
		}
	}
	if(!scrsize.empty()) {
		if(sscanf(scrsize.c_str(),"%zux%zu",&width,&height) != 2)
			usage(argv[0]);
	}

	Painter *painter;
	switch(mode) {
		case GUI:
			painter = new GUIPainter(width,height);
			break;
		case TUI:
			painter = new TUIPainter(width,height);
			break;
		case WIN:
			painter = new WinPainter(width,height);
			break;
	}

	float time = 0;
	uint64_t tscinterval = timetotsc(INTERVAL);
	while(run) {
		uint64_t now = rdtsc();

		painter->paint(add,raster,time);
		time += speed;

		uint64_t next = now + tscinterval;
		now = rdtsc();
		if(next > now)
			usleep(tsctotime(next - now));
	}

	join(0);
	return 0;
}
