/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"

s32 lastError = 0;

void printLastError(void) {
	debugf("Error %d: ",lastError);
	switch(lastError) {
		case ERR_FILE_IN_USE:
			debugf("The file is already in use\n");
			break;

		case ERR_INVALID_SYSC_ARGS:
			debugf("Invalid syscall-arguments\n");
			break;

		case ERR_MAX_PROC_FDS:
			debugf("You have reached the max. possible file-descriptors\n");
			break;

		case ERR_NO_FREE_FD:
			debugf("The max. global, open files have been reached\n");
			break;

		case ERR_VFS_NODE_NOT_FOUND:
			debugf("Unable to resolve the path\n");
			break;

		default:
			debugf("No error\n");
			break;
	}
}
