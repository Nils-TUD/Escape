/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef ERRORS_H_
#define ERRORS_H_

/* error-codes */
#define ERR_FILE_IN_USE				-1
#define ERR_NO_FREE_FD				-2
#define	ERR_MAX_PROC_FDS			-3
#define ERR_VFS_NODE_NOT_FOUND		-4
#define ERR_INVALID_SYSC_ARGS		-5
#define ERR_INVALID_FD				-6
#define ERR_INVALID_FILE			-7
#define ERR_NO_READ_PERM			-8
#define ERR_NO_WRITE_PERM			-9
#define ERR_INV_SERVICE_NAME		-10
#define ERR_NOT_ENOUGH_MEM			-11
#define ERR_SERVICE_EXISTS			-12
#define ERR_PROC_DUP_SERVICE		-13
#define ERR_PROC_DUP_SERVICE_USE	-14
#define ERR_SERVICE_NOT_IN_USE		-15
#define ERR_NOT_OWN_SERVICE			-16
#define ERR_IO_MAP_RANGE_RESERVED	-17
#define ERR_IOMAP_NOT_PRESENT		-18
#define ERR_INTRPT_LISTENER_MSGLEN	-19
#define ERR_INVALID_IRQ_NUMBER		-20
#define ERR_IRQ_LISTENER_MISSING	-21
#define ERR_NO_CLIENT_WAITING		-22
#define ERR_FS_NOT_FOUND			-23
#define ERR_INVALID_SIGNAL			-24
#define ERR_INVALID_PID				-25
#define ERR_NO_DIRECTORY			-26
#define ERR_PATH_NOT_FOUND			-27
#define ERR_FS_READ_FAILED			-28
#define ERR_INVALID_PATH			-29
#define ERR_INVALID_NODENO			-30
#define ERR_SERVUSE_SEEK			-31
#define ERR_MAX_PROCS_REACHED		-32

#if IN_KERNEL
#define ERR_REAL_PATH				-200
#endif

#endif /* ERRORS_H_ */
