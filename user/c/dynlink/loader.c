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
#include <esc/heap.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <sllist.h>
#include <string.h>
#include "elf.h"
#include "lookup.h"
#include "loader.h"

static u32 load_addSegments(void);
static void load_reloc(void);
static void load_library(sSharedLib *dst);
static void load_doLoad(tFD binFd,sSharedLib *dst);
static bool load_addLib(sSharedLib *lib);
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
	main->strtbl = NULL;
	main->dyn = NULL;
	main->name = NULL;
	if(!sll_append(libs,main))
		error("Not enough mem!");

	/* load program including shared libraries into linked list */
	load_doLoad(binFd,main);

	/* load segments into memory */
	entryPoint = load_addSegments();

	/* relocate everything we need so that the program can start */
	load_reloc();

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

static void load_reloc(void) {
	sSLNode *n;
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		Elf32_Addr *got;
		Elf32_Rel *rel;

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
					*ptr = l->symbols[symIndex].st_value + l->loadAddr;
				}
				else if(relType == R_386_COPY) {
					u32 value;
					u32 symIndex = ELF32_R_SYM(rel[x].r_info);
					const char *name = l->strtbl + l->symbols[symIndex].st_name;
					Elf32_Sym *foundSym = lookup_byName(l,name,&value);
					if(foundSym == NULL)
						error("Unable to find symbol %s",name);
					memcpy((void*)(rel[x].r_offset),(void*)value,foundSym->st_size);
				}
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
				if(*addr == 0) {
					u32 value;
					u32 symIndex = ELF32_R_SYM(l->jmprel[x].r_info);
					const char *name = l->strtbl + l->symbols[symIndex].st_name;
					Elf32_Sym *foundSym = lookup_byName(l,name,&value);
					if(!foundSym)
						error("Unable to find symbol %s",name);
					*addr = value;
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
	u32 j,loadSeg;

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
			load_read(binFd,load_getDyn(dst->dyn,DT_STRTAB),dst->strtbl,strtblSize);

			for(i = 0; i < (pheader.p_filesz / sizeof(Elf32_Dyn)); i++) {
				if(dst->dyn[i].d_tag == DT_NEEDED) {
					sSharedLib *lib = (sSharedLib*)malloc(sizeof(sSharedLib));
					if(!lib)
						error("Not enough mem!");
					lib->dyn = NULL;
					lib->strtbl = NULL;
					lib->name = dst->strtbl + dst->dyn[i].d_un.d_val;
					lib->loadAddr = 0;
					if(!load_addLib(lib))
						free(lib);
					else
						load_library(lib);
				}
			}
		}
	}
}

static bool load_addLib(sSharedLib *lib) {
	sSLNode *n;
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		if(l->name && strcmp(l->name,lib->name) == 0)
			return false;
	}
	if(!sll_append(libs,lib))
		error("Not enough mem!");
	return true;
}

static u32 load_getDyn(Elf32_Dyn *dyn,Elf32_Sword tag) {
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
