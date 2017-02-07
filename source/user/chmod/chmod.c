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
#include <sys/proc.h>
#include <sys/stat.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	USER	= 1 << 0,
	GROUP	= 1 << 1,
	OTHERS	= 1 << 2,
	ALL		= USER | GROUP | OTHERS
} Part;

typedef enum {
	ADD,
	SUB,
	ASSIGN
} Operation;

static bool isNumeric(const char *str) {
	while(*str) {
		if(!isxdigit(*str))
			return false;
		str++;
	}
	return true;
}

static mode_t changeMode(mode_t mode,Operation op,int shift,int perm) {
	switch(op) {
		case ADD:
			mode |= perm << shift;
			break;
		case SUB:
			mode &= ~(perm << shift);
			break;
		case ASSIGN:
			// 't' is ignored for USER and GROUP, but not for OTHERS
			if(shift == 0)
				mode &= ~01007;
			else
				mode &= ~(07 << shift);
			mode |= perm << shift;
			break;
	}
	return mode;
}

static mode_t parseMode(const char *path,const char *mode) {
	if(isNumeric(mode))
		return (mode_t)strtol(mode,NULL,0);

	Part part = 0;
	struct { char c; Part p; } parts[] = {
		{'u',USER},{'g',GROUP},{'o',OTHERS},
	};
	while(*mode && strchr("ugo",*mode)) {
		for(size_t i = 0; i < ARRAY_SIZE(parts); ++i) {
			if(*mode == parts[i].c) {
				part |= parts[i].p;
				mode++;
				break;
			}
		}
	}
	if(part == 0)
		part = ALL;

	Operation op = ASSIGN;
	struct { char c; Operation op; } ops[] = {
		{'+',ADD},{'-',SUB},{'=',ASSIGN},
	};
	for(size_t i = 0; i < ARRAY_SIZE(ops); ++i) {
		if(*mode == ops[i].c) {
			op = ops[i].op;
			mode++;
			break;
		}
	}

	int perm = 0;
	struct { char c; int perm; } perms[] = {
		{'t',01000},{'r',04},{'w',02},{'x',01},
	};
	while(*mode) {
		for(size_t i = 0; i < ARRAY_SIZE(perms); ++i) {
			if(*mode == perms[i].c) {
				perm |= perms[i].perm;
				mode++;
				break;
			}
		}
	}

	mode_t imode = 0;
	if(part != ALL || op != ASSIGN) {
		struct stat st;
		if(stat(path,&st) < 0) {
			fprintf(stderr,"Unable to stat '%s'\n",path);
			return -1;
		}
		imode = st.st_mode & MODE_PERM;
	}

	if(part & USER)
		imode = changeMode(imode,op,6,perm & 07);
	if(part & GROUP)
		imode = changeMode(imode,op,3,perm & 07);
	if(part & OTHERS)
		imode = changeMode(imode,op,0,perm);
	return imode;
}

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <mode> <file>...\n",name);
	fprintf(stderr,"  <mode> can be either numeric or of the form [ugo][+-=][trwx]\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	if(getopt_ishelp(argc,argv) || argc < 3)
		usage(argv[0]);

	const char *mode = argv[1];
	for(int i = 2; i < argc; ++i) {
		mode_t imode = parseMode(argv[i],mode);
		if(imode != (mode_t)-1) {
			if(chmod(argv[i],imode) < 0)
				printe("Unable to set mode of '%s' to %05o",argv[i],imode);
		}
	}
	return EXIT_SUCCESS;
}
