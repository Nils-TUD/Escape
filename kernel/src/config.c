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

#include <sys/common.h>
#include <sys/machine/timer.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/config.h>
#include <errors.h>
#include <string.h>
#include <ctype.h>

#define MAX_BPNAME_LEN		16
#define MAX_BPVAL_LEN		32

#define STATE_NAME			0
#define STATE_VAL			1
#define STATE_IGNORE		2

static void conf_set(const char *name,const char *value);

static u8 bootVidMode;
static char swapDev[MAX_BPVAL_LEN + 1] = "";

void conf_parseBootParams(const char *params) {
	char name[MAX_BPNAME_LEN];
	char value[MAX_BPVAL_LEN];
	u8 nameLen = 0,valLen = 0;
	u8 state = STATE_NAME;
	const char *p = params;
	/* skip path to kernel */
	while(!isspace(*p++));
	while(*p) {
		char c = *p++;
		switch(c) {
			case ' ':
			case '\t':
			case '\n':
				if(state == STATE_IGNORE || state == STATE_VAL) {
					value[valLen] = '\0';
					conf_set(name,value);
					state = STATE_NAME;
					nameLen = 0;
					valLen = 0;
				}
				break;

			case '=':
				if(state == STATE_NAME) {
					name[nameLen] = '\0';
					state = STATE_VAL;
				}
				else if(state == STATE_VAL) {
					value[valLen] = '\0';
					state = STATE_IGNORE;
				}
				break;

			default:
				if(state == STATE_NAME) {
					if(nameLen >= MAX_BPNAME_LEN - 1) {
						name[nameLen] = '\0';
						state = STATE_VAL;
					}
					else
						name[nameLen++] = c;
				}
				else if(state == STATE_VAL) {
					if(valLen >= MAX_BPVAL_LEN - 1) {
						value[valLen] = '\0';
						state = STATE_IGNORE;
					}
					else
						value[valLen++] = c;
				}
				break;
		}
	}

	if(state == STATE_IGNORE || state == STATE_VAL) {
		value[valLen] = '\0';
		conf_set(name,value);
	}
}

const char *conf_getStr(u32 id) {
	const char *res = NULL;
	switch(id) {
		case CONF_SWAP_DEVICE:
			res = strlen(swapDev) ? swapDev : NULL;
			break;
	}
	return res;
}

s32 conf_get(u32 id) {
	s32 res;
	switch(id) {
		case CONF_BOOT_VIDEOMODE:
			res = bootVidMode;
			break;
		case CONF_TIMER_FREQ:
			res = TIMER_FREQUENCY;
			break;
		case CONF_MAX_PROCS:
			res = MAX_PROC_COUNT;
			break;
		case CONF_MAX_FDS:
			res = MAX_FD_COUNT;
			break;
		default:
			res = ERR_INVALID_ARGS;
			break;
	}
	return res;
}

static void conf_set(const char *name,const char *value) {
	if(strcmp(name,"videomode") == 0) {
		if(strcmp(value,"vesa") == 0)
			bootVidMode = CONF_VIDMODE_VESATEXT;
		else
			bootVidMode = CONF_VIDMODE_VGATEXT;
	}
	else if(strcmp(name,"swapdev") == 0)
		strcpy(swapDev,value);
}
