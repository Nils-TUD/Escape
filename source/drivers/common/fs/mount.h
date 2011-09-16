/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/fsinterface.h>
#include <stdio.h>

#define MAX_MNTNAME_LEN			32
#define MOUNT_TABLE_SIZE		32
#define ROOT_MNT_DEV			-1
#define ROOT_MNT_INO			-1

typedef struct {
	uid_t uid;
	gid_t gid;
	pid_t pid;
} sFSUser;

/* The handler for the functions of the filesystem */
typedef void *(*fFSInit)(const char *device,char **usedDev);
typedef void (*fFSDeinit)(void *h);
typedef inode_t (*fFSResPath)(void *h,sFSUser *u,const char *path,uint flags,dev_t *dev,
		bool resLastMnt);
typedef inode_t (*fFSOpen)(void *h,sFSUser *u,inode_t ino,uint flags);
typedef void (*fFSClose)(void *h,inode_t ino);
typedef int (*fFSStat)(void *h,inode_t ino,sFileInfo *info);
typedef ssize_t (*fFSRead)(void *h,inode_t inodeNo,void *buffer,off_t offset,size_t count);
typedef ssize_t (*fFSWrite)(void *h,inode_t inodeNo,const void *buffer,off_t offset,size_t count);
typedef int (*fFSLink)(void *h,sFSUser *u,inode_t dstIno,inode_t dirIno,const char *name);
typedef int (*fFSUnlink)(void *h,sFSUser *u,inode_t dirIno,const char *name);
typedef int (*fFSMkDir)(void *h,sFSUser *u,inode_t dirIno,const char *name);
typedef int (*fFSRmDir)(void *h,sFSUser *u,inode_t dirIno,const char *name);
typedef int (*fFSChmod)(void *h,sFSUser *u,inode_t ino,mode_t mode);
typedef int (*fFSChown)(void *h,sFSUser *u,inode_t ino,uid_t uid,gid_t gid);
typedef void (*fFSSync)(void *h);
typedef void (*fFSPrint)(FILE *f,void *h);

/* all information about a filesystem */
typedef struct {
	int type;
	fFSInit init;			/* required */
	fFSDeinit deinit;		/* required */
	fFSResPath resPath;		/* required */
	fFSOpen open;			/* required */
	fFSClose close;			/* required */
	fFSStat stat;			/* required */
	fFSPrint print;			/* required */
	fFSRead read;			/* optional */
	fFSWrite write;			/* optional */
	fFSLink link;			/* optional */
	fFSUnlink unlink;		/* optional */
	fFSMkDir mkdir;			/* optional */
	fFSRmDir rmdir;			/* optional */
	fFSChmod chmod;			/* optional */
	fFSChown chown;			/* optional */
	fFSSync sync;			/* optional */
} sFileSystem;

/* one instance of a filesystem for a specific device */
typedef struct {
	void *handle;
	char device[MAX_MNTNAME_LEN];
	sFileSystem *fs;
	uint refs;
} sFSInst;

/* mount-point that links (inode+dev) to a specific fs-instance */
typedef struct {
	dev_t dev;
	inode_t inode;
	char path[MAX_PATH_LEN + 1];
	sFSInst *mnt;
} sMountPoint;

/**
 * Inits the moint-points
 *
 * @return true if successfull
 */
bool mount_init(void);

/**
 * @param i the index
 * @return the fs-instance with given index or NULL if there is no instance with that index
 */
const sFSInst *mount_getFSInst(size_t i);

/**
 * Adds the given file-system (completely initialized!)
 *
 * @param fs the filesystem
 * @return 0 on success
 */
int mount_addFS(sFileSystem *fs);

/**
 * Adds a moint-point @ (dev+inode) to the given device using the given filesystem
 *
 * @param dev the device-number of the mount-point
 * @param inode the inode-number of the mount-point
 * @param path the path of the mount-point
 * @param device the device-path
 * @param type the fs-type
 * @return the device-number (mount-point) on success or < 0
 */
dev_t mount_addMnt(dev_t dev,inode_t inode,const char *path,const char *device,int type);

/**
 * @param i the mount-point
 * @return the mount-point with index i or NULL if not existing
 */
const sMountPoint *mount_getByIndex(size_t i);

/**
 * Determines the moint-point-id for the given location
 *
 * @param dev the device-number of the mount-point
 * @param inode the inode-number of the mount-point
 * @return the mount-point or < 0
 */
dev_t mount_getByLoc(dev_t dev,inode_t inode);

/**
 * Removes the given mount-point
 *
 * @param dev the device-number of the mount-point
 * @param inode the inode-number of the mount-point
 * @return 0 on success
 */
int mount_remMnt(dev_t dev,inode_t inode);

/**
 * Determines the device-number of an fs-instance by its handle. This gives fs-instances
 * the chance to determine their own device-id.
 *
 * @param h the handle of the fs-instance
 * @return the device-number or < 0
 */
dev_t mount_getDevByHandle(void *h);

/**
 * @return the associated filesystem for the given mount-point
 *
 * @param dev the mount-point
 * @return the file-system-instance
 */
sFSInst *mount_get(dev_t dev);

#endif /* MOUNT_H_ */
