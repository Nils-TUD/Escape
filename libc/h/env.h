/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef ENV_H_
#define ENV_H_

#include "common.h"

/**
 * Returns the env-variable-name with given index
 *
 * @param index the index
 * @return the name of it or NULL if the index does not exist (or it failed for another reason)
 */
s8 *getEnvByIndex(u32 index);

/**
 * Returns the value of the given environment-variable. Note that you have to copy the value
 * if you want to keep it!
 *
 * @param name the environment-variable-name
 * @return the value
 */
s8 *getEnv(const s8 *name);

/**
 * Sets the environment-variable <name> to <value>.
 *
 * @param name the name
 * @param value the value
 */
void setEnv(const s8 *name,const s8* value);

#endif /* ENV_H_ */
