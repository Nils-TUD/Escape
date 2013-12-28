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

#pragma once

#include <esc/common.h>
#include "ext2.h"

/**
 * Reads <secCount> sectors at <lba> into the given buffer
 *
 * @param e the ext2-handle
 * @param buffer the buffer
 * @param lba the start-sector
 * @param secCount the number of sectors
 * @return 0 if successfull
 */
int ext2_rw_readSectors(sExt2 *e,void *buffer,uint64_t lba,size_t secCount);

/**
 * Reads <blockCount> blocks at <start> into the given buffer
 *
 * @param e the ext2-handle
 * @param buffer the buffer
 * @param start the start-block
 * @param blockCount the number of blocks
 * @return 0 if successfull
 */
int ext2_rw_readBlocks(sExt2 *e,void *buffer,block_t start,size_t blockCount);

/**
 * Writes <secCount> sectors at <lba> from the given buffer
 *
 * @param e the ext2-handle
 * @param buffer the buffer
 * @param lba the start-sector
 * @param secCount the number of sectors
 * @return 0 if successfull
 */
int ext2_rw_writeSectors(sExt2 *e,const void *buffer,uint64_t lba,size_t secCount);

/**
 * Writes <blockCount> blocks at <start> from the given buffer
 *
 * @param e the ext2-handle
 * @param buffer the buffer
 * @param start the start-block
 * @param blockCount the number of blocks
 * @return 0 if successfull
 */
int ext2_rw_writeBlocks(sExt2 *e,const void *buffer,block_t start,size_t blockCount);
