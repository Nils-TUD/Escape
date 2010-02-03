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

#ifndef MOUNT_H_
#define MOUNT_H_

#include <esc/common.h>
#include <fsinterface.h>

#define MAX_MNTNAME_LEN			32
#define MOUNT_TABLE_SIZE		32
#define ROOT_MNT_DEV			-1
#define ROOT_MNT_INO			-1

/* The handler for the functions of the filesystem */
typedef void *(*fFSInit)(const char *driver);
typedef void (*fFSDeinit)(void *h);
typedef tInodeNo (*fFSResPath)(void *h,char *path,u8 flags,tDevNo *dev,bool resLastMnt);
typedef s32 (*fFSOpen)(void *h,tInodeNo ino,u8 flags);
typedef void (*fFSClose)(void *h,tInodeNo ino);
typedef s32 (*fFSStat)(void *h,tInodeNo ino,sFileInfo *info);
typedef s32 (*fFSRead)(void *h,tInodeNo inodeNo,void *buffer,u32 offset,u32 count);
typedef s32 (*fFSWrite)(void *h,tInodeNo inodeNo,const void *buffer,u32 offset,u32 count);
typedef s32 (*fFSLink)(void *h,tInodeNo dstIno,tInodeNo dirIno,const char *name);
typedef s32 (*fFSUnlink)(void *h,tInodeNo dirIno,const char *name);
typedef s32 (*fFSMkDir)(void *h,tInodeNo dirIno,const char *name);
typedef s32 (*fFSRmDir)(void *h,tInodeNo dirIno,const char *name);
typedef void (*fFSSync)(void *h);

/* all information about a filesystem */
typedef struct {
	u16 type;
	fFSInit init;			/* required */
	fFSDeinit deinit;		/* required */
	fFSResPath resPath;		/* required */
	fFSOpen open;			/* required */
	fFSClose close;			/* required */
	fFSStat stat;			/* required */
	fFSRead read;			/* optional */
	fFSWrite write;			/* optional */
	fFSLink link;			/* optional */
	fFSUnlink unlink;		/* optional */
	fFSMkDir mkdir;			/* optional */
	fFSRmDir rmdir;			/* optional */
	fFSSync sync;			/* optional */
} sFileSystem;

/* one instance of a filesystem for a specific device */
typedef struct {
	void *handle;
	char driver[MAX_MNTNAME_LEN];
	sFileSystem *fs;
	u32 refs;
} sFSInst;

/* mount-point that links (inode+dev) to a specific fs-instance */
typedef struct {
	tDevNo dev;
	tInodeNo inode;
	sFSInst *mnt;
} sMountPoint;

/**
 * Inits the moint-points
 *
 * @return true if successfull
 */
bool mount_init(void);

/**
 * Adds the given file-system (completely initialized!)
 *
 * @param fs the filesystem
 * @return 0 on success
 */
s32 mount_addFS(sFileSystem *fs);

/**
 * Adds a moint-point @ (dev+inode) to the given driver using the given filesystem
 *
 * @param dev the device-number of the mount-point
 * @param inode the inode-number of the mount-point
 * @param driver the driver-path
 * @param type the fs-type
 * @return the device-number (mount-point) on success or < 0
 */
tDevNo mount_addMnt(tDevNo dev,tInodeNo inode,const char *driver,u16 type);

/**
 * Determines the moint-point-id for the given location
 *
 * @param dev the device-number of the mount-point
 * @param inode the inode-number of the mount-point
 * @return the mount-point or < 0
 */
tDevNo mount_getByLoc(tDevNo dev,tInodeNo inode);

/**
 * Removes the given mount-point
 *
 * @param dev the device-number of the mount-point
 * @param inode the inode-number of the mount-point
 * @return 0 on success
 */
s32 mount_remMnt(tDevNo dev,tInodeNo inode);

/**
 * Determines the device-number of an fs-instance by its handle. This gives fs-instances
 * the chance to determine their own device-id.
 *
 * @param h the handle of the fs-instance
 * @return the device-number or < 0
 */
tDevNo mount_getDevByHandle(void *h);

/**
 * @return the associated filesystem for the given mount-point
 *
 * @param dev the mount-point
 * @return the file-system-instance
 */
sFSInst *mount_get(tDevNo dev);

#endif /* MOUNT_H_ */
