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
#include <mem/kheap.h>
#include <mem/paging.h>
#include <mem/cow.h>
#include <task/proc.h>
#include <util.h>
#include <video.h>
#include <assert.h>
#include <string.h>
#include <sllist.h>

typedef struct {
	u32 frameNumber;
	bool isOwner;
	sProc *proc;
} sCOW;

/**
 * A list which contains each frame for each process that is marked as copy-on-write.
 * If a process causes a page-fault we will remove it from the list. If there is no other
 * entry for the frame in the list, the process can keep the frame, otherwise it is copied.
 */
static sSLList *cowFrames = NULL;

void cow_init(void) {
	cowFrames = sll_create();
	assert(cowFrames != NULL);
}

void cow_pagefault(u32 address) {
	sSLNode *n,*ln;
	sCOW *cow;
	bool owner;
	sSLNode *ourCOW,*ourPrevCOW;
	bool foundOther;
	u32 flags,frameNumber;
	sProc *cp = proc_getRunning();

	/* search through the copy-on-write-list whether there is another one who wants to get
	 * the frame */
	owner = false;
	ourCOW = NULL;
	ourPrevCOW = NULL;
	foundOther = false;
	frameNumber = paging_getFrameNo(address);
	ln = NULL;
	for(n = sll_begin(cowFrames); n != NULL; ln = n, n = n->next) {
		cow = (sCOW*)n->data;
		if(cow->frameNumber == frameNumber) {
			if(cow->proc == cp) {
				ourCOW = n;
				ourPrevCOW = ln;
				owner = cow->isOwner;
			}
			else
				foundOther = true;
			/* if we have both, we can stop here */
			if(ourCOW && foundOther)
				break;
		}
	}

	/* should never happen */
	vassert(ourCOW != NULL,"No COW entry for process %d and address 0x%x",cp->pid,address);

	/* remove our from list and adjust pte */
	kheap_free(ourCOW->data);
	sll_removeNode(cowFrames,ourCOW,ourPrevCOW);
	/* if there is another process who wants to get the frame, we make a copy for us */
	/* otherwise we keep the frame for ourself */
	flags = PG_PRESENT | PG_WRITABLE;
	if(!foundOther)
		flags |= PG_KEEPFRM;
	paging_map(address,NULL,1,flags);
	/* if we're not the owner of this cow-page, we don't "own" the physical mem yet. so
	 * we're changing that here */
	if(!owner)
		cp->frameCount++;

	/* copy? */
	if(foundOther) {
		/* map the frame and copy it */
		paging_map(TEMP_MAP_PAGE,&frameNumber,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
		memcpy((void*)(address & ~(PAGE_SIZE - 1)),(void*)TEMP_MAP_PAGE,PAGE_SIZE);
		paging_unmap(TEMP_MAP_PAGE,1,false);
	}
}

bool cow_add(sProc *p,u32 frameNo,bool isParent) {
	sCOW *cc;
	cc = (sCOW*)kheap_alloc(sizeof(sCOW));
	if(cc == NULL)
		return false;
	cc->frameNumber = frameNo;
	cc->proc = p;
	cc->isOwner = isParent;
	if(!sll_append(cowFrames,cc)) {
		kheap_free(cc);
		return false;
	}
	return true;
}

bool cow_remove(sProc *p,u32 frameNo) {
	sSLNode *n,*tn,*ln;
	sCOW *cow;
	bool foundOwn = false,foundOther = false;

	/* search for the frame in the COW-list */
	ln = NULL;
	for(n = sll_begin(cowFrames); n != NULL; ) {
		cow = (sCOW*)n->data;
		if(cow->proc == p && cow->frameNumber == frameNo) {
			/* remove from COW-list */
			tn = n->next;
			if(cow->isOwner)
				p->frameCount--;
			kheap_free(cow);
			sll_removeNode(cowFrames,n,ln);
			n = tn;
			foundOwn = true;
			continue;
		}

		/* usage of other process? */
		if(cow->frameNumber == frameNo)
			foundOther = true;
		/* we can stop if we have both */
		if(foundOther && foundOwn)
			break;
		ln = n;
		n = n->next;
	}

	/* if no other process uses this frame, we have to free it */
	return !foundOther;
}


#if DEBUGGING

void cow_dbg_print(void) {
	sSLNode *n;
	sCOW *cow;
	vid_printf("COW-Frames:\n");
	for(n = sll_begin(cowFrames); n != NULL; n = n->next) {
		cow = (sCOW*)n->data;
		vid_printf("\tframe=0x%x, proc=%d (%s)\n",cow->frameNumber,cow->proc->pid,cow->proc->command);
	}
}

#endif
