/**
 * $Id$
 */

.section .text

.global locku
.global unlocku

# void locku(tULock *l)
locku:
	PUT		rP,0
	SET		$1,1
	CSWAP	$1,$0,0
	BZ		$1,locku
	POP		0,0

# void unlocku(tULock *l)
unlocku:
	STCO	0,$0,0
	POP		0,0
