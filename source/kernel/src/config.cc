/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/task/smp.h>
#include <sys/config.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

bool Config::lineByLine = false;
bool Config::doLog = true;
bool Config::log2scr = false;
bool Config::smp = true;
char Config::swapDev[MAX_BPVAL_LEN + 1] = "";

void Config::parseBootParams(int argc,const char *const *argv) {
	char name[MAX_BPNAME_LEN + 1];
	char value[MAX_BPVAL_LEN + 1];
	int i;
	for(i = 1; i < argc; i++) {
		size_t len = strlen(argv[i]);
		if(len > MAX_BPNAME_LEN + MAX_BPVAL_LEN + 1)
			continue;
		size_t eq = strchri(argv[i],'=');
		if(eq > MAX_BPNAME_LEN || (len - eq) > MAX_BPVAL_LEN)
			continue;
		strncpy(name,argv[i],eq);
		name[eq] = '\0';
		if(eq != len)
			strnzcpy(value,argv[i] + eq + 1,sizeof(value));
		else
			value[0] = '\0';
		set(name,value);
	}
}

const char *Config::getStr(int id) {
	const char *res = NULL;
	switch(id) {
		case SWAP_DEVICE:
			res = *swapDev ? swapDev : NULL;
			break;
	}
	return res;
}

long Config::get(int id) {
	long res;
	switch(id) {
		case TIMER_FREQ:
			res = Timer::FREQUENCY_DIV;
			break;
		case MAX_PROCS:
			res = MAX_PROC_COUNT;
			break;
		case MAX_FDS:
			res = MAX_FD_COUNT;
			break;
		case LOG:
			res = doLog;
			break;
		case PAGESIZE:
			res = PAGE_SIZE;
			break;
		case LINEBYLINE:
			res = lineByLine;
			break;
		case LOG2SCR:
			res = log2scr;
			break;
		case CPU_COUNT:
			res = SMP::getCPUCount();
			break;
		case SMP:
			res = smp;
			break;
		default:
			res = -EINVAL;
			break;
	}
	return res;
}

void Config::set(const char *name,const char *value) {
	if(strcmp(name,"swapdev") == 0)
		strcpy(swapDev,value);
	else if(strcmp(name,"nolog") == 0)
		doLog = false;
	else if(strcmp(name,"linebyline") == 0)
		lineByLine = true;
	else if(strcmp(name,"log2scr") == 0)
		log2scr = true;
	else if(strcmp(name,"nosmp") == 0)
		smp = false;
}
