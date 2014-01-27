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

#include <esc/common.h>
#include <esc/thread.h>
#include <esc/sync.h>
#include <esc/debug.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

#define MAX_EXIT_FUNCS		32

typedef void (*fRegFrameInfo)(void *callback);
typedef void (*fConstr)(void);
typedef struct {
	void (*f)(void*);
	void *p;
	void *d;
} sGlobalObj;

/**
 * Assembler routines
 */
extern int _startthread(fThreadEntry entryPoint,void *arg);
extern __attribute__((weak)) void sigRetFunc(void);

extern void initStdio(void);
extern void initHeap(void);

/**
 * Inits the c-library
 */
void __libc_init(void);
/**
 * Will be called by gcc at the beginning for every global object to register the
 * destructor of the object
 */
int __cxa_atexit(void (*f)(void *),void *p,void *d);
/**
 * We'll call this function in exit() to call all destructors registered by *atexit()
 */
void __cxa_finalize(void *d);

fConstr libcConstr[1] A_INIT = {
	__libc_init
};

/* this lock is shared with tls.c */
tUserSem __libc_sem;
static size_t threadCount = 1;
static size_t exitFuncCount = 0;
static sGlobalObj exitFuncs[MAX_EXIT_FUNCS];
/* crtbegin has a weak reference to that, which is why we have to provide it somewhere */
void *_Jv_RegisterClasses;

int startthread(fThreadEntry entryPoint,void *arg) {
	int res;
	usemdown(&__libc_sem);
	// we have to increase it first since we don't know when the thread starts to run. if he does
	// and exists before we get to the line below, it might call the exit-functions because
	// threadCount hasn't been increased yet.
	threadCount++;
	res = syscall2(SYSCALL_STARTTHREAD,(ulong)entryPoint,(ulong)arg);
	// undo that, if an error occurred
	if(res < 0)
		threadCount--;
	usemup(&__libc_sem);
	return res;
}

int __cxa_atexit(void (*f)(void *),void *p,void *d) {
	usemdown(&__libc_sem);
	if(exitFuncCount >= MAX_EXIT_FUNCS) {
		usemup(&__libc_sem);
		return -ENOMEM;
	}

	exitFuncs[exitFuncCount].f = f;
	exitFuncs[exitFuncCount].p = p;
	exitFuncs[exitFuncCount].d = d;
	exitFuncCount++;
	usemup(&__libc_sem);
	return 0;
}

void __cxa_finalize(A_UNUSED void *d) {
	/* prevent deadlock; this can happen if an exit-function throws an exception or calls abort()
	 * or similar */
	if(threadCount == 0)
		return;

	usemdown(&__libc_sem);
	/* if we're the last thread, call the exit-functions */
	if(--threadCount == 0) {
		ssize_t i;
		for(i = exitFuncCount - 1; i >= 0; i--)
			exitFuncs[i].f(exitFuncs[i].p);
	}
	usemup(&__libc_sem);
}

void __libc_init(void) {
	if(signal(SIG_RET,(fSignal)&sigRetFunc) == SIG_ERR)
		error("Unable to set signal return address");
	initHeap();
	initStdio();
}
