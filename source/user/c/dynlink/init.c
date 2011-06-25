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
#include <esc/elf.h>
#include <esc/sllist.h>
#include "init.h"
#include "setup.h"

static void load_initLib(sSharedLib *l);

void load_init(void) {
	sSLNode *n;
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		load_initLib(l);
	}
}

static void load_initLib(sSharedLib *l) {
	sSLNode *n;

	/* already initialized? */
	if(l->initialized)
		return;

	/* first go through the dependencies */
	for(n = sll_begin(l->deps); n != NULL; n = n->next) {
		sSharedLib *dl = (sSharedLib*)n->data;
		load_initLib(dl);
	}

	/* if its not the executable, call the init-function */
	if(l->isDSO) {
		uintptr_t initAddr = (uintptr_t)load_getDyn(l->dyn,DT_INIT);
		if(initAddr) {
			void (*initFunc)(void) = (void (*)(void))(initAddr + l->loadAddr);
			initFunc();
		}
	}

	l->initialized = true;
}
