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

#include <task/proc.h>
#include <task/smp.h>
#include <task/thread.h>
#include <task/timer.h>
#include <common.h>
#include <config.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

uint Config::flags = (1 << Config::SMP) | (1 << Config::LOG);
char Config::rootDev[MAX_BPVAL_LEN + 1] = "";
char Config::swapDev[MAX_BPVAL_LEN + 1] = "";

void Config::parseBootParams(int argc,const char *const *argv) {
	char name[MAX_BPNAME_LEN + 1];
	char value[MAX_BPVAL_LEN + 1];
	for(int i = 1; i < argc; i++) {
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

	// TODO no SMP on x86_64 yet
#if defined(__x86_64__)
	flags &= ~(1 << SMP);
#endif
}

const char *Config::getStr(int id) {
	const char *res = NULL;
	switch(id) {
		case ROOT_DEVICE:
			res = rootDev;
			break;
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
		case CPU_COUNT:
			res = SMP::getCPUCount();
			break;
		case TICKS_PER_SEC:
			res = CPU::getSpeed();
			break;
		case LOG:
		case LOG_TO_VGA:
		case LINE_BY_LINE:
		case SMP:
		case FORCE_PIT:
		case FORCE_PIC:
			res = !!(flags & (1 << id));
			break;
		default:
			res = -EINVAL;
			break;
	}
	return res;
}

void Config::set(const char *name,const char *value) {
	if(strcmp(name,"root") == 0)
		strcpy(rootDev,value);
	else if(strcmp(name,"swapdev") == 0)
		strcpy(swapDev,value);
	else if(strcmp(name,"nolog") == 0)
		flags &= ~(1 << LOG);
	else if(strcmp(name,"linebyline") == 0)
		flags |= 1 << LINE_BY_LINE;
	else if(strcmp(name,"logtovga") == 0)
		flags |= 1 << LOG_TO_VGA;
	else if(strcmp(name,"nosmp") == 0)
		flags &= ~(1 << SMP);
	else if(strcmp(name,"forcepit") == 0)
		flags |= 1 << FORCE_PIT;
	else if(strcmp(name,"forcepic") == 0)
		flags |= 1 << FORCE_PIC;
}
