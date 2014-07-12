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

#include <sys/common.h>
#include <sys/sync.h>
#include <assert.h>

int rwcrt(tRWLock *l) {
	int res = usemcrt(&l->mutex,1);
	if(res < 0)
		return res;
	l->sem = semcrt(0);
	if(l->sem < 0) {
		usemdestr(&l->mutex);
		return l->sem;
	}
	l->count = 0;
	l->waits = 0;
	return 0;
}

static void rwwait(tRWLock *l) {
	// store that we're waiting, so that we know that we should up the sem in rwrel().
	l->waits++;
	usemup(&l->mutex);
	IGNSIGS(semdown(l->sem));
	usemdown(&l->mutex);
	l->waits--;
}

void rwreq(tRWLock *l,int op) {
	assert(op == RW_READ || op == RW_WRITE);
	usemdown(&l->mutex);
	if(op == RW_READ) {
		// for fairness: if there is already somebody waiting (always a writer here), always wait
		if(l->waits)
			rwwait(l);
		// wait until there are no writers anymore
		while(l->count < 0)
			rwwait(l);
		// add us to the readers
		l->count++;
	}
	else {
		// same here: if there is somebody waiting (reader or writer), always wait
		if(l->waits)
			rwwait(l);
		// wait until there is no reader and writer anymore
		while(l->count != 0)
			rwwait(l);
		// set it to writing-state
		l->count = -1;
	}
	usemup(&l->mutex);
}

void rwrel(tRWLock *l,int op) {
	assert(op == RW_READ || op == RW_WRITE);
	usemdown(&l->mutex);
	if(op == RW_READ) {
		assert(l->count > 0);
		// if we're the last reader and there is somebody waiting, wake him up
		if(--l->count == 0 && l->waits)
			semup(l->sem);
	}
	else {
		assert(l->count == -1);
		l->count = 0;
		// if there is somebody waiting, wake him up
		if(l->waits)
			semup(l->sem);
	}
	usemup(&l->mutex);
}

void rwdestr(tRWLock *l) {
	semdestr(l->sem);
	usemdestr(&l->mutex);
}
