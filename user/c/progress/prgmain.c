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
#include <esc/io/console.h>
#include <esc/proc.h>
#include <stdio.h>
#include <esc/messages.h>

int main(void) {
	sVTSize consSize;
	u32 maxWidth;
	u32 p,i,j;
	if(recvMsgData(STDIN_FILENO,MSG_VT_GETSIZE,&consSize,sizeof(sVTSize)) < 0)
		error("Unable to get vterm-size");
	maxWidth = consSize.width - 3;

	cout->writes(cout,"Waiting for fun...\n");
	for(p = 0; p <= 100; p++) {
		/* percent to console width */
		j = p == 0 ? 0 : maxWidth / (100. / p);

		cout->writes(cout,"\r[");
		/* completed */
		for(i = 0; i < j; i++)
			cout->writec(cout,'=');
		cout->writec(cout,'>');
		/* uncompleted */
		for(i = j + 1; i <= maxWidth; i++)
			cout->writec(cout,' ');
		cout->writec(cout,']');
		cout->flush(cout);

		/* wait a little bit */
		sleep(100);
	}
	cout->writes(cout,"Ready!\n");
	return EXIT_SUCCESS;
}
