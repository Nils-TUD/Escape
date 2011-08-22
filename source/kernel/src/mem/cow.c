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
#include <sys/mem/cache.h>
#include <sys/mem/paging.h>
#include <sys/mem/cow.h>
#include <sys/task/proc.h>
#include <sys/util.h>
#include <sys/klock.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <assert.h>
#include <string.h>

typedef struct {
	frameno_t frameNumber;
	pid_t pid;
} sCOW;

/**
 * A list which contains each frame for each process that is marked as copy-on-write.
 * If a process causes a page-fault we will remove it from the list. If there is no other
 * entry for the frame in the list, the process can keep the frame, otherwise it is copied.
 */
static sSLList *cowFrames = NULL;
static klock_t cowLock;

void cow_init(void) {
	cowFrames = sll_create();
	assert(cowFrames != NULL);
}

size_t cow_pagefault(pid_t pid,uintptr_t address,frameno_t frameNumber) {
	sSLNode *n,*ln;
	sCOW *cow;
	sSLNode *ourCOW,*ourPrevCOW;
	bool foundOther;
	size_t frmCount;
	uint flags;

	/* search through the copy-on-write-list whether there is another one who wants to get
	 * the frame */
	klock_aquire(&cowLock);
	frmCount = 0;
	ourCOW = NULL;
	ourPrevCOW = NULL;
	foundOther = false;
	ln = NULL;
	for(n = sll_begin(cowFrames); n != NULL; ln = n, n = n->next) {
		cow = (sCOW*)n->data;
		if(cow->frameNumber == frameNumber) {
			if(cow->pid == pid) {
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
	vassert(ourCOW != NULL,"No COW entry for process %d and address 0x%x",pid,address);

	/* remove our from list and adjust pte */
	cache_free(ourCOW->data);
	sll_removeNode(cowFrames,ourCOW,ourPrevCOW);
	/* if there is another process who wants to get the frame, we make a copy for us */
	/* otherwise we keep the frame for ourself */
	flags = PG_PRESENT | PG_WRITABLE;
	if(!foundOther)
		flags |= PG_KEEPFRM;
	paging_map(address,NULL,1,flags);
	frmCount++;

	/* copy? */
	if(foundOther)
		paging_copyFromFrame(frameNumber,(void*)(address & ~(PAGE_SIZE - 1)));
	klock_release(&cowLock);
	return frmCount;
}

bool cow_add(pid_t pid,frameno_t frameNo) {
	sCOW *cc;
	cc = (sCOW*)cache_alloc(sizeof(sCOW));
	if(cc == NULL)
		return false;
	cc->frameNumber = frameNo;
	cc->pid = pid;
	klock_aquire(&cowLock);
	if(!sll_append(cowFrames,cc)) {
		klock_release(&cowLock);
		cache_free(cc);
		return false;
	}
	klock_release(&cowLock);
	return true;
}

size_t cow_remove(pid_t pid,frameno_t frameNo,bool *foundOther) {
	sSLNode *n,*tn,*ln;
	sCOW *cow;
	size_t frmCount = 0;
	bool foundOwn = false;

	/* search for the frame in the COW-list */
	klock_aquire(&cowLock);
	ln = NULL;
	*foundOther = false;
	for(n = sll_begin(cowFrames); n != NULL; ) {
		cow = (sCOW*)n->data;
		if(cow->pid == pid && cow->frameNumber == frameNo) {
			/* remove from COW-list */
			tn = n->next;
			frmCount++;
			cache_free(cow);
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
	vassert(foundOwn,"For frameNo %#x and proc %d",frameNo,pid);
	klock_release(&cowLock);
	return frmCount;
}

size_t cow_getFrmCount(void) {
	sSLNode *n;
	size_t i,count = 0;
	frameno_t *frames;
	klock_aquire(&cowLock);
	frames = (frameno_t*)cache_calloc(sll_length(cowFrames),sizeof(frameno_t));
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
	cache_free(frames);
	klock_release(&cowLock);
	return count;
}

void cow_print(void) {
	sSLNode *n;
	sCOW *cow;
	vid_printf("COW-Frames: (%zu frames)\n",cow_getFrmCount());
	for(n = sll_begin(cowFrames); n != NULL; n = n->next) {
		cow = (sCOW*)n->data;
		vid_printf("\tframe=0x%x, proc=%d (%s)\n",cow->frameNumber,cow->pid,
				proc_getByPid(cow->pid)->command);
	}
}
