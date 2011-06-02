/**
 * $Id$
 */

#ifndef ECO32_THREAD_H_
#define ECO32_THREAD_H_

#include <esc/common.h>

/* the thread-state which will be saved for context-switching */
typedef struct {
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
	uint32_t r25;
	uint32_t r29;
	uint32_t r30;
	uint32_t r31;
} sThreadRegs;

typedef struct {
	char dummy;	/* empty struct not allowed */
} sThreadArchAttr;

#endif /* ECO32_THREAD_H_ */
