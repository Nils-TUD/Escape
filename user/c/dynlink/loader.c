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
#include <esc/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <sllist.h>
#include <string.h>
#include "elf.h"
#include "lookup.h"
#include "loader.h"

typedef void (*fConstr)(void);

static u32 load_addSegments(void);
static void load_init(void);
static void load_initLib(sSharedLib *l);
static void load_reloc(void);
static void load_relocLib(sSharedLib *l);
static void load_adjustCopyGotEntry(const char *name,u32 copyAddr);
static void load_library(sSharedLib *dst);
static void load_doLoad(tFD binFd,sSharedLib *dst);
static sSharedLib *load_addLib(sSharedLib *lib);
static u32 load_getDyn(Elf32_Dyn *dyn,Elf32_Sword tag);
static void load_read(tFD binFd,u32 offset,void *buffer,u32 count);
static u32 load_addSeg(tFD binFd,sBinDesc *bindesc,Elf32_Phdr *pheader,u32 loadSegNo,bool isLib);

sSLList *libs = NULL;

u32 load_setupProg(tFD binFd) {
	u32 entryPoint;
	libs = sll_create();
	if(!libs)
		error("Not enough mem!");

	/* create entry for program */
	sSharedLib *main = (sSharedLib*)malloc(sizeof(sSharedLib));
	if(!main)
		error("Not enough mem!");
	main->relocated = false;
	main->initialized = false;
	main->strtbl = NULL;
	main->dyn = NULL;
	main->name = NULL;
	main->deps = sll_create();
	if(!main->deps || !sll_append(libs,main))
		error("Not enough mem!");

	/* load program including shared libraries into linked list */
	load_doLoad(binFd,main);

#if DEBUG_LOADER
	sSLNode *n,*m;
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		DBGDL("Loaded %s with deps:\n",l->name ? l->name : "-Main-");
		for(m = sll_begin(l->deps); m != NULL; m = m->next) {
			sSharedLib *dl = (sSharedLib*)m->data;
			DBGDL(" %s",dl->name);
		}
		DBGDL("\n");
	}
#endif

	/* load segments into memory */
	entryPoint = load_addSegments();

	/* relocate everything we need so that the program can start */
	load_reloc();

	/* call global constructors */
	load_init();

	return entryPoint;
}

static u32 load_addSegments(void) {
	sSLNode *n;
	u32 entryPoint = 0;
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		Elf32_Ehdr eheader;
		Elf32_Phdr pheader;
		u8 const *datPtr;
		u32 j,loadSeg;

		/* read header */
		load_read(l->fd,0,&eheader,sizeof(Elf32_Ehdr));

		if(l->name == NULL)
			entryPoint = eheader.e_entry;

		loadSeg = 0;
		datPtr = (u8 const*)(eheader.e_phoff);
		for(j = 0; j < eheader.e_phnum; datPtr += eheader.e_phentsize, j++) {
			/* read pheader */
			load_read(l->fd,(u32)datPtr,&pheader,sizeof(Elf32_Phdr));
			if(pheader.p_type == PT_LOAD || pheader.p_type == PT_TLS) {
				u32 addr = load_addSeg(l->fd,&l->bin,&pheader,loadSeg,l->name != NULL);
				if(addr == 0)
					error("Unable to add segment %d (type %d)",j,pheader.p_type);
				/* store load-address of text */
				if(loadSeg == 0 && l->name != NULL)
					l->loadAddr = addr;
				loadSeg++;
			}
		}

		/* store some shortcuts */
		/* TODO just temporary; later we should access the strtbl always in .text */
		free(l->strtbl);
		l->strtbl = (char*)load_getDyn(l->dyn,DT_STRTAB);
		l->hashTbl = (Elf32_Word*)load_getDyn(l->dyn,DT_HASH);
		l->symbols = (Elf32_Sym*)load_getDyn(l->dyn,DT_SYMTAB);
		l->jmprel = (Elf32_Rel*)load_getDyn(l->dyn,DT_JMPREL);
		if(l->strtbl)
			l->strtbl = (char*)((u32)l->strtbl + l->loadAddr);
		if(l->hashTbl)
			l->hashTbl = (Elf32_Word*)((u32)l->hashTbl + l->loadAddr);
		if(l->symbols)
			l->symbols = (Elf32_Sym*)((u32)l->symbols + l->loadAddr);
		if(l->jmprel)
			l->jmprel = (Elf32_Rel*)((u32)l->jmprel + l->loadAddr);

		close(l->fd);
	}
	return entryPoint;
}

static void load_init(void) {
	sSLNode *n;
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		load_initLib(l);
	}
}

