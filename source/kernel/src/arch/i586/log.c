/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/arch/i586/serial.h>
#include <sys/log.h>

void log_writeChar(char c) {
	/* write to COM1 (some chars make no sense here) */
	if(c != '\r' && c != '\b')
		ser_out(SER_COM1,c);
}
