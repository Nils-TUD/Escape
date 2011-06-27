/**
 * $Id$
 */

#ifndef CMDS_H_
#define CMDS_H_

#include <esc/common.h>
#include <esc/messages.h>
#include "mount.h"

sFSInst *cmds_getRoot(void);
void cmds_setRoot(dev_t dev,sFSInst *fs);
bool cmds_execute(int cmd,int fd,sMsg *msg,void *data);

#endif /* CMDS_H_ */
