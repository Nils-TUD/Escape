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
#include <esc/stream/fstream.h>
#include <esc/stream/std.h>
#include <esc/env.h>
#include <sys/common.h>
#include <sys/esccodes.h>
#include <sys/io.h>
#include <sys/keycodes.h>
#include <sys/messages.h>
#include <dirent.h>
#include <getopt.h>
#include <stdlib.h>

using namespace esc;

static void waitForKeyPress(IStream &is);
static void usage(const char *name) {
	serr << "Usage: " << name << " [<file>]\n";
	exit(EXIT_FAILURE);
}

static bool run = true;

int main(int argc,char *argv[]) {
	if(getopt_ishelp(argc,argv))
		usage(argv[0]);

	/* open file, if any */
	FStream *in = &sin;
	if(argc > 1) {
		in = new FStream(argv[1],"r");
		if(!in)
			exitmsg("Unable to open '" << argv[1] << "'");
	}
	else if(isatty(STDIN_FILENO))
		exitmsg("If stdin is a terminal, you have to provide a file");

	/* open the "real" stdin, because stdin maybe redirected to something else */
	std::string vtermPath = env::get("TERM");
	FStream vt(vtermPath.c_str(),"rm");
	if(!vt)
		exitmsg("Unable to open '" << vtermPath << "'");

	VTerm vterm(vt.fd());
	vterm.setFlag(VTerm::FL_READLINE,false);
	Screen::Mode mode = vterm.getMode();

	/* read until vterm is full, ask for continue, and so on */
	size_t line = 0;
	size_t col = 0;
	char c;
	while(run && (c = in->get()) != EOF) {
		sout << c;
		col++;

		if(col == mode.cols - 1 || c == '\n') {
			if(sout.bad())
				break;
			line++;
			col = 0;
		}
		if(line == mode.rows - 1) {
			sout.flush();
			if(sout.bad())
				break;
			waitForKeyPress(vt);
			line = 0;
			col = 0;
		}
	}
	if(in->error())
		exitmsg("Read failed");
	if(sout.error())
		exitmsg("Write failed");

	/* clean up */
	vterm.setFlag(VTerm::FL_READLINE,true);
	if(argc > 1)
		delete in;
	return EXIT_SUCCESS;
}

static void waitForKeyPress(IStream &is) {
	char c;
	while((c = is.get()) != EOF) {
		if(c == '\033') {
			int n1,n2,n3;
			int cmd = is.getesc(n1,n2,n3);
			if(cmd == ESCC_KEYCODE && !(n3 & STATE_BREAK)) {
				if(n2 == VK_SPACE)
					break;
				if(n2 == VK_Q) {
					run = false;
					break;
				}
			}
		}
	}
}
