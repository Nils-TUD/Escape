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

#include <sys/cmdargs.h>
#include <sys/common.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 512

static void printFile(const char *filename,FILE *file);

static char buffer[BUF_SIZE + 1];

int main(int argc,char *argv[]) {
	if(isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s [<file> ...]\n",argv[0]);
		return EXIT_FAILURE;
	}

	if(argc < 2)
		printFile("STDIN",stdin);
	else {
		int i;
		FILE *file;
		for(i = 1; i < argc; i++) {
			int fd = open(argv[i],O_RDONLY);
			if(fd < 0) {
				printe("Unable to open '%s'",argv[i]);
				continue;
			}

			/* check if it's a directory */
			if(fisdir(fd)) {
				close(fd);
				printe("'%s' is a directory!",argv[i]);
				continue;
			}

			file = fattach(fd,"r");
			if(file == NULL) {
				close(fd);
				printe("Unable to fopen '%s'",argv[i]);
				continue;
			}

			printFile(argv[i],file);
			fclose(file);
			close(fd);
		}
	}

	return EXIT_SUCCESS;
}

static void printFile(const char *filename,FILE *file) {
	size_t count;
	while((count = fread(buffer,sizeof(char),BUF_SIZE,file)) > 0) {
		*(buffer + count) = '\0';
		if(fputs(buffer,stdout) == EOF) {
			printe("Write failed");
			break;
		}
	}
	if(count == 0 && ferror(file))
		printe("Read from %s failed",filename);
}
