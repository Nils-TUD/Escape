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

#include <esc/common.h>
#include <esc/fsinterface.h>
#include <esc/cmdargs.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FS_NAME_LEN	12

typedef struct {
	u16 type;
	char name[FS_NAME_LEN];
} sFSType;

static sFSType types[] = {
	{FS_TYPE_EXT2,"ext2"},
	{FS_TYPE_ISO9660,"iso9660"},
};

static void usage(const char *name) {
	u32 i;
	fprintf(stderr,"Usage: %s <device> <path> <type>\n",name);
	fprintf(stderr,"	Types: ");
	for(i = 0; i < ARRAY_SIZE(types); i++)
		fprintf(stderr,"%s ",types[i].name);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	char rpath[MAX_PATH_LEN];
	char rdev[MAX_PATH_LEN];
	char *path = NULL;
	char *dev = NULL;
	char *stype = NULL;
	u32 type,i;

	s32 res = ca_parse(argc,argv,CA_NO_FREE,"=s* =s* =s*",&dev,&path,&stype);
	if(res < 0) {
		fprintf(stderr,"Invalid arguments: %s\n",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	abspath(rdev,MAX_PATH_LEN,dev);
	abspath(rpath,MAX_PATH_LEN,path);

	for(i = 0; i < ARRAY_SIZE(types); i++) {
		if(strcmp(types[i].name,stype) == 0) {
			type = types[i].type;
			break;
		}
	}
	if(i >= ARRAY_SIZE(types))
		error("Unknown type '%s'",argv[3]);

	if(mount(rdev,rpath,type) < 0)
		error("Unable to mount '%s' @ '%s' with type %d",rdev,rpath,type);
	return EXIT_SUCCESS;
}
