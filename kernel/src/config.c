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
#include <sys/task/timer.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/config.h>
#include <errors.h>
#include <string.h>
#include <ctype.h>

#define MAX_BPNAME_LEN		16
#define MAX_BPVAL_LEN		32

static void conf_set(const char *name,const char *value);

static uchar bootVidMode;
static uchar logToCom1 = true;
static char swapDev[MAX_BPVAL_LEN + 1] = "";

void conf_parseBootParams(int argc,const char *const *argv) {
	char name[MAX_BPNAME_LEN + 1];
	char value[MAX_BPVAL_LEN + 1];
	int i;
	for(i = 1; i < argc; i++) {
		size_t len = strlen(argv[i]);
		if(len > MAX_BPNAME_LEN + MAX_BPVAL_LEN + 1)
			continue;
		int eq = strchri(argv[i],'=');
		if(eq > MAX_BPNAME_LEN || (len - eq) > MAX_BPVAL_LEN)
			continue;
		strncpy(name,argv[i],eq);
		name[eq] = '\0';
		if(eq != (int)len) {
			strncpy(value,argv[i] + eq + 1,len - eq - 1);
			value[len - eq - 1] = '\0';
		}
		else
			value[0] = '\0';
		conf_set(name,value);
	}
}

const char *conf_getStr(uint id) {
	const char *res = NULL;
	switch(id) {
		case CONF_SWAP_DEVICE:
			res = strlen(swapDev) ? swapDev : NULL;
			break;
	}
	return res;
}

int conf_get(uint id) {
	int res;
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
		case CONF_LOG_TO_COM1:
			res = logToCom1;
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
	else if(strcmp(name,"nolog") == 0)
		logToCom1 = false;
}
