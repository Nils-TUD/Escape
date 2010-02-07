/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stddef.h>
#include <errors.h>
#include <string.h>

#define MAX_ERR_LEN		255

char *strerror(s32 errnum) {
	static char msg[MAX_ERR_LEN + 1];
	switch(errnum) {
		case ERR_FILE_IN_USE:
			strcpy(msg,"The file is already in use!");
			break;

		case ERR_INVALID_ARGS:
			strcpy(msg,"Invalid arguments!");
			break;

		case ERR_MAX_PROC_FDS:
			strcpy(msg,"You have reached the max. possible file-descriptors!");
			break;

		case ERR_NO_FREE_FILE:
			strcpy(msg,"The max. global, open files have been reached!");
			break;

		case ERR_INVALID_FD:
			strcpy(msg,"Invalid file-descriptor!");
			break;

		case ERR_INVALID_FILE:
			strcpy(msg,"Invalid file!");
			break;

		case ERR_NO_READ_PERM:
			strcpy(msg,"No read-permission!");
			break;

		case ERR_NO_WRITE_PERM:
			strcpy(msg,"No write-permission!");
			break;

		case ERR_INV_SERVICE_NAME:
			strcpy(msg,"Invalid service name. Alphanumeric, not empty name expected!");
			break;

		case ERR_NOT_ENOUGH_MEM:
			strcpy(msg,"Not enough memory!");
			break;

		case ERR_SERVICE_EXISTS:
			strcpy(msg,"The service with desired name does already exist!");
			break;

		case ERR_NOT_OWN_SERVICE:
			strcpy(msg,"The service-node is not your own!");
			break;

		case ERR_IOMAP_RESERVED:
			strcpy(msg,"The given io-port range is reserved!");
			break;

		case ERR_IOMAP_NOT_PRESENT:
			strcpy(msg,"The io-port-map is not present (have you reserved ports?)!");
			break;

		case ERR_INVALID_IRQ_NUMBER:
			strcpy(msg,"The given IRQ-number is invalid!");
			break;

		case ERR_NO_CLIENT_WAITING:
			strcpy(msg,"No client is currently waiting!");
			break;

		case ERR_INVALID_SIGNAL:
			strcpy(msg,"Invalid signal!");
			break;

		case ERR_INVALID_PID:
			strcpy(msg,"Invalid process-id!");
			break;

		case ERR_NO_DIRECTORY:
			strcpy(msg,"A part of the path is no directory!");
			break;

		case ERR_PATH_NOT_FOUND:
			strcpy(msg,"Path not found!");
			break;

		case ERR_INVALID_PATH:
			strcpy(msg,"Invalid path!");
			break;

		case ERR_INVALID_INODENO:
			strcpy(msg,"Invalid inode-number!");
			break;

		case ERR_SERVUSE_SEEK:
			strcpy(msg,"seek() with SEEK_SET is not supported for service-usages!");
			break;

		case ERR_NO_FREE_PROCS:
			strcpy(msg,"No free process-slots!");
			break;

		case ERR_INVALID_ELF_BIN:
			strcpy(msg,"The ELF-binary is invalid!");
			break;

		case ERR_SHARED_MEM_EXISTS:
			strcpy(msg,"The shared memory with specified name or address-range does already exist!");
			break;

		case ERR_SHARED_MEM_NAME:
			strcpy(msg,"The shared memory name is invalid!");
			break;

		case ERR_SHARED_MEM_INVALID:
			strcpy(msg,"The shared memory is invalid!");
			break;

		case ERR_LOCK_NOT_FOUND:
			strcpy(msg,"Lock not found!");
			break;

		case ERR_INVALID_TID:
			strcpy(msg,"Invalid thread-id!");
			break;

		case ERR_UNSUPPORTED_OP:
			strcpy(msg,"Unsupported operation!");
			break;

		case ERR_INO_ALLOC:
			strcpy(msg,"Inode-allocation failed!");
			break;

		case ERR_INO_REQ_FAILED:
			strcpy(msg,"Inode-request failed!");
			break;

		case ERR_IS_DIR:
			strcpy(msg,"Its a directory!");
			break;

		case ERR_FILE_EXISTS:
			strcpy(msg,"The file exists!");
			break;

		case ERR_DIR_NOT_EMPTY:
			strcpy(msg,"Directory is not empty!");
			break;

		case ERR_MNTPNT_EXISTS:
			strcpy(msg,"Mount-point exists!");
			break;

		case ERR_DEV_NOT_FOUND:
			strcpy(msg,"Device not found!");
			break;

		case ERR_NO_MNTPNT:
			strcpy(msg,"No mount-point!");
			break;

		case ERR_FS_INIT_FAILED:
			strcpy(msg,"Initialisation of filesystem failed!");
			break;

		case ERR_LINK_DEVICE:
			strcpy(msg,"Hardlink to a different device not possible!");
			break;

		case ERR_MOUNT_VIRT_PATH:
			strcpy(msg,"Mount in virtual directories not supported!");
			break;

		case ERR_NO_FILE_OR_LINK:
			strcpy(msg,"No file or link!");
			break;

		case ERR_BLO_REQ_FAILED:
			strcpy(msg,"Block-request failed!");
			break;

		case ERR_INVALID_SERVID:
			strcpy(msg,"Invalid service-id!");
			break;

		case ERR_INVALID_APP:
			strcpy(msg,"Invalid application!");
			break;

		case ERR_APP_NOT_FOUND:
			strcpy(msg,"Application not found in AppsDB (maybe not installed?)!");
			break;

		case ERR_APP_IOPORTS_NO_PERM:
			strcpy(msg,"Application has no permission for the requested io-ports!");
			break;

		case ERR_APPS_SIGNAL_NO_PERM:
			strcpy(msg,"Application has no permission to announce a signal-handler for "
					"the specified signal!");
			break;

		case ERR_APP_CRTSHMEM_NO_PERM:
			strcpy(msg,"Application has no permission to create the shared-memory with specified name!");
			break;

		case ERR_APP_JOINSHMEM_NO_PERM:
			strcpy(msg,"Application has no permission to join the shared-memory with specified name!");
			break;

		case ERR_APPS_CRTSERV_NO_PERM:
			strcpy(msg,"Application has no permission to create a service!");
			break;

		case ERR_APPS_CRTFS_NO_PERM:
			strcpy(msg,"Application has no permission to create a fs!");
			break;

		case ERR_APPS_CRTDRV_NO_PERM:
			strcpy(msg,"Application has no permission to create a driver!");
			break;

		case ERR_APPS_DRV_NO_PERM:
			strcpy(msg,"Application has no permission to use the desired driver!");
			break;

		case ERR_APPS_SERV_NO_PERM:
			strcpy(msg,"Application has no permission to use the desired service!");
			break;

		case ERR_APPS_FS_NO_PERM:
			strcpy(msg,"Application has no permission to use the filesystem for the desired operations!");
			break;

		case ERR_NO_CHILD:
			strcpy(msg,"You don't have a child-process!");
			break;

		case ERR_THREAD_WAITING:
			strcpy(msg,"Another thread of your process waits for childs!");
			break;

		case ERR_INTERRUPTED:
			strcpy(msg,"Interrupted by signal!");
			break;

		case ERR_PIPE_SEEK:
			strcpy(msg,"You can't use seek() for pipes!");
			break;

		case ERR_MAX_EXIT_FUNCS:
			strcpy(msg,"You've reached the max. number of exit-functions!");
			break;

		case ERR_INVALID_KEYMAP:
			strcpy(msg,"Invalid keymap-path or content!");
			break;

		default:
			strcpy(msg,"No error");
			break;
	}
	return msg;
}
