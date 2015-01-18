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
#include <esc/env.h>
#include <sys/cmdargs.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <cmd>\n",name);
	fprintf(stderr,"    -l       : list all modes\n");
	fprintf(stderr,"    -s <mode>: set <mode>\n");
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

	esc::VTerm vterm(esc::env::get("TERM").c_str());
	if(list) {
		std::vector<esc::Screen::Mode> modes = vterm.getModes();
		esc::Screen::Mode curMode = vterm.getMode();

		printf("Available modes:\n");
		for(auto it = modes.begin(); it != modes.end(); ++it) {
			printf("%c %5d: %3u x %3u cells, %4u x %4u pixels, %2ubpp, %s (%s,%s)\n",
					curMode.id == it->id ? '*' : ' ',it->id,
					it->cols,it->rows,it->width,it->height,it->bitsPerPixel,
					it->mode == esc::Screen::MODE_TEXT ? "text     " : "graphical",
					(it->type & esc::Screen::MODE_TYPE_TUI) ? "tui" : "-",
					(it->type & esc::Screen::MODE_TYPE_GUI) ? "gui" : "-");
		}
	}
	else
		vterm.setMode(mode);
	return EXIT_SUCCESS;
}
