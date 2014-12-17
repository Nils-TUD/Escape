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

#include <sys/common.h>
#include <sys/stat.h>
#include <esc/cmdargs.h>
#include <usergroup/user.h>
#include <usergroup/group.h>
#include <iostream>
#include <stdlib.h>
#include <dirent.h>

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

static char buffer[BLOCK_SIZE];
static sUser *userList = nullptr;
static sGroup *groupList = nullptr;
static off_t curoff = 0;

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <cmd> [-f <archive>] [files...]\n",name);
	fprintf(stderr,"\n");
	fprintf(stderr,"The commands are:\n");
	fprintf(stderr,"-c: create an archive\n");
	fprintf(stderr,"-t: list files in archive\n");
	fprintf(stderr,"-x: extract archive\n");
	exit(EXIT_FAILURE);
}

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

static void addFolder(FILE *f,const std::string &fpath,const std::string &tpath) {
	struct stat st;
	if(stat(fpath.c_str(),&st) < 0)
		error("Unable to stat '%s'",fpath.c_str());

	// don't touch head afterwards since we're reusing buffer
	{
		FileHeader *head = (FileHeader*)buffer;

		/* fill file header */
		memset(head,0,BLOCK_SIZE);
		strncpy(head->filename,tpath.c_str(),sizeof(head->filename));
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
		sUser *u = user_getById(userList,st.st_uid);
		if(u)
			strncpy(head->uname,u->name,sizeof(head->uname));
		sGroup *g = group_getById(groupList,st.st_gid);
		if(g)
			strncpy(head->gname,g->name,sizeof(head->gname));

		/* write file header */
		writeBlock(f,curoff,buffer);
		curoff += BLOCK_SIZE;
	}

	if(S_ISDIR(st.st_mode)) {
		char nfpath[MAX_PATH_LEN];
		char ntpath[MAX_PATH_LEN];
		DIR *d = opendir(fpath.c_str());
		struct dirent e;
		while(readdir(d,&e)) {
			if(e.d_namelen == 1 && e.d_name[0] == '.')
				continue;
			if(e.d_namelen == 2 && e.d_name[0] == '.' && e.d_name[1] == '.')
				continue;

			snprintf(nfpath,sizeof(nfpath),"%s/%.*s",fpath.c_str(),e.d_namelen,e.d_name);
			snprintf(ntpath,sizeof(ntpath),"%s/%.*s",tpath.c_str(),e.d_namelen,e.d_name);
			addFolder(f,nfpath,ntpath);
		}
		closedir(d);
	}
	else if(S_ISREG(st.st_mode)) {
		/* write file-content */
		int fd = open(fpath.c_str(),O_RDONLY);
		if(fd < 0)
			error("Unable to open '%s' for reading",fpath.c_str());

		for(off_t off = 0; off < st.st_size; off += BLOCK_SIZE) {
			ssize_t count = read(fd,buffer,sizeof(buffer));
			if(count < 0)
				error("Reading from '%s' failed",fpath.c_str());
			memset(buffer + count,0,BLOCK_SIZE - count);
			writeBlock(f,curoff,buffer);
			curoff += BLOCK_SIZE;
		}

		close(fd);
	}
}

static void createDir(const char *path,mode_t mode,bool isdir) {
	char tmp[MAX_PATH_LEN];
	strncpy(tmp,path,sizeof(tmp));
	char *p = isdir ? tmp : dirname(tmp);
	while(*p) {
		while(*p && *p != '/')
			p++;
		char old = *p;
		*p = '\0';
		int res;
		// use the default mode for all but the last
		if((res = mkdir(tmp,old ? DIR_DEF_MODE : mode)) < 0 && res != -EEXIST)
			printe("Unable to create directory '%s'",path);
		if(old) {
			*p = old;
			p++;
		}
	}
}

