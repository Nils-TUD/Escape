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
#include <video.h>
#include <string.h>

u32 elf_loadprog(u8 *code,u32 length) {
	u32 seenLoadSegments = 0;
	sProc *p = proc_getRunning();

	u32 j;
	u8 const *datPtr;
	Elf32_Ehdr *eheader = (Elf32_Ehdr*)code;
	Elf32_Phdr *pheader = NULL;

	/* check magic */
	if(*(u32*)eheader->e_ident != *(u32*)ELFMAG) {
		vid_printf("Error: Invalid magic-number\n");
		return ELF_INVALID_ENTRYPOINT;
	}

	p->textPages = 0;
	p->dataPages = 0;

	/* load the LOAD segments. */
	datPtr = (u8 const*)(code + eheader->e_phoff);
	for(j = 0; j < eheader->e_phnum; datPtr += eheader->e_phentsize, j++) {
		pheader = (Elf32_Phdr*)datPtr;
		/* check if all stuff is in the binary */
		if((u8*)pheader + sizeof(Elf32_Phdr) >= code + length)
			return ELF_INVALID_ENTRYPOINT;

		if(pheader->p_type == PT_LOAD) {
			u32 pages;
			u8 const* segmentSrc;
			segmentSrc = code + pheader->p_offset;

			/* get to know the lowest virtual address. must be 0x0.  */
			if(seenLoadSegments == 0) {
				if(pheader->p_vaddr != 0) {
					vid_printf("Error: p_vaddr != 0\n");
					return ELF_INVALID_ENTRYPOINT;
				}
			}
			else if(seenLoadSegments == 2) {
				/* uh oh a third LOAD segment. that's not allowed
				* indeed */
				vid_printf("Error: too many LOAD segments\n");
				return ELF_INVALID_ENTRYPOINT;
			}

			/* check if the sizes are valid */
			if(pheader->p_filesz > pheader->p_memsz)
				return ELF_INVALID_ENTRYPOINT;
			if(pheader->p_vaddr + pheader->p_filesz >= (u32)(code + length))
				return ELF_INVALID_ENTRYPOINT;

			/* Note that we put everything in the data-segment here atm */
			pages = BYTES_2_PAGES(pheader->p_memsz);
			if(seenLoadSegments != 0) {
				if(pheader->p_vaddr & (PAGE_SIZE - 1))
					pages++;
			}

			/* get more space for the data area and make sure that the segment-sizes are valid */
			if(!proc_segSizesValid(p->textPages,p->dataPages + pages,p->stackPages))
				return ELF_INVALID_ENTRYPOINT;
			if(!proc_changeSize(pages,CHG_DATA))
				return ELF_INVALID_ENTRYPOINT;

			/* copy the data, and zero remaining bytes */
			memcpy((void*)pheader->p_vaddr, (void*)segmentSrc, pheader->p_filesz);
			memset((void*)(pheader->p_vaddr + pheader->p_filesz), 0, pheader->p_memsz - pheader->p_filesz);
			++seenLoadSegments;
		}
	}

	return (u32)eheader->e_entry;
}
