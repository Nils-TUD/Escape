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
#include <esc/esccodes.h>
#include <string.h>

#define MAX_ARG_COUNT		3

int escc_get(const char **str,int *n1,int *n2,int *n3) {
	int cmd = ESCC_INVALID;
	const char *s = *str;
	if(*s == '\0')
		return ESCC_INCOMPLETE;
	if(*s == '[') {
		size_t i,j,cmdlen = 0;
		int n[MAX_ARG_COUNT];
		char c,*start = (char*)s + 1;
		/* read code */
		for(cmdlen = 0, s++; (c = *s) && cmdlen < 2 && c != ';' && c != ']'; cmdlen++)
			s++;
		/* read arguments */
		j = cmdlen;
		for(i = 0; i < MAX_ARG_COUNT; i++) {
			n[i] = ESCC_ARG_UNUSED;
			if(c == ';') {
				s++;
				while(++j < MAX_ESCC_LENGTH && (c = *s) && c >= '0' && c <= '9') {
					if(n[i] == ESCC_ARG_UNUSED)
						n[i] = 0;
					n[i] = 10 * n[i] + (c - '0');
					s++;
				}
			}
		}
		/* too long? */
		if(j >= MAX_ESCC_LENGTH)
			return ESCC_INVALID;
		*n1 = n[0];
		*n2 = n[1];
		*n3 = n[2];
		/* if we're at the end, there is something missing */
		if(c == '\0')
			return ESCC_INCOMPLETE;

		*str = s + 1;
		if(cmdlen == 0 || c != ']')
			return ESCC_INVALID;
		if(strncmp(start,"co",cmdlen) == 0)
			cmd = ESCC_COLOR;
		else if(strncmp(start,"ml",cmdlen) == 0)
			cmd = ESCC_MOVE_LEFT;
		else if(strncmp(start,"mr",cmdlen) == 0)
			cmd = ESCC_MOVE_RIGHT;
		else if(strncmp(start,"mu",cmdlen) == 0)
			cmd = ESCC_MOVE_UP;
		else if(strncmp(start,"md",cmdlen) == 0)
			cmd = ESCC_MOVE_DOWN;
		else if(strncmp(start,"mh",cmdlen) == 0)
			cmd = ESCC_MOVE_HOME;
		else if(strncmp(start,"ms",cmdlen) == 0)
			cmd = ESCC_MOVE_LINESTART;
		else if(strncmp(start,"me",cmdlen) == 0)
			cmd = ESCC_MOVE_LINEEND;
		else if(strncmp(start,"kc",cmdlen) == 0)
			cmd = ESCC_KEYCODE;
		else if(strncmp(start,"df",cmdlen) == 0)
			cmd = ESCC_DEL_FRONT;
		else if(strncmp(start,"db",cmdlen) == 0)
			cmd = ESCC_DEL_BACK;
		else if(strncmp(start,"go",cmdlen) == 0)
			cmd = ESCC_GOTO_XY;
		else if(strncmp(start,"si",cmdlen) == 0)
			cmd = ESCC_SIM_INPUT;

		/* set default-values */
		switch(cmd) {
			case ESCC_GOTO_XY:
				if(*n1 == ESCC_ARG_UNUSED)
					*n1 = 0;
				if(*n2 == ESCC_ARG_UNUSED)
					*n2 = 0;
				*n3 = ESCC_ARG_UNUSED;
				break;
			case ESCC_SIM_INPUT:
			case ESCC_DEL_FRONT:
			case ESCC_DEL_BACK:
			case ESCC_MOVE_LEFT:
			case ESCC_MOVE_RIGHT:
			case ESCC_MOVE_UP:
			case ESCC_MOVE_DOWN:
				if(*n1 == ESCC_ARG_UNUSED)
					*n1 = 1;
				*n2 = ESCC_ARG_UNUSED;
				*n3 = ESCC_ARG_UNUSED;
				break;
			case ESCC_MOVE_LINESTART:
			case ESCC_MOVE_LINEEND:
			case ESCC_MOVE_HOME:
				*n1 = ESCC_ARG_UNUSED;
				*n2 = ESCC_ARG_UNUSED;
				*n3 = ESCC_ARG_UNUSED;
				break;
		}
	}

	return cmd;
}
