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
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/proc.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "iso9660.h"
#include "rw.h"
#include "dir.h"
#include "file.h"
#include "direcache.h"

#define MAX_DEVICE_OPEN_RETRIES		1000

static int iso_setup(const char *device,sISO9660 *iso);

void *iso_init(const char *device,char **usedDev,int *errcode) {
	int res;
	size_t i;
	sISO9660 *iso = (sISO9660*)malloc(sizeof(sISO9660));
	if(iso == NULL) {
		*errcode = -ENOMEM;
		return NULL;
	}

	for(i = 0; i < ARRAY_SIZE(iso->drvFds); i++)
		iso->drvFds[i] = -1;
	iso->blockCache.handle = iso;
	iso->blockCache.blockCacheSize = ISO_BCACHE_SIZE;
	/* cast is ok, because the only difference is that iso_rw_* receive a sISO9660* as first argument
	 * and read/write expect void* */
	iso->blockCache.read = (fReadBlocks)iso_rw_readBlocks;
	/* no writing here ;) */
	iso->blockCache.write = NULL;

	/* if the device is provided, simply use it */
	if(strcmp(device,"cdrom") != 0) {
		if((res = iso_setup(device,iso)) < 0) {
			printe("Unable to find device '%s'",device);
			free(iso);
			*errcode = res;
			return NULL;
		}
	}
	/* otherwise try all possible ATAPI-drives */
	else {
		/* just needed if we would have a mount-point on the cd. this can't happen at this point */
		char path[SSTRLEN("/dev/cda1") + 1];
		dev_t dev = 0x1234;
		inode_t ino;
		sFSUser u;
		u.uid = ROOT_UID;
		u.gid = ROOT_GID;
		u.pid = getpid();
		for(i = 0; i < 4; i++) {
			snprintf(path,sizeof(path),"/dev/cd%c1",'a' + i);
			if(iso_setup(path,iso) < 0)
				continue;

			/* try to find our kernel. if we've found it, it's likely that the user wants to
			 * boot from this device. unfortunatly there doesn't seem to be an easy way
			 * to find out the real boot-device from GRUB */
			ino = iso_dir_resolve(iso,&u,"/boot/escape",IO_READ,&dev,false);
			if(ino >= 0)
				break;

			/* destroy the block-cache; we'll create a new one with iso_setup() in the next loop */
			bcache_destroy(&iso->blockCache);
		}

		/* not found? */
		if(i >= 4) {
			free(iso);
			*errcode = -ENXIO;
			return NULL;
		}

		device = path;
	}

	/* report used device */
	*usedDev = malloc(strlen(device) + 1);
	if(!*usedDev) {
		printe("Not enough mem for device-name");
		bcache_destroy(&iso->blockCache);
		free(iso);
		*errcode = -ENOMEM;
		return NULL;
	}
	strcpy(*usedDev,device);
	return iso;
}

static int iso_setup(const char *device,sISO9660 *iso) {
	size_t i;
	/* now open the device */
	for(i = 0; i < ARRAY_SIZE(iso->drvFds); i++) {
		iso->drvFds[i] = open(device,IO_WRITE | IO_READ);
		if(iso->drvFds[i] < 0)
			return iso->drvFds[i];
	}

	/* read volume descriptors */
	for(i = 0; ; i++) {
		if(iso_rw_readSectors(iso,&iso->primary,ISO_VOL_DESC_START + i,1) != 0)
			error("Unable to read volume descriptor from device");
		/* we need just the primary one */
		if(iso->primary.type == ISO_VOL_TYPE_PRIMARY)
			break;
	}

	iso->blockCache.blockSize = ISO_BLK_SIZE(iso);

	iso_direc_init(iso);
	bcache_init(&iso->blockCache,iso->drvFds[0]);
	return 0;
}

void iso_deinit(void *h) {
	size_t i;
	sISO9660 *iso = (sISO9660*)h;
	for(i = 0; i < ARRAY_SIZE(iso->drvFds); i++) {
		if(iso->drvFds[i] >= 0)
			close(iso->drvFds[i]);
	}
	bcache_destroy(&iso->blockCache);
}

sFileSystem *iso_getFS(void) {
	sFileSystem *fs = malloc(sizeof(sFileSystem));
	if(!fs)
		return NULL;
	fs->type = FS_TYPE_ISO9660;
	fs->readonly = true;
	fs->init = iso_init;
	fs->deinit = iso_deinit;
	fs->resPath = iso_resPath;
	fs->open = iso_open;
	fs->close = iso_close;
	fs->stat = iso_stat;
	fs->read = iso_read;
	fs->write = NULL;
	fs->link = NULL;
	fs->unlink = NULL;
	fs->mkdir = NULL;
	fs->rmdir = NULL;
	fs->chmod = NULL;
	fs->chown = NULL;
	fs->sync = NULL;
	fs->print = iso_print;
	return fs;
}

