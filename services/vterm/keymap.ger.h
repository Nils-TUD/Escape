/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef KEYMAP_GER_H_
#define KEYMAP_GER_H_

#include <esc/common.h>
#include "keymap.h"

/**
 * Returns the keymap-entry for the given keycode or NULL if the keycode is invalid
 *
 * @param keycode the keycode
 * @return the keymap-entry
 */
sKeymapEntry *keymap_ger_get(u8 keyCode);

#endif /* KEYMAP_GER_H_ */
