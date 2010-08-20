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

#include <sys/common.h>
#include <sys/task/proc.h>
#include <sys/task/elf.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/vmm.h>
#include <sys/mem/kheap.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/real.h>
#include <sys/video.h>
#include <string.h>
#include <errors.h>
#include <assert.h>

#define ELF_TYPE_PROG		0
#define ELF_TYPE_INTERP		1

static s32 elf_doLoadFromFile(const char *path,u8 type,sStartupInfo *info);
static s32 elf_addSegment(sBinDesc *bindesc,Elf32_Phdr *pheader,u32 loadSegNo,u8 type);

s32 elf_loadFromFile(const char *path,sStartupInfo *info) {
	return elf_doLoadFromFile(path,ELF_TYPE_PROG,info);
}

s32 elf_loadFromMem(u8 *code,u32 length,sStartupInfo *info) {
	u32 loadSegNo = 0;
	u32 j;
	u8 const *datPtr;
	Elf32_Ehdr *eheader = (Elf32_Ehdr*)code;
	Elf32_Phdr *pheader = NULL;

	/* check magic */
	if(eheader->e_ident.dword != *(u32*)ELFMAG)
		return ERR_INVALID_ELF_BIN;

	/* load the LOAD segments. */
	datPtr = (u8 const*)(code + eheader->e_phoff);
	for(j = 0; j < eheader->e_phnum; datPtr += eheader->e_phentsize, j++) {
		pheader = (Elf32_Phdr*)datPtr;
		/* check if all stuff is in the binary */
		if((u8*)pheader + sizeof(Elf32_Phdr) >= code + length)
			return ERR_INVALID_ELF_BIN;

		if(pheader->p_type == PT_LOAD) {
			if(pheader->p_vaddr + pheader->p_filesz >= (u32)(code + length))
				return ERR_INVALID_ELF_BIN;
			if(elf_addSegment(NULL,pheader,loadSegNo,ELF_TYPE_PROG) < 0)
				return ERR_INVALID_ELF_BIN;
			/* copy the data and zero the rest, if necessary */
			memcpy((void*)pheader->p_vaddr,code + pheader->p_offset,pheader->p_filesz);
			memclear((void*)(pheader->p_vaddr + pheader->p_filesz),
					pheader->p_memsz - pheader->p_filesz);
			loadSegNo++;
		}
	}

	info->linkerEntry = info->progEntry = eheader->e_entry;
	return 0;
}

