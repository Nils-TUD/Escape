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
 * @return true if the terminal reads
 */
bool vterm_isReading(void);

/**
 * Starts reading from the vterm until VK_ENTER is pressed
 *
 * @param maxLength the maximum number of chars to read
 */
void vterm_startReading(u16 maxLength);

/**
 * @return the number of read chars
 */
u16 vterm_getReadLength(void);

/**
 * @return the read line or NULL if not yet finished reading
 */
s8 *vterm_getReadLine(void);

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
void vterm_puts(s8 *str);

#endif /* VTERM_H_ */
