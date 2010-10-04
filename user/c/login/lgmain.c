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
#include <esc/messages.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#define USERNAME	"hrniels"
#define PASSWORD	"test"
#define MAX_LEN		10

static void termHandler(int sig);

int main(void) {
	char un[MAX_LEN];
	char pw[MAX_LEN];

	if(setSigHandler(SIG_TERM,termHandler) < 0)
		error("Unable to announce term-signal-handler");

	while(1) {
		printf("Username: ");
		fgetl(un,sizeof(un),stdin);
		sendRecvMsgData(STDOUT_FILENO,MSG_VT_DIS_ECHO,NULL,0);
		printf("Password: ");
		fgetl(pw,sizeof(pw),stdin);
		sendRecvMsgData(STDOUT_FILENO,MSG_VT_EN_ECHO,NULL,0);
		putchar('\n');

		if(strcmp(un,USERNAME) != 0)
			printf("\033[co;4]Sorry, invalid username. Try again!\033[co]\n");
		else if(strcmp(pw,PASSWORD) != 0)
			printf("\033[co;4]Sorry, invalid password. Try again!\033[co]\n");
		else {
			printf("\033[co;2]Login successfull.\033[co]\n");
			break;
		}
	}
	return EXIT_SUCCESS;
}

static void termHandler(int sig) {
	UNUSED(sig);
	printf("Got TERM-signal but I don't want to die :P\n");
}
