/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/proc.h>
#include <esc/signals.h>

s32 sendSignal(tSig signal,u32 data) {
	return sendSignalTo(INVALID_PID,signal,data);
}
