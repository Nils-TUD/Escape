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
#include <stdio.h>
#include <time.h>

#define MAX_DATE_LEN 100

int main(int argc,char **argv) {
	char *fmt = (char*)"%c";
	char str[MAX_DATE_LEN];
	time_t now;
	struct tm *date;

	if((argc != 1 && argc != 2) || isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s [<format>]\n",argv[0]);
		fprintf(stderr,"	<format> may be anything that dateToString() accepts\n");
		return EXIT_FAILURE;
	}

	/* use format from argument? */
	if(argc == 2)
		fmt = argv[1];

	now = time(NULL);
	date = gmtime(&now);
	strftime(str,sizeof(str),fmt,date);
	puts(str);
	return EXIT_SUCCESS;
}
