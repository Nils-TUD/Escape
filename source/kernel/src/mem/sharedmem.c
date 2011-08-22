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
#include <sys/mem/cache.h>
#include <sys/mem/paging.h>
#include <sys/mem/vmm.h>
#include <sys/task/proc.h>
#include <sys/video.h>
#include <sys/klock.h>
#include <esc/sllist.h>
#include <string.h>
#include <assert.h>
#include <errors.h>

typedef struct {
	sSLList *users;
	char name[MAX_SHAREDMEM_NAME + 1];
} sShMem;

typedef struct {
	pid_t pid;
	vmreg_t region;
} sShMemUser;

/**
 * Leaves the region without using locks
 */
static int shm_doLeave(pid_t pid,const char *name);
/**
 * Destroys the region without using locks
 */
static int shm_doDestroy(pid_t pid,const char *name);
/**
 * Tests whether its the own (creator) shm
 */
static bool shm_isOwn(const sShMem *mem,pid_t pid);
/**
 * Creates and adds a new user
 */
static bool shm_addUser(sShMem *mem,pid_t pid,vmreg_t reg);
/**
 * Retrieve the shared-memory-area by name
 */
static sShMem *shm_get(const char *name);
/**
 * Retrieve the usage of the shared-memory-area of the given process
 */
static sShMemUser *shm_getUser(const sShMem *mem,pid_t pid);

/* list with all shared memories */
static sSLList *shareList = NULL;
static klock_t shmLock;

void shm_init(void) {
	shareList = sll_create();
	assert(shareList != NULL);
}

ssize_t shm_create(pid_t pid,const char *name,size_t pageCount) {
	sShMem *mem;
	uintptr_t start;
	vmreg_t reg;

	/* checks */
	if(strlen(name) > MAX_SHAREDMEM_NAME)
		return ERR_SHARED_MEM_NAME;
	klock_aquire(&shmLock);
	if(shm_get(name) != NULL) {
		klock_release(&shmLock);
		return ERR_SHARED_MEM_EXISTS;
	}

	/* create entry */
	mem = (sShMem*)cache_alloc(sizeof(sShMem));
	if(mem == NULL)
		goto errLock;
	strcpy(mem->name,name);
	mem->users = sll_create();
	if(mem == NULL)
		goto errMem;
	reg = vmm_add(pid,NULL,0,pageCount * PAGE_SIZE,pageCount * PAGE_SIZE,REG_SHM);
	if(reg < 0)
		goto errUList;
	if(!shm_addUser(mem,pid,reg))
		goto errVmm;
	if(!sll_append(shareList,mem))
		goto errVmm;
	vmm_getRegRange(pid,reg,&start,NULL);
	klock_release(&shmLock);
	return start / PAGE_SIZE;

errVmm:
	vmm_remove(pid,reg);
errUList:
	sll_destroy(mem->users,true);
errMem:
	cache_free(mem);
errLock:
	klock_release(&shmLock);
	return ERR_NOT_ENOUGH_MEM;
}

ssize_t shm_join(pid_t pid,const char *name) {
	uintptr_t start;
	vmreg_t reg;
	sShMemUser *owner;
	sShMem *mem;
	klock_aquire(&shmLock);

	mem = shm_get(name);
	if(mem == NULL || shm_getUser(mem,pid) != NULL) {
		klock_release(&shmLock);
		return ERR_SHARED_MEM_INVALID;
	}

	owner = (sShMemUser*)sll_get(mem->users,0);
	reg = vmm_join(owner->pid,owner->region,pid);
	if(reg < 0) {
		klock_release(&shmLock);
		return ERR_NOT_ENOUGH_MEM;
	}
	if(!shm_addUser(mem,pid,reg)) {
		vmm_remove(pid,reg);
		klock_release(&shmLock);
		return ERR_NOT_ENOUGH_MEM;
	}
	vmm_getRegRange(pid,reg,&start,NULL);
	klock_release(&shmLock);
	return start / PAGE_SIZE;
}

int shm_leave(pid_t pid,const char *name) {
	int res;
	klock_aquire(&shmLock);
	res = shm_doLeave(pid,name);
	klock_release(&shmLock);
	return res;
}