inode_t iso_resPath(void *h,sFSUser *u,const char *path,uint flags,dev_t *dev,bool resLastMnt) {
	return iso_dir_resolve((sISO9660*)h,u,path,flags,dev,resLastMnt);
}

inode_t iso_open(A_UNUSED void *h,A_UNUSED sFSUser *u,inode_t ino,A_UNUSED uint flags) {
	/* nothing to do */
	return ino;
}

void iso_close(A_UNUSED void *h,A_UNUSED inode_t ino) {
	/* nothing to do */
}

int iso_stat(void *h,inode_t ino,sFileInfo *info) {
	time_t ts;
	sISO9660 *i = (sISO9660*)h;
	const sISOCDirEntry *e = iso_direc_get(i,ino);
	if(e == NULL)
		return -ENOBUFS;

	ts = iso_dirDate2Timestamp(i,&e->entry.created);
	info->accesstime = ts;
	info->modifytime = ts;
	info->createtime = ts;
	info->blockCount = e->entry.extentSize.littleEndian / ISO_BLK_SIZE(i);
	info->blockSize = ISO_BLK_SIZE(i);
	info->device = mount_getDevByHandle(h);
	info->uid = 0;
	info->gid = 0;
	info->inodeNo = e->id;
	info->linkCount = 1;
	/* readonly here */
	if(e->entry.flags & ISO_FILEFL_DIR)
		info->mode = S_IFDIR | 0555;
	else
		info->mode = S_IFREG | 0555;
	info->size = e->entry.extentSize.littleEndian;
	return 0;
}

ssize_t iso_read(void *h,inode_t inodeNo,void *buffer,off_t offset,size_t count) {
	return iso_file_read((sISO9660*)h,inodeNo,buffer,offset,count);
}

time_t iso_dirDate2Timestamp(A_UNUSED sISO9660 *h,const sISODirDate *ddate) {
	return timeof(ddate->month - 1,ddate->day - 1,ddate->year,
			ddate->hour,ddate->minute,ddate->second);
}

void iso_print(FILE *f,void *h) {
	sISO9660 *i = (sISO9660*)h;
	sISOVolDesc *desc = &i->primary;
	sISOVolDate *date;
	fprintf(f,"\tIdentifier: %.5s\n",desc->identifier);
	fprintf(f,"\tSize: %u bytes\n",desc->data.primary.volSpaceSize.littleEndian * ISO_BLK_SIZE(i));
	fprintf(f,"\tBlocksize: %u bytes\n",ISO_BLK_SIZE(i));
	fprintf(f,"\tSystemIdent: %.32s\n",desc->data.primary.systemIdent);
	fprintf(f,"\tVolumeIdent: %.32s\n",desc->data.primary.volumeIdent);
	date = &desc->data.primary.created;
	fprintf(f,"\tCreated: %.4s-%.2s-%.2s %.2s:%.2s:%.2s\n",
			date->year,date->month,date->day,date->hour,date->minute,date->second);
	date = &desc->data.primary.modified;
	fprintf(f,"\tModified: %.4s-%.2s-%.2s %.2s:%.2s:%.2s\n",
			date->year,date->month,date->day,date->hour,date->minute,date->second);
	fprintf(f,"\tBlock cache:\n");
	bcache_printStats(f,&i->blockCache);
	fprintf(f,"\tDirectory entry cache:\n");
	iso_dire_print(f,i);
}

#if DEBUGGING

void iso_dbg_printPathTbl(sISO9660 *h) {
	size_t tblSize = h->primary.data.primary.pathTableSize.littleEndian;
	size_t secCount = (tblSize + ATAPI_SECTOR_SIZE - 1) / ATAPI_SECTOR_SIZE;
	sISOPathTblEntry *pe = malloc(ROUND_UP(tblSize,ATAPI_SECTOR_SIZE));
	sISOPathTblEntry *start = pe;
	if(iso_rw_readSectors(h,pe,h->primary.data.primary.lPathTblLoc,secCount) != 0) {
		free(start);
		return;
	}
	printf("Path-Table:\n");
	while((uintptr_t)pe < ((uintptr_t)start + tblSize)) {
		printf("	length: %u\n",pe->length);
		printf("	extentLoc: %u\n",pe->extentLoc);
		printf("	parentTblIndx: %u\n",pe->parentTblIndx);
		printf("	name: %s\n",pe->name);
		printf("---\n");
		if(pe->length % 2 == 0)
			pe = (sISOPathTblEntry*)((uintptr_t)pe + sizeof(sISOPathTblEntry) + pe->length);
		else
			pe = (sISOPathTblEntry*)((uintptr_t)pe + sizeof(sISOPathTblEntry) + pe->length + 1);
	}
	free(start);
}

