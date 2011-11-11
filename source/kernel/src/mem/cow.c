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
#include <sys/mem/sllnodes.h>
#include <sys/task/proc.h>
#include <sys/util.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <assert.h>
#include <string.h>

#define COW_HEAP_SIZE	64

typedef struct {
	frameno_t frameNumber;
	size_t refCount;
} sCOW;

static sCOW *cow_getByFrame(frameno_t frameNo,bool dec);

static sSLList cowFrames[COW_HEAP_SIZE];
static klock_t cowLock;

void cow_init(void) {
	size_t i;
	for(i = 0; i < COW_HEAP_SIZE; i++)
		sll_init(cowFrames + i,slln_allocNode,slln_freeNode);
}

size_t cow_pagefault(uintptr_t address,frameno_t frameNumber) {
	sCOW *cow;
	uint flags;

	spinlock_aquire(&cowLock);
	/* find the cow-entry */
	cow = cow_getByFrame(frameNumber,true);
	vassert(cow != NULL,"No COW entry for frame %#x and address %p",frameNumber,address);

	/* if there is another process who wants to get the frame, we make a copy for us */
	/* otherwise we keep the frame for ourself */
	flags = PG_PRESENT | PG_WRITABLE;
	if(cow->refCount == 0)
		flags |= PG_KEEPFRM;
	/* can't fail, we've already allocated the frame */
	paging_map(address,NULL,1,flags);

	/* copy? */
	if(cow->refCount > 0)
		paging_copyFromFrame(frameNumber,(void*)(address & ~(PAGE_SIZE - 1)));
	else
		cache_free(cow);
	spinlock_release(&cowLock);
	return 1;
}

bool cow_add(frameno_t frameNo) {
	spinlock_aquire(&cowLock);
	sCOW *cow = cow_getByFrame(frameNo,false);
	if(!cow) {
		cow = (sCOW*)cache_alloc(sizeof(sCOW));
		if(cow == NULL) {
			spinlock_release(&cowLock);
			return false;
		}
		cow->frameNumber = frameNo;
		cow->refCount = 0;
		if(!sll_append(cowFrames + (frameNo % COW_HEAP_SIZE),cow)) {
			cache_free(cow);
			spinlock_release(&cowLock);
			return false;
		}
	}
	cow->refCount++;
	spinlock_release(&cowLock);
	return true;
}

size_t cow_remove(frameno_t frameNo,bool *foundOther) {
	sCOW *cow;
	spinlock_aquire(&cowLock);
	/* find the cow-entry */
	cow = cow_getByFrame(frameNo,true);
	vassert(cow != NULL,"For frameNo %#x",frameNo);

	*foundOther = cow->refCount > 0;
	if(cow->refCount == 0)
		cache_free(cow);
	spinlock_release(&cowLock);
	return 1;
}

size_t cow_getFrmCount(void) {
	size_t i,count = 0;
	spinlock_aquire(&cowLock);
	for(i = 0; i < COW_HEAP_SIZE; i++)
		count += sll_length(cowFrames + i);
	spinlock_release(&cowLock);
	return count;
}

void cow_print(void) {
	sSLNode *n;
	sCOW *cow;
	size_t i;
	vid_printf("COW-Frames: (%zu frames)\n",cow_getFrmCount());
	for(i = 0; i < COW_HEAP_SIZE; i++) {
		vid_printf("\t%zu:\n",i);
		for(n = sll_begin(cowFrames + i); n != NULL; n = n->next) {
			cow = (sCOW*)n->data;
			vid_printf("\t\t%#x (%zu refs)\n",cow->frameNumber,cow->refCount);
		}
	}
}

static sCOW *cow_getByFrame(frameno_t frameNo,bool dec) {
	sSLNode *n,*p;
	sSLList *list = cowFrames + (frameNo % COW_HEAP_SIZE);
	p = NULL;
	for(n = sll_begin(list); n != NULL; p = n, n = n->next) {
		sCOW *cow = (sCOW*)n->data;
		if(cow->frameNumber == frameNo) {
			if(dec) {
				if(--cow->refCount == 0)
					sll_removeNode(list,n,p);
			}
			return cow;
		}
	}
	return NULL;
}
