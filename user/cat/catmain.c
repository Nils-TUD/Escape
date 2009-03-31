/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/fileio.h>

#define BUF_SIZE 512

int main(int argc,char *argv[]) {
	tFile *file;
	s32 count;
	char *path;
	char buffer[BUF_SIZE];

	if(argc != 1 && argc != 2) {
		fprintf(stderr,"Usage: %s [<file>]\n",argv[0]);
		return 1;
	}

	file = stdin;
	if(argc == 2) {
		path = abspath(argv[1]);
		file = fopen(path,"r");
		if(file == NULL) {
			printe("Unable to open '%s'",path);
			return 1;
		}
	}

	while((count = fread(buffer,sizeof(char),BUF_SIZE - 1,file)) > 0) {
		*(buffer + count) = '\0';
		printf("%s",buffer);
	}

	if(argc == 2)
		fclose(file);

	return 0;
}
