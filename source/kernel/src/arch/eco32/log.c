/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/debug.h>

#define OUTPUT_BASE	0xFF000000	/* physical output device address */

static uint32_t *output = (uint32_t*)OUTPUT_BASE;

void log_writeChar(char c) {
	/* some chars make no sense here */
	if(c != '\r' && c != '\b')
		*output = c;
}
