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
#include <esc/driver/vterm.h>
#include <esc/messages.h>
#include <esc/cmdargs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s -l|-s <mode>\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	bool list;
	int mode = -1;
	int res = ca_parse(argc,argv,CA_NO_FREE,"l s=d",&list,&mode);
	if(argc == 0 || res < 0 || ca_hasHelp()) {
		if(argc == 0 || res < 0)
			fprintf(stderr,"Invalid arguments: %s\n",ca_error(res));
		usage(argv[0]);
	}

	if(list) {
		sVTMode *modes;
		ssize_t i,count = vterm_getModeCount(STDIN_FILENO);
		if(count < 0)
			error("Unable to get number of modes");
		mode = vterm_getMode(STDIN_FILENO);
		if(mode < 0)
			error("Unable to get the current mode");
		modes = (sVTMode*)malloc(sizeof(sVTMode) * count);
		if(!modes)
			error("Unable to allocate buffer for modes");
		if(vterm_getModes(STDIN_FILENO,modes,count) < 0)
			error("Unable to get modes");

		printf("Available modes:\n");
		for(i = 0; i < count; i++) {
			printf("%c %5d: %3u x %3u cells, %2ubpp, %s\n",
					mode == modes[i].id ? '*' : ' ',modes[i].id,
					modes[i].width,modes[i].height,modes[i].bitsPerPixel,
					modes[i].device == VID_MODE_TYPE_TEXT ? "text" : "graphical");
		}
		free(modes);
	}
	else {
		res = vterm_setMode(STDIN_FILENO,mode);
		if(res < 0)
			error("Unable to set mode %d",mode);
	}
	return EXIT_SUCCESS;
}
