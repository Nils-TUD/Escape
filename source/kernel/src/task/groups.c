/**
 * $Id$
 */

#include <sys/common.h>
#include <sys/task/groups.h>
#include <sys/mem/cache.h>
#include <sys/video.h>
#include <string.h>

sProcGroups *groups_alloc(size_t count,const gid_t *groups) {
	sProcGroups *g = (sProcGroups*)cache_alloc(sizeof(sProcGroups));
	if(!g)
		return NULL;
	g->refCount = 1;
	g->count = count;
	g->groups = NULL;
	if(count > 0) {
		g->groups = (gid_t*)cache_alloc(count * sizeof(gid_t));
		if(!g->groups) {
			cache_free(g);
			return NULL;
		}
		memcpy(g->groups,groups,count * sizeof(gid_t));
	}
	return g;
}

sProcGroups *groups_join(sProcGroups *g) {
	if(g)
		g->refCount++;
	return g;
}

bool groups_contains(sProcGroups *g,gid_t gid) {
	if(g) {
		size_t i;
		for(i = 0; i < g->count; i++) {
			if(g->groups[i] == gid)
				return true;
		}
	}
	return false;
}

void groups_leave(sProcGroups *g) {
	if(g) {
		if(--g->refCount == 0) {
			cache_free(g->groups);
			cache_free(g);
		}
	}
}

void groups_print(sProcGroups *g) {
	if(g) {
		size_t i;
		vid_printf("[refs: %u] ",g->refCount);
		for(i = 0; i < g->count; i++)
			vid_printf("%u ",g->groups[i]);
	}
	else
		vid_printf("-");
}
