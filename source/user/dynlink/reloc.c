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
#include <sys/io.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <string.h>

#include "loader.h"
#include "lookup.h"
#include "reloc.h"
#include "setup.h"

static void load_relocLib(sSharedLib *l);
static void load_relocDyn(sSharedLib *l,void *rel,size_t size,uint type);
static void load_adjustCopyGotEntry(const char *name,uintptr_t copyAddr);

#if DEBUG_LOADER
static const char *load_getRelName(int type) {
	switch(type) {
		case R_GLOB_DAT: return "GLOB_DAT";
		case R_COPY: return "COPY";
		case R_RELATIVE: return "REL";
		case R_32: return "32";
		case R_64: return "64";
		case R_PC32: return "PC32";
		case R_JUMP_SLOT: return "JUMP_SLOT";
		default: return "Unknown";
	}
}
#endif

void load_reloc(void) {
	for(sSharedLib *l = libs; l != NULL; l = l->next)
		load_relocLib(l);
}

static void load_relocLib(sSharedLib *l) {
	ElfAddr *got;
	sElfRel *rel;

	/* already relocated? */
	if(l->relocated)
		return;

	/* first go through the dependencies; this may be required for the R_386_COPY-relocation */
	for(sDep *dl = l->deps; dl != NULL; dl = dl->next)
		load_relocLib(dl->lib);

	if(load_hasDyn(l->dyn,DT_TEXTREL))
		load_error("Unable to reloc library %s: requires a writable text segment\n",l->name);

	DBGDL("Relocating %s (loaded @ %p)\n",l->name,l->loadAddr ? l->loadAddr : l->textAddr);

	rel = (sElfRel*)load_getDyn(l->dyn,DT_REL);
	if(rel)
		load_relocDyn(l,rel,load_getDyn(l->dyn,DT_RELSZ),DT_REL);
	rel = (sElfRel*)load_getDyn(l->dyn,DT_RELA);
	if(rel)
		load_relocDyn(l,rel,load_getDyn(l->dyn,DT_RELASZ),DT_RELA);

	/* adjust addresses in PLT-jumps */
	if(l->jmprel) {
		load_relocDyn(l,(void*)((uintptr_t)l->jmprel - l->loadAddr),
			load_getDyn(l->dyn,DT_PLTRELSZ),l->jmprelType);
	}

	/* store pointer to library and lookup-function into GOT */
	got = (ElfAddr*)load_getDyn(l->dyn,DT_PLTGOT);
	DBGDL("GOT-Address of %s: %p\n",l->name,got);
	if(got) {
		got = (ElfAddr*)((uintptr_t)got + l->loadAddr);
		got[1] = (ElfAddr)l;
		got[2] = (ElfAddr)&lookup_resolveStart;
	}

	l->relocated = true;
	/* no longer needed */
	close(l->fd);
}

static void load_relocDyn(sSharedLib *l,void *entries,size_t size,uint type) {
	sElfRel *rel = (sElfRel*)((uintptr_t)entries + l->loadAddr);
	sElfRela *rela = (sElfRela*)((uintptr_t)entries + l->loadAddr);
	size_t count = type == DT_REL ? size / sizeof(sElfRel) : size / sizeof(sElfRela);
	for(size_t x = 0; x < count; x++) {
		ulong info,offset,addend;
		if(type == DT_REL) {
			info = rel[x].r_info;
			offset = rel[x].r_offset;
			addend = 0;
		}
		else {
			info = rela[x].r_info;
			offset = rela[x].r_offset;
			addend = rela[x].r_addend;
		}

		int rtype = ELF_R_TYPE(info);
		if(rtype == R_NONE)
			continue;

		size_t symIndex = ELF_R_SYM(info);
		sElfSym *sym = l->dynsyms + symIndex;
		const char *symname = l->dynstrtbl + sym->st_name;
		uintptr_t value = sym->st_value;
		uintptr_t *ptr = (uintptr_t*)(offset + l->loadAddr);

		if(rtype == R_JUMP_SLOT) {
			value = *ptr;
			if(*ptr == 0 || LD_BIND_NOW) {
				if(!lookup_byName(l,symname,&value)) {
					if(!lookup_byName(NULL,symname,&value))
						load_error("Unable to find symbol '%s'\n",symname);
				}
				value -= l->loadAddr;
			}
		}
		/* if the symbol-value is 0, it seems that we have to lookup the symbol now and
		 * store that value instead. TODO I'm not sure if thats correct */
		else if(rtype != R_RELATIVE && (rtype == R_COPY || value == 0)) {
			if(!lookup_byName(l,symname,&value))
				load_error("Unable to find symbol '%s'\n",symname);
			// we'll add that again
			value -= l->loadAddr;
		}

		switch(rtype) {
			case R_COPY:
				memcpy((void*)offset,(void*)(value + l->loadAddr),sym->st_size);
				/* set the GOT-Entry in the library of the symbol to the address we've copied
				 * the value to. TODO I'm not sure if that's the intended way... */
				load_adjustCopyGotEntry(symname,offset);
				break;

			default:
				if(!perform_reloc(l,rtype,offset,ptr,value,addend)) {
					load_error("In library %s: Unknown relocation: off=%p info=%p type=%d addend=%x\n",
						l->name,offset,info,ELF_R_TYPE(info),addend);
				}
				break;
		}

		DBGDL("Rel (%s) off=%p reloc=%p value=%p symbol=%s\n",
		 	load_getRelName(rtype),offset,*ptr,value,symname);
	}
}

static void load_adjustCopyGotEntry(const char *name,uintptr_t copyAddr) {
	uintptr_t address;
	/* go through all libraries and copy the address of the object (copy-relocated) to
	 * the corresponding GOT-entry. this ensures that all DSO's use the same object */
	for(sSharedLib *l = libs; l != NULL; l = l->next) {
		/* don't do that in the executable */
		if(!l->isDSO)
			continue;

		if(lookup_byNameIn(l,name,&address)) {
			/* TODO we should store DT_REL and DT_RELSZ */
			sElfRel *rel = (sElfRel*)load_getDyn(l->dyn,DT_REL);
			if(rel) {
				size_t x,relCount = load_getDyn(l->dyn,DT_RELSZ);
				relCount /= sizeof(sElfRel);
				rel = (sElfRel*)((uintptr_t)rel + l->loadAddr);
				for(x = 0; x < relCount; x++) {
					if(ELF_R_TYPE(rel[x].r_info) == R_GLOB_DAT) {
						uint symIndex = ELF_R_SYM(rel[x].r_info);
						if(l->dynsyms[symIndex].st_value + l->loadAddr == address) {
							uintptr_t *ptr = (uintptr_t*)(rel[x].r_offset + l->loadAddr);
							*ptr = copyAddr;
							break;
						}
					}
				}
			}

			sElfRela *rela = (sElfRela*)load_getDyn(l->dyn,DT_RELA);
			if(rela) {
				size_t x,relCount = load_getDyn(l->dyn,DT_RELASZ);
				relCount /= sizeof(sElfRela);
				rela = (sElfRela*)((uintptr_t)rela + l->loadAddr);
				for(x = 0; x < relCount; x++) {
					if(ELF_R_TYPE(rela[x].r_info) == R_GLOB_DAT) {
						uint symIndex = ELF_R_SYM(rela[x].r_info);
						if(l->dynsyms[symIndex].st_value + l->loadAddr == address) {
							uintptr_t *ptr = (uintptr_t*)(rela[x].r_offset + l->loadAddr);
							*ptr = copyAddr;
							break;
						}
					}
				}
			}
		}
	}
}