static void load_initLib(sSharedLib *l) {
	sSLNode *n;
	u32 constrStart,constrEnd;
	Elf32_Sym *symStart,*symEnd;
	fConstr *constr;

	/* already initialized? */
	if(l->initialized)
		return;

	/* first go through the dependencies */
	for(n = sll_begin(l->deps); n != NULL; n = n->next) {
		sSharedLib *dl = (sSharedLib*)n->data;
		load_initLib(dl);
	}

	/* lookup symbols */
	symStart = lookup_byNameIn(l,"__libcpp_constr_start",&constrStart);
	if(!symStart)
		return;
	symEnd = lookup_byNameIn(l,"__libcpp_constr_end",&constrEnd);
	if(!symEnd)
		return;

	DBGDL("Calling global constructors of %s\n",l->name ? l->name : "-Main-");

	/* call constructors */
	constr = (fConstr*)constrStart;
	while(constr < (fConstr*)constrEnd) {
		DBGDL("Calling constructor %x\n",*constr);
		(*constr)();
		constr++;
	}

	l->initialized = true;
}

static void load_reloc(void) {
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

	DBGDL("Relocating stuff of %s (loaded @ %x)\n",l->name ? l->name : "-Main-",l->loadAddr);

	rel = (Elf32_Rel*)load_getDyn(l->dyn,DT_REL);
	if(rel) {
		u32 x;
		u32 relCount = load_getDyn(l->dyn,DT_RELSZ);
		relCount /= sizeof(Elf32_Rel);
		rel = (Elf32_Rel*)((u32)rel + l->loadAddr);
		for(x = 0; x < relCount; x++) {
			u32 relType = ELF32_R_TYPE(rel[x].r_info);
			if(relType == R_386_GLOB_DAT) {
				u32 symIndex = ELF32_R_SYM(rel[x].r_info);
				u32 *ptr = (u32*)(rel[x].r_offset + l->loadAddr);
				Elf32_Sym *sym = l->symbols + symIndex;
				/* if the symbol-value is 0, it seems that we have to lookup the symbol now and
				 * store that value instead. TODO I'm not sure if thats correct */
				if(sym->st_value == 0) {
					u32 value;
					if(!lookup_byName(l,l->strtbl + sym->st_name,&value))
						error("Unable to find symbol %s",l->strtbl + sym->st_name);
					*ptr = value;
				}
				else
					*ptr = l->symbols[symIndex].st_value + l->loadAddr;
				DBGDL("Rel (GLOB_DAT) off=%x orgoff=%x reloc=%x orgval=%x\n",
						rel[x].r_offset + l->loadAddr,rel[x].r_offset,*ptr,
						l->symbols[symIndex].st_value);
			}
			else if(relType == R_386_COPY) {
				u32 value;
				u32 symIndex = ELF32_R_SYM(rel[x].r_info);
				const char *name = l->strtbl + l->symbols[symIndex].st_name;
				Elf32_Sym *foundSym = lookup_byName(l,name,&value);
				if(foundSym == NULL)
					error("Unable to find symbol %s",name);
				memcpy((void*)(rel[x].r_offset),(void*)value,foundSym->st_size);
				/* set the GOT-Entry in the library of the symbol to the address we've copied
				 * the value to. TODO I'm not sure if that's the intended way... */
				load_adjustCopyGotEntry(name,rel[x].r_offset);
				DBGDL("Rel (COPY) off=%x sym=%s addr=%x val=%x\n",rel[x].r_offset,name,
						value,*(u32*)value);
			}
			else if(relType == R_386_RELATIVE) {
				u32 *ptr = (u32*)(rel[x].r_offset + l->loadAddr);
				*ptr += l->loadAddr;
				DBGDL("Rel (RELATIVE) off=%x orgoff=%x reloc=%x\n",
						rel[x].r_offset + l->loadAddr,rel[x].r_offset,*ptr);
			}
			else if(relType == R_386_32) {
				u32 *ptr = (u32*)(rel[x].r_offset + l->loadAddr);
				Elf32_Sym *sym = l->symbols + ELF32_R_SYM(rel[x].r_info);
				DBGDL("Rel (32) off=%x orgoff=%x symval=%x org=%x reloc=%x\n",
						rel[x].r_offset + l->loadAddr,rel[x].r_offset,sym->st_value,*ptr,
						sym->st_value + l->loadAddr);
				*ptr += sym->st_value + l->loadAddr;
			}
			else
				error("Unknown relocation: off=%x info=%x\n",rel[x].r_offset,rel[x].r_info);
		}
	}

	/* adjust addresses in PLT-jumps */
	if(l->jmprel) {
		u32 x,jmpCount = load_getDyn(l->dyn,DT_PLTRELSZ);
		jmpCount /= sizeof(Elf32_Rel);
		for(x = 0; x < jmpCount; x++) {
			u32 *addr = (u32*)(l->jmprel[x].r_offset + l->loadAddr);
			/* if the address is 0 (should be the next instruction at the beginning),
			 * do the symbol-lookup immediatly.
			 * TODO i can't imagine that this is the intended way. why is the address 0 in
			 * this case??? (instead of the offset from the beginning of the shared lib,
			 * so that we can simply add the loadAddr) */
			if(LD_BIND_NOW || *addr == 0) {
				u32 value;
				u32 symIndex = ELF32_R_SYM(l->jmprel[x].r_info);
				const char *name = l->strtbl + l->symbols[symIndex].st_name;
				Elf32_Sym *foundSym = lookup_byName(l,name,&value);
				if(!foundSym && !LD_BIND_NOW)
					error("Unable to find symbol %s",name);
				else if(foundSym)
					*addr = value;
				else
					*addr += l->loadAddr;
				DBGDL("JmpRel off=%x addr=%x reloc=%x (%s)\n",l->jmprel[x].r_offset,addr,value,name);
			}
			else {
				*addr += l->loadAddr;
				DBGDL("JmpRel off=%x addr=%x reloc=%x\n",l->jmprel[x].r_offset,addr,*addr);
			}
		}
	}

	/* store pointer to library and lookup-function into GOT */
	got = (u32*)load_getDyn(l->dyn,DT_PLTGOT);
	if(got) {
		got = (Elf32_Addr*)((u32)got + l->loadAddr);
		got[1] = (Elf32_Addr)l;
		got[2] = (Elf32_Addr)&lookup_resolveStart;
	}

	l->relocated = true;
}

