/**
 * $Id$
 */

#include <sys/common.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/vmm.h>
#include <sys/task/elf.h>
#include <sys/task/thread.h>
#include <sys/vfs/vfs.h>
#include <sys/cpu.h>
#include <sys/log.h>
#include <string.h>
#include <errors.h>

static int elf_finish(sThread *t,const sElfEHeader *eheader,const sElfSHeader *headers,
		tFileNo file,sStartupInfo *info);

int elf_finishFromMem(const void *code,size_t length,sStartupInfo *info) {
	UNUSED(length);
	sThread *t = thread_getRunning();
	sElfEHeader *eheader = (sElfEHeader*)code;

	/* at first, SYNCID the text-region */
	uintptr_t begin,start,end;
	vmm_getRegRange(t->proc,REG_TEXT,&start,&end);
	while(start < end) {
		tFrameNo frame = paging_getFrameNo(t->proc->pagedir,start);
		size_t amount = MIN(PAGE_SIZE,end - start);
		begin = DIR_MAPPED_SPACE | frame * PAGE_SIZE;
		cpu_syncid(begin,begin + amount);
		start += amount;
	}

	return elf_finish(t,eheader,(sElfSHeader*)((uintptr_t)code + eheader->e_shoff),-1,info);
}

int elf_finishFromFile(tFileNo file,const sElfEHeader *eheader,sStartupInfo *info) {
	int res = 0;
	sThread *t = thread_getRunning();
	ssize_t readRes,headerSize = eheader->e_shnum * eheader->e_shentsize;
	sElfSHeader *secHeaders = (sElfSHeader*)cache_alloc(headerSize);
	if(secHeaders == NULL) {
		log_printf("[LOADER] Unable to allocate memory for ELF-header (%zu bytes)\n",headerSize);
		return ERR_NOT_ENOUGH_MEM;
	}

	if(vfs_seek(t->proc->pid,file,eheader->e_shoff,SEEK_SET) < 0) {
		log_printf("[LOADER] Unable to seek to ELF-header\n");
		return ERR_INVALID_ELF_BIN;
	}

	if((readRes = vfs_readFile(t->proc->pid,file,secHeaders,headerSize)) != headerSize) {
		log_printf("[LOADER] Unable to read ELF-header: %s\n",strerror(readRes));
		cache_free(secHeaders);
		return ERR_INVALID_ELF_BIN;
	}

	res = elf_finish(t,eheader,secHeaders,file,info);
	cache_free(secHeaders);
	return res;
}

static int elf_finish(sThread *t,const sElfEHeader *eheader,const sElfSHeader *headers,
		tFileNo file,sStartupInfo *info) {
	/* build register-stack */
	uintptr_t end;
	int globalNum = 0;
	size_t j;
	ssize_t res;
	uint64_t *stack;
	vmm_getRegRange(t->proc,t->stackRegions[0],(uintptr_t*)&stack,&end);
	/* make writeable (if copy-on-write is activated, vmm_pagefault() is called to resolve it */
	paging_isRangeUserWritable((uintptr_t)stack,(uintptr_t)stack + PAGE_SIZE);
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
	for(j = 0; j < eheader->e_shnum; datPtr += eheader->e_shentsize, j++) {
		sElfSHeader *sheader = (sElfSHeader*)datPtr;
		/* the first section with type == PROGBITS, addr != 0 and flags = W
		 * should be .MMIX.reg_contents */
		if(sheader->sh_type == SHT_PROGBITS && sheader->sh_flags == SHF_WRITE &&
				sheader->sh_addr != 0) {
			/* append global registers */
			if(file >= 0) {
				if((res = vfs_seek(t->proc->pid,file,sheader->sh_offset,SEEK_SET)) < 0) {
					log_printf("[LOADER] Unable to seek to reg-section: %s\n",strerror(res));
					return res;
				}
				if((res = vfs_readFile(t->proc->pid,file,stack,sheader->sh_size)) !=
						(ssize_t)sheader->sh_size) {
					log_printf("[LOADER] Unable to read reg-section: %s\n",strerror(res));
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
