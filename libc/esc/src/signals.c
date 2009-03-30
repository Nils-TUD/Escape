/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/proc.h"
#include "../h/signals.h"

s32 sendSignal(tSig signal,u32 data) {
	return sendSignalTo(INVALID_PID,signal,data);
}
