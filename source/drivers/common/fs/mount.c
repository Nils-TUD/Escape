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

#include <esc/common.h>
#include <esc/fsinterface.h>
#include <assert.h>
#include <errors.h>
#include <string.h>
#include <esc/sllist.h>
#include <stdlib.h>
#include "mount.h"

static size_t mntPntCount = 0;
static sSLList *fileSystems = NULL;
static sSLList *fsInsts = NULL;
static sMountPoint mounts[MOUNT_TABLE_SIZE];

bool mount_init(void) {
	size_t i;
	fileSystems = sll_create();
	fsInsts = sll_create();
	for(i = 0; i < MOUNT_TABLE_SIZE; i++) {
		mounts[i].dev = 0;
		mounts[i].inode = 0;
		mounts[i].mnt = NULL;
	}
	return fileSystems != NULL && fsInsts != NULL;
}

const sFSInst *mount_getFSInst(size_t i) {
	if(i < sll_length(fsInsts))
		return (const sFSInst*)sll_get(fsInsts,i);
	return NULL;
}

int mount_addFS(sFileSystem *fs) {
	if(!sll_append(fileSystems,fs))
		return ERR_NOT_ENOUGH_MEM;
	return 0;
}

dev_t mount_addMnt(dev_t dev,inode_t inode,const char *path,const char *driver,int type) {
	size_t i;
	sFSInst *inst;
	sFileSystem *fs;
	sSLNode *n;

	/* check name */
	if(strlen(driver) >= MAX_MNTNAME_LEN)
		return ERR_INVALID_PATH;

	/* check if the mount-point exists */
	for(i = 0; i < MOUNT_TABLE_SIZE; i++) {
		if(mounts[i].dev == dev && mounts[i].inode == inode)
			return ERR_MNTPNT_EXISTS;
	}

	/* first find the filesystem */
	for(n = sll_begin(fileSystems); n != NULL; n = n->next) {
		fs = (sFileSystem*)n->data;
		if(fs->type == type)
			break;
	}
	if(n == NULL)
		return ERR_DEV_NOT_FOUND;

	/* find mount-slot */
	for(i = 0; i < MOUNT_TABLE_SIZE; i++) {
		if(mounts[i].mnt == NULL)
			break;
	}
	if(i >= MOUNT_TABLE_SIZE)
		return ERR_NO_MNTPNT;

	/* look if there is an instance we can use */
	for(n = sll_begin(fsInsts); n != NULL; n = n->next) {
		inst = (sFSInst*)n->data;
		if(inst->fs->type == type && strcmp(inst->driver,driver) == 0)
			break;
	}

	/* if not, create a new one */
	if(n == NULL) {
		char *usedDev;
		inst = (sFSInst*)malloc(sizeof(sFSInst));
		if(inst == NULL)
			return ERR_NOT_ENOUGH_MEM;
		inst->refs = 0;
		inst->fs = fs;
		inst->handle = fs->init(driver,&usedDev);
		if(inst->handle == NULL) {
			free(inst);
			return ERR_FS_INIT_FAILED;
		}
		strcpy(inst->driver,usedDev);
		free(usedDev);
		if(!sll_append(fsInsts,inst)) {
			free(inst);
			return ERR_NOT_ENOUGH_MEM;
		}
	}
	inst->refs++;

	/* build moint-point */
	mounts[i].dev = dev;
	mounts[i].inode = inode;
	mounts[i].mnt = inst;
	strnzcpy(mounts[i].path,path,sizeof(mounts[i].path));
	if(dev != ROOT_MNT_DEV && inode != ROOT_MNT_INO)
		mntPntCount++;
	return i;
}

const sMountPoint *mount_getByIndex(size_t i) {
	assert(i < MOUNT_TABLE_SIZE);
	if(mounts[i].mnt)
		return mounts + i;
	return NULL;
}

dev_t mount_getByLoc(dev_t dev,inode_t inode) {
	size_t i,x;
	/* we have mntPntCount+1 because of the root-fs */
	for(i = 0,x = 0; x <= mntPntCount && i < MOUNT_TABLE_SIZE; i++) {
		if(mounts[i].dev == dev && mounts[i].inode == inode)
			return i;
		if(mounts[i].mnt)
			x++;
	}
	return ERR_NO_MNTPNT;
}

int mount_remMnt(dev_t dev,inode_t inode) {
	size_t i,j;
	/* search mount-point */
	for(i = 0; i < MOUNT_TABLE_SIZE; i++) {
		if(mounts[i].dev == dev && mounts[i].inode == inode)
			break;
	}
	if(i >= MOUNT_TABLE_SIZE)
		return ERR_NO_MNTPNT;

	/* remove all mounts in this mounted device recursively */
	for(j = 0; j < MOUNT_TABLE_SIZE; j++) {
		if(mounts[j].mnt && mounts[j].dev == (dev_t)i)
			mount_remMnt(i,mounts[j].inode);
	}

	if(--mounts[i].mnt->refs == 0) {
		/* call deinit to give the fs the chance to write unwritten stuff etc. */
		mounts[i].mnt->fs->deinit(mounts[i].mnt->handle);
		/* free fs-instance */
		sll_removeFirstWith(fsInsts,mounts[i].mnt);
		free(mounts[i].mnt);
	}
	mounts[i].mnt = NULL;
	mounts[i].inode = 0;
	mounts[i].dev = 0;
	mntPntCount--;
	return 0;
}

dev_t mount_getDevByHandle(void *h) {
	size_t i;
	for(i = 0; i < MOUNT_TABLE_SIZE; i++) {
		if(mounts[i].mnt->handle == h)
			return i;
	}
	return ERR_NO_MNTPNT;
}

sFSInst *mount_get(dev_t dev) {
	if(dev >= MOUNT_TABLE_SIZE)
		return NULL;
	return mounts[dev].mnt;
}
