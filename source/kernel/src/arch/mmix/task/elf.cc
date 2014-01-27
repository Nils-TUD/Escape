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
#include <sys/mem/pagedir.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/task/elf.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/cpu.h>
#include <sys/video.h>
#include <sys/log.h>
#include <string.h>
#include <errno.h>

static int finish(Thread *t,const sElfEHeader *eheader,const sElfSHeader *headers,
		OpenFile *file,ELF::StartupInfo *info);

int ELF::finishFromMem(const void *code,A_UNUSED size_t length,StartupInfo *info) {
	Thread *t = Thread::getRunning();
	sElfEHeader *eheader = (sElfEHeader*)code;

	/* at first, SYNCID the text-region */
	VMRegion *textreg = t->getProc()->getVM()->getRegion(eheader->e_entry);
	if(textreg) {
		uintptr_t begin,start,end;
		t->getProc()->getVM()->getRegRange(textreg,&start,&end,true);
		while(start < end) {
			frameno_t frame = t->getProc()->getPageDir()->getFrameNo(start);
			size_t amount = MIN(PAGE_SIZE,end - start);
			begin = DIR_MAPPED_SPACE | frame * PAGE_SIZE;
			CPU::syncid(begin,begin + amount);
			start += amount;
		}
	}
	return finish(t,eheader,(sElfSHeader*)((uintptr_t)code + eheader->e_shoff),NULL,info);
}

int ELF::finishFromFile(OpenFile *file,const sElfEHeader *eheader,StartupInfo *info) {
	int res = -ENOEXEC;
	Thread *t = Thread::getRunning();
	ssize_t readRes,headerSize = eheader->e_shnum * eheader->e_shentsize;
	sElfSHeader *secHeaders = (sElfSHeader*)Cache::alloc(headerSize);
	if(secHeaders == NULL) {
		Log::get().writef("[LOADER] Unable to allocate memory for ELF-header (%zu bytes)\n",headerSize);
		return -ENOEXEC;
	}

	Thread::addHeapAlloc(secHeaders);
	if(file->seek(t->getProc()->getPid(),eheader->e_shoff,SEEK_SET) < 0) {
		Log::get().writef("[LOADER] Unable to seek to ELF-header\n");
		goto error;
	}

	if((readRes = file->read(t->getProc()->getPid(),secHeaders,headerSize)) != headerSize) {
		Log::get().writef("[LOADER] Unable to read ELF-header: %s\n",strerror(-readRes));
		goto error;
	}

	/* elf_finish might segfault */
	res = finish(t,eheader,secHeaders,file,info);

error:
	Thread::remHeapAlloc(secHeaders);
	Cache::free(secHeaders);
	return res;
}

static int finish(Thread *t,const sElfEHeader *eheader,const sElfSHeader *headers,
		OpenFile *file,ELF::StartupInfo *info) {
	/* build register-stack */
	int globalNum = 0;
	uint64_t *stack;
	if(!t->getStackRange((uintptr_t*)&stack,NULL,0))
		return -ENOMEM;
	*stack++ = 0;	/* $0 */
	*stack++ = 0;	/* $1 */
	*stack++ = 0;	/* $2 */
	*stack++ = 0;	/* $3 */
	*stack++ = 0;	/* $4 */
	*stack++ = 0;	/* $5 */
	*stack++ = 0;	/* $6 */
	*stack++ = 7;	/* rL = 7 */

	/* load the regs-section */
	uintptr_t datPtr = (uintptr_t)headers;
	for(size_t j = 0; j < eheader->e_shnum; datPtr += eheader->e_shentsize, j++) {
		sElfSHeader *sheader = (sElfSHeader*)datPtr;
		/* the first section with type == PROGBITS, addr != 0 and flags = W
		 * should be .MMIX.reg_contents */
		if(sheader->sh_type == SHT_PROGBITS && sheader->sh_flags == SHF_WRITE &&
				sheader->sh_addr != 0) {
			/* append global registers */
			if(file != NULL) {
				ssize_t res;
				if((res = file->seek(t->getProc()->getPid(),sheader->sh_offset,SEEK_SET)) < 0) {
					Log::get().writef("[LOADER] Unable to seek to reg-section: %s\n",strerror(-res));
					return res;
				}
				if((res = file->read(t->getProc()->getPid(),stack,sheader->sh_size)) !=
						(ssize_t)sheader->sh_size) {
					Log::get().writef("[LOADER] Unable to read reg-section: %s\n",strerror(-res));
					return res;
				}
			}
			else
				memcpy(stack,(void*)((uintptr_t)eheader + sheader->sh_offset),sheader->sh_size);
			stack += sheader->sh_size / sizeof(uint64_t);
			globalNum = sheader->sh_size / sizeof(uint64_t);
			break;
		}
	}

	/* $255 */
	*stack++ = 0;
	/* 12 slots for special registers */
	memclear(stack,12 * sizeof(uint64_t));
	stack += 12;
	/* set rG|rA */
	*stack = (uint64_t)(255 - globalNum) << 56;
	info->stackBegin = (uintptr_t)stack;
	return 0;
}
