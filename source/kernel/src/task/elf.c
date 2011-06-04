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
#include <sys/util.h>
#include <sys/video.h>
#include <string.h>
#include <errors.h>
#include <assert.h>

#define ELF_TYPE_PROG		0
#define ELF_TYPE_INTERP		1

static int elf_doLoadFromFile(const char *path,uint type,sStartupInfo *info);
static int elf_addSegment(const sBinDesc *bindesc,const Elf32_Phdr *pheader,size_t loadSegNo,uint type);

int elf_loadFromFile(const char *path,sStartupInfo *info) {
	return elf_doLoadFromFile(path,ELF_TYPE_PROG,info);
}

int elf_loadFromMem(const void *code,size_t length,sStartupInfo *info) {
	size_t j,loadSegNo = 0;
	uint8_t const *datPtr;
	Elf32_Ehdr *eheader = (Elf32_Ehdr*)code;
	Elf32_Phdr *pheader = NULL;

	/* check magic */
	if(memcmp(eheader->e_ident.chars,ELFMAG,4) != 0)
		return ERR_INVALID_ELF_BIN;

	/* load the LOAD segments. */
	datPtr = (uint8_t const*)((uintptr_t)code + eheader->e_phoff);
	for(j = 0; j < eheader->e_phnum; datPtr += eheader->e_phentsize, j++) {
		pheader = (Elf32_Phdr*)datPtr;
		/* check if all stuff is in the binary */
		if((uintptr_t)pheader + sizeof(Elf32_Phdr) >= (uintptr_t)code + length)
			return ERR_INVALID_ELF_BIN;

		if(pheader->p_type == PT_LOAD) {
			if(pheader->p_vaddr + pheader->p_filesz >= (uintptr_t)code + length)
				return ERR_INVALID_ELF_BIN;
			if(elf_addSegment(NULL,pheader,loadSegNo,ELF_TYPE_PROG) < 0)
				return ERR_INVALID_ELF_BIN;
			/* copy the data and zero the rest, if necessary */
			util_copyToUser((void*)pheader->p_vaddr,(void*)((uintptr_t)code + pheader->p_offset),
					pheader->p_filesz);
			util_zeroToUser((void*)(pheader->p_vaddr + pheader->p_filesz),
					pheader->p_memsz - pheader->p_filesz);
			loadSegNo++;
		}
	}

	info->linkerEntry = info->progEntry = eheader->e_entry;
	return 0;
}

static int elf_doLoadFromFile(const char *path,uint type,sStartupInfo *info) {
	sThread *t = thread_getRunning();
	sProc *p = t->proc;
	tFileNo file;
	size_t j,loadSeg = 0;
	uint8_t const *datPtr;
	Elf32_Ehdr eheader;
	Elf32_Phdr pheader;
	sFileInfo finfo;
	sBinDesc bindesc;

	file = vfs_real_openPath(p->pid,VFS_READ,path);
	if(file < 0)
		return ERR_INVALID_ELF_BIN;

	/* fill bindesc */
	if(vfs_fstat(p->pid,file,&finfo) < 0)
		goto failed;
	bindesc.ino = finfo.inodeNo;
	bindesc.dev = finfo.device;
	bindesc.modifytime = finfo.modifytime;

	/* first read the header */
	if(vfs_readFile(p->pid,file,&eheader,sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr))
		goto failed;

	/* check magic */
	if(memcmp(eheader.e_ident.chars,ELFMAG,4) != 0)
		goto failed;

	/* by default set the same; the dl will overwrite it when needed */
	if(type == ELF_TYPE_PROG)
		info->linkerEntry = info->progEntry = eheader.e_entry;
	else
		info->linkerEntry = eheader.e_entry;

	/* load the LOAD segments. */
	datPtr = (uint8_t const*)(eheader.e_phoff);
	for(j = 0; j < eheader.e_phnum; datPtr += eheader.e_phentsize, j++) {
		/* go to header */
		if(vfs_seek(p->pid,file,(off_t)datPtr,SEEK_SET) < 0)
			goto failed;
		/* read pheader */
		if(vfs_readFile(p->pid,file,&pheader,sizeof(Elf32_Phdr)) != sizeof(Elf32_Phdr))
			goto failed;

		if(pheader.p_type == PT_INTERP) {
			int res;
			char *interpName;
			/* has to be the first segment and is not allowed for the dynamic linker */
			if(loadSeg > 0 || type != ELF_TYPE_PROG)
				goto failed;
			/* read name of dynamic linker */
			interpName = (char*)kheap_alloc(pheader.p_filesz);
			if(interpName == NULL)
				goto failed;
			if(vfs_seek(p->pid,file,pheader.p_offset,SEEK_SET) < 0) {
				kheap_free(interpName);
				goto failed;
			}
			if(vfs_readFile(p->pid,file,interpName,pheader.p_filesz) != (ssize_t)pheader.p_filesz) {
				kheap_free(interpName);
				goto failed;
			}
			vfs_closeFile(p->pid,file);
			/* now load him and stop loading the 'real' program */
			res = elf_doLoadFromFile(interpName,ELF_TYPE_INTERP,info);
			kheap_free(interpName);
			return res;
		}

		if(pheader.p_type == PT_LOAD || pheader.p_type == PT_TLS) {
			int stype;
			stype = elf_addSegment(&bindesc,&pheader,loadSeg,type);
			if(stype < 0)
				goto failed;
			if(stype == REG_TLS) {
				uintptr_t tlsStart,tlsEnd;
				vmm_getRegRange(p,t->tlsRegion,&tlsStart,&tlsEnd);
				/* read tdata */
				if(vfs_seek(p->pid,file,(off_t)pheader.p_offset,SEEK_SET) < 0)
					goto failed;
				if(vfs_readFile(p->pid,file,(void*)tlsStart,pheader.p_filesz) < 0)
					goto failed;
				/* clear tbss */
				util_zeroToUser((void*)(tlsStart + pheader.p_filesz),pheader.p_memsz - pheader.p_filesz);
			}
			loadSeg++;
		}
	}

	vfs_closeFile(p->pid,file);
	return 0;

failed:
	vfs_closeFile(p->pid,file);
	return ERR_INVALID_ELF_BIN;
}

static int elf_addSegment(const sBinDesc *bindesc,const Elf32_Phdr *pheader,size_t loadSegNo,uint type) {
	sThread *t = thread_getRunning();
	int stype,res;
	size_t memsz = pheader->p_memsz;
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
		memsz += sizeof(void*);
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
	if(pheader->p_filesz > memsz)
		return ERR_INVALID_ELF_BIN;

	/* tls needs no binary */
	if(stype == REG_TLS)
		bindesc = NULL;
	/* add the region */
	if((res = vmm_add(t->proc,bindesc,pheader->p_offset,memsz,pheader->p_filesz,stype)) < 0)
		return res;
	if(stype == REG_TLS)
		t->tlsRegion = res;
	return stype;
}
