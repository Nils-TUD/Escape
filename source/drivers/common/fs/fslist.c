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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ext2/ext2.h"
#include "iso9660/iso9660.h"
#include "mount.h"
#include "fslist.h"

#define FS_NAME_LEN		12

typedef struct {
	int type;
	char name[FS_NAME_LEN];
	sFileSystem *(*fGetFS)(void);
} sFSType;

static sFSType types[] = {
	{FS_TYPE_EXT2,		"ext2",		ext2_getFS},
	{FS_TYPE_ISO9660,	"iso9660",	iso_getFS},
};

void fslist_init(void) {
	size_t i;
	for(i = 0; i < ARRAY_SIZE(types); i++) {
		sFileSystem *fs = types[i].fGetFS();
		if(!fs)
			error("Unable to get %s-filesystem",types[i].name);
		if(mount_addFS(fs) != 0)
			error("Unable to add %s-filesystem",types[i].name);
		printf("[FS] Loaded %s-driver\n",types[i].name);
	}
}

int fslist_getType(const char *name) {
	size_t i;
	for(i = 0; i < ARRAY_SIZE(types); i++) {
		if(strcmp(types[i].name,name) == 0)
			return types[i].type;
	}
	return -1;
}

const char *fslist_getName(int type) {
	size_t i;
	for(i = 0; i < ARRAY_SIZE(types); i++) {
		if(type == types[i].type)
			return types[i].name;
	}
	return NULL;
}
