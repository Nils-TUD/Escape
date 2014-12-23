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

#include <common.h>
#include <boot.h>
#include <interrupts.h>
#include <task/timer.h>
#include <mem/pagedir.h>
#include <mem/cache.h>
#include <task/thread.h>
#include <dbg/console.h>
#include <syscalls.h>
#include <log.h>
#include <config.h>
#include <video.h>
#include <errno.h>

int Syscalls::init(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int res = Boot::init(stack);
	SYSC_RET1(stack,res);
}

int Syscalls::debugc(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	char c = (char)SYSC_ARG1(stack);
	Log::get().writec(c);
	SYSC_RET1(stack,0);
}

int Syscalls::sysconf(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int id = SYSC_ARG1(stack);
	long res = Config::get(id);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::sysconfstr(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int id = SYSC_ARG1(stack);
	char *buf = (char*)SYSC_ARG2(stack);
	size_t len = SYSC_ARG3(stack);

	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)buf,len)))
		SYSC_ERROR(stack,-EINVAL);

	const char *res = Config::getStr(id);
	if(!res)
		SYSC_ERROR(stack,-EINVAL);
	strnzcpy(buf,res,len);
	SYSC_RET1(stack,0);
}

int Syscalls::gettimeofday(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	struct timeval *tv = (struct timeval*)SYSC_ARG1(stack);
	struct timeval ktv;
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)tv,sizeof(*tv))))
		SYSC_ERROR(stack,-EINVAL);

	Timer::getTimeval(&ktv);
	UserAccess::write(tv,&ktv,sizeof(ktv));
	SYSC_RET1(stack,0);
}

int Syscalls::tsctotime(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	uint64_t *tsc = (uint64_t*)SYSC_ARG1(stack);
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)tsc,sizeof(uint64_t))))
		SYSC_ERROR(stack,-EINVAL);
	*tsc = Timer::cyclesToTime(*tsc);
	SYSC_RET1(stack,0);
}