int shm_destroy(pid_t pid,const char *name) {
	int res;
	klock_aquire(&shmLock);
	res = shm_doDestroy(pid,name);
	klock_release(&shmLock);
	return res;
}

int shm_cloneProc(pid_t parent,pid_t child) {
	sSLNode *n;
	klock_aquire(&shmLock);
	for(n = sll_begin(shareList); n != NULL; n = n->next) {
		sShMem *mem = (sShMem*)n->data;
		sShMemUser *user = shm_getUser(mem,parent);
		if(user) {
			if(!shm_addUser(mem,child,user->region)) {
				klock_release(&shmLock);
				/* remove all already created entries */
				shm_remProc(child);
				return ERR_NOT_ENOUGH_MEM;
			}
		}
	}
	klock_release(&shmLock);
	return 0;
}

void shm_remProc(pid_t pid) {
	sSLNode *n,*t;
	klock_aquire(&shmLock);
	for(n = sll_begin(shareList); n != NULL; ) {
		sShMem *mem = (sShMem*)n->data;
		if(shm_isOwn(mem,pid)) {
			t = n->next;
			shm_doDestroy(pid,mem->name);
			n = t;
		}
		else {
			sShMemUser *user = shm_getUser(mem,pid);
			if(user)
				shm_doLeave(pid,mem->name);
			n = n->next;
		}
	}
	klock_release(&shmLock);
}

void shm_print(void) {
	sSLNode *n,*m;
	vid_printf("Shared memory:\n");
	if(shareList == NULL)
		return;
	for(n = sll_begin(shareList); n != NULL; n = n->next) {
		sShMem *mem = (sShMem*)n->data;
		vid_printf("\tname=%s, users:\n",mem->name);
		for(m = sll_begin(mem->users); m != NULL; m = m->next) {
			sShMemUser *user = (sShMemUser*)m->data;
			vid_printf("\t\tproc %d (%s) with region %d\n",user->pid,
					proc_getByPid(user->pid)->command,user->region);
		}
	}
}

static int shm_doLeave(pid_t pid,const char *name) {
	sShMem *mem;
	sShMemUser *user;

	mem = shm_get(name);
	if(mem == NULL)
		return ERR_SHARED_MEM_INVALID;
	user = shm_getUser(mem,pid);
	if(user == NULL || shm_isOwn(mem,pid))
		return ERR_SHARED_MEM_INVALID;

	sll_removeFirstWith(mem->users,user);
	vmm_remove(pid,user->region);
	cache_free(user);
	return 0;
}

static int shm_doDestroy(pid_t pid,const char *name) {
	sShMem *mem;
	sSLNode *n;

	mem = shm_get(name);
	if(mem == NULL)
		return ERR_SHARED_MEM_INVALID;
	if(!shm_isOwn(mem,pid))
		return ERR_SHARED_MEM_INVALID;

	/* unmap it from the processes. otherwise we might reuse the frames which would
	 * lead to unpredictable results. so its better to unmap them which may cause a page-fault
	 * and termination of the process */
	for(n = sll_begin(mem->users); n != NULL; n = n->next) {
		sShMemUser *user = (sShMemUser*)n->data;
		vmm_remove(user->pid,user->region);
	}
	/* free shmem */
	sll_removeFirstWith(shareList,mem);
	sll_destroy(mem->users,true);
	cache_free(mem);
	return 0;
}

static bool shm_isOwn(const sShMem *mem,pid_t pid) {
	return ((sShMemUser*)sll_get(mem->users,0))->pid == pid;
}

static bool shm_addUser(sShMem *mem,pid_t pid,vmreg_t reg) {
	sShMemUser *user = (sShMemUser*)cache_alloc(sizeof(sShMemUser));
	if(user == NULL)
		return false;
	user->pid = pid;
	user->region = reg;
	if(!sll_append(mem->users,user)) {
		cache_free(user);
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

static sShMemUser *shm_getUser(const sShMem *mem,pid_t pid) {
	sSLNode *n;
	for(n = sll_begin(mem->users); n != NULL; n = n->next) {
		sShMemUser *user = (sShMemUser*)n->data;
		if(user->pid == pid)
			return user;
	}
	return NULL;
}
