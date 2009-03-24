/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <bufio.h>
#include <mem.h>

int main(void) {
	sMemUsage mem;
	tFD fd;

	fd = open("system:/memusage",IO_READ);
	if(fd < 0) {
		printf("Unable to open 'system:/memusage'\n");
		return 1;
	}

	if(read(fd,&mem,sizeof(sMemUsage)) < 0) {
		printLastError();
		return 1;
	}

	printf("Total memory:\t% 7d KiB\n",mem.totalMem / K);
	printf("Used memory:\t% 7d KiB\n",(mem.totalMem - mem.freeMem) / K);
	printf("Free memory:\t% 7d KiB\n",mem.freeMem / K);

	close(fd);

	return 0;
}
