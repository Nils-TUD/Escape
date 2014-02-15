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
#include <sys/mem/kheap.h>
#include <sys/spinlock.h>
#include <sys/log.h>
#include <sys/video.h>
#include <sys/util.h>
#include <string.h>
#include <assert.h>

#if DEBUGGING
#define DEBUG_ALLOC_N_FREE	1
#endif

#define GUARD_MAGIC			0xCAFEBABE
#define MIN_OBJ_COUNT		8
#define SIZE_THRESHOLD		128
#define HEAP_THRESHOLD		512

SpinLock Cache::lock;
Cache::Entry Cache::caches[] = {
	{16,0,0,NULL},
	{32,0,0,NULL},
	{64,0,0,NULL},
	{128,0,0,NULL},
	{256,0,0,NULL},
	{512,0,0,NULL},
	{1024,0,0,NULL},
	{2048,0,0,NULL},
	{4096,0,0,NULL},
	{8192,0,0,NULL},
	{16384,0,0,NULL},
};
#if DEBUGGING
bool Cache::aafEnabled = false;
#endif

void *Cache::alloc(size_t size) {
	void *res;
	if(size == 0)
		return NULL;

	for(size_t i = 0; i < ARRAY_SIZE(caches); i++) {
		size_t objSize = caches[i].objSize;
		if(objSize >= size) {
			if((objSize - size) >= SIZE_THRESHOLD)
				break;
			res = get(caches + i,i);
			goto done;
		}
	}
	res = KHeap::alloc(size);

done:
#if DEBUG_ALLOC_N_FREE
	if(aafEnabled) {
		lock.down();
		Util::printEventTrace(Log::get(),Util::getKernelStackTrace(),"\n[A] %Px %zu ",res,size);
		lock.up();
	}
#endif
	return res;
}

void *Cache::calloc(size_t num,size_t size) {
	void *p = alloc(num * size);
	if(p)
		memclear(p,num * size);
	return p;
}

void *Cache::realloc(void *p,size_t size) {
	if(p == NULL)
		return alloc(size);

	ulong *area = (ulong*)p - 2;
	/* if the guard is not ours, perhaps it has been allocated on the fallback-heap */
	if(area[1] != GUARD_MAGIC)
		return KHeap::realloc(p,size);

	assert(area[0] < ARRAY_SIZE(caches));
	size_t objSize = caches[area[0]].objSize;
	if(objSize >= size)
		return p;
	void *res = alloc(size);
	if(res) {
		memcpy(res,p,objSize);
		free(p);
	}
	return res;
}

void Cache::free(void *p) {
	ulong *area = (ulong*)p - 2;
	if(p == NULL)
		return;

#if DEBUG_ALLOC_N_FREE
	if(aafEnabled) {
		lock.down();
		Util::printEventTrace(Log::get(),Util::getKernelStackTrace(),"\n[F] %Px %zu ",
		                      p,caches[area[0]].objSize);
		lock.up();
	}
#endif

	/* if the guard is not ours, perhaps it has been allocated on the fallback-heap */
	if(area[1] != GUARD_MAGIC) {
		KHeap::free(p);
		return;
	}

	/* check whether objSize is within the existing sizes */
	assert(area[0] < ARRAY_SIZE(caches));
	size_t objSize = caches[area[0]].objSize;
	assert(objSize >= caches[0].objSize && objSize <= caches[ARRAY_SIZE(caches) - 1].objSize);
	/* check guard */
	assert(area[(objSize / sizeof(ulong)) + 2] == GUARD_MAGIC);

	/* put on freelist */
	Entry *c = caches + area[0];
	lock.down();
	area[0] = (ulong)c->freeList;
	c->freeList = area;
	c->freeObjs++;
	lock.up();
}

size_t Cache::getOccMem() {
	size_t count = 0;
	for(size_t i = 0; i < ARRAY_SIZE(caches); i++)
		count += BYTES_2_PAGES(caches[i].totalObjs * (caches[i].objSize + sizeof(ulong) * 3));
	return count * PAGE_SIZE;
}

size_t Cache::getUsedMem() {
	size_t count = 0;
	for(size_t i = 0; i < ARRAY_SIZE(caches); i++)
		count += (caches[i].totalObjs - caches[i].freeObjs) * (caches[i].objSize + sizeof(ulong) * 3);
	return count;
}

void Cache::print(OStream &os) {
	size_t total = 0,maxMem = 0;
	for(size_t i = 0; i < ARRAY_SIZE(caches); i++) {
		size_t amount = caches[i].totalObjs * (caches[i].objSize + sizeof(ulong) * 3);
		if(amount > maxMem)
			maxMem = amount;
		total += amount;
	}
	os.writef("Total: %zu bytes\n",total);
	for(size_t i = 0; i < ARRAY_SIZE(caches); i++) {
		size_t mem = caches[i].totalObjs * (caches[i].objSize + sizeof(ulong) * 3);
		os.writef("Cache %zu [size=%zu, total=%zu, free=%zu, pages=%zu]:\n",i,caches[i].objSize,
				caches[i].totalObjs,caches[i].freeObjs,BYTES_2_PAGES(mem));
		printBar(os,mem,maxMem,caches[i].totalObjs,caches[i].freeObjs);
	}
}

void Cache::printBar(OStream &os,size_t mem,size_t maxMem,size_t total,size_t free) {
	size_t memTotal = maxMem == 0 ? 0 : (VID_COLS * mem) / maxMem;
	size_t full = total == 0 ? 0 : (memTotal * (total - free)) / total;
	for(size_t i = 0; i < VID_COLS; i++) {
		if(i < full)
			os.writef("%c",0xDB);
		else if(i < memTotal)
			os.writef("%c",0xB0);
	}
	os.writef("\n");
}

void *Cache::get(Entry *c,size_t i) {
	lock.down();
	if(!c->freeList) {
		size_t pageCount = BYTES_2_PAGES(MIN_OBJ_COUNT * c->objSize);
		size_t bytes = pageCount * PAGE_SIZE;
		size_t totalObjSize = c->objSize + sizeof(ulong) * 3;
		size_t objs = bytes / totalObjSize;
		size_t rem = bytes - objs * totalObjSize;
		ulong *space = (ulong*)KHeap::allocSpace(pageCount);
		if(space == NULL) {
			lock.up();
			return NULL;
		}

		/* if the remaining space is big enough (it won't bring advantages to add dozens e.g. 8
		 * byte large areas to the heap), add it to the fallback-heap */
		if(rem >= HEAP_THRESHOLD)
			KHeap::addMemory((uintptr_t)space + bytes - rem,rem);

		c->totalObjs += objs;
		c->freeObjs += objs;
		for(size_t j = 0; j < objs; j++) {
			space[0] = (ulong)c->freeList;
			c->freeList = space;
			space += (c->objSize / sizeof(ulong)) + 3;
		}
	}

	/* get first from freelist */
	ulong *area = (ulong*)c->freeList;
	c->freeList = (void*)area[0];

	/* store size and put guards in front and behind the area */
	area[0] = i;
	area[1] = GUARD_MAGIC;
	area[(c->objSize / sizeof(ulong)) + 2] = GUARD_MAGIC;
	c->freeObjs--;
	lock.up();
	return area + 2;
}
