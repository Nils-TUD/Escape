/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef KEYMAP_US_H_
#define KEYMAP_US_H_

#include <common.h>
#include "keymap.h"

/**
 * Returns the keymap-entry for the given keycode or NULL if the keycode is invalid
 *
 * @param keycode the keycode
 * @return the keymap-entry
 */
sKeymapEntry *keymap_us_get(u8 keyCode);

#endif /* KEYMAP_US_H_ */
