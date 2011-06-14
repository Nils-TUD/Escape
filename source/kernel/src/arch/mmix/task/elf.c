/**
 * $Id$
 */

#include <sys/common.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/vmm.h>
#include <sys/task/elf.h>
#include <sys/task/thread.h>
#include <sys/vfs/vfs.h>
#include <string.h>
#include <errors.h>

static int elf_finish(sThread *t,const sElfEHeader *eheader,const sElfSHeader *headers,
		tFileNo file,sStartupInfo *info);

int elf_finishFromMem(const void *code,size_t length,sStartupInfo *info) {
	UNUSED(length);
	sThread *t = thread_getRunning();
	sElfEHeader *eheader = (sElfEHeader*)code;
	return elf_finish(t,eheader,(sElfSHeader*)((uintptr_t)code + eheader->e_shoff),-1,info);
}

int elf_finishFromFile(tFileNo file,const sElfEHeader *eheader,sStartupInfo *info) {
	int res = 0;
	sThread *t = thread_getRunning();
	ssize_t headerSize = eheader->e_shnum * eheader->e_shentsize;
	sElfSHeader *secHeaders = (sElfSHeader*)kheap_alloc(headerSize);
	if(secHeaders == NULL)
		return ERR_NOT_ENOUGH_MEM;

	if(vfs_readFile(t->proc->pid,file,secHeaders,headerSize) != headerSize) {
		kheap_free(secHeaders);
		return ERR_INVALID_ELF_BIN;
	}

	res = elf_finish(t,eheader,secHeaders,file,info);
	kheap_free(secHeaders);
	return res;
}

static int elf_finish(sThread *t,const sElfEHeader *eheader,const sElfSHeader *headers,
		tFileNo file,sStartupInfo *info) {
	int globalNum = 0;
	size_t j;
	ssize_t res;
	uint64_t *stack;
	vmm_getRegRange(t->proc,t->stackRegions[0],(uintptr_t*)&stack,NULL);
	/* set rL to 1 */
	*stack++ = 0;

	/* load the regs-section */
	uintptr_t datPtr = (uintptr_t)headers;
	for(j = 0; j < eheader->e_shnum; datPtr += eheader->e_shentsize, j++) {
		sElfSHeader *sheader = (sElfSHeader*)datPtr;
		/* the first section with type == PROGBITS and addr != 0 should be .MMIX.reg_contents */
		if(sheader->sh_type == SHT_PROGBITS && sheader->sh_addr < eheader->e_entry &&
				sheader->sh_addr != 0) {
			/* append global registers */
			if(file >= 0) {
				if((res = vfs_seek(t->proc->pid,file,sheader->sh_offset,SEEK_SET)) < 0)
					return res;
				if((res = vfs_readFile(t->proc->pid,file,stack,sheader->sh_size)) != sheader->sh_size)
					return res;
			}
			else
				memcpy(stack,(void*)sheader->sh_offset,sheader->sh_size);
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
