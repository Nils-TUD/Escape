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

#ifndef BLOCKGROUP_H_
#define BLOCKGROUP_H_

#include <esc/common.h>
#include "ext2.h"


#if DEBUGGING

/**
 * Prints the given block-group
 *
 * @param e the ext2-data
 * @param no the block-group-number
 * @param bg the block-group
 */
void ext2_dbg_printBlockGroup(sExt2 *e,u32 no,sBlockGroup *bg);

#endif

#endif /* BLOCKGROUP_H_ */
