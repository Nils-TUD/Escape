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

#include <sys/common.h>
#include <sys/dbg/console.h>
#include <sys/dbg/cmd/locks.h>
#include <sys/util.h>
#include <string.h>

int cons_cmd_locks(OStream &os,size_t argc,char **argv) {
	if(Console::isHelp(argc,argv) || argc != 2) {
		os.writef("Usage: %s on|off\n",argv[0]);
		return 0;
	}

	Util::setPanicStarted(strcmp(argv[1],"off") == 0);
	return 0;
}
