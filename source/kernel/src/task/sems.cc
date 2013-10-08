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
#include <sys/task/sems.h>
#include <sys/task/proc.h>
#include <errno.h>

void Sems::init(Proc *p) {
	memclear(p->sems,sizeof(p->sems));
}

int Sems::clone(Proc *p,const Proc *old) {
	int res = 0;
	old->lock(PLOCK_SEMS);
	for(size_t i = 0; i < MAX_SEM_COUNT; ++i) {
		Entry *e = old->sems[i];
		if(e) {
			p->sems[i] = new Entry(*e);
			if(!p->sems[i]) {
				while(i-- > 0)
					delete p->sems[i];
				res = -ENOMEM;
				goto error;
			}
		}
		else
			p->sems[i] = NULL;
	}
error:
	old->unlock(PLOCK_SEMS);
	return res;
}

int Sems::create(Proc *p,uint value) {
	p->lock(PLOCK_SEMS);
	for(size_t i = 0; i < MAX_SEM_COUNT; ++i) {
		if(p->sems[i] == NULL) {
			p->sems[i] = new Entry(value);
			int res = p->sems[i] ? i : -ENOMEM;
			p->unlock(PLOCK_SEMS);
			return res;
		}
	}
	p->unlock(PLOCK_SEMS);
	return -EMFILE;
}

int Sems::op(Proc *p,int sem,int amount) {
	p->lock(PLOCK_SEMS);
	Entry *e = p->sems[sem];
	if(sem < 0 || sem >= MAX_SEM_COUNT || !e) {
		p->unlock(PLOCK_SEMS);
		return -EINVAL;
	}
	e->refs++;
	p->unlock(PLOCK_SEMS);

	if(amount < 0)
		e->s.down();
	else
		e->s.up();

	p->lock(PLOCK_SEMS);
	if(--e->refs == 0)
		delete e;
	p->unlock(PLOCK_SEMS);
	return 0;
}

void Sems::destroy(Proc *p,int sem) {
	if(sem < 0 || sem >= MAX_SEM_COUNT)
		return;

	p->lock(PLOCK_SEMS);
	Entry *e = p->sems[sem];
	if(e && --e->refs == 0)
		delete e;
	p->sems[sem] = NULL;
	p->unlock(PLOCK_SEMS);
}

void Sems::destroyAll(Proc *p) {
	p->lock(PLOCK_SEMS);
	for(size_t i = 0; i < MAX_SEM_COUNT; ++i) {
		delete p->sems[i];
		p->sems[i] = NULL;
	}
	p->unlock(PLOCK_SEMS);
}
