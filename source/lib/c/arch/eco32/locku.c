/**
 * $Id$
 */

#include <esc/common.h>
#include <esc/thread.h>

void locku(tULock *l) {
	/* TODO eco32 has no atomic compare and swap instruction or similar :/ */
	lock((uint)l,LOCK_EXCLUSIVE);
}

void unlocku(tULock *l) {
	unlock((uint)l);
}
