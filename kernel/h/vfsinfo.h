/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef VFSINFO_H_
#define VFSINFO_H_

#include "common.h"

/**
 * Creates the vfs-info nodes
 */
void vfsinfo_init(void);

/**
 * The proc-read-handler
 */
s32 vfsinfo_procReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count);

#endif /* VFSINFO_H_ */
