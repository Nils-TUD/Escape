/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../../esc/h/common.h"
#include "../h/signal.h"

void (*signal(int sig,void (*handler)(int)))(int) {
	setSigHandler(sig,(fSigHandler)handler);
	/* TODO return old handler */
	return NULL;
}

int raise(int sig) {
	return sendSignal(sig,0);
}
