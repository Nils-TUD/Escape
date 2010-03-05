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
#include <esc/fileio.h>
#include <string.h>
#include "printf.h"

int mod_printf(int argc,char *argv[]) {
	int c1,c2,c3;
	char str[] = "- This, a sample string.";
	char *pch;
	UNUSED(argc);
	UNUSED(argv);

	printf("Splitting string \"%s\" into tokens:\n",str);
	pch = strtok(str," ,.-");
	while(pch != NULL) {
		printf("'%s'\n",pch);
		pch = strtok(NULL," ,.-");
	}

	printf("str=%p,%n pch=%p,%n abc=%p,%n\n",str,&c1,pch,&c2,0x1234,&c3);
	printf("c1=%d, c2=%d, c3=%d\n",c1,c2,c3);

	printf("num1: '%-8d', num2: '%8d', num3='%-16x', num4='%-012X'\n",
			100,200,0x1bca,0x12345678);

	printf("num1: '%-+4d', num2: '%-+4d'\n",-100,50);
	printf("num1: '%- 4d', num2: '%- 4d'\n",-100,50);
	printf("num1: '%#4x', num2: '%#4X', num3 = '%#4o'\n",0x123,0x456,0377);
	printf("Var padding: %*d\n",8,-123);
	printf("short: %4hx\n",0x1234);
	return EXIT_SUCCESS;
}
