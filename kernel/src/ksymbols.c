/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../pub/common.h"
#include "../pub/ksymbols.h"
#include "../pub/video.h"

static sSymbol ksymbols[] = {
	#include "../../build/ksymbols.txt"
};

void ksym_print(void) {
	u32 i;
	vid_printf("Kernel-Symbols:\n");
	for(i = 0; i < ARRAY_SIZE(ksymbols); i++)
		vid_printf("\t0x%08x -> %s\n",ksymbols[i].address,ksymbols[i].funcName);
}

sSymbol *ksym_gesSymbolAt(u32 address) {
	sSymbol *sym = &ksymbols[ARRAY_SIZE(ksymbols) - 2];
	while(sym >= &ksymbols[0]) {
		if(address > sym->address)
			return sym;
		sym--;
	}
	return &ksymbols[0];
}
