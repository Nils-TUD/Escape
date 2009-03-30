/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <dir.h>
#include <bufio.h>

#define BUF_SIZE 512

int main(int argc,char *argv[]) {
	tFD fd;
	s32 count;
	char *path;
	char buffer[BUF_SIZE];

	if(argc != 1 && argc != 2) {
		printf("Usage: %s [<file>]\n",argv[0]);
		return 1;
	}

	fd = STDIN_FILENO;
	if(argc == 2) {
		path = abspath(argv[1]);
		fd = open(path,IO_READ | IO_WRITE);
		if(fd < 0) {
			printLastError();
			return 1;
		}
	}

	while((count = fscanl(fd,buffer,BUF_SIZE - 1)) > 0) {
		*(buffer + count) = '\0';
		printf("%s\n",buffer);
	}

	if(argc == 2)
		close(fd);

	return 0;
}
