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

#ifndef MULTIBOOT_H_
#define MULTIBOOT_H_

#include <sys/common.h>
#include <sys/machine/intrpt.h>

#define MMAP_TYPE_AVAILABLE 0x1

typedef struct {
	u32 modStart;
	u32 modEnd;
	char *name;					/* may be 0 */
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

typedef struct {
	u32 size;
	u8 number;
	u8 mode;
	u16 cylinders;
	u8 heads;
	u8 sectors;
	u8 ports[];
} sDrive;

/* multi-boot-information */
typedef struct {
	u32 flags;
	u32 memLower;				/* present if flags[0] is set */
	u32 memUpper;				/* present if flags[0] is set */
	struct {
		u8 partition3;			/* sub-sub-partition (0xFF = not used) */
		u8 partition2;			/* sub-partition (0xFF = not used) */
		u8 partition1;			/* top-level partition-number (0xFF = not used) */
		u8 drive;				/* contains the bios drive number as understood by the bios
								   INT 0x13 low-level disk interface: e.g. 0x00 for the first
								   floppy disk or 0x80 for the first hard disk */
	} bootDevice;				/* present if flags[1] is set */
	char *cmdLine;				/* present if flags[2] is set */
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
	u32 drivesLength;			/* present if flags[7] is set */
	sDrive *drivesAddr;			/* present if flags[7] is set */
#if 0
	u32 configTable;			/* present if flags[8] is set */
	char *bootLoaderName;			/* present if flags[9] is set */
	sAPMTable *apmTable;		/* present if flags[10] is set */
#endif
} sMultiBoot;

/**
 * Inits the multi-boot infos
 *
 * @param mbp the pointer to the multi-boot-structure
 */
void mboot_init(sMultiBoot *mbp);

/**
 * @return the multiboot-info-structure
 */
const sMultiBoot *mboot_getInfo(void);

/**
 * @return size of the kernel (in bytes)
 */
u32 mboot_getKernelSize(void);

/**
 * @return size of the multiboot-modules (in bytes)
 */
u32 mboot_getModuleSize(void);

/**
 * Loads all multiboot-modules
 *
 * @param stack the interrupt-stack-frame
 */
void mboot_loadModules(sIntrptStackFrame *stack);

/**
 * @return the usable memory in bytes
 */
u32 mboot_getUsableMemCount(void);

#if DEBUGGING

/**
 * Prints all interesting elements of the multi-boot-structure
 */
void mboot_dbg_print(void);

#endif

#endif

