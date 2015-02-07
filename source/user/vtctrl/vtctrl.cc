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

#include <esc/proto/vterm.h>
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <esc/env.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <stdlib.h>
#include <string.h>

using namespace esc;

static void usage(const char *name) {
	serr << "Usage: " << name << " <cmd>\n";
	serr << "    -l       : list all modes\n";
	serr << "    -s <mode>: set <mode>\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	int list = 0;
	int mode = -1;

	cmdargs args(argc,argv,cmdargs::NO_FREE);
	try {
		args.parse("l s=d",&list,&mode);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const esc::cmdargs_error& e) {
		errmsg("Invalid arguments: " << e.what());
		usage(argv[0]);
	}

	esc::VTerm vterm(esc::env::get("TERM").c_str());
	if(list) {
		std::vector<esc::Screen::Mode> modes = vterm.getModes();
		esc::Screen::Mode curMode = vterm.getMode();

		sout << "Available modes:\n";
		for(auto it = modes.begin(); it != modes.end(); ++it) {
			sout << (curMode.id == it->id ? '*' : ' ') << " ";
			sout << fmt(it->id,5) << ": ";
			sout << fmt(it->cols,3) << " x " << fmt(it->rows,3) << " cells, ";
			sout << fmt(it->width,4) << " x " << fmt(it->height,4) << " pixels, ";
			sout << fmt(it->bitsPerPixel,2) << "bpp, ";
			sout << (it->mode == esc::Screen::MODE_TEXT ? "text     " : "graphical") << " ";
			sout << "(" << ((it->type & esc::Screen::MODE_TYPE_TUI) ? "tui" : "-") << ",";
			sout << ((it->type & esc::Screen::MODE_TYPE_GUI) ? "gui" : "-") << ")";
			sout << "\n";
		}
	}
	else
		vterm.setMode(mode);
	return EXIT_SUCCESS;
}
