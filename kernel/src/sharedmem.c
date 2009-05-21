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
#include <sharedmem.h>
#include <proc.h>
#include <kheap.h>
#include <paging.h>
#include <video.h>
#include <sllist.h>
#include <string.h>
#include <errors.h>

#define MAX_SHAREDMEM_NAME	16

/* a shared memory */
typedef struct {
	sProc *owner;
	sSLList *member;
	char name[MAX_SHAREDMEM_NAME + 1];
	u32 startPage;
	u32 pageCount;
} sSharedMem;

/**
 * Retrieves the shared-memory with given name. If p != NULL you'll also get a matching area if
 * the address-range overlaps.
 *
 * @param name the name
 * @param p the process
 * @param startPage the start-page
 * @param pageCount the number of pages
 * @return the shared mem or NULL
 */
static sSharedMem *shm_get(char *name,sProc *p,u32 startPage,u32 pageCount);

/* list with all shared memories */
static sSLList *shareList = NULL;

s32 shm_create(char *name,u32 pageCount) {
	sSharedMem *mem;
	sProc *p = proc_getRunning();
	u32 startPage = p->textPages + p->dataPages;
	/* checks */
	if(shm_get(name,p,startPage,pageCount) != NULL)
		return ERR_SHARED_MEM_EXISTS;

	if(strlen(name) > MAX_SHAREDMEM_NAME)
		return ERR_SHARED_MEM_NAME;

	/* change size */
	if(!proc_changeSize(pageCount,CHG_DATA))
		return ERR_NOT_ENOUGH_MEM;

	/* create list */
	if(shareList == NULL) {
		shareList = sll_create();
		if(shareList == NULL) {
			proc_changeSize(-pageCount,CHG_DATA);
			return ERR_NOT_ENOUGH_MEM;
		}
	}

	/* create entry */
	mem = (sSharedMem*)kheap_alloc(sizeof(sSharedMem));
	if(mem == NULL) {
		proc_changeSize(-pageCount,CHG_DATA);
		return ERR_NOT_ENOUGH_MEM;
	}
	mem->member = sll_create();
	if(mem->member == NULL) {
		kheap_free(mem);
		proc_changeSize(-pageCount,CHG_DATA);
		return ERR_NOT_ENOUGH_MEM;
	}
	mem->owner = p;
	strcpy(mem->name,name);
	mem->startPage = startPage;
	mem->pageCount = pageCount;
	if(!sll_append(shareList,mem)) {
		sll_destroy(mem->member,false);
		kheap_free(mem);
		proc_changeSize(-pageCount,CHG_DATA);
		return ERR_NOT_ENOUGH_MEM;
	}
	return startPage;
}

s32 shm_join(char *name) {
	sProc *p = proc_getRunning();
	sSharedMem *mem = shm_get(name,NULL,0,0);
	if(mem == NULL || mem->owner == p || sll_indexOf(mem->member,p) >= 0)
		return ERR_SHARED_MEM_INVALID;

	if(!sll_append(mem->member,p))
		return ERR_NOT_ENOUGH_MEM;

	/* copy the pages from the owner */
	paging_mapForeignPages(mem->owner,mem->startPage * PAGE_SIZE,
			(p->textPages + p->dataPages) * PAGE_SIZE,mem->pageCount,PG_WRITABLE | PG_NOFREE);
	p->dataPages += mem->pageCount;

	return (p->textPages + p->dataPages) - mem->pageCount;
}

s32 shm_leave(char *name) {
	sProc *p = proc_getRunning();
	sSharedMem *mem = shm_get(name,NULL,0,0);
	if(mem == NULL || mem->owner == p)
		return ERR_SHARED_MEM_INVALID;

	sll_removeFirst(mem->member,p);
	return 0;
}

s32 shm_destroy(char *name) {
	sSLNode *n;
	sProc *p = proc_getRunning();
	sSharedMem *mem = shm_get(name,NULL,0,0);
	if(mem == NULL || mem->owner != p)
		return ERR_SHARED_MEM_INVALID;

	/* unmap it from the joined processes. otherwise we might reuse the frames which would
	 * lead to unpredictable results. so its better to unmap them which may lead to a page-fault
	 * and termination of the process */
	for(n = sll_begin(mem->member); n != NULL; n = n->next) {
		p = (sProc*)n->data;
		paging_unmapForeignPages(p,mem->startPage,mem->pageCount);
	}

	/* free mem */
	sll_destroy(mem->member,false);
	sll_removeFirst(shareList,mem);
	kheap_free(mem);
	return 0;
}

void shm_remProc(sProc *p) {
	sSLNode *n,*t;
	sSharedMem *mem;
	for(n = sll_begin(shareList); n != NULL; ) {
		mem = (sSharedMem*)n->data;
		if(mem->owner == p) {
			t = n->next;
			/* will remove the node */
			shm_destroy(mem->name);
			n = t;
		}
		/* try to remove the process; doesn't matter if the process is a member or not */
		else {
			sll_removeFirst(mem->member,p);
			n = n->next;
		}
	}
}

static sSharedMem *shm_get(char *name,sProc *p,u32 startPage,u32 pageCount) {
	sSLNode *n;
	sSharedMem *mem;
	if(shareList == NULL)
		return NULL;
	for(n = sll_begin(shareList); n != NULL; n = n->next) {
		mem = (sSharedMem*)n->data;
		if(strcmp(mem->name,name) == 0)
			return mem;
		/* check wether the address-range overlaps */
		if(p && mem->owner == p && OVERLAPS(mem->startPage,mem->startPage + mem->pageCount,
				startPage,startPage + pageCount))
			return mem;
	}
	return NULL;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void shm_dbg_print(void) {
	sSLNode *n;
	sSLNode *mn;
	sSharedMem *mem;
	vid_printf("Shared memory:\n");
	if(shareList == NULL)
		return;
	for(n = sll_begin(shareList); n != NULL; n = n->next) {
		mem = (sSharedMem*)n->data;
		vid_printf("\towner=%s, startPage=%d, pageCount=%d, member:\n",mem->owner->command,
				mem->startPage,mem->pageCount);
		for(mn = sll_begin(mem->member); mn != NULL; mn = mn->next)
			vid_printf("\t\t- %s\n",((sProc*)mn->data)->command);
	}
}

#endif
