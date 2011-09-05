/**
 * $Id: gimmx2bin.c 178 2011-04-16 14:15:23Z nasmussen $
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "core/bus.h"
#include "mmix/io.h"
#include "mmix/error.h"

#define SECTOR_SIZE		512

static const char *usage = "Usage: %s <hex-file> <bin-file> [--mbr]";

static void writeOcta(FILE *f,octa data) {
	data = ((data >> 56) & 0xFF) << 0 |
			((data >> 48) & 0xFF) << 8 |
			((data >> 40) & 0xFF) << 16 |
			((data >> 32) & 0xFF) << 24 |
			((data >> 24) & 0xFF) << 32 |
			((data >> 16) & 0xFF) << 40 |
			((data >> 8) & 0xFF) << 48 |
			((data >> 0) & 0xFF) << 56;
	if(fwrite(&data,sizeof(octa),1,f) != 1)
		error("Unable to write to file: %s",strerror(errno));
}

int main(int argc,char **argv) {
	bool mbr = false;
	if(argc != 3 && argc != 4)
		error(usage,argv[0]);
	if(argc == 4) {
		if(strcmp(argv[3],"--mbr") != 0)
			error(usage,argv[0]);
		mbr = true;
	}

	FILE *in = fopen(argv[1],"r");
	if(!in)
		error("Unable to open file '%s' for reading: %s",argv[1],strerror(errno));
	FILE *out = fopen(argv[2],"wb");
	if(!out)
		error("Unable to open file '%s' for writing: %s",argv[2],strerror(errno));

	// invalid address
	octa loc = 1;
	size_t count = 0;
	while(!feof(in)) {
		int c = fgetc(in);
		if(c == EOF)
			break;
		if(c == '\n')
			continue;
		if(c != ' ') {
			if(loc != 1)
				error("Invalid hex-file: It can't contain multiple target-addresses!");
			ungetc(c,in);
			if(mfscanf(in,"%Ox:",&loc) != 1)
				error("Invalid hex-file");
		}
		else {
			octa o;
			if(mfscanf(in,"%Ox",&o) != 1)
				error("Invalid hex-file");
			writeOcta(out,o);
			count++;
			loc += sizeof(loc);
			if(mbr && count >= SECTOR_SIZE / sizeof(octa)) {
				mprintf("%d bytes reached, i.e. MBR complete\n",count * sizeof(octa));
				break;
			}
		}
		// walk to line end
		while(!feof(in) && fgetc(in) != '\n');
	}

	if(mbr) {
		while(count < SECTOR_SIZE / sizeof(octa)) {
			writeOcta(out,0);
			count++;
		}
	}

	fclose(out);
	fclose(in);
	return EXIT_SUCCESS;
}
