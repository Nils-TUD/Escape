/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SET1_H_
#define SET1_H_

#include <esc/common.h>

/**
 * Converts the given scancode to a keycode
 *
 * @param res the location where to store the result
 * @param scanCode the received scancode
 * @return true if it was a keycode
 */
bool kb_set1_getKeycode(sMsgKbResponse *res,u8 scanCode);

#endif /* SET1_H_ */
