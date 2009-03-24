/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef VTERM_H_
#define VTERM_H_

#include <common.h>

/**
 * Inits the vterm
 */
void vterm_init(void);

/**
 * Releases resources
 */
void vterm_destroy(void);

/**
 * Handles the given keycode
 *
 * @param msg the message from the keyboard-driver
 */
void vterm_handleKeycode(sMsgKbResponse *msg);

/**
 * Prints the given string
 *
 * @param str the string
 */
void vterm_puts(char *str);

#endif /* VTERM_H_ */
