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

#include <sys/common.h>
#include <stdio.h>
#include "partition.h"

/* offset of partition-table in MBR */
#define PART_TABLE_OFFSET	0x1BE

/* a partition on the disk */
typedef struct {
	/* Boot indicator bit flag: 0 = no, 0x80 = bootable (or "active") */
	uint8_t bootable;
	/* start: Cylinder, Head, Sector */
	uint8_t startHead;
	uint16_t startSector : 6,
		startCylinder: 10;
	uint8_t systemId;
	/* end: Cylinder, Head, Sector */
	uint8_t endHead;
	uint16_t endSector : 6,
		endCylinder : 10;
	/* Relative Sector (to start of partition -- also equals the partition's starting LBA value) */
	uint32_t start;
	/* Total Sectors in partition */
	uint32_t size;
} A_PACKED sDiskPart;

void part_fillPartitions(sPartition *table,void *mbr) {
	size_t i;
	sDiskPart *src = (sDiskPart*)((uintptr_t)mbr + PART_TABLE_OFFSET);
	for(i = 0; i < PARTITION_COUNT; i++) {
		table->present = src->systemId != 0;
		table->start = src->start;
		table->size = src->size;
		table++;
		src++;
	}
}

void part_print(sPartition *table) {
	size_t i;
	for(i = 0; i < PARTITION_COUNT; i++) {
		printf("%zu: present=%d start=%zu size=%zu\n",i,table->present,table->start,table->size);
		table++;
	}
}
