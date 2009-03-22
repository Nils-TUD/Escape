/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <env.h>
#include <string.h>
#include <io.h>

/* TODO not the right place for this constant */
#define MAX_PATH_LEN	255

s32 shell_cmdCd(u32 argc,s8 **argv) {
	s8 path[MAX_PATH_LEN + 1];
	s8 *current,*curtemp,*pathtemp,*p;
	u32 layer,pos;
	s32 fd;

	if(argc != 2) {
		printf("Usage: cd <directory>\n");
		return 1;
	}

	p = argv[1];
	/* skip file: */
	if(strncmp(p,"file:",5) == 0)
		p += 5;
	strcpy(path,"file:");
	pathtemp = path + 5;

	layer = 0;
	if(*p != '/') {
		current = getEnv("CWD");
		/* copy current to path */
		/* skip file:/ */
		*pathtemp++ = '/';
		curtemp = current + 6;
		while(*curtemp) {
			if(*curtemp == '/')
				layer++;
			*pathtemp = *curtemp;
			pathtemp++;
			curtemp++;
		}
		/* remove '/' at the end */
		pathtemp--;
	}
	else {
		/* skip leading '/' */
		do {
			p++;
		}
		while(*p == '/');
	}

	while(*p) {
		pos = strchri(p,'/');

		/* simply skip '.' */
		if(pos == 1 && strncmp(p,".",1) == 0)
			p += 2;
		/* one layer back */
		else if(pos == 2 && strncmp(p,"..",2) == 0) {
			if(layer > 0) {
				do {
					pathtemp--;
				}
				while(*pathtemp != '/');
				*pathtemp = '\0';
				layer--;
			}
			p += 3;
		}
		else {
			/* append to path */
			*pathtemp++ = '/';
			strncpy(pathtemp,p,pos);
			pathtemp += pos;
			p += pos + 1;
			layer++;
		}

		/* one step too far? */
		if(*(p - 1) == '\0')
			break;

		/* skip multiple '/' */
		while(*p == '/')
			p++;
	}

	/* terminate */
	*pathtemp++ = '/';
	*pathtemp = '\0';

	/* check if the path is valid */
	/* TODO this does not work yet since the fs-stuff doesn't handle errors really well */
	if((fd = open(path,IO_READ)) >= 0) {
		close(fd);
		setEnv("CWD",path);
	}
	else {
		printf("Directory '%s' does not exist\n");
		return 1;
	}

	return 0;
}