void iso_dbg_printTree(sISO9660 *h,size_t extLoc,size_t extSize,size_t layer) {
	size_t i;
	sISODirEntry *e;
	sISODirEntry *content = (sISODirEntry*)malloc(extSize);
	if(content == NULL)
		return;
	if(iso_rw_readSectors(h,content,extLoc,extSize / ATAPI_SECTOR_SIZE) != 0)
		return;

	e = content;
	while((uintptr_t)e < (uintptr_t)content + extSize) {
		char bak;
		if(e->length == 0)
			break;
		for(i = 0; i < layer; i++)
			printf("  ");
		if(e->name[0] == ISO_FILENAME_THIS)
			printf("name: '.'\n");
		else if(e->name[0] == ISO_FILENAME_PARENT)
			printf("name: '..'\n");
		else {
			bak = e->name[e->nameLen];
			e->name[e->nameLen] = '\0';
			printf("name: '%s'\n",e->name);
			e->name[e->nameLen] = bak;
		}
		if((e->flags & ISO_FILEFL_DIR) && e->name[0] != ISO_FILENAME_PARENT &&
				e->name[0] != ISO_FILENAME_THIS) {
			iso_dbg_printTree(h,e->extentLoc.littleEndian,e->extentSize.littleEndian,layer + 1);
		}
		e = (sISODirEntry*)((uintptr_t)e + e->length);
	}
	free(content);
}

void iso_dbg_printVolDesc(sISOVolDesc *desc) {
	printf("VolumeDescriptor @ %p\n",desc);
	printf("	version: 0x%02x\n",desc->version);
	printf("	identifier: %.5s\n",desc->identifier);
	switch(desc->type) {
		case ISO_VOL_TYPE_BOOTRECORD:
			printf("	type: bootrecord\n");
			printf("	bootSystemIdent: %.32s\n",desc->data.bootrecord.bootSystemIdent);
			printf("	bootIdent: %.32s\n",desc->data.bootrecord.bootIdent);
			break;
		case ISO_VOL_TYPE_PRIMARY:
			printf("	type: primary\n");
			printf("	systemIdent: %.32s\n",desc->data.primary.systemIdent);
			printf("	volumeIdent: %.32s\n",desc->data.primary.volumeIdent);
			printf("	volumeSetIdent: %.128s\n",desc->data.primary.volumeSetIdent);
			printf("	copyrightFile: %.38s\n",desc->data.primary.copyrightFile);
			printf("	abstractFile: %.36s\n",desc->data.primary.abstractFile);
			printf("	bibliographicFile: %.37s\n",desc->data.primary.bibliographicFile);
			printf("	applicationIdent: %.128s\n",desc->data.primary.applicationIdent);
			printf("	dataPreparerIdent: %.128s\n",desc->data.primary.dataPreparerIdent);
			printf("	publisherIdent: %.128s\n",desc->data.primary.publisherIdent);
			printf("	fileStructureVersion: %u\n",desc->data.primary.fileStructureVersion);
			printf("	lOptPathTblLoc: %u\n",desc->data.primary.lOptPathTblLoc);
			printf("	lPathTblLoc: %u\n",desc->data.primary.lPathTblLoc);
			printf("	mOptPathTblLoc: %u\n",desc->data.primary.mOptPathTblLoc);
			printf("	mPathTblLoc: %u\n",desc->data.primary.mPathTblLoc);
			printf("	logBlkSize: %u\n",desc->data.primary.logBlkSize.littleEndian);
			printf("	pathTableSize: %u\n",desc->data.primary.pathTableSize.littleEndian);
			printf("	volSeqNo: %u\n",desc->data.primary.volSeqNo.littleEndian);
			printf("	volSpaceSize: %u\n",desc->data.primary.volSpaceSize.littleEndian);
			printf("	created: ");
			iso_dbg_printVolDate(&desc->data.primary.created);
			printf("\n");
			printf("	modified: ");
			iso_dbg_printVolDate(&desc->data.primary.modified);
			printf("\n");
			printf("	expires: ");
			iso_dbg_printVolDate(&desc->data.primary.expires);
			printf("\n");
			printf("	effective: ");
			iso_dbg_printVolDate(&desc->data.primary.effective);
			printf("\n");
			break;
		case ISO_VOL_TYPE_PARTITION:
			printf("	type: partition\n");
			break;
		case ISO_VOL_TYPE_SUPPLEMENTARY:
			printf("	type: supplementary\n");
			break;
		case ISO_VOL_TYPE_TERMINATOR:
			printf("	type: terminator\n");
			break;
		default:
			printf("	type: *UNKNOWN*\n");
			break;
	}
	printf("\n");
}

void iso_dbg_printVolDate(sISOVolDate *date) {
	printf("%.4s-%.2s-%.2s %.2s:%.2s:%.2s.%.2s @%d",
			date->year,date->month,date->day,date->hour,date->minute,date->second,
			date->second100ths,date->offset);
}

#endif
