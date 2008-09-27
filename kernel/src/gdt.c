#include "../h/gdt.h"
#include "../h/mm.h"
#include "../h/video.h"
#include "../h/util.h"

/* the GDT descriptor */
typedef struct {
	u16 size;		/* the size of the table -1 (size=0 is not allowed) */
	u32 offset;
} __attribute__((packed)) tGDTDesc;

/* a GDT entry */
typedef struct {
	/* size[0..15] */
	u16 sizeLow;
	
	/* address[0..15] */
	u16 addrLow;
	
	/* address[16..23] */
	u8 addrMiddle;
	
	/* 
	 *     7      6   5     4     3    2      1        0
	 * |-------|----|----|------|----|---|---------|--------|
	 * |present|privilege|unused|exec|dir|readWrite|accessed|
	 * ------------------------------------------------------
	 * 
	 * present:		This must be 1 for all valid selectors.
	 * privilege:	Contains the ring level, 0 = highest (kernel), 3 = lowest (user applications).
	 * unused:		always 1
	 * exec:		If 1 code in this segment can be executed
	 * dir:			Direction bit for data selectors: Tells the direction. 0 the segment grows up.
	 * 				1 the segment grows down, ie. the offset has to be greater than the base. 
	 * 				Conforming bit for code selectors: Privilege-related.
	 * 				(TODO Please add more information)
	 * readWrite:	Readable bit for code selectors: Whether read access for this segment
	 * 				is allowed. Write access is never allowed for code segments.
	 * 				Writable bit for data selectors: Whether write access for this segment
	 * 				is allowed. Read access is always allowed for data segments.
	 * accessed:	Just set to 0. The CPU sets this to 1 when the segment is accessed.
	 */
	u8 access;
	
	/*
	 *       7       6   5   4  3  2  1  0
	 * |-----------|----|--|---|--|--|--|--|
	 * |granularity|size|unused|  sizeHigh |
	 * -------------------------------------
	 * 
	 * granularity:	If 0 the limit is in 1 B blocks (byte granularity), if 1 the limit is in
	 * 				4 KiB blocks (page granularity).
	 * size:		If 0 the selector defines 16 bit protected mode. If 1 it defines 32 bit
	 * 				protected mode. You can have both 16 bit and 32 bit selectors at once.
	 * unused:		always 0
	 * sizeHigh:	size[16..19]
	 */
	u8 sizeHigh;
	
	/* address[24..31] */
	u8 addrHigh;
} __attribute__((packed)) tGDTEntry;

/* we need 5 entries: null-entry, code for kernel, data for kernel, user-code and user-data */
#define GDT_ENTRY_COUNT 5

/* the GDT masks */
#define GDT_MASK_PRESENT		(1 << 7 | 1 << 4) /* ensure that the unused bit is 1 */
#define GDT_MASK_DPL			(1 << 5)
#define GDT_MASK_EXEC			(1 << 3)
#define GDT_MASK_DIR			(1 << 2)
#define GDT_MASK_READWRITE		(1 << 1)

/* privilege level */
#define GDT_DPL_KERNEL			0
#define GDT_DPL_USER			3

/**
 * Assembler routine to flush the GDT
 * 
 * @param gdt the pointer to the GDT-descriptor
 */
extern void gdt_flush(tGDTDesc *gdt);

/**
 * Sets the descriptor with given index to the given attributes
 * 
 * @param index the index of the descriptor
 * @param address the address of the segment
 * @param size the size of the segment (in pages)
 * @param access the access-byte
 * @param ringLevel the ring-level for the segment (0 = kernel, 3 = user)
 */
static void gdt_set_descriptor(u16 index,u32 address,u32 size,u8 access,u8 ringLevel);

/* the GDT */
static tGDTEntry gdt[GDT_ENTRY_COUNT];

void gdt_init(void) {
	tGDTDesc gdtDesc;
	gdtDesc.offset = (u32)gdt;
	gdtDesc.size = GDT_ENTRY_COUNT * 8 - 1;
	
	/* clear gdt */
	memset(gdt,0,GDT_ENTRY_COUNT * 2);
	
	/* kernel code */
	gdt_set_descriptor(1,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_MASK_PRESENT | GDT_MASK_EXEC | GDT_MASK_READWRITE,GDT_DPL_KERNEL);
	/* kernel data */
	gdt_set_descriptor(2,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_MASK_PRESENT | GDT_MASK_READWRITE,GDT_DPL_KERNEL);
	
	/* user code */
	gdt_set_descriptor(3,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_MASK_PRESENT | GDT_MASK_EXEC | GDT_MASK_READWRITE,GDT_DPL_USER);
	/* user data */
	gdt_set_descriptor(4,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_MASK_PRESENT | GDT_MASK_READWRITE,GDT_DPL_USER);
	
	/*int i;
	for(i = 0;i < GDT_ENTRY_COUNT; i++) {
		vid_printf("%d: addrLow=0x%x, addrMiddle=0x%x, addrHigh=0x%x",
				i,gdt[i].addrLow,gdt[i].addrMiddle,gdt[i].addrHigh);
		vid_printf(", sizeLow=0x%x, sizeHigh=0x%x, access=0x%x\n",
				gdt[i].sizeLow,gdt[i].sizeHigh,gdt[i].access);
	}*/
	
	/* now load the GDT */
	gdt_flush(&gdtDesc);
	
	/* We needed the area 0x0 .. 0x00400000 because in the first phase the GDT was setup so that
	 * the stuff at 0xC0000000 has been mapped to 0x00000000. Therefore, after enabling paging
	 * (the GDT is still the same) we have to map 0x00000000 to our kernel-stuff so that we can
	 * still access the kernel (because segmentation translates 0xC0000000 to 0x00000000 before
	 * it passes it to the MMU).
	 * Now our GDT is setup in the "right" way, so that 0xC0000000 will arrive at the MMU.
	 * Therefore we can unmap the 0x0 area. */
	paging_gdtFinished();
}

static void gdt_set_descriptor(u16 index,u32 address,u32 size,u8 access,u8 ringLevel) {
	gdt[index].addrLow = address & 0xFFFF;
	gdt[index].addrMiddle = (address >> 16) & 0xFF;
	gdt[index].addrHigh = (address >> 24) & 0xFF;
	gdt[index].sizeLow = size & 0xFFFF;
	/* page-granularity & 32-bit p-mode */
	gdt[index].sizeHigh = ((size >> 16) & 0xF) | 1 << 7 | 1 << 6;
	gdt[index].access = access | ((ringLevel & 3) << 5);
}
