/**
 * $Id: edmain.c 1082 2011-10-17 20:43:22Z nasmussen $
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

#include <esc/common.h>
#include <esc/dir.h>
#include <stdlib.h>
#include <stdio.h>
#include <cmdargs.h>

static std::string filterName;
static std::string filterPath;
static char filterType = '\0';

static bool matches(const char *path,const char *file,size_t flen,sFileInfo *info) {
	bool match = true;
	if(!filterName.empty()) {
		char filename[MAX_PATH_LEN];
		size_t count = MAX(sizeof(filename) - 1,flen);
		memcpy(filename,file,count);
		filename[count] = '\0';
		match = strmatch(filterName.c_str(),filename);
	}
	if(match && !filterPath.empty())
		match = strmatch(filterPath.c_str(),path);
	if(match && filterType != '\0') {
		switch(filterType) {
			case 'b':
				match = S_ISBLK(info->mode);
				break;
			case 'c':
				match = S_ISCHR(info->mode);
				break;
			case 'd':
				match = S_ISDIR(info->mode);
				break;
			case 'r':
				match = S_ISREG(info->mode);
				break;
			case 'f':
				match = S_ISFS(info->mode);
				break;
			case 's':
				match = S_ISSERV(info->mode);
				break;
			default:
				match = false;
				break;
		}
	}
	return match;
}

static void listDir(const char *path) {
	char filepath[MAX_PATH_LEN];
	DIR *d = opendir(path);
	if(!d) {
		printe("Unable to open dir '%s'",path);
		return;
	}

	bool endsWithSlash = path[strlen(path) - 1] == '/';
	sDirEntry e;
	while(readdir(d,&e)) {
		if((e.nameLen == 1 && e.name[0] == '.') ||
			(e.nameLen == 2 && e.name[0] == '.' && e.name[1] == '.'))
			continue;

		if(endsWithSlash)
			snprintf(filepath,sizeof(filepath),"%s%s",path,e.name);
		else
			snprintf(filepath,sizeof(filepath),"%s/%s",path,e.name);
		sFileInfo info;
		if(stat(filepath,&info) < 0) {
			printe("Stat for '%s' failed");
			continue;
		}

		if(S_ISDIR(info.mode))
			listDir(filepath);
		if(matches(filepath,e.name,e.nameLen,&info))
			puts(filepath);
	}

	closedir(d);
}

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <path> [<expr>...]\n",name);
	fprintf(stderr,"Available expressions are:\n");
	fprintf(stderr,"    --path <pattern> : the pathname matches <pattern> (* = wildcard)\n");
	fprintf(stderr,"    --name <pattern> : the filename matches <pattern> (* = wildcard)\n");
	fprintf(stderr,"    --type <type>    : the file type is:\n");
	fprintf(stderr,"        b for block device\n");
	fprintf(stderr,"        c for character device\n");
	fprintf(stderr,"        d for directory\n");
	fprintf(stderr,"        r for regular file\n");
	fprintf(stderr,"        f for filesystem\n");
	fprintf(stderr,"        s for service\n");
	fprintf(stderr,"Note that currently, all expressions ANDed.\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	const char *prog = argv[0];
	const char *path = ".";
	// if the first argument is no filter-option, assume it's the path
	if(argc > 1 && argv[1][0] != '-') {
		argc--;
		argv++;
		path = argv[0];
	}

	// parse params
	std::cmdargs args(argc,argv,std::cmdargs::NO_FREE);
	try {
		args.parse("path=s name=s type=c",&filterPath,&filterName,&filterType);
		if(args.is_help())
			usage(prog);
		if(filterType && (filterType != 'b' && filterType != 'c' && filterType != 'd' &&
			filterType != 'r' && filterType != 'f' && filterType != 's')) {
			fprintf(stderr,"Invalid type: %c\n",filterType);
			usage(prog);
		}
	}
	catch(const std::cmdargs_error& e) {
		fprintf(stderr,"Invalid arguments: %s\n",e.what());
		usage(prog);
	}

	char clean[MAX_PATH_LEN];
	cleanpath(clean,sizeof(clean),path);
	listDir(clean);
	return 0;
}
