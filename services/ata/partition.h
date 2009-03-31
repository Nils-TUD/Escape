/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PARTITION_H_
#define PARTITION_H_

#include <esc/common.h>

/* the number of partitions per disk */
#define PARTITION_COUNT		4

/* represents a partition (in memory) */
typedef struct {
	u8 present;
	/* start sector */
	u32 start;
	/* sector count */
	u32 size;
} sPartition;

/**
 * Fills the partition-table with the given MBR
 *
 * @param table the table to fill
 * @param mbr the content of the first sector
 */
void part_fillPartitions(sPartition *table,void *mbr);

#endif /* PARTITION_H_ */
