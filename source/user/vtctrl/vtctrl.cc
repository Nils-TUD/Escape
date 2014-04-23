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
#include <esc/messages.h>
#include <esc/cmdargs.h>
#include <ipc/proto/vterm.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <env.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s -l|-s <mode>\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	int list = 0;
	int mode = -1;
	int res = ca_parse(argc,argv,CA_NO_FREE,"l s=d",&list,&mode);
	if(argc == 1 || res < 0 || ca_hasHelp()) {
		if(argc == 0 || res < 0)
			printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}

	ipc::VTerm vterm(std::env::get("TERM").c_str());
	if(list) {
		std::vector<ipc::Screen::Mode> modes = vterm.getModes();
		ipc::Screen::Mode curMode = vterm.getMode();

		printf("Available modes:\n");
		for(auto it = modes.begin(); it != modes.end(); ++it) {
			printf("%c %5d: %3u x %3u cells, %4u x %4u pixels, %2ubpp, %s (%s,%s)\n",
					curMode.id == it->id ? '*' : ' ',it->id,
					it->cols,it->rows,it->width,it->height,it->bitsPerPixel,
					it->mode == ipc::Screen::MODE_TEXT ? "text     " : "graphical",
					(it->type & ipc::Screen::MODE_TYPE_TUI) ? "tui" : "-",
					(it->type & ipc::Screen::MODE_TYPE_GUI) ? "gui" : "-");
		}
	}
	else
		vterm.setMode(mode);
	return EXIT_SUCCESS;
}
