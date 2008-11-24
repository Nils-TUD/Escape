/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVUTIL_H_
#define PRIVUTIL_H_

#include "../pub/common.h"
#include "../pub/util.h"

/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE 5

/**
 * @return the address of the stack-frame-start
 */
extern u32 getStackFrameStart(void);

#endif /* PRIVUTIL_H_ */
