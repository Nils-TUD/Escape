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

#ifndef ERRORS_H_
#define ERRORS_H_

/* error-codes */
#define ERR_FILE_IN_USE				-1
#define ERR_NO_FREE_FILE			-2
#define	ERR_MAX_PROC_FDS			-3
#define ERR_INVALID_ARGS			-4
#define ERR_INVALID_FD				-5
#define ERR_INVALID_FILE			-6
#define ERR_NO_READ_PERM			-7
#define ERR_NO_WRITE_PERM			-8
#define ERR_INV_SERVICE_NAME		-9
#define ERR_NOT_ENOUGH_MEM			-10
#define ERR_SERVICE_EXISTS			-11
#define ERR_NOT_OWN_SERVICE			-12
#define ERR_IOMAP_RESERVED			-13
#define ERR_IOMAP_NOT_PRESENT		-14
#define ERR_INVALID_IRQ_NUMBER		-15
#define ERR_NO_CLIENT_WAITING		-16
#define ERR_INVALID_SIGNAL			-17
#define ERR_INVALID_PID				-18
#define ERR_NO_DIRECTORY			-19
#define ERR_PATH_NOT_FOUND			-20
#define ERR_INVALID_PATH			-21
#define ERR_INVALID_INODENO			-22
#define ERR_SERVUSE_SEEK			-23
#define ERR_NO_FREE_PROCS			-24
#define ERR_INVALID_ELF_BIN			-25
#define ERR_SHARED_MEM_EXISTS		-26
#define ERR_SHARED_MEM_NAME			-27
#define ERR_SHARED_MEM_INVALID		-28
#define ERR_LOCK_NOT_FOUND			-29
#define ERR_INVALID_TID				-30
#define ERR_UNSUPPORTED_OP			-31
#define ERR_INO_ALLOC				-32
#define ERR_INO_REQ_FAILED			-33
#define ERR_IS_DIR					-34
#define ERR_FILE_EXISTS				-35
#define ERR_DIR_NOT_EMPTY			-36
#define ERR_MNTPNT_EXISTS			-37
#define ERR_DEV_NOT_FOUND			-38
#define ERR_NO_MNTPNT				-39
#define ERR_FS_INIT_FAILED			-40
#define ERR_LINK_DEVICE				-41
#define ERR_MOUNT_VIRT_PATH			-42
#define ERR_NO_FILE_OR_LINK			-43
#define ERR_BLO_REQ_FAILED			-44
#define ERR_INVALID_SERVID			-45
#define ERR_INVALID_APP				-46
#define ERR_APP_NOT_FOUND			-47
#define ERR_APP_IOPORTS_NO_PERM		-48
#define ERR_APPS_SIGNAL_NO_PERM		-49
#define ERR_APP_CRTSHMEM_NO_PERM	-50
#define ERR_APP_JOINSHMEM_NO_PERM	-51
#define ERR_APPS_CRTSERV_NO_PERM	-52
#define ERR_APPS_CRTFS_NO_PERM		-53
#define ERR_APPS_CRTDRV_NO_PERM		-54
#define ERR_APPS_DRV_NO_PERM		-55
#define ERR_APPS_SERV_NO_PERM		-56
#define ERR_APPS_FS_NO_PERM			-57
#define ERR_NO_CHILD				-58
#define ERR_THREAD_WAITING			-59
#define ERR_INTERRUPTED				-60
#define ERR_PIPE_SEEK				-61
#define ERR_MAX_EXIT_FUNCS			-62
#define ERR_INVALID_KEYMAP			-63
#define ERR_VESA_SETMODE_FAILED		-64
#define ERR_VESA_MODE_NOT_FOUND		-65

#if IN_KERNEL
#define ERR_REAL_PATH				-200
#endif

#endif /* ERRORS_H_ */
