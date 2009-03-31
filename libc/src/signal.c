/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <signal.h>

void (*signal(int sig,void (*handler)(int)))(int) {
	setSigHandler(sig,(fSigHandler)handler);
	/* TODO return old handler */
	return NULL;
}

int raise(int sig) {
	return sendSignal(sig,0);
}