static void load_adjustCopyGotEntry(const char *name,u32 copyAddr) {
	sSLNode *n;
	u32 address;
	/* go through all libraries and copy the address of the object (copy-relocated) to
	 * the corresponding GOT-entry. this ensures that all DSO's use the same object */
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		/* don't do that in the executable */
		if(l->name == NULL)
			continue;

		if(lookup_byNameIn(l,name,&address)) {
			/* TODO we should store DT_REL and DT_RELSZ */
			Elf32_Rel *rel = (Elf32_Rel*)load_getDyn(l->dyn,DT_REL);
			if(rel) {
				u32 x;
				u32 relCount = load_getDyn(l->dyn,DT_RELSZ);
				relCount /= sizeof(Elf32_Rel);
				rel = (Elf32_Rel*)((u32)rel + l->loadAddr);
				for(x = 0; x < relCount; x++) {
					if(ELF32_R_TYPE(rel[x].r_info) == R_386_GLOB_DAT) {
						u32 symIndex = ELF32_R_SYM(rel[x].r_info);
						if(l->symbols[symIndex].st_value + l->loadAddr == address) {
							u32 *ptr = (u32*)(rel[x].r_offset + l->loadAddr);
							*ptr = copyAddr;
							break;
						}
					}
				}
			}
		}
	}
}

static void load_library(sSharedLib *dst) {
	char path[MAX_PATH_LEN];
	tFD fd;
	snprintf(path,sizeof(path),"/lib/%s",dst->name);
	fd = open(path,IO_READ);
	if(fd < 0)
		error("Unable to open '%s'",path);
	load_doLoad(fd,dst);
}

