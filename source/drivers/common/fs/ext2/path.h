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
 * Resolves the given path to the inode-number
 *
 * @param e the ext2-handle
 * @param u the user
 * @param path the path
 * @param flags the flags with which to open the file
 * @param dev should be set to the device-number
 * @param resLastMnt whether mount-points should be resolved if the path is finished
 * @return the inode-Number or EXT2_BAD_INO
 */
inode_t ext2_path_resolve(sExt2 *e,sFSUser *u,const char *path,uint flags,dev_t *dev,
		bool resLastMnt);
