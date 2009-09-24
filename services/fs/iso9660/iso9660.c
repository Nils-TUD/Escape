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

#define MAX_DRIVER_OPEN_RETRIES		100000

void *iso_init(const char *driver) {
	u32 i,tries = 0;
	tFD fd;
	sISO9660 *iso = (sISO9660*)malloc(sizeof(sISO9660));
	if(iso == NULL)
		return NULL;

	/* we have to try it multiple times in this case since the kernel loads ata and fs
	 * directly after another and we don't know who's ready first */
	do {
		fd = open(driver,IO_WRITE | IO_READ);
		if(fd < 0)
			yield();
		tries++;
	}
	while(fd < 0/* && tries < MAX_DRIVER_OPEN_RETRIES*/);
	if(fd < 0)
		error("Unable to find driver '%s' after %d retries",driver,tries);

	iso->driverFd = fd;
	/* read volume descriptors */
	for(i = 0; ; i++) {
		if(!iso_rw_readSectors(iso,&iso->primary,ISO_VOL_DESC_START + i,1))
			error("Unable to read volume descriptor from driver");
		/* we need just the primary one */
		if(iso->primary.type == ISO_VOL_TYPE_PRIMARY)
			break;
	}

	iso_direc_init(iso);
	return iso;
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

tInodeNo iso_resPath(void *h,char *path,u8 flags,tDevNo *dev,bool resLastMnt) {
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
	/* TODO */
	info->device = 0;
	info->rdevice = 0;
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
	debugf("Path-Table:\n");
	while((u8*)pe < ((u8*)start + tblSize)) {
		debugf("	length: %u\n",pe->length);
		debugf("	extentLoc: %u\n",pe->extentLoc);
		debugf("	parentTblIndx: %u\n",pe->parentTblIndx);
		debugf("	name: %s\n",pe->name);
		debugf("---\n");
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
			debugf("  ");
		if(e->name[0] == ISO_FILENAME_THIS)
			debugf("name: '.'\n");
		else if(e->name[0] == ISO_FILENAME_PARENT)
			debugf("name: '..'\n");
		else {
			bak = e->name[e->nameLen];
			e->name[e->nameLen] = '\0';
			debugf("name: '%s'\n",e->name);
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
	debugf("VolumeDescriptor @ %x\n",desc);
	debugf("	version: 0x%02x\n",desc->version);
	debugf("	identifier: %.5s\n",desc->identifier);
	switch(desc->type) {
		case ISO_VOL_TYPE_BOOTRECORD:
			debugf("	type: bootrecord\n");
			debugf("	bootSystemIdent: %.32s\n",desc->data.bootrecord.bootSystemIdent);
			debugf("	bootIdent: %.32s\n",desc->data.bootrecord.bootIdent);
			break;
		case ISO_VOL_TYPE_PRIMARY:
			debugf("	type: primary\n");
			debugf("	systemIdent: %.32s\n",desc->data.primary.systemIdent);
			debugf("	volumeIdent: %.32s\n",desc->data.primary.volumeIdent);
			debugf("	volumeSetIdent: %.128s\n",desc->data.primary.volumeSetIdent);
			debugf("	copyrightFile: %.38s\n",desc->data.primary.copyrightFile);
			debugf("	abstractFile: %.36s\n",desc->data.primary.abstractFile);
			debugf("	bibliographicFile: %.37s\n",desc->data.primary.bibliographicFile);
			debugf("	applicationIdent: %.128s\n",desc->data.primary.applicationIdent);
			debugf("	dataPreparerIdent: %.128s\n",desc->data.primary.dataPreparerIdent);
			debugf("	publisherIdent: %.128s\n",desc->data.primary.publisherIdent);
			debugf("	fileStructureVersion: %u\n",desc->data.primary.fileStructureVersion);
			debugf("	lOptPathTblLoc: %u\n",desc->data.primary.lOptPathTblLoc);
			debugf("	lPathTblLoc: %u\n",desc->data.primary.lPathTblLoc);
			debugf("	mOptPathTblLoc: %u\n",desc->data.primary.mOptPathTblLoc);
			debugf("	mPathTblLoc: %u\n",desc->data.primary.mPathTblLoc);
			debugf("	logBlkSize: %u\n",desc->data.primary.logBlkSize.littleEndian);
			debugf("	pathTableSize: %u\n",desc->data.primary.pathTableSize.littleEndian);
			debugf("	volSeqNo: %u\n",desc->data.primary.volSeqNo.littleEndian);
			debugf("	volSpaceSize: %u\n",desc->data.primary.volSpaceSize.littleEndian);
			debugf("	created: ");
			iso_dbg_printVolDate(&desc->data.primary.created);
			debugf("\n");
			debugf("	modified: ");
			iso_dbg_printVolDate(&desc->data.primary.modified);
			debugf("\n");
			debugf("	expires: ");
			iso_dbg_printVolDate(&desc->data.primary.expires);
			debugf("\n");
			debugf("	effective: ");
			iso_dbg_printVolDate(&desc->data.primary.effective);
			debugf("\n");
			break;
		case ISO_VOL_TYPE_PARTITION:
			debugf("	type: partition\n");
			break;
		case ISO_VOL_TYPE_SUPPLEMENTARY:
			debugf("	type: supplementary\n");
			break;
		case ISO_VOL_TYPE_TERMINATOR:
			debugf("	type: terminator\n");
			break;
		default:
			debugf("	type: *UNKNOWN*\n");
			break;
	}
	debugf("\n");
}

void iso_dbg_printVolDate(sISOVolDate *date) {
	debugf("%.4s-%.2s-%.2s %.2s:%.2s:%.2s.%.2s @%d",
			date->year,date->month,date->day,date->hour,date->minute,date->second,
			date->second100ths,date->offset);
}

#endif
