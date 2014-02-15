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
#include <sys/task/groups.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <string.h>

SpinLock Groups::lock;

bool Groups::set(pid_t pid,size_t count,USER const gid_t *groups) {
	gid_t *grpCpy = NULL;
	if(count > 0) {
		grpCpy = (gid_t*)Cache::alloc(count * sizeof(gid_t));
		if(!grpCpy)
			return false;
		Thread::addHeapAlloc(grpCpy);
		memcpy(grpCpy,groups,count * sizeof(gid_t));
		Thread::remHeapAlloc(grpCpy);
	}

	Entries *g = (Entries*)Cache::alloc(sizeof(Entries));
	if(!g) {
		Cache::free(grpCpy);
		return false;
	}
	g->refCount = 1;
	g->count = count;
	g->groups = grpCpy;
	leave(pid);
	Proc *p = Proc::getRef(pid);
	if(!p) {
		Cache::free(g);
		Cache::free(grpCpy);
		return false;
	}
	p->groups = g;
	Proc::relRef(p);
	return true;
}

void Groups::join(Proc *dst,Proc *src) {
	lock.down();
	Entries *g = src->groups;
	dst->groups = g;
	if(g)
		g->refCount++;
	lock.up();
}

size_t Groups::get(pid_t pid,USER gid_t *list,size_t count) {
	size_t res = 0;
	Proc *p = Proc::getRef(pid);
	if(p) {
		lock.down();
		Entries *g = p->groups;
		/* we can't hold the reference during the access to user-memory. we might die */
		/* since we're holding the spinlock now, nobody can destroy the groups anyway */
		Proc::relRef(p);
		if(count == 0)
			res = g ? g->count : 0;
		else if(g) {
			res = MIN(g->count,count);
			memcpy(list,g->groups,res * sizeof(gid_t));
		}
		lock.up();
	}
	return res;
}

bool Groups::contains(pid_t pid,gid_t gid) {
	bool res = false;
	Proc *p = Proc::getRef(pid);
	if(p) {
		lock.down();
		Entries *g = p->groups;
		if(g) {
			for(size_t i = 0; i < g->count; i++) {
				if(g->groups[i] == gid) {
					res = true;
					break;
				}
			}
		}
		lock.up();
		Proc::relRef(p);
	}
	return res;
}

void Groups::leave(pid_t pid) {
	Proc *p = Proc::getRef(pid);
	if(!p)
		return;
	lock.down();
	Entries *g = p->groups;
	if(g) {
		if(--g->refCount == 0) {
			Cache::free(g->groups);
			Cache::free(g);
		}
	}
	lock.up();
	p->groups = NULL;
	Proc::relRef(p);
}

void Groups::print(OStream &os,pid_t pid) {
	Proc *p = Proc::getRef(pid);
	if(p) {
		lock.down();
		Entries *g = p->groups;
		if(g) {
			os.writef("[refs: %u] ",g->refCount);
			for(size_t i = 0; i < g->count; i++)
				os.writef("%u ",g->groups[i]);
		}
		else
			os.writef("-");
		lock.up();
		Proc::relRef(p);
	}
}
