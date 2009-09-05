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

#ifndef LINK_H_
#define LINK_H_

#include <esc/common.h>
#include "ext2.h"

/**
 * Creates a entry for cnode->inodeNo+name in the given directory. Increases the link-count
 * for the given inode.
 *
 * @param e the ext2-data
 * @param dir the directory
 * @param cnode the cached inode
 * @param name the name
 * @return 0 on success
 */
s32 ext2_link_create(sExt2 *e,sExt2CInode *dir,sExt2CInode *cnode,const char *name);

/**
 * Removes the given name from the given directory
 *
 * @param e the ext2-data
 * @param dir the directory
 * @param name the entry-name
 * @return 0 on success
 */
s32 ext2_link_delete(sExt2 *e,sExt2CInode *dir,const char *name);

#endif /* LINK_H_ */
