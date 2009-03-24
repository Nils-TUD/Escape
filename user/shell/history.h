/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef HISTORY_H_
#define HISTORY_H_

#include <common.h>

/**
 * Adds the given line to history. It will be assumed that line has been allocated on the heap!
 *
 * @param line the line
 */
void shell_addToHistory(char *line);

/**
 * Moves one step up in the history
 *
 * @return the current history-entry (NULL if no available)
 */
char *shell_histUp(void);

/**
 * Moves one step down in the history
 *
 * @return the current history-entry (NULL if no available)
 */
char *shell_histDown(void);

/**
 * Prints the history
 */
void shell_histPrint(void);

#endif /* HISTORY_H_ */
