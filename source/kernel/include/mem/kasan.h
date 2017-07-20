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

#pragma once

#include <sys/common.h>
#include <esc/util.h>
#include <assert.h>

class KASan {
	KASan() = delete;

public:
	static const size_t MAX_CPUS				= 64;
	static const uint8_t POISON_UNALLOC			= 0xFF;
	static const uint8_t POISON_OBJECT_FREED	= 0xFE;
	static const uint8_t POISON_OBJECT_START	= 0xFD;
	static const uint64_t ALLOC_MAGIC			= 0xDEADBEEFDEADBEEF;
	static const uint64_t FREE_MAGIC			= 0xCAFEBABECAFEBABE;

	static void init();

	static void kheap_alloc(const void *address,size_t size) {
		unpoison_shadow(address,size);
	}
	static void kheap_free(const void *address,size_t size) {
		poison_shadow(address,esc::Util::min(size,static_cast<size_t>(8)),POISON_OBJECT_START);
		if(size >= 8)
			poison_shadow((uint8_t*)address + 8,size - 8,POISON_OBJECT_FREED);
	}

	static void kheap_flush();

	A_NOASAN static void enable(cpuid_t cpu) {
		_disabled[cpu]--;
	}
	A_NOASAN static void disable(cpuid_t cpu) {
		_disabled[cpu]++;
	}

	static void check_access(uintptr_t addr,size_t size,bool write,uintptr_t retaddr);

private:
	static void poison_shadow(const void *address,size_t size,uint8_t value);
	static void unpoison_shadow(const void *address,size_t size);

	static bool mem_is_poisoned_1(uintptr_t addr);
	static bool mem_is_poisoned_2(uintptr_t addr);
	static bool mem_is_poisoned_4(uintptr_t addr);
	static bool mem_is_poisoned_8(uintptr_t addr);
	static bool mem_is_poisoned_16(uintptr_t addr);
	static bool mem_is_poisoned_n(uintptr_t addr,size_t size);

	static unsigned long bytes_is_zero(const uint8_t *start,size_t size);
	static unsigned long mem_is_zero(const uint8_t *start,const uint8_t *end);

	static bool mem_is_poisoned(uintptr_t addr,size_t size);
	static void report(uintptr_t addr,size_t size,bool write,uintptr_t retaddr);

	static bool _initialized;
	static int _disabled[];
};

#if !defined(KASAN)
inline void KASan::init() {
}
inline void KASan::kheap_flush() {
}
inline void KASan::check_access(uintptr_t,size_t,bool,uintptr_t) {
}
#endif
