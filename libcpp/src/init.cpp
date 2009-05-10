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
#include <esc/fileio.h>

#define MAX_GLOBAL_OBJS		32

typedef void (*fConstr)();

extern "C" {
	/**
	 * Will be called by gcc at the beginning for every global object to register the
	 * destructor of the object
	 */
	s32 __cxa_atexit(void (*f)(void *),void *p,void *d);
	/**
	 * We'll call this function before exit() to call all destructors registered by __cxa_atexit()
	 */
	void __cxa_finalize(void *d);

	/**
	 * Start of pointer-array to constructors to call
	 */
	extern fConstr __libcpp_constr_start;
	/**
	 * End of array
	 */
	extern fConstr __libcpp_constr_end;

	/**
	 * We'll call this function before main() to call the constructors for global objects
	 */
	void __libcpp_start();
};

typedef struct {
	void (*f)(void*);
	void *p;
	void *d;
} sGlobalObj;

/* gcc needs the symbol __dso_handle */
void *__dso_handle;

/* global objects */
static sGlobalObj objs[MAX_GLOBAL_OBJS];
static s32 objPos = 0;

s32 __cxa_atexit(void (*f)(void *),void *p,void *d) {
	if(objPos >= MAX_GLOBAL_OBJS)
		return -1;

	objs[objPos].f = f;
	objs[objPos].p = p;
	objs[objPos].d = d;
	objPos++;
	return 0;
}

void __cxa_finalize(void *d) {
	UNUSED(d);
	s32 i;
	for(i = objPos - 1; i >= 0; i--)
		objs[i].f(objs[i].p);
}

void __libcpp_start() {
	fConstr *constr = &__libcpp_constr_start;
	while(constr < &__libcpp_constr_end) {
		(*constr)();
		constr++;
	}
}