static void create(FILE *f,off_t cur,const FileHeader *header) {
	// create parents
	mode_t mode = strtoul(header->mode,NULL,8);
	createDir(header->filename,
		header->type == T_DIR ? S_IFDIR | mode : DIR_DEF_MODE,header->type == T_DIR);

	switch(header->type) {
		case T_REGULAR: {
			int fd = create(header->filename,O_WRONLY | O_TRUNC | O_CREAT | O_EXCL,S_IFREG | mode);
			if(fd < 0) {
				printe("Unable to create file '%s'",header->filename);
				break;
			}

			off_t total = strtoul(header->size,NULL,8);
			for(off_t off = 0; total > 0; off += BLOCK_SIZE) {
				readBlock(f,cur + off,buffer);

				ssize_t count = total > BLOCK_SIZE ? (size_t)BLOCK_SIZE : total;
				if(write(fd,buffer,count) != count) {
					printe("Writing to '%s' failed",header->filename);
					break;
				}
				total -= count;
			}
			close(fd);
		}
		break;

		case T_DIR:
			// createDir has created it already
			// but repeat the chmod because we might have set the default mode last time
			if(chmod(header->filename,mode) < 0)
				printe("chmod(%s,%o) failed",header->filename,mode);
			break;

		default: {
			fprintf(stderr,"Warning: type %c not supported\n",header->type);
		}
		break;
	}

	if(*header->uname) {
		sUser *u = user_getByName(userList,header->uname);
		if(u && chown(header->filename,u->uid,-1) < 0)
			printe("chown(%s,%d,-1) failed",header->filename,u->uid);
	}
	if(*header->gname) {
		sGroup *g = group_getByName(groupList,header->gname);
		if(g && chown(header->filename,-1,g->gid) < 0)
			printe("chown(%s,-1,%d) failed",header->filename,g->gid);
	}

	/* TODO set file modification time */
}

static void listArchive(FILE *f) {
	FileHeader header;
	off_t total = lseek(fileno(f),0,SEEK_END);
	off_t offset = 0;
	while(offset < total) {
		readHeader(f,offset,&header);
		if(header.filename[0] == '\0')
			break;

		std::cout << header.filename << "\n";

		// to next header
		size_t fsize = strtoul(header.size,NULL,8);
		offset += (fsize + BLOCK_SIZE * 2 - 1) & ~(BLOCK_SIZE - 1);
	}
}

static void createArchive(FILE *f,esc::cmdargs &args) {
	if(args.get_free().empty())
		error("Please provide at least one file to store");

	for(auto it = args.get_free().begin(); it != args.get_free().end(); ++it) {
		char tmp[MAX_PATH_LEN];
		strncpy(tmp,(*it)->c_str(),sizeof(tmp));
		addFolder(f,**it,basename(tmp));
	}
}

static void extractArchive(FILE *f) {
	FileHeader header;
	off_t total = lseek(fileno(f),0,SEEK_END);
	off_t offset = 0;
	while(offset < total) {
		readHeader(f,offset,&header);
		if(header.filename[0] == '\0')
			break;

		create(f,offset + BLOCK_SIZE,&header);

		// createa next header
		size_t fsize = strtoul(header.size,NULL,8);
		offset += (fsize + BLOCK_SIZE * 2 - 1) & ~(BLOCK_SIZE - 1);
	}
}

int main(int argc, char **argv) {
	std::string archive;
	std::string cmd;

	// parse params
	esc::cmdargs args(argc,argv,0);
	try {
		args.parse("=s f=s",&cmd,&archive);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const esc::cmdargs_error& e) {
		std::cerr << "Invalid arguments: " << e.what() << '\n';
		usage(argv[0]);
	}

	userList = user_parseFromFile(USERS_PATH,nullptr);
	if(!userList)
		printe("Warning: unable to parse users from file");
	groupList = group_parseFromFile(GROUPS_PATH,nullptr);
	if(!groupList)
		printe("Unable to parse groups from file");

	FILE *ar = cmd == "-c" ? stdout : stdin;
	if(!archive.empty()) {
		ar = fopen(archive.c_str(),cmd == "-c" ? "w" : "r");
		if(ar == NULL)
			error("Opening '%s' failed",archive.c_str());
	}

	if(cmd == "-t")
		listArchive(ar);
	else if(cmd == "-c")
		createArchive(ar,args);
	else if(cmd == "-x")
		extractArchive(ar);
	else
		error("Invalid command: %s",cmd.c_str());

	if(!archive.empty())
		fclose(ar);
	return 0;
}
