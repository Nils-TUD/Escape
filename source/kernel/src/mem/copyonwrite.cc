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
#include <sys/mem/copyonwrite.h>
#include <sys/mem/sllnodes.h>
#include <sys/task/proc.h>
#include <sys/util.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <assert.h>
#include <string.h>

sSLList CopyOnWrite::frames[HEAP_SIZE];
klock_t CopyOnWrite::lock;

void CopyOnWrite::init(void) {
	size_t i;
	for(i = 0; i < HEAP_SIZE; i++)
		sll_init(frames + i,slln_allocNode,slln_freeNode);
}

size_t CopyOnWrite::pagefault(uintptr_t address,frameno_t frameNumber) {
	Entry *cow;
	uint flags;

	spinlock_aquire(&lock);
	/* find the cow-entry */
	cow = getByFrame(frameNumber,true);
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
		paging_copyFromFrame(frameNumber,(void*)(ROUND_PAGE_DN(address)));
	else
		Cache::free(cow);
	spinlock_release(&lock);
	return 1;
}

bool CopyOnWrite::add(frameno_t frameNo) {
	spinlock_aquire(&lock);
	Entry *cow = getByFrame(frameNo,false);
	if(!cow) {
		cow = (Entry*)Cache::alloc(sizeof(Entry));
		if(cow == NULL) {
			spinlock_release(&lock);
			return false;
		}
		cow->frameNumber = frameNo;
		cow->refCount = 0;
		if(!sll_append(frames + (frameNo % HEAP_SIZE),cow)) {
			Cache::free(cow);
			spinlock_release(&lock);
			return false;
		}
	}
	cow->refCount++;
	spinlock_release(&lock);
	return true;
}

size_t CopyOnWrite::remove(frameno_t frameNo,bool *foundOther) {
	Entry *cow;
	spinlock_aquire(&lock);
	/* find the cow-entry */
	cow = getByFrame(frameNo,true);
	vassert(cow != NULL,"For frameNo %#x",frameNo);

	*foundOther = cow->refCount > 0;
	if(cow->refCount == 0)
		Cache::free(cow);
	spinlock_release(&lock);
	return 1;
}

size_t CopyOnWrite::getFrmCount(void) {
	size_t i,count = 0;
	spinlock_aquire(&lock);
	for(i = 0; i < HEAP_SIZE; i++)
		count += sll_length(frames + i);
	spinlock_release(&lock);
	return count;
}

void CopyOnWrite::print(void) {
	sSLNode *n;
	Entry *cow;
	size_t i;
	vid_printf("COW-Frames: (%zu frames)\n",getFrmCount());
	for(i = 0; i < HEAP_SIZE; i++) {
		vid_printf("\t%zu:\n",i);
		for(n = sll_begin(frames + i); n != NULL; n = n->next) {
			cow = (Entry*)n->data;
			vid_printf("\t\t%#x (%zu refs)\n",cow->frameNumber,cow->refCount);
		}
	}
}

CopyOnWrite::Entry *CopyOnWrite::getByFrame(frameno_t frameNo,bool dec) {
	sSLNode *n,*p;
	sSLList *list = frames + (frameNo % HEAP_SIZE);
	p = NULL;
	for(n = sll_begin(list); n != NULL; p = n, n = n->next) {
		Entry *cow = (Entry*)n->data;
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
