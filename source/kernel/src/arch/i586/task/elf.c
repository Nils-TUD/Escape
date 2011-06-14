/**
 * $Id$
 */

#include <sys/common.h>
#include <sys/task/elf.h>

int elf_finishFromMem(const void *code,size_t length,sStartupInfo *info) {
	UNUSED(code);
	UNUSED(length);
	UNUSED(info);
	return 0;
}

int elf_finishFromFile(tFileNo file,const sElfEHeader *eheader,sStartupInfo *info) {
	UNUSED(file);
	UNUSED(eheader);
	UNUSED(info);
	return 0;
}
