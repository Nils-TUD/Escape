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

#include <common.h>
#include <proc.h>
#include <paging.h>
#include <elf.h>
#include <text.h>
#include <vfs.h>
#include <vfsreal.h>
#include <video.h>
#include <string.h>
#include <errors.h>
#include <assert.h>

/*#define BUF_SIZE (16 * K)*/
#define BUF_SIZE 1024

s32 elf_loadFromFile(const char *path) {
	sProc *p = proc_getRunning();
	sThread *t = thread_getRunning();
	tFileNo file;
	s32 res;
	u32 j,loadSeg = 0;
	u8 const *datPtr;
	Elf32_Ehdr eheader;
	Elf32_Phdr pheader;

	vassert(p->textPages == 0 && p->dataPages == 0,"Process is not empty");

	file = vfsr_openFile(t->tid,VFS_READ,path);
	if(file < 0)
		return ERR_INVALID_ELF_BINARY;

	/* first read the header */
	if((res = vfs_readFile(t->tid,file,(u8*)&eheader,sizeof(Elf32_Ehdr))) < 0)
		goto failed;

	/* check magic */
	if(*(u32*)eheader.e_ident != *(u32*)ELFMAG)
		goto failed;

	/* load the LOAD segments. */
	datPtr = (u8 const*)(eheader.e_phoff);
	for(j = 0; j < eheader.e_phnum; datPtr += eheader.e_phentsize, j++) {
		/* go to header */
		if(vfs_seek(t->tid,file,(u32)datPtr,SEEK_SET) < 0)
			goto failed;
		/* read pheader */
		res = vfs_readFile(t->tid,file,(u8*)&pheader,sizeof(Elf32_Phdr));
		if(res < 0 || res != sizeof(Elf32_Phdr))
			goto failed;

		if(pheader.p_type == PT_LOAD) {
			u32 pages;

			if(loadSeg >= 2)
				goto failed;

			/* check if the sizes are valid */
			if(pheader.p_filesz > pheader.p_memsz)
				goto failed;

			pages = BYTES_2_PAGES(pheader.p_memsz);

			/* text */
			if(loadSeg == 0) {
				/* get to know the lowest virtual address. must be 0x0.  */
				if(pheader.p_vaddr != 0)
					goto failed;
				if(!proc_segSizesValid(pages,p->dataPages,p->stackPages))
					goto failed;

				/* load text */
				res = text_alloc(path,file,pheader.p_offset,pheader.p_filesz,&p->text);
				if(res < 0)
					goto failed;
			}
			/* data */
			else {
				u8 *target;
				s32 rem;
				/* has to be directly behind the text */
				if(pheader.p_vaddr != p->textPages * PAGE_SIZE)
					goto failed;
				/* get more space for the data area and make sure that the segment-sizes are valid */
				if(!proc_segSizesValid(p->textPages,p->dataPages + pages,p->stackPages))
					goto failed;
				if(!proc_changeSize(pages,CHG_DATA))
					goto failed;

				/* load data from fs */
				if(vfs_seek(t->tid,file,pheader.p_offset,SEEK_SET) < 0)
					goto failed;
				target = (u8*)pheader.p_vaddr;
				rem = pheader.p_filesz;
				while(rem > 0) {
					res = vfs_readFile(t->tid,file,target,MIN(BUF_SIZE,rem));
					if(res < 0)
						goto failed;
					rem -= res;
					target += res;
				}
				/* ensure that the specified size was correct */
				if(rem != 0)
					goto failed;

				/* zero remaining bytes */
				memclear((void*)(pheader.p_vaddr + pheader.p_filesz),pheader.p_memsz - pheader.p_filesz);
			}
			loadSeg++;
		}
	}

	vfs_closeFile(t->tid,file);
	return (u32)eheader.e_entry;

failed:
	vfs_closeFile(t->tid,file);
	return ERR_INVALID_ELF_BINARY;
}

s32 elf_loadFromMem(u8 *code,u32 length) {
	u32 seenLoadSegments = 0;
	sProc *p = proc_getRunning();
	u32 j;
	u8 const *datPtr;
	Elf32_Ehdr *eheader = (Elf32_Ehdr*)code;
	Elf32_Phdr *pheader = NULL;

	vassert(p->textPages == 0 && p->dataPages == 0,"Process is not empty");

	/* check magic */
	if(*(u32*)eheader->e_ident != *(u32*)ELFMAG)
		return ERR_INVALID_ELF_BINARY;

	/* load the LOAD segments. */
	datPtr = (u8 const*)(code + eheader->e_phoff);
	for(j = 0; j < eheader->e_phnum; datPtr += eheader->e_phentsize, j++) {
		pheader = (Elf32_Phdr*)datPtr;
		/* check if all stuff is in the binary */
		if((u8*)pheader + sizeof(Elf32_Phdr) >= code + length)
			return ERR_INVALID_ELF_BINARY;

		if(pheader->p_type == PT_LOAD) {
			u32 pages;
			u8 const* segmentSrc;
			segmentSrc = code + pheader->p_offset;

			/* get to know the lowest virtual address. must be 0x0.  */
			if(seenLoadSegments == 0) {
				if(pheader->p_vaddr != 0)
					return ERR_INVALID_ELF_BINARY;
			}
			else if(seenLoadSegments == 2) {
				/* uh oh a third LOAD segment. that's not allowed
				* indeed */
				return ERR_INVALID_ELF_BINARY;
			}

			/* check if the sizes are valid */
			if(pheader->p_filesz > pheader->p_memsz)
				return ERR_INVALID_ELF_BINARY;
			if(pheader->p_vaddr + pheader->p_filesz >= (u32)(code + length))
				return ERR_INVALID_ELF_BINARY;

			/* Note that we put everything in the data-segment here atm */
			pages = BYTES_2_PAGES(pheader->p_memsz);
			if(seenLoadSegments != 0) {
				if(pheader->p_vaddr & (PAGE_SIZE - 1))
					pages++;
			}

			/* get more space for the data area and make sure that the segment-sizes are valid */
			if(!proc_segSizesValid(p->textPages,p->dataPages + pages,p->stackPages))
				return ERR_INVALID_ELF_BINARY;
			if(!proc_changeSize(pages,CHG_DATA))
				return ERR_INVALID_ELF_BINARY;

			/* copy the data, and zero remaining bytes */
			memcpy((void*)pheader->p_vaddr, (void*)segmentSrc, pheader->p_filesz);
			memclear((void*)(pheader->p_vaddr + pheader->p_filesz),pheader->p_memsz - pheader->p_filesz);
			seenLoadSegments++;
		}
	}

	return (u32)eheader->e_entry;
}
