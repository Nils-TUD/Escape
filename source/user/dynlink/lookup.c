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
#include <sys/elf.h>
#include <sys/debug.h>
#include <sys/proc.h>
#include <string.h>
#include <stdlib.h>

#include "lookup.h"
#include "loader.h"

static sElfSym *lookup_byNameIntern(sSharedLib *lib,const char *name,uint32_t hash);
static uint32_t lookup_getHash(const uint8_t *name);

#if defined(CALLTRACE_PID)
static int pid = -1;
static int depth = 0;
#endif

#if defined(__x86_64__)
uintptr_t lookup_resolve(sSharedLib *lib,size_t off) {
#elif defined(CALLTRACE_PID)
A_REGPARM(0) uintptr_t lookup_resolve(A_UNUSED SavedRegs regs,uintptr_t retAddr,sSharedLib *lib,size_t off) {
#else
A_REGPARM(0) uintptr_t lookup_resolve(A_UNUSED SavedRegs regs,sSharedLib *lib,size_t off) {
#endif
	sElfSym *foundSym;
	ulong info,offset;
	if(lib->jmprelType == DT_REL) {
		sElfRel *rel = (sElfRel*)((uintptr_t)lib->jmprel + off);
		info = rel->r_info;
		offset = rel->r_offset;
	}
	else {
		// TODO it's only the index on x86_64?
		sElfRela *rel = (sElfRela*)lib->jmprel + off;
		info = rel->r_info;
		offset = rel->r_offset;
	}

	sElfSym *sym = lib->dynsyms + ELF_R_SYM(info);
	uintptr_t value = 0,*addr;
	foundSym = lookup_byName(NULL,lib->dynstrtbl + sym->st_name,&value);
	if(foundSym == NULL)
		error("Unable to find symbol %s",lib->dynstrtbl + sym->st_name);
	addr = (uintptr_t*)(offset + lib->loadAddr);

#if defined(CALLTRACE_PID)
	pid = getpid();
	if(pid == CALLTRACE_PID) {
		if(depth < 100) {
			sSharedLib *calling = libs;
			for(sSharedLib *l = libs; l != NULL; l = l->next) {
				if(retAddr >= l->loadAddr && retAddr < l->loadAddr + l->textSize) {
					calling = l;
					break;
				}
			}
			debugf("%*s\\ %x(%s) -> %s (%x)\n",depth,"",retAddr - calling->loadAddr,
					calling->name,lib->dynstrtbl + sym->st_name,value);
			depth++;
		}
	}
	else
		*addr = value;
#else
	DBGDL("Found: %s @ %p (off: %p) in %s (GOT: 0x%x)\n",
		lib->dynstrtbl + sym->st_name,value,value - lib->loadAddr,lib->name,off);
	*addr = value;
#endif

	return value;
}

#if defined(CALLTRACE_PID)
int getpid_wrapper(void);
void lookup_tracePop(void);

int getpid_wrapper(void) {
	return getpid();
}

void lookup_tracePop(void) {
	if(pid == CALLTRACE_PID) {
		depth--;
		debugf("%*s/\n",depth,"");
	}
}
#endif

sElfSym *lookup_byName(sSharedLib *skip,const char *name,uintptr_t *value) {
	sElfSym *s;
	uint32_t hash = lookup_getHash((const uint8_t*)name);
	for(sSharedLib *l = libs; l != NULL; l = l->next) {
		if(l == skip)
			continue;
		s = lookup_byNameIntern(l,name,hash);
		if(s) {
			*value = s->st_value + l->loadAddr;
			return s;
		}
	}
	return NULL;
}

sElfSym *lookup_byNameIn(sSharedLib *lib,const char *name,uintptr_t *value) {
	uint32_t hash = lookup_getHash((const uint8_t*)name);
	sElfSym *sym = lookup_byNameIntern(lib,name,hash);
	if(sym)
		*value = sym->st_value + lib->loadAddr;
	return sym;
}

static sElfSym *lookup_byNameIntern(sSharedLib *lib,const char *name,uint32_t hash) {
	ElfWord nhash;
	ElfWord symindex;
	sElfSym *sym;
	if(lib->hashTbl == NULL || (nhash = lib->hashTbl[0]) == 0)
		return NULL;
	symindex = lib->hashTbl[(hash % nhash) + 2];
	while(symindex != STN_UNDEF) {
		sym = lib->dynsyms + symindex;
		if(sym->st_shndx != STN_UNDEF && strcmp(name,lib->dynstrtbl + sym->st_name) == 0)
			return sym;
		symindex = lib->hashTbl[2 + nhash + symindex];
	}
	return NULL;
}

static uint32_t lookup_getHash(const uint8_t *name) {
	uint32_t h = 0,g;
	while(*name) {
		h = (h << 4) + *name++;
		if((g = (h & 0xf0000000)))
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}
