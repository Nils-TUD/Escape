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
#include <esc/date.h>
#include <esc/io/console.h>
#include <esc/exceptions/date.h>
#include <stdio.h>

#define MAX_DATE_LEN 100

int main(int argc,char **argv) {
	char *fmt = (char*)"%c";
	char str[MAX_DATE_LEN];

	if((argc != 1 && argc != 2) || isHelpCmd(argc,argv)) {
		cerr->writef(cerr,"Usage: %s [<format>]\n",argv[0]);
		cerr->writef(cerr,"	<format> may be anything that dateToString() accepts\n");
		return EXIT_FAILURE;
	}

	/* use format from argument? */
	if(argc == 2)
		fmt = argv[1];

	TRY {
		sDate d = date_get();
		d.format(&d,str,MAX_DATE_LEN,fmt);
	}
	CATCH(DateException,e) {
		error("Unable to retrieve date");
	}
	ENDCATCH
	cout->writeln(cout,str);
	return EXIT_SUCCESS;
}
