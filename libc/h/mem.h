/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef MEM_H_
#define MEM_H_

/**
 * Changes the size of the process's data-area. If <count> is positive <count> pages
 * will be added to the end of the data-area. Otherwise <count> pages will be removed at the
 * end.
 *
 * @param count the number of pages to add / remove
 * @return the *old* end of the data-segment. A negative value indicates an error
 */
s32 changeSize(s32 count);

/**
 * Maps <count> bytes at <phys> into the virtual user-space and returns the start-address.
 *
 * @param phys the physical start-address to map
 * @param count the number of bytes to map
 * @return the start-address or NULL if an error occurred
 */
void *mapPhysical(u32 phys,u32 count);

#endif /* MEM_H_ */
