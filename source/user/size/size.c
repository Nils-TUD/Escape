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
#include <sys/elf.h>
#include <sys/cmdargs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <file>...\n",name);
	exit(EXIT_FAILURE);
}

static void parseELF(const char *file) {
	int fd = open(file,O_RDONLY);
	if(fd < 0) {
		printe("Unable to open '%s' for reading",file);
		return;
	}

	sElfEHeader header;
	if(read(fd,&header,sizeof(header)) != sizeof(header)) {
		printe("Unable to read ELF-header from '%s'",file);
		goto done;
	}
	if(memcmp(header.e_ident,ELFMAG,4) != 0) {
		fprintf(stderr,"ELF magic of '%s' is wrong\n",file);
		goto done;
	}

	size_t textSize = 0;
	size_t dataSize = 0;
	size_t bssSize = 0;

	uintptr_t datPtr = header.e_phoff;
	for(size_t j = 0; j < header.e_phnum; datPtr += header.e_phentsize, j++) {
		if(lseek(fd,(off_t)datPtr,SEEK_SET) < 0) {
			printe("Seeking to program header %zu in '%s' failed",j,file);
			goto done;
		}

		sElfPHeader pheader;
		if(read(fd,&pheader,sizeof(sElfPHeader)) != sizeof(sElfPHeader)) {
			printe("Reading program-header %zu of '%s' failed",j,file);
			goto done;
		}

		if(pheader.p_type == PT_LOAD) {
			if(pheader.p_flags == (PF_R | PF_X))
				textSize += pheader.p_memsz;
			else if(pheader.p_flags == (PF_R | PF_W)) {
				dataSize += pheader.p_filesz;
				bssSize += pheader.p_memsz - pheader.p_filesz;
			}
		}
	}

	printf("%7zu %7zu %7zu %7zu %7zx %s\n",
		textSize,dataSize,bssSize,
		textSize + dataSize + bssSize,
		textSize + dataSize + bssSize,
		file);

done:
	close(fd);
}

int main(int argc,char **argv) {
	if(isHelpCmd(argc,argv) || argc < 2)
		usage(argv[0]);

	printf("   text	   data	    bss	    dec	    hex	filename\n");
	for(int i = 1; i < argc; ++i)
		parseELF(argv[i]);
	return 0;
}
