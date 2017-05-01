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

#include <sys/stat.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

enum {
	FL_BYTES	= 1 << 0,
	FL_TOTAL	= 1 << 1,
	FL_HUMAN	= 1 << 2,
	FL_SUMMARY	= 1 << 3,
};

static const off_t BLOCK_SIZE = 1024;

static uint flags = 0;

static void printsize(const char *path,off_t size) {
	if(flags & FL_HUMAN) {
		static const char sizes[] = {'B','K','M','G','T','P'};
		if(!(flags & FL_BYTES))
			size *= BLOCK_SIZE;
		off_t sz = 1;
		for(size_t i = 0; i < ARRAY_SIZE(sizes); ++i) {
			if(size < sz * 1024) {
				printf("%lu%c\t%s\n",size / sz,sizes[i],path);
				break;
			}
			sz *= 1024;
		}
	}
	else
		printf("%lu\t%s\n",size,path);
}

static off_t getsize(const char *path) {
	struct stat info;
	if(lstat(path,&info) < 0) {
		printe("stat failed for '%s'",path);
		return 0;
	}

	off_t total;
	if(flags & FL_BYTES)
		total = info.st_size;
	else
		total = (info.st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

	if(S_ISDIR(info.st_mode)) {
		DIR *d = opendir(path);
		if(d) {
			struct dirent *e;
			while((e = readdir(d))) {
				if(e->d_namelen == 1 && e->d_name[0] == '.')
					continue;
				if(e->d_namelen == 2 && e->d_name[0] == '.' && e->d_name[1] == '.')
					continue;

				char fpath[MAX_PATH_LEN];
				snprintf(fpath,sizeof(fpath),"%s/%s",path,e->d_name);
				total += getsize(fpath);
			}
			closedir(d);
		}
		else
			printe("unable to open directory '%s'",path);

		if(!(flags & FL_SUMMARY))
			printsize(path,total);
	}
	return total;
}

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-b] [-c] [-h] [-s] <path>...\n",name);
	fprintf(stderr,"  -b: print sizes in bytes\n");
	fprintf(stderr,"  -c: print total size\n");
	fprintf(stderr,"  -h: print sizes in human readable form\n");
	fprintf(stderr,"  -s: print summary for each argument\n");
	exit(1);
}

int main(int argc,char **argv) {
	int opt;
	while((opt = getopt(argc,argv,"bchs")) != -1) {
		switch(opt) {
			case 'b': flags |= FL_BYTES; break;
			case 'c': flags |= FL_TOTAL; break;
			case 'h': flags |= FL_HUMAN; break;
			case 's': flags |= FL_SUMMARY; break;
			default:
				usage(argv[0]);
		}
	}

	off_t total = 0;

	for(int i = optind; i < argc; ++i) {
		off_t size = getsize(argv[i]);
		if(flags & FL_SUMMARY)
			printsize(argv[i],size);
		total += size;
	}

	if(flags & FL_TOTAL)
		printsize("total",total);
	return 0;
}
