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

#include <mem/cache.h>
#include <mem/useraccess.h>
#include <mem/virtmem.h>
#include <task/groups.h>
#include <task/proc.h>
#include <task/thread.h>
#include <common.h>
#include <spinlock.h>
#include <string.h>
#include <video.h>

SpinLock Groups::lock;

bool Groups::set(pid_t pid,size_t count,USER const gid_t *groups) {
	gid_t *grpCpy = NULL;
	if(count > 0) {
		grpCpy = (gid_t*)Cache::alloc(count * sizeof(gid_t));
		if(!grpCpy)
			return false;
		if(UserAccess::read(grpCpy,groups,count * sizeof(gid_t)) < 0)
			return false;
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
	LockGuard<SpinLock> guard(&lock);
	Entries *g = src->groups;
	dst->groups = g;
	if(g)
		g->refCount++;
}

size_t Groups::get(pid_t pid,USER gid_t *list,size_t count) {
	size_t res = 0;
	Proc *p = Proc::getRef(pid);
	if(p) {
		LockGuard<SpinLock> guard(&lock);
		Entries *g = p->groups;
		if(count == 0)
			res = g ? g->count : 0;
		else if(g) {
			res = esc::Util::min(g->count,count);
			UserAccess::write(list,g->groups,res * sizeof(gid_t));
		}
		Proc::relRef(p);
	}
	return res;
}

int Groups::contains(pid_t pid,gid_t gid) {
	int res = 0;
	Proc *p = Proc::getRef(pid);
	if(p) {
		LockGuard<SpinLock> guard(&lock);
		Entries *g = p->groups;
		if(g) {
			for(size_t i = 0; i < g->count; i++) {
				if(g->groups[i] == gid) {
					res = 1;
					break;
				}
			}
		}
		Proc::relRef(p);
	}
	return res;
}

void Groups::leave(pid_t pid) {
	Proc *p = Proc::getRef(pid);
	if(!p)
		return;
	LockGuard<SpinLock> guard(&lock);
	Entries *g = p->groups;
	if(g) {
		if(--g->refCount == 0) {
			Cache::free(g->groups);
			Cache::free(g);
		}
	}
	p->groups = NULL;
	Proc::relRef(p);
}

void Groups::print(OStream &os,pid_t pid) {
	Proc *p = Proc::getRef(pid);
	if(p) {
		LockGuard<SpinLock> guard(&lock);
		Entries *g = p->groups;
		if(g) {
			os.writef("[refs: %u] ",g->refCount);
			for(size_t i = 0; i < g->count; i++)
				os.writef("%u ",g->groups[i]);
		}
		else
			os.writef("-");
		Proc::relRef(p);
	}
}
