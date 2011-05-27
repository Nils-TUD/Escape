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

#ifndef PARTITION_H_
#define PARTITION_H_

#include <esc/common.h>

/* the number of partitions per disk */
#define PARTITION_COUNT		4

/* represents a partition (in memory) */
typedef struct {
	uchar present;
	/* start sector */
	size_t start;
	/* sector count */
	size_t size;
} sPartition;

/**
 * Fills the partition-table with the given MBR
 *
 * @param table the table to fill
 * @param mbr the content of the first sector
 */
void part_fillPartitions(sPartition *table,void *mbr);

#endif /* PARTITION_H_ */
