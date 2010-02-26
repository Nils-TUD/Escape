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

#include <esc/common.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/heap.h>
#include <esc/proc.h>
#include <esc/fileio.h>
#include <esc/date.h>
#include <errors.h>
#include <string.h>
#include "iso9660.h"
#include "rw.h"
#include "dir.h"
#include "file.h"
#include "direcache.h"
#include "../mount.h"
#include "../blockcache.h"

#define MAX_DRIVER_OPEN_RETRIES		1000

static bool iso_setup(const char *driver,sISO9660 *iso);

void *iso_init(const char *driver) {
	s32 res;
	sFileInfo info;
	sISO9660 *iso = (sISO9660*)malloc(sizeof(sISO9660));
	if(iso == NULL)
		return NULL;

	iso->blockCache.handle = iso;
	iso->blockCache.blockCache = NULL;
	iso->blockCache.blockCacheSize = ISO_BCACHE_SIZE;
	iso->blockCache.freeBlocks = NULL;
	iso->blockCache.usedBlocks = NULL;
	iso->blockCache.oldestBlock = NULL;
	/* cast is ok, because the only difference is that iso_rw_* receive a sISO9660* as first argument
	 * and read/write expect void* */
	iso->blockCache.read = (fReadBlocks)iso_rw_readBlocks;
	/* no writing here ;) */
	iso->blockCache.write = NULL;

	/* wait until ata is ready */
	/* we have to try it multiple times in this case since the kernel loads ata and fs
	 * directly after another and we don't know who's ready first */
	do {
		res = stat("/system/devices/ata",&info);
		if(res < 0)
			yield();
	}
	while(res < 0);

	/* if the driver is provided, simply use it */
	if(strcmp(driver,"cdrom") != 0) {
		if(!iso_setup(driver,iso)) {
			printe("Unable to find driver '%s'",driver);
			free(iso);
			return NULL;
		}
	}
	/* otherwise try all possible ATAPI-drives */
	else {
		u32 i;
		/* just needed if we would have a mount-point on the cd. this can't happen at this point */
		tDevNo dev = 0x1234;
		tInodeNo ino;
		char path[SSTRLEN("/drivers/cda1") + 1];
		for(i = 0; i < 4; i++) {
			snprintf(path,sizeof(path),"/drivers/cd%c1",'a' + i);
			if(!iso_setup(path,iso))
				continue;

			/* try to find our kernel. if we've found it, it's likely that the user wants to
			 * boot from this device. unfortunatly there doesn't seem to be an easy way
			 * to find out the real boot-device from GRUB */
			ino = iso_dir_resolve(iso,"/boot/escape.bin",IO_READ,&dev,false);
			if(ino >= 0)
				break;

			/* destroy the block-cache; we'll create a new one with iso_setup() in the next loop */
			bcache_destroy(&iso->blockCache);
		}

		/* not found? */
		if(i >= 4) {
			free(iso);
			return NULL;
		}
	}
	return iso;
}

static bool iso_setup(const char *driver,sISO9660 *iso) {
	u32 i;
	/* now open the driver */
	tFD fd = open(driver,IO_WRITE | IO_READ);
	if(fd < 0)
		return false;

	iso->driverFd = fd;
	/* read volume descriptors */
	for(i = 0; ; i++) {
		if(!iso_rw_readSectors(iso,&iso->primary,ISO_VOL_DESC_START + i,1))
			error("Unable to read volume descriptor from driver");
		/* we need just the primary one */
		if(iso->primary.type == ISO_VOL_TYPE_PRIMARY)
			break;
	}

	iso->blockCache.blockSize = ISO_BLK_SIZE(iso);

	iso_direc_init(iso);
	bcache_init(&iso->blockCache);
	return true;
}

void iso_deinit(void *h) {
	UNUSED(h);
}

sFileSystem *iso_getFS(void) {
	sFileSystem *fs = malloc(sizeof(sFileSystem));
	if(!fs)
		return NULL;
	fs->type = FS_TYPE_ISO9660;
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
	fs->sync = NULL;
	return fs;
}

