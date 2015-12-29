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

#include <fs/tar/tar.h>
#include <esc/stream/obufstream.h>
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <usergroup/group.h>
#include <usergroup/user.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <utime.h>

using namespace esc;
using namespace fs;

static char buffer[Tar::BLOCK_SIZE];
static sUser *userList = nullptr;
static sGroup *groupList = nullptr;
static off_t curoff = 0;

static void addFolder(FILE *f,const std::string &fpath,const std::string &tpath) {
	struct stat st;
	if(stat(fpath.c_str(),&st) < 0)
		exitmsg("Unable to stat '" << fpath << "'");

	Tar::writeHeader(f,curoff,buffer,tpath.c_str(),st,userList,groupList);
	curoff += Tar::BLOCK_SIZE;

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

			OBufStream obnf(nfpath,sizeof(nfpath));
			OBufStream obnt(ntpath,sizeof(ntpath));
			obnf << fpath << "/" << fmt(e.d_name,1,e.d_namelen);
			obnt << tpath << "/" << fmt(e.d_name,1,e.d_namelen);
			addFolder(f,nfpath,ntpath);
		}
		closedir(d);
	}
	else if(S_ISREG(st.st_mode)) {
		/* write file-content */
		int fd = open(fpath.c_str(),O_RDONLY);
		if(fd < 0)
			exitmsg("Unable to open '" << fpath << "' for reading");

		for(off_t off = 0; off < st.st_size; off += Tar::BLOCK_SIZE) {
			ssize_t count = read(fd,buffer,sizeof(buffer));
			if(count < 0)
				exitmsg("Reading from '" << fpath << "' failed");
			memset(buffer + count,0,Tar::BLOCK_SIZE - count);
			Tar::writeBlock(f,curoff,buffer);
			curoff += Tar::BLOCK_SIZE;
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
			errmsg("Unable to create directory '" << path << "'");
		if(old) {
			*p = old;
			p++;
		}
	}
}

static void create(FILE *f,off_t cur,const Tar::FileHeader *header) {
	// create parents
	mode_t mode = strtoul(header->mode,NULL,8);
	createDir(header->filename,
		header->type == Tar::T_DIR ? S_IFDIR | mode : DIR_DEF_MODE,header->type == Tar::T_DIR);

	switch(header->type) {
		case Tar::T_REGULAR: {
			int fd = create(header->filename,O_WRONLY | O_TRUNC | O_CREAT | O_EXCL,S_IFREG | mode);
			if(fd < 0) {
				errmsg("Unable to create file '" << header->filename << "'");
				break;
			}

			off_t total = strtoul(header->size,NULL,8);
			for(off_t off = 0; total > 0; off += Tar::BLOCK_SIZE) {
				Tar::readBlock(f,cur + off,buffer);

				ssize_t count = total > Tar::BLOCK_SIZE ? (size_t)Tar::BLOCK_SIZE : total;
				if(write(fd,buffer,count) != count) {
					errmsg("Writing to '" << header->filename << "' failed");
					break;
				}
				total -= count;
			}
			close(fd);
		}
		break;

		case Tar::T_DIR: {
			// createDir has created it already
			// but repeat the chmod because we might have set the default mode last time
			if(chmod(header->filename,mode) < 0)
				errmsg("chmod(" << header->filename << "," << fmt(mode,"0o",4) << ") failed");
		}
		break;

		default: {
			errmsg("Warning: type " << header->type << " not supported");
		}
		break;
	}

	/* set file modification time */
	struct utimbuf utimes;
	utimes.modtime = strtoul(header->mtime,NULL,8);
	if(utimes.modtime) {
		utimes.actime = time(NULL);
		if(utime(header->filename,&utimes) < 0)
			errmsg("utime(" << header->filename << ") failed");
	}

	if(*header->uname) {
		sUser *u = user_getByName(userList,header->uname);
		if(u && chown(header->filename,u->uid,-1) < 0)
			errmsg("chown(" << header->filename << "," << u->uid << ",-1) failed");
	}
	if(*header->gname) {
		sGroup *g = group_getByName(groupList,header->gname);
		if(g && chown(header->filename,-1,g->gid) < 0)
			errmsg("chown(" << header->filename << ",-1," << g->gid << ") failed");
	}
}

static void listArchive(FILE *f) {
	Tar::FileHeader header;
	off_t total = filesize(fileno(f));
	off_t offset = 0;
	while(offset < total) {
		Tar::readHeader(f,offset,&header);
		if(header.filename[0] == '\0')
			break;

		sout << header.filename << "\n";

		// to next header
		size_t fsize = strtoul(header.size,NULL,8);
		offset += (fsize + Tar::BLOCK_SIZE * 2 - 1) & ~(Tar::BLOCK_SIZE - 1);
	}
}

static void createArchive(FILE *f,esc::cmdargs &args) {
	if(args.get_free().empty())
		exitmsg("Please provide at least one file to store");

	for(auto it = args.get_free().begin(); it != args.get_free().end(); ++it) {
		char tmp[MAX_PATH_LEN];
		strncpy(tmp,(*it)->c_str(),sizeof(tmp));
		addFolder(f,**it,basename(tmp));
	}
}

static void extractArchive(FILE *f) {
	Tar::FileHeader header;
	off_t total = filesize(fileno(f));
	off_t offset = 0;
	while(offset < total) {
		Tar::readHeader(f,offset,&header);
		if(header.filename[0] == '\0')
			break;

		create(f,offset + Tar::BLOCK_SIZE,&header);

		// createa next header
		size_t fsize = strtoul(header.size,NULL,8);
		offset += (fsize + Tar::BLOCK_SIZE * 2 - 1) & ~(Tar::BLOCK_SIZE - 1);
	}
}

static void usage(const char *name) {
	serr << "Usage: " << name << " <cmd> [-f <archive>] [files...]\n";
	serr << "\n";
	serr << "The commands are:\n";
	serr << "-c: create an archive\n";
	serr << "-t: list files in archive\n";
	serr << "-x: extract archive\n";
	exit(EXIT_FAILURE);
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
		errmsg("Invalid arguments: " << e.what());
		usage(argv[0]);
	}

	userList = user_parseFromFile(USERS_PATH,nullptr);
	if(!userList)
		errmsg("Warning: unable to parse users from file");
	groupList = group_parseFromFile(GROUPS_PATH,nullptr);
	if(!groupList)
		errmsg("Warning: unable to parse groups from file");

	FILE *ar = cmd == "-c" ? stdout : stdin;
	if(!archive.empty()) {
		ar = fopen(archive.c_str(),cmd == "-c" ? "w" : "r");
		if(ar == NULL)
			exitmsg("Opening '" << archive << "' failed");
	}

	if(cmd == "-t")
		listArchive(ar);
	else if(cmd == "-c")
		createArchive(ar,args);
	else if(cmd == "-x")
		extractArchive(ar);
	else
		exitmsg("Invalid command: " << cmd);

	if(!archive.empty())
		fclose(ar);
	return 0;
}
