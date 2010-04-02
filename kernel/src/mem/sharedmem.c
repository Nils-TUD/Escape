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

#include <common.h>
#include <mem/sharedmem.h>
#include <mem/kheap.h>
#include <mem/paging.h>
#include <mem/pmem.h>
#include <task/proc.h>
#include <video.h>
#include <sllist.h>
#include <string.h>
#include <errors.h>

#define MAX_SHAREDMEM_NAME	15

typedef struct {
	/* all users of this area, including the owner; owner is always the first */
	sSLList *user;
	/* start-page of the area in the page-dir of the owner */
	u32 ownerStart;
	u32 pageCount;
	char name[MAX_SHAREDMEM_NAME + 1];
} sShMem;

typedef struct {
	/* pointer to the area */
	sShMem *mem;
	/* start-page of the area for this process */
	u32 startPage;
	sProc *proc;
} sShMemUsage;

/**
 * Creates and adds a new usage
 */
static bool shm_addUsage(sShMem *mem,sProc *p,u32 startPage);
/**
 * Retrieve the shared-memory-area by name
 */
static sShMem *shm_get(const char *name);
/**
 * Retrieve the usage of the shared-memory-area with given name of the given process
 */
static sShMemUsage *shm_getUsage(const char *name,sProc *p);

/* list with all shared memories */
static sSLList *shareList = NULL;

s32 shm_create(const char *name,u32 pageCount) {
	sShMem *mem;
	sProc *p = proc_getRunning();
	u32 startPage = 0/*p->textPages + p->dataPages*/;
	/* checks */
	/*if(!proc_segSizesValid(p->textPages,p->dataPages + pageCount,p->stackPages))
		return ERR_NOT_ENOUGH_MEM;*/
	if(strlen(name) > MAX_SHAREDMEM_NAME)
		return ERR_SHARED_MEM_NAME;

	/* create list */
	if(shareList == NULL) {
		shareList = sll_create();
		if(shareList == NULL)
			return ERR_NOT_ENOUGH_MEM;
	}

	/* change size */
	/*if(!proc_changeSize(pageCount,CHG_DATA))
		return ERR_NOT_ENOUGH_MEM;*/
	/* check here because proc_changeSize() can cause a thread-switch! */
	if(shm_get(name) != NULL) {
		/*proc_changeSize(-pageCount,CHG_DATA);*/
		return ERR_SHARED_MEM_EXISTS;
	}

	/* create entry */
	mem = (sShMem*)kheap_alloc(sizeof(sShMem));
	if(mem == NULL)
		goto errChangeSize;
	mem->ownerStart = startPage;
	mem->pageCount = pageCount;
	strcpy(mem->name,name);
	mem->user = sll_create();
	if(mem->user == NULL)
		goto errMem;
	if(!sll_append(mem->user,p))
		goto errUser;

	if(!shm_addUsage(mem,p,startPage))
		goto errUser;
	return startPage;

errUser:
	sll_destroy(mem->user,false);
errMem:
	kheap_free(mem);
errChangeSize:
	/*proc_changeSize(-pageCount,CHG_DATA);*/
	return ERR_NOT_ENOUGH_MEM;
}

bool shm_isSharedMem(sProc *p,u32 addr,u32 *pageCount,bool *isOwner) {
	sShMemUsage *use = shm_getUsage(NULL,p);
	if(use && addr >= use->startPage * PAGE_SIZE &&
			addr < (use->startPage + use->mem->pageCount) * PAGE_SIZE) {
		*pageCount = use->mem->pageCount;
		*isOwner = sll_get(use->mem->user,0) == p;
		return true;
	}
	return false;
}

u32 shm_getAddrOfOther(sProc *p,u32 addr,sProc *other) {
	sShMemUsage *use = shm_getUsage(NULL,p);
	if(use && addr >= use->startPage * PAGE_SIZE &&
		addr < (use->startPage + use->mem->pageCount) * PAGE_SIZE) {
		sShMemUsage *useOther = shm_getUsage(NULL,other);
		if(useOther)
			return useOther->startPage * PAGE_SIZE + (addr - use->startPage * PAGE_SIZE);
	}
	return 0;
}

sSLList *shm_getMembers(sProc *owner,u32 addr) {
	sShMemUsage *use = shm_getUsage(NULL,owner);
	if(use && addr >= use->startPage * PAGE_SIZE &&
			addr < (use->startPage + use->mem->pageCount) * PAGE_SIZE)
		return use->mem->user;
	return NULL;
}

