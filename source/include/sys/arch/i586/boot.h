/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#ifndef I586_BOOT_H_
#define I586_BOOT_H_

#include <sys/common.h>
#include <sys/intrpt.h>

#define MMAP_TYPE_AVAILABLE 0x1

typedef struct {
	uint32_t modStart;
	uint32_t modEnd;
	char *name;						/* may be 0 */
	uint32_t reserved;				/* should be ignored */
} A_PACKED sModule;

typedef struct {
	uint32_t size;
	uint64_t baseAddr;
	uint64_t length;
	uint32_t type;
} A_PACKED sMemMap;

typedef struct {
	uint16_t version;
	uint16_t cseg;
	uint16_t offset;
	uint16_t cseg16;
	uint16_t dseg;
	uint16_t flags;
	uint16_t csegLen;
	uint16_t cseg16Len;
	uint16_t dsegLen;
} A_PACKED sAPMTable;

typedef struct {
	uint32_t size;
	uint8_t number;
	uint8_t mode;
	uint16_t cylinders;
	uint8_t heads;
	uint8_t sectors;
	uint8_t ports[];
} A_PACKED sDrive;

/* multi-boot-information */
typedef struct {
	uint32_t flags;
	uint32_t memLower;				/* present if flags[0] is set */
	uint32_t memUpper;				/* present if flags[0] is set */
	struct {
		uint8_t partition3;			/* sub-sub-partition (0xFF = not used) */
		uint8_t partition2;			/* sub-partition (0xFF = not used) */
		uint8_t partition1;			/* top-level partition-number (0xFF = not used) */
		uint8_t drive;				/* contains the bios drive number as understood by the bios
								   	   INT 0x13 low-level disk interface: e.g. 0x00 for the first
								   	   floppy disk or 0x80 for the first hard disk */
	} A_PACKED bootDevice;			/* present if flags[1] is set */
	char *cmdLine;					/* present if flags[2] is set */
	uint32_t modsCount;				/* present if flags[3] is set */
	sModule *modsAddr;				/* present if flags[3] is set */
	union {
		struct {
			uint32_t tabSize;
			uint32_t strSize;
			uint32_t addr;
			uint32_t reserved;
		} A_PACKED aDotOut;			/* present if flags[4] is set */
		struct {
			uint32_t num;
			uint32_t size;
			uint32_t addr;
			uint32_t shndx;
		} A_PACKED ELF;				/* present if flags[5] is set */
	} syms;
	uint32_t mmapLength;			/* present if flags[6] is set */
	sMemMap *mmapAddr;				/* present if flags[6] is set */
	uint32_t drivesLength;			/* present if flags[7] is set */
	sDrive *drivesAddr;				/* present if flags[7] is set */
#if 0
	uint32_t configTable;			/* present if flags[8] is set */
	char *bootLoaderName;			/* present if flags[9] is set */
	sAPMTable *apmTable;			/* present if flags[10] is set */
#endif
} A_PACKED sBootInfo;

/**
 * Performs the architecture-dependend stuff at the beginning of the boot-process
 *
 * @param info the boot-information
 */
void boot_arch_start(sBootInfo *info);

/**
 * @return the address for the mm-stack
 */
uintptr_t boot_getMMStackBegin(void);

/**
 * @return the multiboot-info-structure
 */
const sBootInfo *boot_getInfo(void);

/**
 * The boot-tasks to load
 */
sBootTaskList bootTaskList;

#endif /* I586_BOOT_H_ */
