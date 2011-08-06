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

#include <esc/common.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <string.h>
#include "reloc.h"
#include "loader.h"
#include "setup.h"
#include "lookup.h"

#define RELOC_LOCK	0x6612AACC

static void load_relocLib(sSharedLib *l);
static void load_adjustCopyGotEntry(const char *name,uintptr_t copyAddr);

void load_reloc(void) {
	sSLNode *n;
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		load_relocLib(l);
	}
}

static void load_relocLib(sSharedLib *l) {
	Elf32_Addr *got;
	Elf32_Rel *rel;
	sSLNode *n;

	/* already relocated? */
	if(l->relocated)
		return;

	/* first go through the dependencies; this may be required for the R_386_COPY-relocation */
	for(n = sll_begin(l->deps); n != NULL; n = n->next) {
		sSharedLib *dl = (sSharedLib*)n->data;
		load_relocLib(dl);
	}

	/* we can't relocate the same region multiple times in parallel, because the region-protection
	 * is changed for all processes that use that region */
	lockg(RELOC_LOCK,LOCK_EXCLUSIVE);
	/* make text writable because we may have to change something in it */
	if(setRegProt(l->loadAddr ? l->loadAddr : TEXT_BEGIN,PROT_READ | PROT_WRITE) < 0)
		load_error("Unable to make text (@ %p) of %s writable",l->loadAddr,l->name);

	DBGDL("Relocating stuff of %s (loaded @ %p)\n",l->name,l->loadAddr ? l->loadAddr : TEXT_BEGIN);

	rel = (Elf32_Rel*)load_getDyn(l->dyn,DT_REL);
	if(rel) {
		size_t x;
		size_t relCount = load_getDyn(l->dyn,DT_RELSZ);
		relCount /= sizeof(Elf32_Rel);
		rel = (Elf32_Rel*)((uintptr_t)rel + l->loadAddr);
		for(x = 0; x < relCount; x++) {
			uint relType = ELF32_R_TYPE(rel[x].r_info);
			if(relType == R_386_NONE)
				continue;
			if(relType == R_386_GLOB_DAT) {
				uint symIndex = ELF32_R_SYM(rel[x].r_info);
				uintptr_t *ptr = (uintptr_t*)(rel[x].r_offset + l->loadAddr);
				Elf32_Sym *sym = l->dynsyms + symIndex;
				/* if the symbol-value is 0, it seems that we have to lookup the symbol now and
				 * store that value instead. TODO I'm not sure if thats correct */
				if(sym->st_value == 0) {
					uintptr_t value;
					if(!lookup_byName(l,l->dynstrtbl + sym->st_name,&value))
						DBGDL("Unable to find symbol %s\n",l->dynstrtbl + sym->st_name);
					else
						*ptr = value;
				}
				else
					*ptr = sym->st_value + l->loadAddr;
				DBGDL("Rel (GLOB_DAT) off=%p orgoff=%p reloc=%p orgval=%p\n",
						rel[x].r_offset + l->loadAddr,rel[x].r_offset,*ptr,sym->st_value);
			}
			else if(relType == R_386_COPY) {
				uintptr_t value;
				uint symIndex = ELF32_R_SYM(rel[x].r_info);
				const char *name = l->dynstrtbl + l->dynsyms[symIndex].st_name;
				Elf32_Sym *foundSym = lookup_byName(l,name,&value);
				if(foundSym == NULL)
					load_error("Unable to find symbol %s",name);
				memcpy((void*)(rel[x].r_offset),(void*)value,foundSym->st_size);
				/* set the GOT-Entry in the library of the symbol to the address we've copied
				 * the value to. TODO I'm not sure if that's the intended way... */
				load_adjustCopyGotEntry(name,rel[x].r_offset);
				DBGDL("Rel (COPY) off=%p sym=%s addr=%p val=%p\n",rel[x].r_offset,name,
						value,*(uintptr_t*)value);
			}
			else if(relType == R_386_RELATIVE) {
				uintptr_t *ptr = (uintptr_t*)(rel[x].r_offset + l->loadAddr);
				*ptr += l->loadAddr;
				DBGDL("Rel (RELATIVE) off=%p orgoff=%p reloc=%p\n",
						rel[x].r_offset + l->loadAddr,rel[x].r_offset,*ptr);
			}
			else if(relType == R_386_32) {
				uintptr_t *ptr = (uintptr_t*)(rel[x].r_offset + l->loadAddr);
				Elf32_Sym *sym = l->dynsyms + ELF32_R_SYM(rel[x].r_info);
				/* TODO well, again I can't explain why this is needed. If I understand
				 * the code in glibc/sysdeps/i386/dl-machine.h correctly, they just do a
				 * *ptr += sym->st_value + l->loadAddr. But resolving the symbol when the
				 * address is 0 seems to be the only way to get exception-throwing in c++
				 * working. Because otherwise the typeinfo-stuff doesn't get relocated
				 * correctly. And of course the elf-format-specification does not tell
				 * anything about this.. :D
				 */
				if(sym->st_value == 0) {
					uintptr_t value;
					if(!lookup_byName(l,l->dynstrtbl + sym->st_name,&value))
						load_error("Unable to find symbol %s",l->dynstrtbl + sym->st_name);
					*ptr += value;
				}
				else
				/*if(sym->st_value != 0)*/
					*ptr += sym->st_value + l->loadAddr;
				DBGDL("Rel (32) off=%p orgoff=%p symval=%p reloc=%p\n",
						rel[x].r_offset + l->loadAddr,rel[x].r_offset,sym->st_value,*ptr);
			}
			else if(relType == R_386_PC32) {
				uintptr_t *ptr = (uintptr_t*)(rel[x].r_offset + l->loadAddr);
				Elf32_Sym *sym = l->dynsyms + ELF32_R_SYM(rel[x].r_info);
				DBGDL("Rel (PC32) off=%p orgoff=%p symval=%p org=%p reloc=%p\n",
						rel[x].r_offset + l->loadAddr,rel[x].r_offset,sym->st_value,*ptr,
						sym->st_value - rel[x].r_offset - 4);
				*ptr = sym->st_value - rel[x].r_offset - 4;
			}
			else
				load_error("In library %s: Unknown relocation: off=%p info=%p\n",
						l->name,rel[x].r_offset,rel[x].r_info);
		}
	}

	/* make text non-writable again */
	if(setRegProt(l->loadAddr ? l->loadAddr : TEXT_BEGIN,PROT_READ) < 0) {
		load_error("Unable to make text (@ %x) of %s non-writable",
				l->loadAddr ? l->loadAddr : TEXT_BEGIN,l->name);
	}
	unlockg(RELOC_LOCK);

	/* adjust addresses in PLT-jumps */
	if(l->jmprel) {
		size_t x,jmpCount = load_getDyn(l->dyn,DT_PLTRELSZ);
		jmpCount /= sizeof(Elf32_Rel);
		for(x = 0; x < jmpCount; x++) {
			uintptr_t *addr = (uintptr_t*)(l->jmprel[x].r_offset + l->loadAddr);
			/* if the address is 0 (should be the next instruction at the beginning),
			 * do the symbol-lookup immediatly.
			 * TODO i can't imagine that this is the intended way. why is the address 0 in
			 * this case??? (instead of the offset from the beginning of the shared lib,
			 * so that we can simply add the loadAddr) */
			if(LD_BIND_NOW || *addr == 0) {
				uintptr_t value;
				uint symIndex = ELF32_R_SYM(l->jmprel[x].r_info);
				const char *name = l->dynstrtbl + l->dynsyms[symIndex].st_name;
				Elf32_Sym *foundSym = lookup_byName(l,name,&value);
				/* TODO there must be a better way... */
				/*if(!foundSym && !LD_BIND_NOW)
					load_error("Unable to find symbol %s",name);
				else */if(foundSym)
					*addr = value;
				else {
					foundSym = lookup_byName(NULL,name,&value);
					if(!foundSym)
						load_error("Unable to find symbol %s",name);
					*addr = value;
				}
				DBGDL("JmpRel off=%p addr=%p reloc=%p (%s)\n",l->jmprel[x].r_offset,addr,value,name);
			}
			else {
				*addr += l->loadAddr;
				DBGDL("JmpRel off=%p addr=%p reloc=%p\n",l->jmprel[x].r_offset,addr,*addr);
			}
		}
	}

	/* store pointer to library and lookup-function into GOT */
	got = (Elf32_Addr*)load_getDyn(l->dyn,DT_PLTGOT);
	DBGDL("GOT-Address of %s: %p\n",l->name,got);
	if(got) {
		got = (Elf32_Addr*)((uintptr_t)got + l->loadAddr);
		got[1] = (Elf32_Addr)l;
		got[2] = (Elf32_Addr)&lookup_resolveStart;
	}

	l->relocated = true;
	/* no longer needed */
	close(l->fd);
}

