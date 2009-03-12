/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/ksymbols.h"
#include "../h/video.h"

static sSymbol ksymbols[] = {
#if TESTING
#	include "../../build/tsymbols.txt"
#else
#	include "../../build/ksymbols.txt"
#endif
};

void ksym_print(void) {
	u32 i;
	vid_printf("Kernel-Symbols:\n");
	for(i = 0; i < ARRAY_SIZE(ksymbols); i++)
		vid_printf("\t0x%08x -> %s\n",ksymbols[i].address,ksymbols[i].funcName);
}

sSymbol *ksym_getSymbolAt(u32 address) {
	sSymbol *sym = &ksymbols[ARRAY_SIZE(ksymbols) - 2];
	while(sym >= &ksymbols[0]) {
		if(address > sym->address)
			return sym;
		sym--;
	}
	return &ksymbols[0];
}