s32 shm_join(const char *name) {
	sProc *p = proc_getRunning();
	sProc *owner;
	sShMem *mem = shm_get(name);
	if(mem == NULL || sll_indexOf(mem->user,p) >= 0)
		return ERR_SHARED_MEM_INVALID;

	/* check process-size */
	/*if(!proc_segSizesValid(p->textPages,p->dataPages + mem->pageCount,p->stackPages))
		return ERR_NOT_ENOUGH_MEM;*/

	if(!sll_append(mem->user,p))
		return ERR_NOT_ENOUGH_MEM;

	/*if(!shm_addUsage(mem,p,p->textPages + p->dataPages)) {
		sll_removeFirst(mem->user,p);
		return ERR_NOT_ENOUGH_MEM;
	}*/

	/* copy the pages from the owner */
	owner = (sProc*)sll_get(mem->user,0);
	/* TODO */
	/*paging_getPagesOf(owner,mem->ownerStart * PAGE_SIZE,
			(p->textPages + p->dataPages) * PAGE_SIZE,mem->pageCount,PG_WRITABLE);*/
	/*p->dataPages += mem->pageCount;*/

	return 0/*(p->textPages + p->dataPages) - mem->pageCount*/;
}

s32 shm_leave(const char *name) {
	sProc *p = proc_getRunning();
	sShMemUsage *usage = shm_getUsage(name,p);
	if(usage == NULL)
		return ERR_SHARED_MEM_INVALID;

	sll_removeFirst(usage->mem->user,p);
	sll_removeFirst(shareList,usage);
	kheap_free(usage);
	return 0;
}

s32 shm_destroy(const char *name) {
	sProc *p = proc_getRunning();
	sShMemUsage *usage;
	sShMem *mem = shm_get(name);
	if(mem == NULL || sll_get(mem->user,0) != p)
		return ERR_SHARED_MEM_INVALID;

	/* unmap it from the joined processes. otherwise we might reuse the frames which would
	 * lead to unpredictable results. so its better to unmap them which may cause a page-fault
	 * and termination of the process */
	while((usage = shm_getUsage(name,NULL))) {
		/* TODO */
		/*if(usage->proc != p)
			paging_remPagesOf(usage->proc,usage->startPage,mem->pageCount);*/
		sll_removeFirst(shareList,usage);
		kheap_free(usage);
	}

	/* Note that we can't remove the area from us (via proc_changeSize()) because maybe the
	 * data-area has been increased afterwards */

	/* free mem */
	sll_destroy(mem->user,false);
	kheap_free(mem);
	return 0;
}

void shm_remProc(sProc *p) {
	sSLNode *n,*t;
	sShMemUsage *use;
	for(n = sll_begin(shareList); n != NULL; ) {
		use = (sShMemUsage*)n->data;
		if(use->proc == p) {
			if(sll_get(use->mem->user,0) == p) {
				shm_destroy(use->mem->name);
				/* start from the beginning because several usages may have been removed */
				n = sll_begin(shareList);
			}
			else {
				t = n->next;
				/* will remove this usage */
				shm_leave(use->mem->name);
				n = t;
			}
		}
		else
			n = n->next;
	}
}

static bool shm_addUsage(sShMem *mem,sProc *p,u32 startPage) {
	sShMemUsage *usage = (sShMemUsage*)kheap_alloc(sizeof(sShMemUsage));
	if(usage == NULL)
		return false;
	usage->mem = mem;
	usage->proc = p;
	usage->startPage = startPage;

	if(!sll_append(shareList,usage)) {
		kheap_free(usage);
		return false;
	}
	return true;
}

static sShMem *shm_get(const char *name) {
	sSLNode *n;
	sShMemUsage *use;
	if(shareList == NULL)
		return NULL;
	for(n = sll_begin(shareList); n != NULL; n = n->next) {
		use = (sShMemUsage*)n->data;
		if(strcmp(use->mem->name,name) == 0)
			return use->mem;
	}
	return NULL;
}

static sShMemUsage *shm_getUsage(const char *name,sProc *p) {
	sSLNode *n;
	sShMemUsage *use;
	if(shareList == NULL)
		return NULL;
	for(n = sll_begin(shareList); n != NULL; n = n->next) {
		use = (sShMemUsage*)n->data;
		if(name == NULL && use->proc == p)
			return use;
		if((p == NULL || use->proc == p) && strcmp(use->mem->name,name) == 0)
			return use;
	}
	return NULL;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void shm_dbg_print(void) {
	sSLNode *n;
	sShMemUsage *use;
	vid_printf("Shared memory:\n");
	if(shareList == NULL)
		return;
	for(n = sll_begin(shareList); n != NULL; n = n->next) {
		use = (sShMemUsage*)n->data;
		vid_printf("\tproc=%s, startPage=%d [%s of %s with %d pages]\n",use->proc->command,
				use->startPage,use->mem->name,((sProc*)sll_get(use->mem->user,0))->command,
				use->mem->pageCount);
	}
}

#endif
