/**
 * @version		$Id: ksymbols.h 77 2008-11-22 22:27:35Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef KSYMBOLS_H_
#define KSYMBOLS_H_

#include "../h/common.h"

/* Represents one symbol (address -> name) */
typedef struct {
	u32 address;
	const char *funcName;
} sSymbol;

/**
 * Prints all kernel-symbols
 */
void ksym_print(void);

/**
 * Searches for the highest address lower than the given one in the symbol-table. In other
 * words: The name of the function to which the given address belongs
 *
 * @param address the address
 * @return the pointer to the symbol (no copy!)
 */
sSymbol *ksym_getSymbolAt(u32 address);

#endif /* KSYMBOLS_H_ */
