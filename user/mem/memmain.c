/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/mem.h>
#include <stdlib.h>

int main(void) {
	sMemUsage mem;
	tFD fd;

	fd = open("system:/memusage",IO_READ);
	if(fd < 0) {
		printe("Unable to open 'system:/memusage'\n");
		return EXIT_FAILURE;
	}

	if(read(fd,&mem,sizeof(sMemUsage)) < 0) {
		printe("Unable to read mem-usage");
		return EXIT_FAILURE;
	}

	printf("Total memory:\t% 7d KiB\n",mem.totalMem / K);
	printf("Used memory:\t% 7d KiB\n",(mem.totalMem - mem.freeMem) / K);
	printf("Free memory:\t% 7d KiB\n",mem.freeMem / K);

	close(fd);

	return EXIT_SUCCESS;
}
