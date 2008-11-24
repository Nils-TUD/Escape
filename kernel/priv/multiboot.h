/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVMULTIBOOT_H_
#define PRIVMULTIBOOT_H_

#include "../pub/common.h"
#include "../pub/multiboot.h"

/* checks wether the flag with at given bit is set */
#define CHECK_FLAG(flags,bit) (flags & (1 << bit))

#endif /* PRIVMULTIBOOT_H_ */
