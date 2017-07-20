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

#include <arch/x86/ports.h>
#include <esc/util.h>
#include <mem/pagedir.h>
#include <mem/kasan.h>
#include <task/smp.h>
#include <log.h>
#include <util.h>

/* some parts are inspired by Linux's kasan code */

#define RET_ADDR			(uintptr_t)__builtin_return_address(0)
#define SHADOW_SCALE_SHIFT	3
#define SHADOW_SCALE_SIZE 	(1UL << SHADOW_SCALE_SHIFT)
#define SHADOW_MASK			(SHADOW_SCALE_SIZE - 1)
#define MAX_OBJ_SIZE		(1024 * 1024)

bool KASan::_initialized = false;
int KASan::_disabled[MAX_CPUS];

#if defined(KASAN)

A_NOASAN static inline void *mem_to_shadow(const void *addr) {
	uintptr_t iaddr = (uintptr_t)addr - KHEAP_START;
  	return (void*)((iaddr >> SHADOW_SCALE_SHIFT) + SHADOW_AREA);
}

A_NOASAN static inline const void *shadow_to_mem(const void *shadow_addr) {
	uintptr_t iaddr = ((uintptr_t)shadow_addr - SHADOW_AREA) << SHADOW_SCALE_SHIFT;
	return (void*)(iaddr + KHEAP_START);
}

A_NOASAN static inline cpuid_t getCPU() {
	Thread *t = Thread::getRunning();
	return t ? t->getCPU() : 0;
}

// make everything (at least) 16-bytes aligned. e.g., required for fxsave
struct HeapHeader {
	uint64_t size;
	uint64_t magic;
	uintptr_t freetrace[8];
} A_PACKED;

