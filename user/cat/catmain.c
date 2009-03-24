/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <bufio.h>

#define BUF_SIZE 512

s32 main(u32 argc,char *argv[]) {
	s32 fd;
	s32 count;
	char buffer[BUF_SIZE];

	if(argc != 1 && argc != 2) {
		printf("Usage: %s [<file>]\n",argv[0]);
		return 1;
	}

	fd = STDIN_FILENO;
	if(argc == 2) {
		fd = open(argv[1],IO_READ | IO_WRITE);
		if(fd < 0) {
			printLastError();
			return 1;
		}
	}

	while((count = read(fd,buffer,BUF_SIZE - 1)) > 0) {
		*(buffer + count) = '\0';
		printf("%s",buffer);
	}

	if(argc == 2)
		close(fd);

	return 0;
}
