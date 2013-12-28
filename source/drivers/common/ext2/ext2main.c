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

#include <esc/common.h>
#include <esc/fsinterface.h>
#include <fs/fsdev.h>
#include <fs/mount.h>
#include <stdio.h>
#include <stdlib.h>

#include "ext2.h"

int main(int argc,char *argv[]) {
	char fspath[MAX_PATH_LEN];
	if(argc != 3)
		error("Usage: %s <wait> <devicePath>",argv[0]);

	/* build fs device name */
	char *dev = strrchr(argv[2],'/');
	if(!dev)
		error("Invalid device path '%s'",argv[2]);
	snprintf(fspath,sizeof(fspath),"/dev/ext2-%s",dev + 1);

	printf("[ext2] Mounting '%s' at '/'\n",argv[2]);
	fflush(stdout);

	mount_init();
	mount_addFS(ext2_getFS());
	return fs_driverLoop(argv[2],fspath,FS_TYPE_EXT2);
}