A_NOASAN void KASan::init() {
	PageTables::KAllocator alloc;
	if(PageDir::mapToCur(SHADOW_AREA,SHADOW_AREA_SIZE / PAGE_SIZE,alloc,
		PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0)
		Util::panic("Unable to map shadow memory");

	assert(Cache::getUsedMem() == 0);
	poison_shadow((void*)KHEAP_START,KHEAP_SIZE,POISON_UNALLOC);

	_initialized = true;
}

A_NOASAN static void report_object(HeapHeader *obj) {
	if(obj->magic == KASan::FREE_MAGIC) {
		Log::get().writef("Object %p of %zu bytes was free'd, from here:\n",
			obj + 1,(size_t)obj->size);
		Util::printStackTrace(Log::get(),obj->freetrace);
	}
	else {
		Log::get().writef("Object %p of %zu bytes is still allocated, from here:\n",
			obj + 1,(size_t)obj->size);
		// add potentially missing null-termination
		// simply overwrite the memory; we'll panic anyway
		obj->freetrace[ARRAY_SIZE(obj->freetrace)] = 0;
		Util::printStackTrace(Log::get(),obj->freetrace);
	}
}

A_NOASAN static void report_detail(uintptr_t addr) {
	uint8_t *shadow = (uint8_t*)mem_to_shadow((void*)addr);

	// find prev object
	uint8_t *prev = shadow;
	if(*prev == KASan::POISON_OBJECT_FREED) {
		// we accessed a free'd object. simply walk to its start
		for(size_t i = 0; i < MAX_OBJ_SIZE && *prev != KASan::POISON_OBJECT_START; ++i)
			prev--;
	}
	else if(*prev != KASan::POISON_OBJECT_START) {
		// we accessed unallocated or allocated memory
		size_t i = 0;
		// walk to the end of the prev free'd/allocated object
		for(; i < MAX_OBJ_SIZE && *prev != KASan::POISON_OBJECT_FREED && (*prev & 0x80) != 0; ++i)
			prev--;
		if((*prev & 0x80) == 0) {
			// search beginning of allocated object
			for(; i < MAX_OBJ_SIZE && (*prev & 0x80) == 0; ++i)
				prev--;
			prev++;
		}
		else {
			// search beginning of free'd object
			for(; i < MAX_OBJ_SIZE && *prev != KASan::POISON_OBJECT_START; ++i)
				prev--;
		}
	}

	HeapHeader *prevhead = NULL;
	if((*prev & 0x80) == 0 || *prev == KASan::POISON_OBJECT_START) {
		prevhead = (HeapHeader*)shadow_to_mem(prev) - 1;
		uintptr_t objstart = (uintptr_t)(prevhead + 1);
		// ensure that this is a valid alloc'ed/free'd object
		if(prevhead->magic != KASan::ALLOC_MAGIC && prevhead->magic != KASan::FREE_MAGIC)
			prevhead = NULL;
		// access inside the object?
		else if(addr < objstart + prevhead->size) {
			Log::get().writef("Address %p is inside %p..%p (+%zu)\n",
				addr,objstart,objstart + (size_t)prevhead->size,addr - objstart);
			report_object(prevhead);
			return;
		}
	}

	// find next object
	uint8_t *next = shadow;
	for(size_t i = 0; i < MAX_OBJ_SIZE && *next != KASan::POISON_OBJECT_START && (*next & 0x80) != 0; ++i)
		next++;

	HeapHeader *nexthead = NULL;
	if((*next & 0x80) == 0 || *next == KASan::POISON_OBJECT_START) {
		nexthead = (HeapHeader*)shadow_to_mem(next) - 1;
		if(nexthead->magic != KASan::ALLOC_MAGIC && nexthead->magic != KASan::FREE_MAGIC)
			nexthead = NULL;
	}

	uintptr_t prevstart = (uintptr_t)(prevhead + 1);
	uintptr_t nextstart = (uintptr_t)(nexthead + 1);

	if(prevhead && nexthead) {
		Log::get().writef("Address %p is between %p..%p (+%zu) and %p..%p (-%zu)\n",
			addr,prevstart,prevstart + (size_t)prevhead->size,addr - (prevstart + (size_t)prevhead->size),
			nextstart,nextstart + (size_t)nexthead->size,nextstart - addr);
	}
	else if(prevhead) {
		Log::get().writef("Address %p is behind %p..%p (+%zu)\n",
			addr,prevstart,prevstart + (size_t)prevhead->size,addr - (prevstart + (size_t)prevhead->size));
	}
	else if(nexthead) {
		Log::get().writef("Address %p is before %p..%p (-%zu)\n",
			addr,nextstart,nextstart + (size_t)nexthead->size,nextstart - addr);
	}

	if(prevhead)
		report_object(prevhead);
	if(nexthead)
		report_object(nexthead);
}

A_NOASAN A_NOINLINE void KASan::report(uintptr_t addr,size_t size,bool write,uintptr_t retaddr) {
	_initialized = false;

	Log::get().writef("KASan detected illegal %zu byte %s access to %p @ %p\n",
		size,write ? "write" : "read",addr,retaddr);
	report_detail(addr);

	Util::panic("Stopping here");
}

EXTERN_C void *__wrap_cache_alloc(size_t size);
EXTERN_C void *__real_cache_alloc(size_t size);
EXTERN_C void *__wrap_cache_calloc(size_t num,size_t size);
EXTERN_C void *__real_cache_calloc(size_t num,size_t size);
EXTERN_C void *__wrap_cache_realloc(void *p,size_t size);
EXTERN_C void *__real_cache_realloc(void *p,size_t size);
EXTERN_C void __wrap_cache_free(void *p);
EXTERN_C void __real_cache_free(void *p);

A_NOASAN A_NOINLINE static void store_trace(HeapHeader *head,size_t space) {
	uintptr_t *dst = (uintptr_t*)head->freetrace;
	size_t rem = sizeof(head->freetrace) + (space >= sizeof(uintptr_t) ? space - sizeof(uintptr_t) : 0);
	const uintptr_t *calltrace = Util::getKernelStackTrace();
	if(calltrace) {
		calltrace++; // skip our own function
		while(rem >= sizeof(uintptr_t) && *calltrace != 0) {
			*dst++ = *calltrace++;
			rem -= sizeof(uintptr_t);
		}
	}
	// only null-terminate it, if there is space. that is, if we only use the header, it might be
	// non-null-terminated.
	if(rem > 0)
		*dst = 0;
}

A_NOASAN void *__wrap_cache_alloc(size_t size) {
	// disable KASan during the call, because it might perform a memcpy or similar on the heap data
	// before we marked it as non-poisened.
	KASan::disable(getCPU());
	HeapHeader *res = (HeapHeader*)__real_cache_alloc(size + sizeof(HeapHeader));
	KASan::enable(getCPU());
	if(res) {
		res->magic = KASan::ALLOC_MAGIC;
		res->size = size;
		store_trace(res,0);
		KASan::kheap_alloc(res + 1,size);
		return res + 1;
	}
	return NULL;
}

A_NOASAN void *__wrap_cache_calloc(size_t num,size_t size) {
	void *res = __wrap_cache_alloc(num * size);
	if(res)
		memclear(res,num * size);
	return res;
}

A_NOASAN void *__wrap_cache_realloc(void *p,size_t size) {
	HeapHeader *ptr = (HeapHeader*)p - 1;
	size_t oldsize = p ? ptr->size : 0;

	KASan::disable(getCPU());
	HeapHeader *res = (HeapHeader*)__real_cache_realloc(p ? ptr : NULL,size + sizeof(HeapHeader));
	KASan::enable(getCPU());
	if(res) {
		if(p)
			KASan::kheap_free(ptr + 1,oldsize);
		res->magic = KASan::ALLOC_MAGIC;
		res->size = size;
		store_trace(res,0);
		KASan::kheap_alloc(res + 1,size);
		return res + 1;
	}
	return NULL;
}

static bool quaranEmpty = true;
static size_t quaranWrite = 0;
static size_t quaranRead = 0;
static void *quarantine[64];
static SpinLock quaranLock;

A_NOASAN void __wrap_cache_free(void *p) {
	if(!p)
		return;

	HeapHeader *ptr = (HeapHeader*)p - 1;

	LockGuard<SpinLock> guard(&quaranLock);
	{
		if(!quaranEmpty && quaranWrite == quaranRead) {
			KASan::disable(getCPU());
			__real_cache_free(quarantine[quaranRead]);
			KASan::enable(getCPU());
			quaranRead = (quaranRead + 1) % ARRAY_SIZE(quarantine);
		}

		quarantine[quaranWrite] = ptr;
		quaranWrite = (quaranWrite + 1) % ARRAY_SIZE(quarantine);
		quaranEmpty = false;
	}

	size_t objsize = ptr->size;
	ptr->magic = KASan::FREE_MAGIC;
	KASan::kheap_free(ptr + 1,objsize);

	store_trace(ptr,objsize);
}

A_NOASAN void KASan::kheap_flush() {
	LockGuard<SpinLock> guard(&quaranLock);
	if(quaranEmpty)
		return;

	KASan::disable(getCPU());
	do {
		__real_cache_free(quarantine[quaranRead]);
		quaranRead = (quaranRead + 1) % ARRAY_SIZE(quarantine);
	}
	while(quaranWrite != quaranRead);
	KASan::enable(getCPU());

	quaranEmpty = true;
}

A_NOASAN void KASan::poison_shadow(const void *address,size_t size,uint8_t value) {
	uint8_t *shadow_start,*shadow_end;

	shadow_start = (uint8_t*)mem_to_shadow(address);
	shadow_end = (uint8_t*)mem_to_shadow((uint8_t*)address + size);

	memset(shadow_start,value,shadow_end - shadow_start);
}

A_NOASAN void KASan::unpoison_shadow(const void *address,size_t size) {
	poison_shadow(address,size,0);

	if (size & SHADOW_MASK) {
		uint8_t *shadow = (uint8_t *)mem_to_shadow((uint8_t*)address + size);
		*shadow = size & SHADOW_MASK;
	}
}

A_NOASAN A_NOINLINE bool KASan::mem_is_poisoned_1(uintptr_t addr) {
	int8_t shadow_value = *(int8_t *)mem_to_shadow((void *)addr);

	if(EXPECT_FALSE(shadow_value)) {
		int8_t last_accessible_byte = addr & SHADOW_MASK;
		return EXPECT_FALSE(last_accessible_byte >= shadow_value);
	}

	return false;
}

A_NOASAN A_NOINLINE bool KASan::mem_is_poisoned_2(uintptr_t addr) {
	uint16_t *shadow_addr = (uint16_t*)mem_to_shadow((void*)addr);

	if(EXPECT_FALSE(*shadow_addr)) {
		if(mem_is_poisoned_1(addr + 1))
			return true;

		/*
		 * If single shadow byte covers 2-byte access, we don't
		 * need to do anything more. Otherwise, test the first
		 * shadow byte.
		 */
		if(EXPECT_TRUE(((addr + 1) & SHADOW_MASK) != 0))
			return false;

		return EXPECT_FALSE(*(uint8_t*)shadow_addr);
	}

	return false;
}

A_NOASAN A_NOINLINE bool KASan::mem_is_poisoned_4(uintptr_t addr) {
	uint16_t *shadow_addr = (uint16_t*)mem_to_shadow((void*)addr);

	if(EXPECT_FALSE(*shadow_addr)) {
		if(mem_is_poisoned_1(addr + 3))
			return true;

		/*
		 * If single shadow byte covers 4-byte access, we don't
		 * need to do anything more. Otherwise, test the first
		 * shadow byte.
		 */
		if(EXPECT_TRUE(((addr + 3) & SHADOW_MASK) >= 3))
			return false;

		return EXPECT_FALSE(*(uint8_t*)shadow_addr);
	}

	return false;
}

A_NOASAN A_NOINLINE bool KASan::mem_is_poisoned_8(uintptr_t addr) {
	uint16_t *shadow_addr = (uint16_t*)mem_to_shadow((void*)addr);

	if(EXPECT_FALSE(*shadow_addr)) {
		if(mem_is_poisoned_1(addr + 7))
			return true;

		/*
		 * If single shadow byte covers 8-byte access, we don't
		 * need to do anything more. Otherwise, test the first
		 * shadow byte.
		 */
		if(EXPECT_TRUE(esc::Util::is_aligned(addr,SHADOW_SCALE_SIZE)))
			return false;

		return EXPECT_FALSE(*(uint8_t*)shadow_addr);
	}

	return false;
}

A_NOASAN A_NOINLINE bool KASan::mem_is_poisoned_16(uintptr_t addr) {
	uint32_t *shadow_addr = (uint32_t*)mem_to_shadow((void*)addr);

	if(EXPECT_FALSE(*shadow_addr)) {
		uint16_t shadow_first_bytes = *(uint16_t*)shadow_addr;

		if(EXPECT_FALSE(shadow_first_bytes))
			return true;

		/*
		 * If two shadow bytes covers 16-byte access, we don't
		 * need to do anything more. Otherwise, test the last
		 * shadow byte.
		 */
		if(EXPECT_TRUE(esc::Util::is_aligned(addr,SHADOW_SCALE_SIZE)))
			return false;

		return mem_is_poisoned_1(addr + 15);
	}

	return false;
}

A_NOASAN A_NOINLINE uintptr_t KASan::bytes_is_zero(const uint8_t *start,size_t size) {
	while(size) {
		if(EXPECT_FALSE(*start))
			return (uintptr_t)start;
		start++;
		size--;
	}

	return 0;
}

A_NOASAN A_NOINLINE uintptr_t KASan::mem_is_zero(const uint8_t *start,const uint8_t *end) {
	size_t words;
	uintptr_t ret;
	size_t prefix = (uintptr_t)start % 8;

	if((end - start) <= 16)
		return bytes_is_zero((const uint8_t*)start,(end - start));

	if(prefix) {
		prefix = 8 - prefix;
		ret = bytes_is_zero((const uint8_t*)start,prefix);
		if(EXPECT_FALSE(ret))
			return ret;
		start += prefix;
	}

	words = (end - start) / 8;
	while(words) {
		if(EXPECT_FALSE(*(uint64_t *)start))
			return bytes_is_zero((const uint8_t*)start,8);
		start += 8;
		words--;
	}

	return bytes_is_zero((const uint8_t*)start,(end - start) % 8);
}

A_NOASAN A_NOINLINE bool KASan::mem_is_poisoned_n(uintptr_t addr,size_t size) {
	uintptr_t ret;

	ret = mem_is_zero((uint8_t*)mem_to_shadow((void*)addr),
			(uint8_t*)mem_to_shadow((void*)(addr + size - 1)) + 1);

	if(EXPECT_FALSE(ret)) {
		uintptr_t last_byte = addr + size - 1;
		int8_t *last_shadow = (int8_t *)mem_to_shadow((void*)last_byte);

		if(EXPECT_FALSE(ret != (uintptr_t)last_shadow ||
			((long)(last_byte & SHADOW_MASK) >= *last_shadow)))
			return true;
	}
	return false;
}

A_ALWAYS_INLINE bool KASan::mem_is_poisoned(uintptr_t addr,size_t size) {
	if(__builtin_constant_p(size)) {
		switch (size) {
			case 1:
				return mem_is_poisoned_1(addr);
			case 2:
				return mem_is_poisoned_2(addr);
			case 4:
				return mem_is_poisoned_4(addr);
			case 8:
				return mem_is_poisoned_8(addr);
			case 16:
				return mem_is_poisoned_16(addr);
			default:
				assert(false);
		}
	}

	return mem_is_poisoned_n(addr,size);
}

A_ALWAYS_INLINE void KASan::check_access(uintptr_t addr,size_t size,bool write,uintptr_t retaddr) {
	if(!_initialized)
		return;

	if(addr < KHEAP_START || addr >= KHEAP_START + KHEAP_SIZE)
		return;

	if(_disabled[getCPU()])
		return;

	if(EXPECT_TRUE(!mem_is_poisoned(addr,size)))
		return;

	report(addr,size,write,retaddr);
}


/**
 * Generated by the compiler for load instructions
 */
EXTERN_C void __asan_load1(uintptr_t addr);
EXTERN_C void __asan_load1_noabort(uintptr_t addr);
EXTERN_C void __asan_load2(uintptr_t addr);
EXTERN_C void __asan_load2_noabort(uintptr_t addr);
EXTERN_C void __asan_load4(uintptr_t addr);
EXTERN_C void __asan_load4_noabort(uintptr_t addr);
EXTERN_C void __asan_load8(uintptr_t addr);
EXTERN_C void __asan_load8_noabort(uintptr_t addr);
EXTERN_C void __asan_load16(uintptr_t addr);
EXTERN_C void __asan_load16_noabort(uintptr_t addr);
EXTERN_C void __asan_loadN(uintptr_t addr,size_t size);
EXTERN_C void __asan_loadN_noabort(uintptr_t addr,size_t size);

/**
 * Generated by the compiler for store instructions
 */
EXTERN_C void __asan_store1(uintptr_t addr);
EXTERN_C void __asan_store1_noabort(uintptr_t addr);
EXTERN_C void __asan_store2(uintptr_t addr);
EXTERN_C void __asan_store2_noabort(uintptr_t addr);
EXTERN_C void __asan_store4(uintptr_t addr);
EXTERN_C void __asan_store4_noabort(uintptr_t addr);
EXTERN_C void __asan_store8(uintptr_t addr);
EXTERN_C void __asan_store8_noabort(uintptr_t addr);
EXTERN_C void __asan_store16(uintptr_t addr);
EXTERN_C void __asan_store16_noabort(uintptr_t addr);
EXTERN_C void __asan_storeN(uintptr_t addr,size_t size);
EXTERN_C void __asan_storeN_noabort(uintptr_t addr,size_t size);

/**
 * Generated by the compiler before and after the initialization of global objects
 */
EXTERN_C void __asan_before_dynamic_init(const char *);
EXTERN_C void __asan_after_dynamic_init();

/**
 * Generated by the compiler for functions that do not return
 */
EXTERN_C void __asan_handle_no_return();


#define DEFINE_ASAN_LOAD_STORE(size)								\
	EXTERN_C void __asan_load##size(uintptr_t addr) {				\
		KASan::check_access(addr,size,false,RET_ADDR);				\
	}																\
	EXTERN_C void __asan_load##size##_noabort(uintptr_t addr) {		\
		KASan::check_access(addr,size,false,RET_ADDR);				\
	}																\
	EXTERN_C void __asan_store##size(uintptr_t addr) {				\
		KASan::check_access(addr,size,true,RET_ADDR);				\
	}																\
	EXTERN_C void __asan_store##size##_noabort(uintptr_t addr) {	\
		KASan::check_access(addr,size,true,RET_ADDR);				\
	}

DEFINE_ASAN_LOAD_STORE(1);
DEFINE_ASAN_LOAD_STORE(2);
DEFINE_ASAN_LOAD_STORE(4);
DEFINE_ASAN_LOAD_STORE(8);
DEFINE_ASAN_LOAD_STORE(16);

EXTERN_C void __asan_loadN(uintptr_t addr,size_t size) {
	KASan::check_access(addr,size,false,RET_ADDR);
}
EXTERN_C void __asan_loadN_noabort(uintptr_t addr,size_t size) {
	KASan::check_access(addr,size,false,RET_ADDR);
}

EXTERN_C void __asan_storeN(uintptr_t addr,size_t size) {
	KASan::check_access(addr,size,true,RET_ADDR);
}
EXTERN_C void __asan_storeN_noabort(uintptr_t addr,size_t size) {
	KASan::check_access(addr,size,true,RET_ADDR);
}

EXTERN_C void __asan_before_dynamic_init(const char *) {
}
EXTERN_C void __asan_after_dynamic_init() {
}

EXTERN_C void __asan_handle_no_return() {
}

#endif
