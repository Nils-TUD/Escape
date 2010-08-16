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

#include <esc/common.h>
#include <esc/debug.h>
#include <string.h>
#include <esc/sllist.h>
#include <esc/proc.h>
#include "elf.h"
#include "lookup.h"
#include "loader.h"

static Elf32_Sym *lookup_byNameIntern(sSharedLib *lib,const char *name,u32 hash);
static u32 lookup_getHash(const unsigned char *name);

#ifdef CALLTRACE_PID
static s32 pid = -1;
static s32 depth = 0;
#endif

#ifdef CALLTRACE_PID
u32 lookup_resolve(u32 retAddr,sSharedLib *lib,u32 offset) {
#else
u32 lookup_resolve(sSharedLib *lib,u32 offset) {
#endif
	Elf32_Sym *foundSym;
	Elf32_Rel *rel = (Elf32_Rel*)((u32)lib->jmprel + offset);
	Elf32_Sym *sym = lib->dynsyms + ELF32_R_SYM(rel->r_info);
	u32 value,*addr;
#ifndef CALLTRACE_PID
	DBGDL("Lookup symbol @ %x (%s) in lib %s\n",offset,lib->dynstrtbl + sym->st_name,
			lib->name ? lib->name : "-Main-");
#endif
	foundSym = lookup_byName(NULL,lib->dynstrtbl + sym->st_name,&value);
	if(foundSym == NULL)
		error("Unable to find symbol %s",lib->dynstrtbl + sym->st_name);
	addr = (u32*)(rel->r_offset + lib->loadAddr);
#ifdef CALLTRACE_PID
	if(pid == -1)
		pid = getpid();
	if(pid == CALLTRACE_PID) {
		if(depth < 100) {
			sSLNode *n;
			sSharedLib *calling = (sSharedLib*)sll_get(libs,0);
			for(n = sll_begin(libs); n != NULL; n = n->next) {
				sSharedLib *l = (sSharedLib*)n->data;
				if(retAddr >= l->loadAddr && retAddr < l->loadAddr + l->textSize) {
					calling = l;
					break;
				}
			}
			debugf("%*s\\ %x(%s) -> %s (%x)\n",depth,"",retAddr - calling->loadAddr,
					calling->name ? calling->name : "-Main-",lib->dynstrtbl + sym->st_name,value);
			depth++;
		}
	}
	else
		*addr = value;
#else
	DBGDL("Found: %x, GOT-entry: %x\n",value,addr);
	*addr = value;
#endif
	return value;
}

#ifdef CALLTRACE_PID
void lookup_tracePop(void);
void lookup_tracePop(void) {
	if(pid == CALLTRACE_PID) {
		depth--;
		debugf("%*s/\n",depth,"");
	}
}
#endif

Elf32_Sym *lookup_byName(sSharedLib *skip,const char *name,u32 *value) {
	sSLNode *n;
	Elf32_Sym *s;
	u32 hash = lookup_getHash((const unsigned char*)name);
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
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

Elf32_Sym *lookup_byNameIn(sSharedLib *lib,const char *name,u32 *value) {
	u32 hash = lookup_getHash((const unsigned char*)name);
	Elf32_Sym *sym = lookup_byNameIntern(lib,name,hash);
	if(sym)
		*value = sym->st_value + lib->loadAddr;
	return sym;
}

static Elf32_Sym *lookup_byNameIntern(sSharedLib *lib,const char *name,u32 hash) {
	Elf32_Word nhash;
	Elf32_Word symindex;
	Elf32_Sym *sym;
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

static u32 lookup_getHash(const unsigned char *name) {
	u32 h = 0,g;
	while(*name) {
		h = (h << 4) + *name++;
		if((g = (h & 0xf0000000)))
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}
