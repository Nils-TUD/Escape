/**
 * $Id$
 */

#ifndef MOUSE_H_
#define MOUSE_H_

#include <esc/common.h>
#include "window.h"

int mouse_start(void *drvIdPtr);
tCoord mouse_getX(void);
tCoord mouse_getY(void);

#endif /* MOUSE_H_ */
