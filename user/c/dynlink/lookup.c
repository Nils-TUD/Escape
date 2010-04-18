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
#include <string.h>
#include <sllist.h>
#include "elf.h"
#include "lookup.h"
#include "loader.h"

static u32 lookup_getHash(const unsigned char *name);
static Elf32_Sym *lookup_byNameIn(sSharedLib *lib,const char *name,u32 hash);

u32 lookup_resolve(sSharedLib *lib,u32 offset) {
	Elf32_Sym *foundSym;
	Elf32_Rel *rel = (Elf32_Rel*)((u32)lib->jmprel + offset);
	Elf32_Sym *sym = lib->symbols + ELF32_M_SYM(rel->r_info);
	u32 value,*addr;
	foundSym = lookup_byName(NULL,lib->strtbl + sym->st_name,&value);
	if(foundSym == NULL)
		error("Unable to find symbol %s",lib->strtbl + sym->st_name);
	addr = (u32*)(rel->r_offset + lib->loadAddr);
	*addr = value;
	return value;
}

Elf32_Sym *lookup_byName(sSharedLib *skip,const char *name,u32 *value) {
	sSLNode *n;
	Elf32_Sym *s;
	u32 hash = lookup_getHash((const unsigned char*)name);
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		if(l == skip)
			continue;
		s = lookup_byNameIn(l,name,hash);
		if(s) {
			*value = s->st_value + l->loadAddr;
			return s;
		}
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

static Elf32_Sym *lookup_byNameIn(sSharedLib *lib,const char *name,u32 hash) {
	Elf32_Word nhash;
	Elf32_Word symindex;
	Elf32_Sym *sym;
	if(lib->hashTbl == NULL || (nhash = lib->hashTbl[0]) == 0)
		return NULL;
	symindex = lib->hashTbl[(hash % nhash) + 2];
	while(symindex != STN_UNDEF) {
		sym = lib->symbols + symindex;
		if(sym->st_size != 0 && strcmp(name,lib->strtbl + sym->st_name) == 0)
			return sym;
		symindex = lib->hashTbl[2 + nhash + symindex];
	}
	return NULL;
}
