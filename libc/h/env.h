/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef ENV_H_
#define ENV_H_

#include "common.h"

/**
 * Returns the value of the given environment-variable. Note that you have to copy the value
 * if you want to keep it!
 *
 * @param name the environment-variable-name
 * @return the value
 */
s8 *getenv(s8 *name);

/**
 * Sets the environment-variable <name> to <value>.
 *
 * @param name the name
 * @param value the value
 */
void setenv(s8 *name,s8* value);

#endif /* ENV_H_ */