static void load_adjustCopyGotEntry(const char *name,uintptr_t copyAddr) {
	sSLNode *n;
	uintptr_t address;
	/* go through all libraries and copy the address of the object (copy-relocated) to
	 * the corresponding GOT-entry. this ensures that all DSO's use the same object */
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		/* don't do that in the executable */
		if(!l->isDSO)
			continue;

		if(lookup_byNameIn(l,name,&address)) {
			/* TODO we should store DT_REL and DT_RELSZ */
			Elf32_Rel *rel = (Elf32_Rel*)load_getDyn(l->dyn,DT_REL);
			if(rel) {
				size_t x,relCount = load_getDyn(l->dyn,DT_RELSZ);
				relCount /= sizeof(Elf32_Rel);
				rel = (Elf32_Rel*)((uintptr_t)rel + l->loadAddr);
				for(x = 0; x < relCount; x++) {
					if(ELF32_R_TYPE(rel[x].r_info) == R_386_GLOB_DAT) {
						uint symIndex = ELF32_R_SYM(rel[x].r_info);
						if(l->dynsyms[symIndex].st_value + l->loadAddr == address) {
							uintptr_t *ptr = (uintptr_t*)(rel[x].r_offset + l->loadAddr);
							*ptr = copyAddr;
							break;
						}
					}
				}
			}
		}
	}
}
