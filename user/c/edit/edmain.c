/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/cmdargs.h>
#include <esc/proc.h>
#include <esc/fileio.h>
#include <esccodes.h>
#include <stdlib.h>
#include "buffer.h"
#include "display.h"

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [<file>]\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	char c;
	s32 cmd,n1,n2,n3;
	if(argc > 2 || isHelpCmd(argc,argv))
		usage(argv[0]);

	displ_init();
	buf_open(argc > 1 ? argv[1] : NULL);
	displ_update(buf_getLines());
	while((c = fscanc(stdin)) != IO_EOF) {
		if(c == '\033') {
			cmd = freadesc(stdin,&n1,&n2,&n3);
			if(cmd != ESCC_KEYCODE)
				continue;
			break;
		}
	}
	displ_finish();
	return EXIT_SUCCESS;
}
