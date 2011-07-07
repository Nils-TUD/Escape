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
#include <sys/boot.h>
#include <sys/intrpt.h>
#include <sys/task/timer.h>
#include <sys/mem/paging.h>
#include <sys/task/thread.h>
#include <sys/task/lock.h>
#include <sys/dbg/console.h>
#include <sys/syscalls/other.h>
#include <sys/syscalls.h>
#include <sys/log.h>
#include <sys/config.h>
#include <sys/video.h>
#include <errors.h>

int sysc_loadMods(sIntrptStackFrame *stack) {
	int res = boot_loadModules(stack);
	SYSC_RET1(stack,res);
}

int sysc_debugc(sIntrptStackFrame *stack) {
	char c = (char)SYSC_ARG1(stack);
	vid_putchar(c);
	log_printf("%c",c);
	SYSC_RET1(stack,0);
}

int sysc_debug(sIntrptStackFrame *stack) {
	UNUSED(stack);
#if 0
	static size_t foo = 0;
	if(foo == 0) {
		proc_dbg_startProf();
		foo = 1;
	}
	else {
		proc_dbg_stopProf();
		foo = 0;
	}
#else
	cons_start();
#endif
	SYSC_RET1(stack,0);
}

int sysc_getConf(sIntrptStackFrame *stack) {
	uint id = SYSC_ARG1(stack);
	long res = conf_get(id);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
