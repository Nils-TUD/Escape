/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SET1_H_
#define SET1_H_

#include <common.h>

/**
 * Converts the given scancode to a keycode
 *
 * @param scanCode the received scancode
 * @return the keycode (!= 0 if valid)
 */
u8 kb_set1_getKeycode(u8 scanCode);

#endif /* SET1_H_ */
