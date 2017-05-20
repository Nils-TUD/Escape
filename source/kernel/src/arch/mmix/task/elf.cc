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

#include <mem/cache.h>
#include <mem/pagedir.h>
#include <mem/useraccess.h>
#include <mem/virtmem.h>
#include <task/elf.h>
#include <task/proc.h>
#include <task/thread.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <common.h>
#include <cpu.h>
#include <errno.h>
#include <log.h>
#include <string.h>
#include <video.h>

static int doFinish(Thread *t,const sElfEHeader *eheader,const sElfSHeader *headers,
		OpenFile *file,ELF::StartupInfo *info);

int ELF::finish(OpenFile *file,const sElfEHeader *eheader,StartupInfo *info) {
	int res = -ENOEXEC;
	Thread *t = Thread::getRunning();
	ssize_t readRes,headerSize = eheader->e_shnum * eheader->e_shentsize;
	sElfSHeader *secHeaders = (sElfSHeader*)Cache::alloc(headerSize);
	if(secHeaders == NULL) {
		Log::get().writef("[LOADER] Unable to allocate memory for ELF-header (%zu bytes)\n",headerSize);
		return -ENOEXEC;
	}

	if(file->seek(eheader->e_shoff,SEEK_SET) < 0) {
		Log::get().writef("[LOADER] Unable to seek to ELF-header\n");
		goto error;
	}

	if((readRes = file->read(t->getProc()->getPid(),secHeaders,headerSize)) != headerSize) {
		Log::get().writef("[LOADER] Unable to read ELF-header: %s\n",strerror(-readRes));
		goto error;
	}

	/* elf_finish might segfault */
	res = doFinish(t,eheader,secHeaders,file,info);

error:
	Cache::free(secHeaders);
	return res;
}

static int doFinish(Thread *t,const sElfEHeader *eheader,const sElfSHeader *headers,
		OpenFile *file,ELF::StartupInfo *info) {
	/* build register-stack */
	int globalNum = 0;
	ulong *stack;
	if(!t->getStackRange((uintptr_t*)&stack,NULL,0))
		return -ENOMEM;

	/* $0 .. $10 */
	for(int i = 0; i <= 10; ++i)
		UserAccess::writeVar(stack++,(ulong)0);
	/* rL = 11 */
	UserAccess::writeVar(stack++,(ulong)11);

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
				if((res = file->seek(sheader->sh_offset,SEEK_SET)) < 0) {
					Log::get().writef("[LOADER] Unable to seek to reg-section: %s\n",strerror(res));
					return res;
				}
				if((res = file->read(t->getProc()->getPid(),stack,sheader->sh_size)) !=
						(ssize_t)sheader->sh_size) {
					Log::get().writef("[LOADER] Unable to read reg-section: %s\n",strerror(res));
					return res;
				}
			}
			else
				UserAccess::write(stack,(void*)((uintptr_t)eheader + sheader->sh_offset),sheader->sh_size);
			stack += sheader->sh_size / sizeof(ulong);
			globalNum = sheader->sh_size / sizeof(ulong);
			break;
		}
	}

	/* $255 */
	UserAccess::writeVar(stack++,(ulong)0);
	/* 12 slots for special registers */
	for(int i = 0; i < 12; ++i)
		UserAccess::writeVar(stack++,(ulong)0);
	/* set rG|rA */
	UserAccess::writeVar(stack,(ulong)(255 - globalNum) << 56);
	info->stackBegin = (uintptr_t)stack;
	if(t->isFaulted())
		return -EFAULT;
	return 0;
}
