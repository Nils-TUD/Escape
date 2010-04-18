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
#include <esc/fileio.h>
#include <esc/proc.h>
#include <esc/mem.h>
#include <esc/heap.h>
#include <stdarg.h>
#include <string.h>
#include <errors.h>
#include <sllist.h>
#include "elf.h"

#define TEXT_BEGIN		0x1000

typedef struct {
	char *name;
	tFD fd;
	sBinDesc bin;
	u32 loadAddr;
	Elf32_Dyn *dyn;
	Elf32_Addr *got;
	Elf32_Word *hashTbl;
	Elf32_Rel *jmprel;
	Elf32_Rel *rel;
	Elf32_Sym *symbols;
	char *strtbl;
} sSharedLib;

/* make gcc happy */
u32 loadProg(tFD binFd);
u32 lookupSymbol(sSharedLib *lib,u32 offset);
extern void lookupSymbolStart(void);
static Elf32_Sym *doLookupSymbol(sSharedLib *skip,const char *name,u32 *value);
static Elf32_Sym *lookupSymbolIn(sSharedLib *lib,const char *name,u32 hash);
static u32 elf_hash(const unsigned char *name);
static void loadLib(sSharedLib *main);
static void loadFile(tFD binFd,sSharedLib *main);
static bool addLibrary(sSharedLib *lib);
static u32 getFromDyn(Elf32_Dyn *dyn,Elf32_Sword tag);
static void readPart(tFD binFd,u32 offset,void *buffer,u32 count);
static u32 addSegment(tFD binFd,sBinDesc *bindesc,Elf32_Phdr *pheader,u32 loadSegNo,bool isLib);

static sSLList *libs = NULL;

u32 lookupSymbol(sSharedLib *lib,u32 offset) {
	Elf32_Sym *foundSym;
	Elf32_Rel *rel = (Elf32_Rel*)((u32)lib->jmprel + offset);
	Elf32_Sym *sym = lib->symbols + ELF32_M_SYM(rel->r_info);
	u32 value,*addr;
	foundSym = doLookupSymbol(NULL,lib->strtbl + sym->st_name,&value);
	if(foundSym == NULL)
		error("Unable to find symbol %s",lib->strtbl + sym->st_name);
	addr = (u32*)(rel->r_offset + lib->loadAddr);
	*addr = value;
	return value;
}

static u32 elf_hash(const unsigned char *name) {
	u32 h = 0,g;
	while(*name) {
		h = (h << 4) + *name++;
		if((g = (h & 0xf0000000)))
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

static Elf32_Sym *doLookupSymbol(sSharedLib *skip,const char *name,u32 *value) {
	sSLNode *n;
	Elf32_Sym *s;
	u32 hash = elf_hash((const unsigned char*)name);
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		if(l == skip)
			continue;
		s = lookupSymbolIn(l,name,hash);
		if(s) {
			*value = s->st_value + l->loadAddr;
			return s;
		}
	}
	return NULL;
}

static Elf32_Sym *lookupSymbolIn(sSharedLib *lib,const char *name,u32 hash) {
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

u32 loadProg(tFD binFd) {
	u32 entryPoint;
	libs = sll_create();
	if(!libs)
		error("Not enough mem!");

	/* create entry for program / library */
	sSharedLib *main = (sSharedLib*)malloc(sizeof(sSharedLib));
	if(!main)
		error("Not enough mem!");
	main->strtbl = NULL;
	main->dyn = NULL;
	main->name = NULL;
	if(!sll_append(libs,main))
		error("Not enough mem!");

	/* load program including shared libraries into linked list */
	loadFile(binFd,main);

	sSLNode *n;
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		Elf32_Ehdr eheader;
		Elf32_Phdr pheader;
		u8 const *datPtr;
		u32 j,loadSeg;

		/* read header */
		readPart(l->fd,0,&eheader,sizeof(Elf32_Ehdr));

		if(l->name == NULL)
			entryPoint = eheader.e_entry;

		loadSeg = 0;
		datPtr = (u8 const*)(eheader.e_phoff);
		for(j = 0; j < eheader.e_phnum; datPtr += eheader.e_phentsize, j++) {
			/* read pheader */
			readPart(l->fd,(u32)datPtr,&pheader,sizeof(Elf32_Phdr));
			if(pheader.p_type == PT_LOAD || pheader.p_type == PT_TLS) {
				u32 addr = addSegment(l->fd,&l->bin,&pheader,loadSeg,l->name != NULL);
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
		l->strtbl = (char*)getFromDyn(l->dyn,DT_STRTAB);
		l->hashTbl = (Elf32_Word*)getFromDyn(l->dyn,DT_HASH);
		l->symbols = (Elf32_Sym*)getFromDyn(l->dyn,DT_SYMTAB);
		l->jmprel = (Elf32_Rel*)getFromDyn(l->dyn,DT_JMPREL);
		l->rel = (Elf32_Rel*)getFromDyn(l->dyn,DT_REL);
		l->got = (u32*)getFromDyn(l->dyn,DT_PLTGOT);
		if(l->strtbl)
			l->strtbl = (char*)((u32)l->strtbl + l->loadAddr);
		if(l->hashTbl)
			l->hashTbl = (Elf32_Word*)((u32)l->hashTbl + l->loadAddr);
		if(l->symbols)
			l->symbols = (Elf32_Sym*)((u32)l->symbols + l->loadAddr);
		if(l->jmprel)
			l->jmprel = (Elf32_Rel*)((u32)l->jmprel + l->loadAddr);
		if(l->rel)
			l->rel = (Elf32_Rel*)((u32)l->rel + l->loadAddr);
		if(l->got)
			l->got = (Elf32_Addr*)((u32)l->got + l->loadAddr);

		close(l->fd);
	}

	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		if(l->rel) {
			u32 x;
			u32 relCount = getFromDyn(l->dyn,DT_RELSZ);
			relCount /= sizeof(Elf32_Rel);
			for(x = 0; x < relCount; x++) {
				u32 relType = ELF32_R_TYPE(l->rel[x].r_info);
				if(relType == R_386_GLOB_DAT) {
					u32 symIndex = ELF32_R_SYM(l->rel[x].r_info);
					u32 *ptr = (u32*)(l->rel[x].r_offset + l->loadAddr);
					*ptr = l->symbols[symIndex].st_value + l->loadAddr;
				}
				else if(relType == R_386_COPY) {
					u32 value;
					u32 symIndex = ELF32_R_SYM(l->rel[x].r_info);
					const char *name = l->strtbl + l->symbols[symIndex].st_name;
					Elf32_Sym *foundSym = doLookupSymbol(l,name,&value);
					if(foundSym == NULL)
						error("Unable to find symbol %s",name);
					memcpy((void*)(l->rel[x].r_offset),(void*)value,foundSym->st_size);
				}
			}
		}

		/* adjust addresses in PLT-jumps */
		if(l->jmprel) {
			u32 x,jmpCount = getFromDyn(l->dyn,DT_PLTRELSZ);
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
					Elf32_Sym *foundSym = doLookupSymbol(l,name,&value);
					if(!foundSym)
						error("Unable to find symbol %s",name);
					*addr = value;
				}
			}
		}

		if(l->got) {
			/* store pointer to library and lookup-function into GOT */
			l->got[1] = (Elf32_Addr)l;
			l->got[2] = (Elf32_Addr)&lookupSymbolStart;
		}
	}

	return entryPoint;
}

