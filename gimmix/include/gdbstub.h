/**
 * $Id$
 */

#ifndef GDBSTUB_H_
#define GDBSTUB_H_

#include "common.h"

/**
 * Inits the GDB-stub. Will listen on the given port and wait for commands
 */
void gdbstub_init(int portn);

#endif /* GDBSTUB_H_ */
