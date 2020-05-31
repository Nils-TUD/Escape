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

#include <sys/boot.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/elf.h>
#include <string.h>

#include "serial.h"

enum {
	MAX_MMOD_ENTRIES	= 8,
	MAX_MMAP_ENTRIES	= 16,
};

typedef struct E820Entry {
	uint64_t baseAddr;
	uint64_t length;
	uint32_t type;
} A_PACKED E820Entry;

#ifdef NDEBUG
A_ALIGNED(4096) static const uint8_t dump_kernel[] = {
#	include "../../build/x86_64-release/dump/escape.dump"
};
A_ALIGNED(4096) static const uint8_t dump_initloader[] = {
	#include "../../build/x86_64-release/dump/initloader.dump"
};
A_ALIGNED(4096) static const uint8_t dump_pci[] = {
	#include "../../build/x86_64-release/dump/pci.dump"
};
A_ALIGNED(4096) static const uint8_t dump_ata[] = {
	#include "../../build/x86_64-release/dump/ata.dump"
};
A_ALIGNED(4096) static const uint8_t dump_ext2[] = {
	#include "../../build/x86_64-release/dump/ext2.dump"
};
#else
A_ALIGNED(4096) static const uint8_t dump_kernel[] = {
#	include "../../build/x86_64-debug/dump/escape.dump"
};
A_ALIGNED(4096) static const uint8_t dump_initloader[] = {
	#include "../../build/x86_64-debug/dump/initloader.dump"
};
A_ALIGNED(4096) static const uint8_t dump_pci[] = {
	#include "../../build/x86_64-debug/dump/pci.dump"
};
A_ALIGNED(4096) static const uint8_t dump_ata[] = {
	#include "../../build/x86_64-debug/dump/ata.dump"
};
A_ALIGNED(4096) static const uint8_t dump_ext2[] = {
	#include "../../build/x86_64-debug/dump/ext2.dump"
};
#endif

struct Module {
	const char cmdline[64];
	const uint8_t *dump;
	size_t size;
};

__attribute__((section(".mb"))) static Module mods[] = {
	{"/sbin/initloader",								dump_initloader,	sizeof(dump_initloader)},
	{"/sbin/pci /dev/pci",								dump_pci, 			sizeof(dump_pci)},
	{"/sbin/ata /sys/dev/ata nodma noirq",				dump_ata, 			sizeof(dump_ata)},
	{"/sbin/ext2 /dev/ext2-hda1 /dev/hda1",				dump_ext2, 			sizeof(dump_ext2)},
};

__attribute__((section(".mb"))) static BootModule mmod[MAX_MMOD_ENTRIES];
__attribute__((section(".mb"))) static BootMemMap mmap[MAX_MMAP_ENTRIES];
__attribute__((section(".mb"))) MultiBootInfo mbi;

static uintptr_t load(const void *elf) {
	const sElfEHeader *header = (const sElfEHeader*)elf;

	// check magic
	if(memcmp(header->e_ident,ELFMAG,4) != 0) {
		debugf("Invalid ELF magic!\n");
		return 0;
	}

	// load the LOAD segments.
	const sElfPHeader *pheader = (const sElfPHeader*)((char*)elf + header->e_phoff);
	for(int j = 0; j < header->e_phnum; j++) {
		if(pheader->p_type == PT_LOAD) {
			memcpy((void*)pheader->p_paddr,(const char*)elf + pheader->p_offset,pheader->p_filesz);
			memclear((void*)(pheader->p_paddr + pheader->p_filesz),pheader->p_memsz - pheader->p_filesz);
		}

		// to next
		pheader = (const sElfPHeader*)((char*)pheader + header->e_phentsize);
	}

	return header->e_entry;
}

extern "C" uintptr_t loader(const char *cmdline,size_t mapCount,const E820Entry *map);

uintptr_t loader(const char *cmdline,size_t mapCount,const E820Entry *map) {
	initSerial();

	debugf("Loading kernel...");
	uintptr_t entry = load(dump_kernel);
	if(!entry) {
		while(1)
			;
	}
	debugf("done\n");

	// prepare multiboot info
	mbi.flags = (1 << 2) | (1 << 3) | (1 << 6);	// cmdline, modules and mmap
	mbi.cmdLine = (uintptr_t)cmdline;

	mbi.modsCount = ARRAY_SIZE(mods);
	mbi.modsAddr = (uintptr_t)mmod;
	for(size_t i = 0; i < ARRAY_SIZE(mods); ++i) {
		mmod[i].modStart = (uintptr_t)mods[i].dump;
		mmod[i].modEnd = (uintptr_t)mods[i].dump + mods[i].size;
		mmod[i].name = (uintptr_t)mods[i].cmdline;
	}

	mbi.mmapLength = mapCount * sizeof(BootMemMap);
	mbi.mmapAddr = (uintptr_t)mmap;
	for(size_t i = 0; i < mapCount; ++i) {
		mmap[i].baseAddr = map[i].baseAddr;
		mmap[i].length = map[i].length;
		mmap[i].type = map[i].type == 1 ? 1 : 2;
		mmap[i].size = 20;
	}

	return entry;
}
