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
#include <task/proc.h>
#include <mem/paging.h>
#include <mem/kheap.h>
#include <mem/sharedmem.h>
#include <syscalls/mem.h>
#include <syscalls.h>
#include <errors.h>

void sysc_changeSize(sIntrptStackFrame *stack) {
	s32 count = SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	u32 oldEnd = p->textPages + p->dataPages;

	/* check argument */
	if(count > 0) {
		if(!proc_segSizesValid(p->textPages,p->dataPages + count,p->stackPages))
			SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	}
	else if(count == 0) {
		SYSC_RET1(stack,oldEnd);
		return;
	}
	else if((u32)-count > p->dataPages)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* change size */
	if(proc_changeSize(count,CHG_DATA) == false)
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);

	SYSC_RET1(stack,oldEnd);
}

void sysc_mapPhysical(sIntrptStackFrame *stack) {
	u32 addr,origPages;
	u32 phys = SYSC_ARG1(stack);
	u32 pages = BYTES_2_PAGES(SYSC_ARG2(stack));
	sProc *p = proc_getRunning();
	u32 i,*frames;

	/* trying to map memory in kernel area? */
	/* TODO is this ok? */
	if(OVERLAPS(phys,phys + pages,KERNEL_P_ADDR,KERNEL_P_ADDR + PAGE_SIZE * PT_ENTRY_COUNT))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* determine start-address */
	addr = (p->textPages + p->dataPages) * PAGE_SIZE;
	origPages = p->textPages + p->dataPages;

	/* not enough mem? */
	if(mm_getFreeFrmCount(MM_DEF) < (paging_countFramesForMap(addr,pages) - pages))
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);

	/* invalid segment sizes? */
	if(!proc_segSizesValid(p->textPages,p->dataPages + pages,p->stackPages))
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);

	/* we have to allocate temporary space for the frame-address :( */
	frames = (u32*)kheap_alloc(sizeof(u32) * pages);
	if(frames == NULL)
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);

	for(i = 0; i < pages; i++) {
		frames[i] = phys / PAGE_SIZE;
		phys += PAGE_SIZE;
	}
	/* map the physical memory and free the temporary memory */
	paging_map(addr,frames,pages,PG_WRITABLE | PG_NOFREE,false);
	kheap_free(frames);

	/* increase datapages */
	p->dataPages += pages;
	p->unswappable += pages;
	/* return start-addr */
	SYSC_RET1(stack,addr);
}

void sysc_createSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	u32 byteCount = SYSC_ARG2(stack);
	s32 res;

	if(!sysc_isStringReadable(name) || byteCount == 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_create(name,BYTES_2_PAGES(byteCount));
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

void sysc_joinSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_join(name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

void sysc_leaveSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_leave(name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_destroySharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_destroy(name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