static s32 elf_doLoadFromFile(const char *path,u8 type,sStartupInfo *info) {
	sThread *t = thread_getRunning();
	tFileNo file;
	u32 j,loadSeg = 0;
	u8 const *datPtr;
	Elf32_Ehdr eheader;
	Elf32_Phdr pheader;
	sFileInfo finfo;
	sBinDesc bindesc;

	file = vfsr_openFile(t->tid,VFS_READ,path);
	if(file < 0)
		return ERR_INVALID_ELF_BIN;

	/* fill bindesc */
	if(vfs_fstat(t->tid,file,&finfo) < 0)
		goto failed;
	bindesc.ino = finfo.inodeNo;
	bindesc.dev = finfo.device;
	bindesc.modifytime = finfo.modifytime;

	/* first read the header */
	if(vfs_readFile(t->tid,file,(u8*)&eheader,sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr))
		goto failed;

	/* check magic */
	if(eheader.e_ident.dword != *(u32*)ELFMAG)
		goto failed;

	/* by default set the same; the dl will overwrite it when needed */
	if(type == ELF_TYPE_PROG)
		info->linkerEntry = info->progEntry = eheader.e_entry;
	else
		info->linkerEntry = eheader.e_entry;

	/* load the LOAD segments. */
	datPtr = (u8 const*)(eheader.e_phoff);
	for(j = 0; j < eheader.e_phnum; datPtr += eheader.e_phentsize, j++) {
		/* go to header */
		if(vfs_seek(t->tid,file,(u32)datPtr,SEEK_SET) < 0)
			goto failed;
		/* read pheader */
		if(vfs_readFile(t->tid,file,(u8*)&pheader,sizeof(Elf32_Phdr)) != sizeof(Elf32_Phdr))
			goto failed;

		if(pheader.p_type == PT_INTERP) {
			s32 res;
			char *interpName;
			/* has to be the first segment and is not allowed for the dynamic linker */
			if(loadSeg > 0 || type != ELF_TYPE_PROG)
				goto failed;
			/* read name of dynamic linker */
			interpName = (char*)kheap_alloc(pheader.p_filesz);
			if(interpName == NULL)
				goto failed;
			if(vfs_seek(t->tid,file,pheader.p_offset,SEEK_SET) < 0) {
				kheap_free(interpName);
				goto failed;
			}
			if(vfs_readFile(t->tid,file,(u8*)interpName,pheader.p_filesz) != (s32)pheader.p_filesz) {
				kheap_free(interpName);
				goto failed;
			}
			vfs_closeFile(t->tid,file);
			/* now load him and stop loading the 'real' program */
			res = elf_doLoadFromFile(interpName,ELF_TYPE_INTERP,info);
			kheap_free(interpName);
			return res;
		}

		if(pheader.p_type == PT_LOAD || pheader.p_type == PT_TLS) {
			s32 stype;
			stype = elf_addSegment(&bindesc,&pheader,loadSeg,type);
			if(stype < 0)
				goto failed;
			if(stype == REG_TLS) {
				u32 tlsStart,tlsEnd;
				vmm_getRegRange(t->proc,t->tlsRegion,&tlsStart,&tlsEnd);
				/* read tdata */
				if(vfs_seek(t->tid,file,(u32)pheader.p_offset,SEEK_SET) < 0)
					goto failed;
				if(vfs_readFile(t->tid,file,(u8*)tlsStart,pheader.p_filesz) < 0)
					goto failed;
				/* clear tbss */
				memclear((void*)(tlsStart + pheader.p_filesz),pheader.p_memsz - pheader.p_filesz);
			}
			loadSeg++;
		}
	}

	vfs_closeFile(t->tid,file);
	return 0;

failed:
	vfs_closeFile(t->tid,file);
	return ERR_INVALID_ELF_BIN;
}

static s32 elf_addSegment(sBinDesc *bindesc,Elf32_Phdr *pheader,u32 loadSegNo,u8 type) {
	sThread *t = thread_getRunning();
	u8 stype;
	s32 res;
	/* determine type */
	if(loadSegNo == 0) {
		/* dynamic linker has a special entrypoint */
		if(type == ELF_TYPE_INTERP && pheader->p_vaddr != INTERP_TEXT_BEGIN)
			return ERR_INVALID_ELF_BIN;
		if(pheader->p_flags != (PF_X | PF_R) ||
			(type == ELF_TYPE_INTERP && pheader->p_vaddr != INTERP_TEXT_BEGIN) ||
			(type == ELF_TYPE_PROG && pheader->p_vaddr != TEXT_BEGIN))
			return ERR_INVALID_ELF_BIN;
		stype = type == ELF_TYPE_INTERP ? REG_SHLIBTEXT : REG_TEXT;
	}
	else if(pheader->p_type == PT_TLS) {
		/* not allowed for the dynamic linker */
		if(type == ELF_TYPE_INTERP)
			return ERR_INVALID_ELF_BIN;
		stype = REG_TLS;
		/* we need the thread-control-block at the end */
		pheader->p_memsz += sizeof(void*);
	}
	else if(pheader->p_flags == PF_R) {
		/* not allowed for the dynamic linker */
		if(type == ELF_TYPE_INTERP)
			return ERR_INVALID_ELF_BIN;
		stype = REG_RODATA;
	}
	else if(pheader->p_flags == (PF_R | PF_W))
		stype = type == ELF_TYPE_INTERP ? REG_DLDATA : REG_DATA;
	else
		return ERR_INVALID_ELF_BIN;

	/* check if the sizes are valid */
	if(pheader->p_filesz > pheader->p_memsz)
		return ERR_INVALID_ELF_BIN;

	/* tls needs no binary */
	if(stype == REG_TLS)
		bindesc = NULL;
	/* add the region */
	if((res = vmm_add(t->proc,bindesc,pheader->p_offset,pheader->p_memsz,pheader->p_filesz,stype)) < 0)
		return res;
	if(stype == REG_TLS)
		t->tlsRegion = res;
	return stype;
}
