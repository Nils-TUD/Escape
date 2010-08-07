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

#include <sys/common.h>
#include <sys/mem/sharedmem.h>
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/mem/vmm.h>
#include <sys/task/proc.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <string.h>
#include <errors.h>

#define MAX_SHAREDMEM_NAME	15

typedef struct {
	sSLList *users;
	char name[MAX_SHAREDMEM_NAME + 1];
} sShMem;

typedef struct {
	sProc *proc;
	tVMRegNo region;
} sShMemUser;

/**
 * Tests wether its the own (creator) shm
 */
static bool shm_isOwn(sShMem *mem,sProc *p);
/**
 * Creates and adds a new user
 */
static bool shm_addUser(sShMem *mem,sProc *p,tVMRegNo reg);
/**
 * Retrieve the shared-memory-area by name
 */
static sShMem *shm_get(const char *name);
/**
 * Retrieve the usage of the shared-memory-area of the given process
 */
static sShMemUser *shm_getUser(sShMem *mem,sProc *p);

/* list with all shared memories */
static sSLList *shareList = NULL;

void shm_init(void) {
	shareList = sll_create();
	assert(shareList != NULL);
}

s32 shm_create(sProc *p,const char *name,u32 pageCount) {
	sShMem *mem;
	u32 start;
	tVMRegNo reg;

	/* checks */
	if(strlen(name) > MAX_SHAREDMEM_NAME)
		return ERR_SHARED_MEM_NAME;
	if(shm_get(name) != NULL)
		return ERR_SHARED_MEM_EXISTS;

	/* create entry */
	mem = (sShMem*)kheap_alloc(sizeof(sShMem));
	if(mem == NULL)
		return ERR_NOT_ENOUGH_MEM;
	strcpy(mem->name,name);
	mem->users = sll_create();
	if(mem == NULL)
		goto errMem;
	reg = vmm_add(p,NULL,0,pageCount * PAGE_SIZE,pageCount * PAGE_SIZE,REG_SHM);
	if(reg < 0)
		goto errUList;
	if(!shm_addUser(mem,p,reg))
		goto errVmm;
	if(!sll_append(shareList,mem))
		goto errVmm;
	vmm_getRegRange(p,reg,&start,NULL);
	return start / PAGE_SIZE;

errVmm:
	vmm_remove(p,reg);
errUList:
	sll_destroy(mem->users,true);
errMem:
	kheap_free(mem);
	return ERR_NOT_ENOUGH_MEM;
}

s32 shm_join(sProc *p,const char *name) {
	u32 start;
	tVMRegNo reg;
	sShMemUser *owner;
	sShMem *mem = shm_get(name);
	if(mem == NULL || shm_getUser(mem,p) != NULL)
		return ERR_SHARED_MEM_INVALID;

	owner = (sShMemUser*)sll_get(mem->users,0);
	reg = vmm_join(owner->proc,owner->region,p);
	if(reg < 0)
		return ERR_NOT_ENOUGH_MEM;
	if(!shm_addUser(mem,p,reg)) {
		vmm_remove(p,reg);
		return ERR_NOT_ENOUGH_MEM;
	}
	vmm_getRegRange(p,reg,&start,NULL);
	return start / PAGE_SIZE;
}

s32 shm_leave(sProc *p,const char *name) {
	sShMem *mem = shm_get(name);
	sShMemUser *user;
	if(mem == NULL)
		return ERR_SHARED_MEM_INVALID;
	user = shm_getUser(mem,p);
	if(user == NULL || shm_isOwn(mem,p))
		return ERR_SHARED_MEM_INVALID;

	sll_removeFirst(mem->users,user);
	vmm_remove(p,user->region);
	kheap_free(user);
	return 0;
}

s32 shm_destroy(sProc *p,const char *name) {
	sShMem *mem = shm_get(name);
	sSLNode *n;
	if(mem == NULL)
		return ERR_SHARED_MEM_INVALID;
	if(!shm_isOwn(mem,p))
		return ERR_SHARED_MEM_INVALID;

	/* unmap it from the processes. otherwise we might reuse the frames which would
	 * lead to unpredictable results. so its better to unmap them which may cause a page-fault
	 * and termination of the process */
	for(n = sll_begin(mem->users); n != NULL; n = n->next) {
		sShMemUser *user = (sShMemUser*)n->data;
		vmm_remove(user->proc,user->region);
	}
	/* free shmem */
	sll_removeFirst(shareList,mem);
	sll_destroy(mem->users,true);
	kheap_free(mem);
	return 0;
}

s32 shm_cloneProc(sProc *parent,sProc *child) {
	s32 res;
	sSLNode *n;
	for(n = sll_begin(shareList); n != NULL; n = n->next) {
		sShMem *mem = (sShMem*)n->data;
		sShMemUser *user = shm_getUser(mem,parent);
		if(user) {
			if((res = shm_addUser(mem,child,user->region)) < 0) {
				/* remove all already created entries */
				shm_remProc(child);
				return res;
			}
		}
	}
	return 0;
}

void shm_remProc(sProc *p) {
	sSLNode *n,*t;
	for(n = sll_begin(shareList); n != NULL; ) {
		sShMem *mem = (sShMem*)n->data;
		if(shm_isOwn(mem,p)) {
			t = n->next;
			shm_destroy(p,mem->name);
			n = t;
		}
		else {
			sShMemUser *user = shm_getUser(mem,p);
			if(user)
				shm_leave(p,mem->name);
			n = n->next;
		}
	}
}

static bool shm_isOwn(sShMem *mem,sProc *p) {
	return ((sShMemUser*)sll_get(mem->users,0))->proc == p;
}

static bool shm_addUser(sShMem *mem,sProc *p,tVMRegNo reg) {
	sShMemUser *user = (sShMemUser*)kheap_alloc(sizeof(sShMemUser));
	if(user == NULL)
		return false;
	user->proc = p;
	user->region = reg;
	if(!sll_append(mem->users,user)) {
		kheap_free(user);
		return false;
	}
	return true;
}

static sShMem *shm_get(const char *name) {
	sSLNode *n;
	if(shareList == NULL)
		return NULL;
	for(n = sll_begin(shareList); n != NULL; n = n->next) {
		sShMem *mem = (sShMem*)n->data;
		if(strcmp(mem->name,name) == 0)
			return mem;
	}
	return NULL;
}

static sShMemUser *shm_getUser(sShMem *mem,sProc *p) {
	sSLNode *n;
	for(n = sll_begin(mem->users); n != NULL; n = n->next) {
		sShMemUser *user = (sShMemUser*)n->data;
		if(user->proc == p)
			return user;
	}
	return NULL;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void shm_dbg_print(void) {
	sSLNode *n,*m;
	vid_printf("Shared memory:\n");
	if(shareList == NULL)
		return;
	for(n = sll_begin(shareList); n != NULL; n = n->next) {
		sShMem *mem = (sShMem*)n->data;
		vid_printf("\tname=%s, users:\n",mem->name);
		for(m = sll_begin(mem->users); m != NULL; m = m->next) {
			sShMemUser *user = (sShMemUser*)m->data;
			vid_printf("\t\tproc %d (%s) with region %d\n",user->proc->pid,user->proc->command,
					user->region);
		}
	}
}

#endif