tInodeNo iso_resPath(void *h,const char *path,u8 flags,tDevNo *dev,bool resLastMnt) {
	return iso_dir_resolve((sISO9660*)h,path,flags,dev,resLastMnt);
}

s32 iso_open(void *h,tInodeNo ino,u8 flags) {
	UNUSED(h);
	UNUSED(flags);
	return ino;
}

void iso_close(void *h,tInodeNo ino) {
	UNUSED(h);
	UNUSED(ino);
}

s32 iso_stat(void *h,tInodeNo ino,sFileInfo *info) {
	u32 ts;
	sISO9660 *i = (sISO9660*)h;
	sISOCDirEntry *e = iso_direc_get(i,ino);
	if(e == NULL)
		return ERR_INO_REQ_FAILED;

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
	info->mode = 0777;
	if(e->entry.flags & ISO_FILEFL_DIR)
		info->mode |= MODE_TYPE_DIR;
	else
		info->mode |= MODE_TYPE_FILE;
	info->size = e->entry.extentSize.littleEndian;
	return 0;
}

s32 iso_read(void *h,tInodeNo inodeNo,void *buffer,u32 offset,u32 count) {
	return iso_file_read((sISO9660*)h,inodeNo,buffer,offset,count);
}

u32 iso_dirDate2Timestamp(sISO9660 *h,sISODirDate *ddate) {
	sDate date;
	UNUSED(h);
	date.year = ddate->year + 1900;
	date.month = ddate->month;
	date.monthDay = ddate->day;
	date.hour = ddate->hour;
	date.min = ddate->minute;
	date.sec = ddate->second;
	/* note that we assume here that getTimeOf() doesn't need the other fields
	 * (weekDay, yearDay and isDst) */
	return getTimeOf(&date);
}

#if DEBUGGING

void iso_dbg_printPathTbl(sISO9660 *h) {
	u32 tblSize = h->primary.data.primary.pathTableSize.littleEndian;
	u32 secCount = (tblSize + ATAPI_SECTOR_SIZE - 1) / ATAPI_SECTOR_SIZE;
	sISOPathTblEntry *pe = malloc((tblSize + ATAPI_SECTOR_SIZE - 1) & ~(ATAPI_SECTOR_SIZE - 1));
	sISOPathTblEntry *start = pe;
	if(!iso_rw_readSectors(h,pe,h->primary.data.primary.lPathTblLoc,secCount)) {
		free(start);
		return;
	}
	printf("Path-Table:\n");
	while((u8*)pe < ((u8*)start + tblSize)) {
		printf("	length: %u\n",pe->length);
		printf("	extentLoc: %u\n",pe->extentLoc);
		printf("	parentTblIndx: %u\n",pe->parentTblIndx);
		printf("	name: %s\n",pe->name);
		printf("---\n");
		if(pe->length % 2 == 0)
			pe = (sISOPathTblEntry*)((u8*)pe + sizeof(sISOPathTblEntry) + pe->length);
		else
			pe = (sISOPathTblEntry*)((u8*)pe + sizeof(sISOPathTblEntry) + pe->length + 1);
	}
	free(start);
}

void iso_dbg_printTree(sISO9660 *h,u32 extLoc,u32 extSize,u32 layer) {
	u32 i;
	sISODirEntry *e;
	sISODirEntry *content = (sISODirEntry*)malloc(extSize);
	if(content == NULL)
		return;
	if(!iso_rw_readSectors(h,content,extLoc,extSize / ATAPI_SECTOR_SIZE))
		return;

	e = content;
	while((u8*)e < (u8*)content + extSize) {
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
		e = (sISODirEntry*)((u8*)e + e->length);
	}
	free(content);
}

void iso_dbg_printVolDesc(sISOVolDesc *desc) {
	printf("VolumeDescriptor @ %x\n",desc);
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
