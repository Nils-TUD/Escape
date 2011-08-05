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
#include <sys/task/groups.h>
#include <sys/task/thread.h>
#include <sys/mem/cache.h>
#include <sys/klock.h>
#include <sys/video.h>
#include <string.h>

static sProcGroups *groups_getByPid(pid_t pid);

bool groups_set(pid_t pid,size_t count,USER const gid_t *groups) {
	sProcGroups *g;
	sProc *p;
	gid_t *grpCpy = NULL;
	if(count > 0) {
		grpCpy = (gid_t*)cache_alloc(count * sizeof(gid_t));
		if(!grpCpy)
			return false;
		thread_addHeapAlloc(grpCpy);
		memcpy(grpCpy,groups,count * sizeof(gid_t));
		thread_remHeapAlloc(grpCpy);
	}

	g = (sProcGroups*)cache_alloc(sizeof(sProcGroups));
	if(!g) {
		cache_free(grpCpy);
		return false;
	}
	g->lock = 0;
	g->refCount = 1;
	g->count = count;
	g->groups = grpCpy;
	groups_leave(pid);
	p = proc_getByPid(pid);
	if(!p) {
		cache_free(g);
		cache_free(grpCpy);
		return false;
	}
	p->groups = g;
	return true;
}

void groups_join(sProc *dst,sProc *src) {
	sProcGroups *g = src->groups;
	dst->groups = g;
	if(g) {
		klock_aquire(&g->lock);
		g->refCount++;
		klock_release(&g->lock);
	}
}

size_t groups_get(pid_t pid,USER gid_t *list,size_t size) {
	sProcGroups *g = groups_getByPid(pid);
	if(size == 0)
		return g ? g->count : 0;
	if(g) {
		size = MIN(g->count,size);
		memcpy(list,g->groups,size);
		return size;
	}
	return 0;
}

bool groups_contains(pid_t pid,gid_t gid) {
	sProcGroups *g = groups_getByPid(pid);
	if(g) {
		size_t i;
		for(i = 0; i < g->count; i++) {
			if(g->groups[i] == gid)
				return true;
		}
	}
	return false;
}

void groups_leave(pid_t pid) {
	sProcGroups *g;
	sProc *p = proc_getByPid(pid);
	if(!p)
		return;
	g = p->groups;
	if(g) {
		klock_aquire(&g->lock);
		if(--g->refCount == 0) {
			cache_free(g->groups);
			cache_free(g);
		}
		klock_release(&g->lock);
	}
	p->groups = NULL;
}

void groups_print(pid_t pid) {
	sProcGroups *g = groups_getByPid(pid);
	if(g) {
		size_t i;
		vid_printf("[refs: %u] ",g->refCount);
		for(i = 0; i < g->count; i++)
			vid_printf("%u ",g->groups[i]);
	}
	else
		vid_printf("-");
}

static sProcGroups *groups_getByPid(pid_t pid) {
	sProc *p = proc_getByPid(pid);
	if(!p)
		return NULL;
	return p->groups;
}
