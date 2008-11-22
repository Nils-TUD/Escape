/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef MULTIBOOT_H_
#define MULTIBOOT_H_

#include "../h/common.h"

#define MMAP_TYPE_AVAILABLE 0x1

typedef struct {
	u32 modStart;
	u32 modEnd;
	s8 *name;					/* may be 0 */
	u32 reserved;				/* should be ignored */
} sModule;

typedef struct {
	u32 size;
	u64 baseAddr;
	u64 length;
	u32 type;
} sMemMap;

typedef struct {
	u16 version;
	u16 cseg;
	u16 offset;
	u16 cseg16;
	u16 dseg;
	u16 flags;
	u16 csegLen;
	u16 cseg16Len;
	u16 dsegLen;
} sAPMTable;

/* multi-boot-information */
typedef struct {
	u32 flags;
	u32 memLower;				/* present if flags[0] is set */
	u32 memUpper;				/* present if flags[0] is set */
	struct {
		s8 partition3;			/* sub-sub-partition (0xFF = not used) */
		s8 partition2;			/* sub-partition (0xFF = not used) */
		s8 partition1;			/* top-level partition-number (0xFF = not used) */
		s8 drive;				/* contains the bios drive number as understood by the bios
								   INT 0x13 low-level disk interface: e.g. 0x00 for the first
								   floppy disk or 0x80 for the first hard disk */
	} bootDevice;				/* present if flags[1] is set */
	s8 *cmdLine;				/* present if flags[2] is set */
	u32 modsCount;				/* present if flags[3] is set */
	sModule *modsAddr;			/* present if flags[3] is set */
	union {
		struct {
			u32 tabSize;
			u32 strSize;
			u32 addr;
			u32 reserved;
		} aDotOut;				/* present if flags[4] is set */
		struct {
			u32 num;
			u32 size;
			u32 addr;
			u32 shndx;
		} ELF;					/* present if flags[5] is set */
	} syms;
	u32 mmapLength;				/* present if flags[6] is set */
	sMemMap *mmapAddr;			/* present if flags[6] is set */
#if 0
	u32 drivesLength;			/* present if flags[7] is set */
	tDrive *drivesAddr;			/* present if flags[7] is set */
	u32 configTable;			/* present if flags[8] is set */
	s8 *bootLoaderName;			/* present if flags[9] is set */
	sAPMTable *apmTable;		/* present if flags[10] is set */
#endif
} sMultiBoot;

/* the multiboot-information */
extern sMultiBoot *mb;

/**
 * Inits the multi-boot infos
 * 
 * @param mbp the pointer to the multi-boot-structure
 */
void mboot_init(sMultiBoot *mbp);

/**
 * Prints all interesting elements of the multi-boot-structure
 */
void printMultiBootInfo(void);

#endif

