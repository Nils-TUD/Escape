/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <sys/mem/pagedir.h>
#include <sys/mem/copyonwrite.h>
#include <sys/task/proc.h>
#include <sys/util.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <assert.h>
#include <string.h>

SList<CopyOnWrite::Entry> CopyOnWrite::frames[HEAP_SIZE];
SpinLock CopyOnWrite::lock;

size_t CopyOnWrite::pagefault(uintptr_t address,frameno_t frameNumber) {
	lock.down();
	/* find the cow-entry */
	Entry *cow = getByFrame(frameNumber,true);
	vassert(cow != NULL,"No COW entry for frame %#x and address %p",frameNumber,address);

	/* if there is another process who wants to get the frame, we make a copy for us */
	/* otherwise we keep the frame for ourself */
	uint flags = PG_PRESENT | PG_WRITABLE;
	if(cow->refCount == 0)
		flags |= PG_KEEPFRM;
	/* can't fail, we've already allocated the frame */
	PageDir::mapToCur(address,NULL,1,flags);

	/* copy? */
	if(cow->refCount > 0)
		PageDir::copyFromFrame(frameNumber,(void*)(ROUND_PAGE_DN(address)));
	else
		delete cow;
	lock.up();
	return 1;
}

bool CopyOnWrite::add(frameno_t frameNo) {
	lock.down();
	Entry *cow = getByFrame(frameNo,false);
	if(!cow) {
		cow = new Entry(frameNo);
		if(cow == NULL) {
			lock.up();
			return false;
		}
		frames[frameNo % HEAP_SIZE].append(cow);
	}
	cow->refCount++;
	lock.up();
	return true;
}

size_t CopyOnWrite::remove(frameno_t frameNo,bool *foundOther) {
	lock.down();
	/* find the cow-entry */
	Entry *cow = getByFrame(frameNo,true);
	vassert(cow != NULL,"For frameNo %#x",frameNo);

	*foundOther = cow->refCount > 0;
	if(cow->refCount == 0)
		delete cow;
	lock.up();
	return 1;
}

size_t CopyOnWrite::getFrmCount() {
	size_t count = 0;
	lock.down();
	for(size_t i = 0; i < HEAP_SIZE; i++)
		count += frames[i].length();
	lock.up();
	return count;
}

void CopyOnWrite::print(OStream &os) {
	os.writef("COW-Frames: (%zu frames)\n",getFrmCount());
	for(size_t i = 0; i < HEAP_SIZE; i++) {
		os.writef("\t%zu:\n",i);
		for(auto it = frames[i].cbegin(); it != frames[i].cend(); ++it)
			os.writef("\t\t%#x (%zu refs)\n",it->frameNumber,it->refCount);
	}
}

CopyOnWrite::Entry *CopyOnWrite::getByFrame(frameno_t frameNo,bool dec) {
	SList<Entry> *list = frames + (frameNo % HEAP_SIZE);
	Entry *p = NULL;
	for(auto it = list->begin(); it != list->end(); p = &*it, ++it) {
		if(it->frameNumber == frameNo) {
			if(dec) {
				if(--it->refCount == 0)
					list->removeAt(p,&*it);
			}
			return &*it;
		}
	}
	return NULL;
}
