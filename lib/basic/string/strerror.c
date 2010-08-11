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

static const char *msgs[] = {
	/* 0 (No error) */						"No error",
	/* -1 (ERR_FILE_IN_USE) */				"The file is already in use!",
	/* -2 (ERR_NO_FREE_FILE) */				"The max. global, open files have been reached!",
	/* -3 (ERR_MAX_PROC_FDS) */				"You have reached the max. possible file-descriptors!",
	/* -4 (ERR_INVALID_ARGS) */				"Invalid arguments!",
	/* -5 (ERR_INVALID_FD) */				"Invalid file-descriptor!",
	/* -6 (ERR_INVALID_FILE) */				"Invalid file!",
	/* -7 (ERR_NO_READ_PERM) */				"No read-permission!",
	/* -8 (ERR_NO_WRITE_PERM) */			"No write-permission!",
	/* -9 (ERR_INV_DRIVER_NAME) */			"Invalid driver name. Alphanumeric, not empty name expected!",
	/* -10 (ERR_NOT_ENOUGH_MEM) */			"Not enough memory!",
	/* -11 (ERR_DRIVER_EXISTS) */			"The driver with desired name does already exist!",
	/* -12 (ERR_NOT_OWN_DRIVER) */			"The driver-node is not your own!",
	/* -13 (ERR_IOMAP_RESERVED) */			"The given io-port range is reserved!",
	/* -14 (ERR_IOMAP_NOT_PRESENT) */		"The io-port-map is not present (have you reserved ports?)!",
	/* -15 (ERR_INVALID_IRQ_NUMBER) */		"The given IRQ-number is invalid!",
	/* -16 (ERR_NO_CLIENT_WAITING) */		"No client is currently waiting!",
	/* -17 (ERR_INVALID_SIGNAL) */			"Invalid signal!",
	/* -18 (ERR_INVALID_PID) */				"Invalid process-id!",
	/* -19 (ERR_NO_DIRECTORY) */			"A part of the path is no directory!",
	/* -20 (ERR_PATH_NOT_FOUND) */			"Path not found!",
	/* -21 (ERR_INVALID_PATH) */			"Invalid path!",
	/* -22 (ERR_INVALID_INODENO) */			"Invalid inode-number!",
	/* -23 (ERR_DRVUSE_SEEK) */				"seek() with SEEK_SET is not supported for driver-usages!",
	/* -24 (ERR_NO_FREE_PROCS) */			"No free process-slots!",
	/* -25 (ERR_INVALID_ELF_BIN) */			"The ELF-binary is invalid!",
	/* -26 (ERR_SHARED_MEM_EXISTS) */		"The shared memory with specified name or address-range does already exist!",
	/* -27 (ERR_SHARED_MEM_NAME) */			"The shared memory name is invalid!",
	/* -28 (ERR_SHARED_MEM_INVALID) */		"The shared memory is invalid!",
	/* -29 (ERR_LOCK_NOT_FOUND) */			"Lock not found!",
	/* -30 (ERR_INVALID_TID) */				"Invalid thread-id!",
	/* -31 (ERR_UNSUPPORTED_OP) */			"Unsupported operation!",
	/* -32 (ERR_INO_ALLOC) */				"Inode-allocation failed!",
	/* -33 (ERR_INO_REQ_FAILED) */			"Inode-request failed!",
	/* -34 (ERR_IS_DIR) */					"Its a directory!",
	/* -35 (ERR_FILE_EXISTS) */				"The file exists!",
	/* -36 (ERR_DIR_NOT_EMPTY) */			"Directory is not empty!",
	/* -37 (ERR_MNTPNT_EXISTS) */			"Mount-point exists!",
	/* -38 (ERR_DEV_NOT_FOUND) */			"Device not found!",
	/* -39 (ERR_NO_MNTPNT) */				"No mount-point!",
	/* -40 (ERR_FS_INIT_FAILED) */			"Initialisation of filesystem failed!",
	/* -41 (ERR_LINK_DEVICE) */				"Hardlink to a different device not possible!",
	/* -42 (ERR_MOUNT_VIRT_PATH) */			"Mount in virtual directories not supported!",
	/* -43 (ERR_NO_FILE_OR_LINK) */			"No file or link!",
	/* -44 (ERR_BLO_REQ_FAILED) */			"Block-request failed!",
	/* -45 (ERR_INVALID_DRVID) */			"Invalid driver-id!",
	/* -46 (ERR_INVALID_APP) */				"Invalid application!",
	/* -47 (ERR_APP_NOT_FOUND) */			"Application not found in AppsDB (maybe not installed?)!",
	/* -48 (ERR_APP_IOPORTS_NO_PERM) */		"Application has no permission for the requested io-ports!",
	/* -49 (ERR_APPS_SIGNAL_NO_PERM) */		"Application has no permission to announce a signal-handler for the specified signal!",
	/* -50 (ERR_APP_CRTSHMEM_NO_PERM) */	"Application has no permission to create the shared-memory with specified name!",
	/* -51 (ERR_APP_JOINSHMEM_NO_PERM) */	"Application has no permission to join the shared-memory with specified name!",
	/* -52 (ERR_APPS_CRTDRV_NO_PERM) */		"Application has no permission to create a driver!",
	/* -53 (ERR_APPS_CRTFS_NO_PERM) */		"Application has no permission to create a fs!",
	/* -54 (--) */							"",
	/* -55 (ERR_APPS_DRV_NO_PERM) */		"Application has no permission to use the desired driver!",
	/* -56 (--) */							"",
	/* -57 (ERR_APPS_FS_NO_PERM) */			"Application has no permission to use the filesystem for the desired operations!",
	/* -58 (ERR_NO_CHILD) */				"You don't have a child-process!",
	/* -59 (ERR_THREAD_WAITING) */			"Another thread of your process waits for childs!",
	/* -60 (ERR_INTERRUPTED) */				"Interrupted by signal!",
	/* -61 (ERR_PIPE_SEEK) */				"You can't use seek() for pipes!",
	/* -62 (ERR_MAX_EXIT_FUNCS) */			"You've reached the max. number of exit-functions!",
	/* -63 (ERR_INVALID_KEYMAP) */			"Invalid keymap-path or content!",
	/* -64 (ERR_VESA_SETMODE_FAILED) */		"Unable to set VESA-mode!",
	/* -65 (ERR_VESA_MODE_NOT_FOUND) */		"Unable to find the requested VESA-mode!",
	/* -66 (ERR_NO_VM86_TASK) */			"A VM86-task doesn't exist (maybe killed?)!",
	/* -67 (ERR_LISTENER_EXISTS) */			"The keylistener does already exist!",
	/* -68 (ERR_EOF) */						"EOF reached!",
	/* -69 (ERR_DRIVER_DIED) */				"The driver died!",
	/* -70 (ERR_SETPROT_IMPOSSIBLE) */		"Setting the region-protection is not possible for the chosen region",
};

const char *strerror(s32 errnum) {
	if((u32)-errnum < ARRAY_SIZE(msgs))
		return msgs[-errnum];
	else
		return msgs[0];
}
