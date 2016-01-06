/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>
#include <sys/stat.h>
#include <usergroup/usergroup.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace fs {

class Tar {
public:
	enum {
		BLOCK_SIZE	= 512
	};

	enum FileType {
		T_REGULAR	= '0',
		T_HARDLINK	= '1',
		T_SYMLINK	= '2',
		T_CHARDEV	= '3',
		T_BLKDEV	= '4',
		T_DIR		= '5',
		T_FIFO		= '6',
		T_CONTFILE	= '7',	// contiguous file
		T_META		= '8', 	// global extended header with meta data
	};

	struct FileHeader {
		// Pre-POSIX.1-1988 tar header
		char filename[100];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char checksum[8];
		char type;
		char linkname[100];
		// ustar extension
		char ustar[6];
		char version[2];
		char uname[32];
		char gname[32];
		char major[8];
		char minor[8];
		char prefix[155];
	};

	static void readBlock(FILE *f,off_t offset,void *data,size_t size = BLOCK_SIZE) {
		if(fseek(f,offset,SEEK_SET) < 0)
			error("Unable to seek to %lu", offset);
		if(fread(data,size,1,f) != 1)
			error("Unable to read header from %lu", offset);
	}

	static void readHeader(FILE *f,off_t offset,FileHeader *header) {
		readBlock(f,offset,header,sizeof(*header));
	}

	static void writeBlock(FILE *f,off_t offset,const void *data) {
		if(fseek(f,offset,SEEK_SET) < 0)
			error("Unable to seek to %lu", offset);
		if(fwrite(data,BLOCK_SIZE,1,f) != 1)
			error("Unable to write block to %lu", offset);
	}

	static void writeHeader(FILE *f,off_t offset,void *buffer,const char *path,struct stat &st,
			sNamedItem *userList,sNamedItem *groupList) {
		Tar::FileHeader *head = (Tar::FileHeader*)buffer;

		/* fill file header */
		memset(head,0,Tar::BLOCK_SIZE);
		strncpy(head->filename,path,sizeof(head->filename));
		snprintf(head->mode,sizeof(head->mode),"%07o",st.st_mode & MODE_PERM);
		snprintf(head->uid,sizeof(head->uid),"%07o",st.st_uid);
		snprintf(head->gid,sizeof(head->gid),"%07o",st.st_gid);
		if(S_ISREG(st.st_mode))
			snprintf(head->size,sizeof(head->size),"%011lo",st.st_size);
		snprintf(head->mtime,sizeof(head->mtime),"%011o",st.st_mtime);

		if(S_ISDIR(st.st_mode))
			head->type = T_DIR;
		else if(S_ISREG(st.st_mode))
			head->type = T_REGULAR;
		else if(S_ISCHR(st.st_mode))
			head->type = T_CHARDEV;
		else if(S_ISBLK(st.st_mode))
			head->type = T_BLKDEV;
		else if(S_ISLNK(st.st_mode))
			head->type = T_SYMLINK;
		else
			error("Unsupported file type: %o",st.st_mode);

		strncpy(head->ustar,"ustar ",6);
		strncpy(head->version," ",1);
		sNamedItem *u = usergroup_getById(userList,st.st_uid);
		if(u)
			strncpy(head->uname,u->name,sizeof(head->uname));
		sNamedItem *g = usergroup_getById(groupList,st.st_gid);
		if(g)
			strncpy(head->gname,g->name,sizeof(head->gname));

		/* write file header */
		writeBlock(f,offset,buffer);
	}
};

}
