/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVPROC_H_
#define PRIVPROC_H_

#include "../pub/common.h"
#include "../pub/proc.h"

/* public process-data */
typedef struct {
	u8 state;
	tPid pid;
	tPid parentPid;
	u32 textPages;
	u32 dataPages;
	u32 stackPages;
} sProcPub;

/**
 * Our VFS read handler that should read process information into a given buffer
 *
 * @param node the VFS node
 * @param buffer the buffer where to copy the info to
 * @param offset the offset where to start
 * @param count the number of bytes
 * @return the number of read bytes
 */
static s32 proc_vfsReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count);

#endif /* PRIVPROC_H_ */
