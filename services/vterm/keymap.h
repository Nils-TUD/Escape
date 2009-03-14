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

#endif /* KEYMAP_H_ */
