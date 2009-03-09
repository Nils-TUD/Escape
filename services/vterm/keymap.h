/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef KEYMAP_H_
#define KEYMAP_H_

#include <common.h>

/* represents a not-printable character */
#define NPRINT			'\0'

/* an entry in the keymap */
typedef struct {
	s8 def;
	s8 shift;
	s8 alt;
} sKeymapEntry;

/**
 * Returns the keymap-entry for the given keycode or NULL if the keycode is invalid
 *
 * @param keycode the keycode
 * @return the keymap-entry
 */
sKeymapEntry *keymap_get(u8 keyCode);

#endif /* KEYMAP_H_ */
