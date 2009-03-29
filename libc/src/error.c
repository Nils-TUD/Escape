/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/debug.h"
#include "../h/proc.h"

#define MAX_STACK_DEPTH 20
/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE 5

s32 lastError = 0;

u32 *getStackTrace(void) {
	static u32 frames[MAX_STACK_DEPTH];
	u32 i;
	u32 *ebp;
	/* TODO just temporary */
	u32 end = 0xC0000000;
	u32 start = end - 0x1000 * 2;
	u32 *frame = &frames[0];

	GET_REG("ebp",ebp)
	for(i = 0; i < MAX_STACK_DEPTH; i++) {
		/* prevent page-fault */
		if((u32)ebp < start || (u32)ebp >= end)
			break;
		*frame = *(ebp + 1) - CALL_INSTR_SIZE;
		ebp = (u32*)*ebp;
		frame++;
	}

	/* terminate */
	*frame = 0;
	return &frames[0];
}

void printStackTrace(void) {
	u32 *trace = getStackTrace();
	debugf("Stack-trace:\n");
	/* TODO maybe we should skip printStackTrace here? */
	while(*trace != 0) {
		debugf("\t0x%08x\n",*trace);
		trace++;
	}
}

s32 getLastError(void) {
	return lastError;
}

void printLastError(void) {
	debugf("[proc %d] Error %d: ",getpid(),lastError);
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

		case ERR_IO_MAP_RANGE_RESERVED:
			debugf("The given io-port range is reserved!\n");
			break;

		case ERR_IOMAP_NOT_PRESENT:
			debugf("The io-port-map is not present (have you reserved ports?)\n");
			break;

		case ERR_INTRPT_LISTENER_MSGLEN:
			debugf("The length of the interrupt-notify-message is too long!\n");
			break;

		case ERR_INVALID_IRQ_NUMBER:
			debugf("The given IRQ-number is invalid!\n");
			break;

		case ERR_IRQ_LISTENER_MISSING:
			debugf("The IRQ-listener is not present!\n");
			break;

		case ERR_NO_CLIENT_WAITING:
			debugf("No client is currently waiting\n");
			break;

		case ERR_FS_NOT_FOUND:
			debugf("Filesystem-service not found\n");
			break;

		case ERR_INVALID_SIGNAL:
			debugf("Invalid signal\n");
			break;

		case ERR_INVALID_PID:
			debugf("Invalid process-id\n");
			break;

		case ERR_NO_DIRECTORY:
			debugf("A part of the path is no directory\n");
			break;

		case ERR_PATH_NOT_FOUND:
			debugf("Path not found\n");
			break;

		case ERR_FS_READ_FAILED:
			debugf("Read from fs failed\n");
			break;

		case ERR_INVALID_PATH:
			debugf("Invalid path\n");
			break;

		case ERR_INVALID_NODENO:
			debugf("Invalid Node-Number\n");
			break;

		case ERR_SERVUSE_SEEK:
			debugf("seek() is not possible for service-usages!\n");
			break;

		default:
			debugf("No error\n");
			break;
	}
	printStackTrace();
}
