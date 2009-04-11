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

#include <esc/common.h>
#include "partition.h"

/* offset of partition-table in MBR */
#define PART_TABLE_OFFSET	0x1BE

/* a partition on the disk */
typedef struct {
	/* Boot indicator bit flag: 0 = no, 0x80 = bootable (or "active") */
	u8 bootable;
	/* start: Cylinder, Head, Sector */
	u8 startHead;
	u16 startSector : 6,
		startCylinder: 10;
	u8 systemId;
	/* end: Cylinder, Head, Sector */
	u8 endHead;
	u16 endSector : 6,
		endCylinder : 10;
	/* Relative Sector (to start of partition -- also equals the partition's starting LBA value) */
	u32 start;
	/* Total Sectors in partition */
	u32 size;
} __attribute__((packed)) sDiskPart;

void part_fillPartitions(sPartition *table,void *mbr) {
	u32 i;
	sDiskPart *src = (sDiskPart*)((u8*)mbr + PART_TABLE_OFFSET);
	for(i = 0; i < PARTITION_COUNT; i++) {
		table->present = src->systemId != 0;
		table->start = src->start;
		table->size = src->size;
		table++;
		src++;
	}
}
