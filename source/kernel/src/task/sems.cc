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

int Sems::init(Proc *p) {
	p->sems = (Entry**)cache_calloc(INIT_SEMS_COUNT,sizeof(Entry*));
	if(p->sems == NULL)
		return -ENOMEM;
	p->semsSize = INIT_SEMS_COUNT;
	return 0;
}

int Sems::clone(Proc *p,const Proc *old) {
	int res = 0;
	old->lock(PLOCK_SEMS);
	p->sems = (Entry**)cache_calloc(old->semsSize,sizeof(Entry*));
	if(!p->sems)
		goto error;
	p->semsSize = old->semsSize;
	for(size_t i = 0; i < old->semsSize; ++i) {
		Entry *e = old->sems[i];
		if(e) {
			p->sems[i] = new Entry(*e);
			if(!p->sems[i]) {
				while(i-- > 0)
					delete p->sems[i];
				res = -ENOMEM;
				goto errorFree;
			}
		}
		else
			p->sems[i] = NULL;
	}
	old->unlock(PLOCK_SEMS);
	return 0;

errorFree:
	cache_free(p->sems);
error:
	old->unlock(PLOCK_SEMS);
	return res;
}

int Sems::create(Proc *p,uint value) {
	int res;
	Entry **sems;
	Entry *e = new Entry(value);
	if(!e)
		return -ENOMEM;

	p->lock(PLOCK_SEMS);
	/* search for a free entry */
	for(size_t i = 0; i < p->semsSize; ++i) {
		if(p->sems[i] == NULL) {
			p->sems[i] = e;
			p->unlock(PLOCK_SEMS);
			return i;
		}
	}

	/* too many? */
	if(p->semsSize == MAX_SEM_COUNT) {
		res = -EMFILE;
		goto error;
	}

	/* realloc the array */
	sems = (Entry**)cache_realloc(p->sems,p->semsSize * sizeof(Entry*) * 2);
	if(!sems) {
		res = -ENOMEM;
		goto error;
	}
	memclear(sems + p->semsSize,p->semsSize * sizeof(Entry*));

	/* insert entry */
	res = p->semsSize;
	sems[res] = e;
	p->semsSize *= 2;
	p->sems = sems;

	p->unlock(PLOCK_SEMS);
	return res;

error:
	delete e;
	p->unlock(PLOCK_SEMS);
	return res;
}

int Sems::op(Proc *p,int sem,int amount) {
	p->lock(PLOCK_SEMS);
	Entry *e = p->sems[sem];
	if(sem < 0 || sem >= (int)p->semsSize || !e) {
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
	if(sem < 0 || sem >= (int)p->semsSize)
		return;

	p->lock(PLOCK_SEMS);
	Entry *e = p->sems[sem];
	if(e && --e->refs == 0)
		delete e;
	p->sems[sem] = NULL;
	p->unlock(PLOCK_SEMS);
}

void Sems::destroyAll(Proc *p,bool complete) {
	p->lock(PLOCK_SEMS);
	for(size_t i = 0; i < p->semsSize; ++i) {
		delete p->sems[i];
		p->sems[i] = NULL;
	}
	if(complete) {
		cache_free(p->sems);
		p->sems = NULL;
	}
	p->unlock(PLOCK_SEMS);
}

void Sems::print(OStream &os,const Proc *p) {
	os.writef("Semaphores (current max=%zu):\n",p->semsSize);
	for(size_t i = 0; i < p->semsSize; i++) {
		if(p->sems[i] != NULL)
			os.writef("\t%-2d: %u (%d refs)\n",i,p->sems[i]->s.getValue(),p->sems[i]->refs);
	}
}
