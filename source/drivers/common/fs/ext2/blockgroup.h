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
 * Inits the block-groups, i.e. reads them from disk and stores them in the given ext2-handle
 *
 * @param e the ext2-handle
 * @return 0 on success
 */
int ext2_bg_init(sExt2 *e);

/**
 * Destroys the blockgroups
 */
void ext2_bg_destroy(sExt2 *e);

/**
 * Updates the block-group-descriptor-table, if it is dirty
 *
 * @param e the ext2-data
 */
void ext2_bg_update(sExt2 *e);

#if DEBUGGING

/**
 * Prints the given block-group
 *
 * @param e the ext2-data
 * @param no the block-group-number
 * @param bg the block-group
 */
void ext2_bg_print(sExt2 *e,block_t no,sExt2BlockGrp *bg);

#endif
