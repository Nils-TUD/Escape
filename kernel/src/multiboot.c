/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/multiboot.h"
#include "../h/common.h"
#include "../h/paging.h"
#include "../h/video.h"

#define CHECK_FLAG(flags,bit) (flags & (1 << bit))

sMultiBoot *mb;

void mboot_init(sMultiBoot *mbp) {
	/* save the multiboot-structure
	 * (change to 0xC...0 since we get the address at 0x0...0 from GRUB) */
	mb = (sMultiBoot*)((u32)mbp | KERNEL_AREA_V_ADDR);

	/* change the address of the pointers in the structure, too */
	mb->cmdLine = (s8*)((u32)mb->cmdLine | KERNEL_AREA_V_ADDR);
	mb->modsAddr = (sModule*)((u32)mb->modsAddr | KERNEL_AREA_V_ADDR);
	mb->mmapAddr = (sMemMap*)((u32)mb->mmapAddr | KERNEL_AREA_V_ADDR);
}

u32 mboot_getUsableMemCount(void) {
	sMemMap *mmap;
	u32 size = 0;
	for(mmap = mb->mmapAddr;
		(u32)mmap < (u32)mb->mmapAddr + mb->mmapLength;
		mmap = (sMemMap*)((u32)mmap + mmap->size + sizeof(mmap->size))) {
		if(mmap != NULL && mmap->type == MMAP_TYPE_AVAILABLE)
			size += mmap->length;
	}
	return size;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void mboot_dbg_print(void) {
	u32 x;
	sMemMap *mmap;
	vid_printf("MultiBoot-Structure:\n---------------------\nflags=0x%x\n",mb->flags);
	if(CHECK_FLAG(mb->flags,0)) {
		vid_printf("memLower=%d KB, memUpper=%d KB\n",mb->memLower,mb->memUpper);
	}
	if(CHECK_FLAG(mb->flags,1)) {
		vid_printf("biosDriveNumber=%d, part1=%d, part2=%d, part3=%d\n",mb->bootDevice.drive,
			mb->bootDevice.partition1,mb->bootDevice.partition2,mb->bootDevice.partition3);
	}
	if(CHECK_FLAG(mb->flags,2)) {
		vid_printf("cmdLine=%s\n",mb->cmdLine);
	}
	if(CHECK_FLAG(mb->flags,3)) {
		vid_printf("modsCount=%d, modsAddr=0x%x\n",mb->modsCount,mb->modsAddr);
	}
	if(CHECK_FLAG(mb->flags,4)) {
		vid_printf("a.out: tabSize=%d, strSize=%d, addr=0x%x\n",mb->syms.aDotOut.tabSize,mb->syms.aDotOut.strSize,
			mb->syms.aDotOut.addr);
	}
	else if(CHECK_FLAG(mb->flags,5)) {
		vid_printf("ELF: num=%d, size=%d, addr=0x%x, shndx=0x%x\n",mb->syms.ELF.num,mb->syms.ELF.size,
			mb->syms.ELF.addr,mb->syms.ELF.shndx);
	}
	if(CHECK_FLAG(mb->flags,6)) {
		vid_printf("mmapLength=%d, mmapAddr=0x%x\n",mb->mmapLength,mb->mmapAddr);
		vid_printf("Available memory:\n");
		x = 0;
		for(mmap = (sMemMap*)mb->mmapAddr;
			(u32)mmap < (u32)mb->mmapAddr + mb->mmapLength;
			mmap = (sMemMap*)((u32)mmap + mmap->size + sizeof(mmap->size))) {
			if(mmap != NULL && mmap->type == MMAP_TYPE_AVAILABLE) {
				vid_printf("\t%d: addr=0x%08x, size=0x%08x, type=0x%08x\n",
						x,(u32)mmap->baseAddr,(u32)mmap->length,mmap->type);
				x++;
			}
		}
	}

	vid_printf("---------------------\n");
}

#endif
