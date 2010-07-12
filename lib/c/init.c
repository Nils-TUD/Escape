/**
 * $Id: init.c 634 2010-05-01 12:20:20Z nasmussen $
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
#include <esc/lock.h>
#include <esc/thread.h>
#include <esc/debug.h>
#include <signal.h>
#include <errors.h>

#define MAX_EXIT_FUNCS		32

typedef void (*fConstr)(void);
typedef struct {
	void (*f)(void*);
	void *p;
	void *d;
} sGlobalObj;

void __libc_init(void);
/*extern fConstr __libcpp_constr_start;
extern fConstr __libcpp_constr_end;*/

static tULock exitLock = 0;
static s16 exitFuncCount = 0;
static sGlobalObj exitFuncs[MAX_EXIT_FUNCS];

/**
 * Will be called by gcc at the beginning for every global object to register the
 * destructor of the object
 */
int __cxa_atexit(void (*f)(void *),void *p,void *d);
/**
 * We'll call this function in exit() to call all destructors registered by *atexit()
 */
void __cxa_finalize(void *d);

int __cxa_atexit(void (*f)(void *),void *p,void *d) {
	locku(&exitLock);
	if(exitFuncCount >= MAX_EXIT_FUNCS) {
		unlocku(&exitLock);
		return ERR_MAX_EXIT_FUNCS;
	}

	exitFuncs[exitFuncCount].f = f;
	exitFuncs[exitFuncCount].p = p;
	exitFuncs[exitFuncCount].d = d;
	exitFuncCount++;
	unlocku(&exitLock);
	return 0;
}

void __cxa_finalize(void *d) {
	UNUSED(d);
	/* if we're the last thread, call the exit-functions */
	if(getThreadCount() == 1) {
		s16 i;
		for(i = exitFuncCount - 1; i >= 0; i--)
			exitFuncs[i].f(exitFuncs[i].p);
	}
}

extern void sigRetFunc(void);
extern void streamConstr(void);

void __libc_init(void) {
	/* TODO */
	if(setSigHandler(SIG_RET,(fSigHandler)&sigRetFunc) < 0)
		error("Unable to tell kernel sigRet-address");
	streamConstr();
	/*fConstr *constr = &__libcpp_constr_start;
	while(constr < &__libcpp_constr_end) {
		(*constr)();
		constr++;
	}*/
}
