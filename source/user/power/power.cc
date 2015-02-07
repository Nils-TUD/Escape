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

#include <esc/proto/init.h>
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <stdlib.h>

static void usage(const char *name) {
	esc::serr << "Usage: " << name << " -r|-s\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	int reboot = 0;
	int shutdown = 0;

	esc::cmdargs args(argc,argv,esc::cmdargs::NO_FREE);
	try {
		args.parse("r s",&reboot,&shutdown);
		if(args.is_help() || (!reboot && !shutdown) || (reboot && shutdown))
			usage(argv[0]);
	}
	catch(const esc::cmdargs_error& e) {
		errmsg("Invalid arguments: " << e.what());
		usage(argv[0]);
	}

	esc::Init init("/dev/init");
	if(reboot)
		init.reboot();
	else if(shutdown)
		init.shutdown();
	return EXIT_SUCCESS;
}
