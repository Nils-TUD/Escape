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
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/mem/cow.h>
#include <sys/task/proc.h>
#include <sys/util.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <assert.h>
#include <string.h>

typedef struct {
	u32 frameNumber;
	const sProc *proc;
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

u32 cow_pagefault(u32 address) {
	sSLNode *n,*ln;
	sCOW *cow;
	sSLNode *ourCOW,*ourPrevCOW;
	bool foundOther;
	u32 frmCount,flags,frameNumber;
	sProc *cp = proc_getRunning();

	/* search through the copy-on-write-list whether there is another one who wants to get
	 * the frame */
	frmCount = 0;
	ourCOW = NULL;
	ourPrevCOW = NULL;
	foundOther = false;
	frameNumber = paging_getFrameNo(cp->pagedir,address);
	ln = NULL;
	for(n = sll_begin(cowFrames); n != NULL; ln = n, n = n->next) {
		cow = (sCOW*)n->data;
		if(cow->frameNumber == frameNumber) {
			if(cow->proc == cp) {
				ourCOW = n;
				ourPrevCOW = ln;
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
	frmCount++;

	/* copy? */
	if(foundOther) {
		/* map the frame and copy it */
		u32 temp = paging_mapToTemp(&frameNumber,1);
		memcpy((void*)(address & ~(PAGE_SIZE - 1)),(void*)temp,PAGE_SIZE);
		paging_unmapFromTemp(1);
	}
	return frmCount;
}

bool cow_add(const sProc *p,u32 frameNo) {
	sCOW *cc;
	cc = (sCOW*)kheap_alloc(sizeof(sCOW));
	if(cc == NULL)
		return false;
	cc->frameNumber = frameNo;
	cc->proc = p;
	if(!sll_append(cowFrames,cc)) {
		kheap_free(cc);
		return false;
	}
	return true;
}

u32 cow_remove(const sProc *p,u32 frameNo,bool *foundOther) {
	sSLNode *n,*tn,*ln;
	sCOW *cow;
	u32 frmCount = 0;
	bool foundOwn = false;

	/* search for the frame in the COW-list */
	ln = NULL;
	*foundOther = false;
	for(n = sll_begin(cowFrames); n != NULL; ) {
		cow = (sCOW*)n->data;
		if(cow->proc == p && cow->frameNumber == frameNo) {
			/* remove from COW-list */
			tn = n->next;
			frmCount++;
			kheap_free(cow);
			sll_removeNode(cowFrames,n,ln);
			n = tn;
			foundOwn = true;
			continue;
		}

		/* usage of other process? */
		if(cow->frameNumber == frameNo)
			*foundOther = true;
		/* we can stop if we have both */
		if(*foundOther && foundOwn)
			break;
		ln = n;
		n = n->next;
	}
	vassert(foundOwn,"For frameNo %#x and proc %d",frameNo,p->pid);
	return frmCount;
}

u32 cow_getFrmCount(void) {
	sSLNode *n;
	u32 i,count = 0;
	u32 *frames = (u32*)kheap_calloc(sll_length(cowFrames),sizeof(u32));
	if(!frames)
		return 0;
	for(n = sll_begin(cowFrames); n != NULL; n = n->next) {
		sCOW *cow = (sCOW*)n->data;
		bool found = false;
		for(i = 0; i < count; i++) {
			if(frames[i] == cow->frameNumber) {
				found = true;
				break;
			}
		}
		if(!found)
			frames[count++] = cow->frameNumber;
	}
	kheap_free(frames);
	return count;
}


#if DEBUGGING

void cow_dbg_print(void) {
	sSLNode *n;
	sCOW *cow;
	vid_printf("COW-Frames: (%d frames)\n",cow_getFrmCount());
	for(n = sll_begin(cowFrames); n != NULL; n = n->next) {
		cow = (sCOW*)n->data;
		vid_printf("\tframe=0x%x, proc=%d (%s)\n",cow->frameNumber,cow->proc->pid,cow->proc->command);
	}
}

#endif
