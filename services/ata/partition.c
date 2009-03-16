/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
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
	sDiskPart *src = (u8*)mbr + PART_TABLE_OFFSET;
	for(i = 0; i < PARTITION_COUNT; i++) {
		table->present = src->systemId != 0;
		table->start = src->start;
		table->size = src->size;
		table++;
		src++;
	}
}
