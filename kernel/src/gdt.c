/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../pub/mm.h"
#include "../pub/video.h"
#include "../pub/string.h"
#include "../pub/paging.h"

#include "../priv/gdt.h"

/* the GDT */
static sGDTDesc gdt[GDT_ENTRY_COUNT];

/* our TSS (should not contain a page-boundary) */
static sTSS tss __attribute__((aligned (PAGE_SIZE)));

void gdt_init(void) {
	sGDTTable gdtTable;
	gdtTable.offset = (u32)gdt;
	gdtTable.size = GDT_ENTRY_COUNT * sizeof(sGDTDesc) - 1;

	/* clear gdt */
	memset(gdt,0,GDT_ENTRY_COUNT * sizeof(sGDTDesc));

	/* kernel code */
	gdt_set_desc(1,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_CODE | GDT_PRESENT | GDT_CODE_READ,GDT_DPL_KERNEL);
	/* kernel data */
	gdt_set_desc(2,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_DATA | GDT_PRESENT | GDT_DATA_WRITE,GDT_DPL_KERNEL);

	/* user code */
	gdt_set_desc(3,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_CODE | GDT_PRESENT | GDT_CODE_READ,GDT_DPL_USER);
	/* user data */
	gdt_set_desc(4,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_DATA | GDT_PRESENT | GDT_DATA_WRITE,GDT_DPL_USER);

	/* tss */
	tss.esp0 = KERNEL_STACK + PAGE_SIZE - 4;
	tss.ss0 = 0x10;
	gdt_set_tss_desc(5,(u32)&tss,sizeof(sTSS) - 1);

	/*int i;
	for(i = 0;i < GDT_ENTRY_COUNT; i++) {
		vid_printf("%d: addrLow=0x%x, addrMiddle=0x%x, addrHigh=0x%x",
				i,gdt[i].addrLow,gdt[i].addrMiddle,gdt[i].addrHigh);
		vid_printf(", sizeLow=0x%x, sizeHigh=0x%x, access=0x%x\n",
				gdt[i].sizeLow,gdt[i].sizeHigh,gdt[i].access);
	}*/

	/* now load the GDT */
	gdt_flush(&gdtTable);

	/* load tss */
	tss_load(5 * sizeof(sGDTDesc));

	/* We needed the area 0x0 .. 0x00400000 because in the first phase the GDT was setup so that
	 * the stuff at 0xC0000000 has been mapped to 0x00000000. Therefore, after enabling paging
	 * (the GDT is still the same) we have to map 0x00000000 to our kernel-stuff so that we can
	 * still access the kernel (because segmentation translates 0xC0000000 to 0x00000000 before
	 * it passes it to the MMU).
	 * Now our GDT is setup in the "right" way, so that 0xC0000000 will arrive at the MMU.
	 * Therefore we can unmap the 0x0 area. */
	paging_gdtFinished();
}

static void gdt_set_tss_desc(u16 index,u32 address,u32 size) {
	gdt[index].addrLow = address & 0xFFFF;
	gdt[index].addrMiddle = (address >> 16) & 0xFF;
	gdt[index].addrHigh = (address >> 24) & 0xFF;
	gdt[index].sizeLow = size & 0xFFFF;
	gdt[index].sizeHigh = ((size >> 16) & 0xF) | GDT_32BIT_PMODE;
	gdt[index].access = GDT_PRESENT | GDT_TYPE_32BIT_TSS | (GDT_DPL_KERNEL << 5);
}

static void gdt_set_desc(u16 index,u32 address,u32 size,u8 access,u8 ringLevel) {
	gdt[index].addrLow = address & 0xFFFF;
	gdt[index].addrMiddle = (address >> 16) & 0xFF;
	gdt[index].addrHigh = (address >> 24) & 0xFF;
	gdt[index].sizeLow = size & 0xFFFF;
	gdt[index].sizeHigh = ((size >> 16) & 0xF) | GDT_PAGE_GRANULARITY | GDT_32BIT_PMODE;
	gdt[index].access = access | ((ringLevel & 3) << 5);
}
