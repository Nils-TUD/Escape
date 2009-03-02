/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/debug.h"

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

		case ERR_INVALID_FD:
			debugf("Invalid file-descriptor\n");
			break;

		case ERR_INVALID_FILE:
			debugf("Invalid file\n");
			break;

		case ERR_NO_READ_PERM:
			debugf("No read-permission\n");
			break;

		case ERR_NO_WRITE_PERM:
			debugf("No write-permission\n");
			break;

		case ERR_INV_SERVICE_NAME:
			debugf("Invalid service name. Alphanumeric, not empty name expected!\n");
			break;

		case ERR_NOT_ENOUGH_MEM:
			debugf("Not enough memory!\n");
			break;

		case ERR_SERVICE_EXISTS:
			debugf("The service with desired name does already exist!\n");
			break;

		case ERR_PROC_DUP_SERVICE:
			debugf("You are already a service!\n");
			break;

		case ERR_PROC_DUP_SERVICE_USE:
			debugf("You are already using the requested service!\n");
			break;

		case ERR_SERVICE_NOT_IN_USE:
			debugf("You are not using the service at the moment!\n");
			break;

		case ERR_NOT_OWN_SERVICE:
			debugf("The service-node is not your own!\n");
			break;

		default:
			debugf("No error\n");
			break;
	}
}
