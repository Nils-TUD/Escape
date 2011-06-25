/**
 * $Id$
 */

#include <esc/common.h>
#include <esc/arch/i586/ports.h>
#include <esc/log.h>
#include <assert.h>

static bool reqPorts = false;

void logc(char c) {
	if(!reqPorts) {
		/* request io-ports for qemu and bochs */
		assert(requestIOPort(0xe9) >= 0);
		assert(requestIOPort(0x3f8) >= 0);
		assert(requestIOPort(0x3fd) >= 0);
		reqPorts = true;
	}
	while((inByte(0x3f8 + 5) & 0x20) == 0)
		;
	outByte(0x3f8,c);
}