static void load_doLoad(tFD binFd,sSharedLib *dst) {
	Elf32_Ehdr eheader;
	Elf32_Phdr pheader;
	sFileInfo info;
	u8 const *datPtr;
	u32 j,loadSeg,textOffset = 0xFFFFFFFF;

	/* build bindesc */
	if(fstat(binFd,&info) < 0)
		error("stat() for binary failed");
	dst->bin.ino = info.inodeNo;
	dst->bin.dev = info.device;
	dst->bin.modifytime = info.modifytime;
	dst->fd = binFd;
	dst->loadAddr = 0;

	/* read header */
	load_read(binFd,0,&eheader,sizeof(Elf32_Ehdr));

	/* check magic-number */
	if(eheader.e_ident.dword != *(u32*)ELFMAG)
		error("Invalid ELF-magic");

	loadSeg = 0;
	datPtr = (u8 const*)(eheader.e_phoff);
	for(j = 0; j < eheader.e_phnum; datPtr += eheader.e_phentsize, j++) {
		/* read pheader */
		load_read(binFd,(u32)datPtr,&pheader,sizeof(Elf32_Phdr));

		/* in shared libraries the text is @ 0x0 (will be relocated anyway), but the entry
		 * DT_STRTAB in the dynamic table is the virtual address, not the file offset. Therefore
		 * we have to add the difference of the text in the file and in virtual memory.
		 * This will be 0 for executables and 0x1000 for shared libraries
		 * (Note that the first load-segment is the text) */
		if(textOffset == 0xFFFFFFFF && pheader.p_type == PT_LOAD)
			textOffset = pheader.p_offset - pheader.p_vaddr;

		if(pheader.p_type == PT_DYNAMIC) {
			u32 strtblSize,i;
			/* TODO we don't have to read them from file; if we load data and text first, we
			 * already have them */
			/* read dynamic-entries */
			dst->dyn = (Elf32_Dyn*)malloc(pheader.p_filesz);
			if(!dst->dyn)
				error("Not enough mem!");
			load_read(binFd,pheader.p_offset,dst->dyn,pheader.p_filesz);

			/* read string-table */
			strtblSize = load_getDyn(dst->dyn,DT_STRSZ);
			dst->strtbl = (char*)malloc(strtblSize);
			if(!dst->strtbl)
				error("Not enough mem!");
			load_read(binFd,load_getDyn(dst->dyn,DT_STRTAB) + textOffset,dst->strtbl,strtblSize);

			for(i = 0; i < (pheader.p_filesz / sizeof(Elf32_Dyn)); i++) {
				if(dst->dyn[i].d_tag == DT_NEEDED) {
					sSharedLib *nlib,*lib = (sSharedLib*)malloc(sizeof(sSharedLib));
					if(!lib)
						error("Not enough mem!");
					lib->relocated = false;
					lib->initialized = false;
					lib->dyn = NULL;
					lib->strtbl = NULL;
					lib->name = dst->strtbl + dst->dyn[i].d_un.d_val;
					lib->loadAddr = 0;
					lib->deps = sll_create();
					if(!lib->deps)
						error("Not enough mem!");
					nlib = load_addLib(lib);
					if(nlib == NULL) {
						load_library(lib);
						nlib = lib;
					}
					else
						free(lib);
					sll_append(dst->deps,nlib);
				}
			}
		}
	}
}

static sSharedLib *load_addLib(sSharedLib *lib) {
	sSLNode *n;
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		if(l->name && strcmp(l->name,lib->name) == 0)
			return l;
	}
	if(!sll_append(libs,lib))
		error("Not enough mem!");
	return NULL;
}

static u32 load_getDyn(Elf32_Dyn *dyn,Elf32_Sword tag) {
	if(dyn == NULL)
		error("No dynamic entries");
	while(dyn->d_tag != DT_NULL) {
		if(dyn->d_tag == tag)
			return dyn->d_un.d_val;
		dyn++;
	}
	return 0;
}

static u32 load_addSeg(tFD binFd,sBinDesc *bindesc,Elf32_Phdr *pheader,u32 loadSegNo,bool isLib) {
	u8 stype;
	void *addr;
	/* determine type */
	if(loadSegNo == 0) {
		/* dynamic linker has a special entrypoint */
		if(!isLib && (pheader->p_flags != (PF_X | PF_R) || pheader->p_vaddr != TEXT_BEGIN))
			return 0;
		stype = isLib ? REG_SHLIBTEXT : REG_TEXT;
	}
	else if(pheader->p_type == PT_TLS) {
		/* TODO not supported atm */
		if(isLib)
			return 0;
		stype = REG_TLS;
		/* we need the thread-control-block at the end */
		pheader->p_memsz += sizeof(void*);
	}
	else if(pheader->p_flags == PF_R)
		stype = REG_RODATA;
	else if(pheader->p_flags == (PF_R | PF_W)) {
		if(pheader->p_filesz == 0)
			stype = isLib ? REG_SHLIBBSS : REG_BSS;
		else
			stype = isLib ? REG_SHLIBDATA : REG_DATA;
	}
	else
		return 0;

	/* check if the sizes are valid */
	if(pheader->p_filesz > pheader->p_memsz)
		return 0;

	/* bss and tls need no binary */
	if(stype == REG_BSS || stype == REG_SHLIBBSS || stype == REG_TLS)
		bindesc = NULL;
	/* add the region */
	if((addr = addRegion(bindesc,pheader->p_offset,pheader->p_memsz,stype)) == NULL)
		return 0;
	if(stype == REG_TLS) {
		/* read tdata and clear tbss */
		load_read(binFd,(u32)pheader->p_offset,addr,pheader->p_filesz);
		memclear((void*)((u32)addr + pheader->p_filesz),pheader->p_memsz - pheader->p_filesz);
	}
	return (u32)addr;
}

static void load_read(tFD binFd,u32 offset,void *buffer,u32 count) {
	if(seek(binFd,(u32)offset,SEEK_SET) < 0)
		error("Unable to seek to %x",offset);
	if(read(binFd,buffer,count) != (s32)count)
		error("Unable to read %d bytes @ %x",count,offset);
}
