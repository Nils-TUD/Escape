/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVMM_H_
#define PRIVMM_H_

#include "../pub/common.h"
#include "../pub/mm.h"

/**
 * Marks the given range as used or not used
 *
 * @param from the start-address
 * @param to the end-address
 * @param used wether the frame is used
 */
static void mm_markAddrRangeUsed(u32 from,u32 to,bool used);

/**
 * Marks the given frame-number as used or not used
 *
 * @param frame the frame-number
 * @param used wether the frame is used
 */
static void mm_markFrameUsed(u32 frame,bool used);

#endif /* PRIVMM_H_ */
