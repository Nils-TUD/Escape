/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/mem.h"

/* the assembler-routine */
extern s32 _mapPhysical(u32 phys,u32 count);

/* just a convenience for the user because the return-value is negative if an error occurred */
/* since it will be mapped in the user-space (< 0x80000000) the MSB is never set */
void *mapPhysical(u32 phys,u32 count) {
	s32 err = _mapPhysical(phys,count);
	if(err < 0)
		return NULL;
	return (void*)err;
}
