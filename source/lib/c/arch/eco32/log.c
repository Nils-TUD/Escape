/**
 * $Id$
 */

#include <esc/common.h>
#include <esc/log.h>
#include <esc/mem.h>
#include <assert.h>

#define OUTPUT_START_ADDR		0x3F000000

static uint32_t *outRegs = NULL;

void logc(char c) {
	if(outRegs == NULL) {
		outRegs = mapPhysical(OUTPUT_START_ADDR,8);
		assert(outRegs != NULL);
	}
	*outRegs = c;
}