static void loadLib(sSharedLib *main) {
	char path[MAX_PATH_LEN];
	tFD fd;
	snprintf(path,sizeof(path),"/lib/%s",main->name);
	fd = open(path,IO_READ);
	if(fd < 0)
		error("Unable to open '%s'",path);
	loadFile(fd,main);
}

static void loadFile(tFD binFd,sSharedLib *main) {
	Elf32_Ehdr eheader;
	Elf32_Phdr pheader;
	sFileInfo info;
	u8 const *datPtr;
	u32 j,loadSeg;

	/* build bindesc */
	if(fstat(binFd,&info) < 0)
		error("stat() for binary failed");
	main->bin.ino = info.inodeNo;
	main->bin.dev = info.device;
	main->bin.modifytime = info.modifytime;
	main->fd = binFd;
	main->loadAddr = 0;

	/* read header */
	readPart(binFd,0,&eheader,sizeof(Elf32_Ehdr));

	/* check magic-number */
	if(eheader.e_ident.dword != *(u32*)ELFMAG)
		error("Invalid ELF-magic");

	loadSeg = 0;
	datPtr = (u8 const*)(eheader.e_phoff);
	for(j = 0; j < eheader.e_phnum; datPtr += eheader.e_phentsize, j++) {
		/* read pheader */
		readPart(binFd,(u32)datPtr,&pheader,sizeof(Elf32_Phdr));

		if(pheader.p_type == PT_DYNAMIC) {
			u32 strtblSize,i;
			/* TODO we don't have to read them from file; if we load data and text first, we
			 * already have them */
			/* read dynamic-entries */
			main->dyn = (Elf32_Dyn*)malloc(pheader.p_filesz);
			if(!main->dyn)
				error("Not enough mem!");
			readPart(binFd,pheader.p_offset,main->dyn,pheader.p_filesz);

			/* read string-table */
			strtblSize = getFromDyn(main->dyn,DT_STRSZ);
			main->strtbl = (char*)malloc(strtblSize);
			if(!main->strtbl)
				error("Not enough mem!");
			readPart(binFd,getFromDyn(main->dyn,DT_STRTAB),main->strtbl,strtblSize);

			for(i = 0; i < (pheader.p_filesz / sizeof(Elf32_Dyn)); i++) {
				if(main->dyn[i].d_tag == DT_NEEDED) {
					sSharedLib *lib = (sSharedLib*)malloc(sizeof(sSharedLib));
					if(!lib)
						error("Not enough mem!");
					lib->dyn = NULL;
					lib->strtbl = NULL;
					lib->name = main->strtbl + main->dyn[i].d_un.d_val;
					lib->loadAddr = 0;
					if(!addLibrary(lib))
						free(lib);
					else
						loadLib(lib);
				}
			}
		}
	}
}

static bool addLibrary(sSharedLib *lib) {
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

static u32 getFromDyn(Elf32_Dyn *dyn,Elf32_Sword tag) {
	while(dyn->d_tag != DT_NULL) {
		if(dyn->d_tag == tag)
			return dyn->d_un.d_val;
		dyn++;
	}
	return 0;
}

static void readPart(tFD binFd,u32 offset,void *buffer,u32 count) {
	if(seek(binFd,(u32)offset,SEEK_SET) < 0)
		error("Unable to seek to %x",offset);
	if(read(binFd,buffer,count) != (s32)count)
		error("Unable to read %d bytes @ %x",count,offset);
}

static u32 addSegment(tFD binFd,sBinDesc *bindesc,Elf32_Phdr *pheader,u32 loadSegNo,bool isLib) {
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
		readPart(binFd,(u32)pheader->p_offset,addr,pheader->p_filesz);
		memclear((void*)((u32)addr + pheader->p_filesz),pheader->p_memsz - pheader->p_filesz);
	}
	return (u32)addr;
}
