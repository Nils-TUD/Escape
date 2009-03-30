/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/ports.h"

s32 requestIOPort(u16 port) {
	return requestIOPorts(port,1);
}

s32 releaseIOPort(u16 port) {
	return releaseIOPorts(port,1);
}
