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

#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

using namespace esc;

static std::string filterName;
static std::string filterPath;
static char filterType = '\0';

static bool matches(const char *path,const char *file,size_t flen,struct stat *info) {
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
				match = S_ISBLK(info->st_mode);
				break;
			case 'c':
				match = S_ISCHR(info->st_mode);
				break;
			case 'd':
				match = S_ISDIR(info->st_mode);
				break;
			case 'r':
				match = S_ISREG(info->st_mode);
				break;
			case 'f':
				match = S_ISFS(info->st_mode);
				break;
			case 's':
				match = S_ISSERV(info->st_mode);
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
		errmsg("Unable to open dir '" << path << "'");
		return;
	}

	bool endsWithSlash = path[strlen(path) - 1] == '/';
	struct dirent e;
	while(readdir(d,&e)) {
		if((e.d_namelen == 1 && e.d_name[0] == '.') ||
			(e.d_namelen == 2 && e.d_name[0] == '.' && e.d_name[1] == '.'))
			continue;

		if(endsWithSlash)
			snprintf(filepath,sizeof(filepath),"%s%s",path,e.d_name);
		else
			snprintf(filepath,sizeof(filepath),"%s/%s",path,e.d_name);
		struct stat info;
		if(stat(filepath,&info) < 0) {
			errmsg("Stat for '" << filepath << "' failed");
			continue;
		}

		if(S_ISDIR(info.st_mode))
			listDir(filepath);
		if(matches(filepath,e.d_name,e.d_namelen,&info))
			puts(filepath);
	}

	closedir(d);
}

static void usage(const char *name) {
	serr << "Usage: " << name << " <path> [<expr>...]\n";
	serr << "Available expressions are:\n";
	serr << "    --path <pattern> : the pathname matches <pattern> (* = wildcard)\n";
	serr << "    --name <pattern> : the filename matches <pattern> (* = wildcard)\n";
	serr << "    --type <type>    : the file type is:\n";
	serr << "        b for block device\n";
	serr << "        c for character device\n";
	serr << "        d for directory\n";
	serr << "        r for regular file\n";
	serr << "        f for filesystem\n";
	serr << "        s for service\n";
	serr << "Note that currently, all expressions ANDed.\n";
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
	esc::cmdargs args(argc,argv,esc::cmdargs::NO_FREE);
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
	catch(const esc::cmdargs_error& e) {
		errmsg("Invalid arguments: " << e.what());
		usage(prog);
	}

	char clean[MAX_PATH_LEN];
	cleanpath(clean,sizeof(clean),path);
	listDir(clean);
	return 0;
}
