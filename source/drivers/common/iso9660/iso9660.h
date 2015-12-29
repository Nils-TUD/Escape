/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#pragma once

#include <fs/blockcache.h>
#include <fs/filesystem.h>
#include <fs/fsdev.h>
#include <sys/common.h>
#include <sys/stat.h>

#include "common.h"
#include "direcache.h"

#define ATAPI_SECTOR_SIZE			2048

#define ISO_BCACHE_SIZE				1024
#define ISO_DIRE_CACHE_SIZE			128

#define ISO_VOL_DESC_START			0x10

#define ISO_VOL_TYPE_BOOTRECORD		0
#define ISO_VOL_TYPE_PRIMARY		1
#define ISO_VOL_TYPE_SUPPLEMENTARY	2
#define ISO_VOL_TYPE_PARTITION		3
#define ISO_VOL_TYPE_TERMINATOR		255

/* special filenames */
#define ISO_FILENAME_THIS			0x00
#define ISO_FILENAME_PARENT			0x01

/* If set, the existence of this file need not be made known to the user (basically a 'hidden' flag. */
#define ISO_FILEFL_HIDDEN			(1 << 0)
/* If set, this record describes a directory (in other words, it is a subdirectory extent). */
#define ISO_FILEFL_DIR				(1 << 1)
/* If set, this file is an "Associated File". */
#define ISO_FILEFL_ASSOC_FILE		(1 << 2)
/* If set, the extended attribute record contains information about the format of this file. */
#define ISO_FILEFL_EXTATTR_FORMAT	(1 << 3)
/* If set, owner and group permissions are set in the extended attribute record. */
#define ISO_FILEFL_EXTATTR_PERMS	(1 << 4)
/* If set, this is not the final directory record for this file (for files spanning several
 * extents, for example files over 4GiB long. */
#define ISO_FILEFL_NOT_FINAL		(1 << 7)

/* an entry in the path-table */
struct ISOPathTblEntry {
	/* Length of Directory Identifier */
	uint8_t length;
	/* Extended Attribute Record Length */
	uint8_t extAttrLen;
	/* Location of Extent (LBA). This is in a different format depending on whether this is the
	 * L-Table or M-Table. */
	uint32_t extentLoc;
	/* Directory number of parent directory (an index in to the path table). */
	uint16_t parentTblIndx;
	/* Directory Identifier (name) in d-characters. padded with a zero, if necessary, so
	 * that each table-entry starts on a even byte number. */
	char name[];
} A_PACKED;

struct ISOVolDesc {
	/* see ISO_VOL_TYPE_* */
	uint8_t type;
	/* always 'CD001' */
	char identifier[5];
	/* always 0x01 */
	uint8_t version;
	union {
		/* just to make the union 2041 bytes big.. */
		uint8_t raw[2041];
		struct {
			/* ID of the system which can act on and boot the system from the boot record in
			 * a-characters */
			char bootSystemIdent[32];
			/* Identification of the boot system defined in the rest of this descriptor in
			 * a-characters */
			char bootIdent[32];
		} A_PACKED bootrecord;
		struct {
			/* unused; always 0x00 */
			uint8_t : 8;
			/* The name of the system that can act upon sectors 0x00-0x0F for the volume in a-characters */
			char systemIdent[32];
			/* Identification of this volume in d-characters */
			char volumeIdent[32];
			/* unused */
			uint64_t : 64;
			/* Number of Logical Blocks in which the volume is recorded */
			ISOInt32 volSpaceSize;
			/* unused */
			uint64_t : 64;
			uint64_t : 64;
			uint64_t : 64;
			uint64_t : 64;
			/* The size of the set in this logical volume (number of disks). */
			ISOInt16 volSetSize;
			/* The number of this disk in the Volume Set. */
			ISOInt16 volSeqNo;
			/* The size in bytes of a logical block in both-endian format. */
			ISOInt16 logBlkSize;
			/* The size in bytes of the path table */
			ISOInt32 pathTableSize;
			/* LBA location of the (optional) path table for little-endian */
			uint32_t lPathTblLoc;
			uint32_t lOptPathTblLoc;
			/* LBA location of the (optional) path table for big-endian */
			uint32_t mPathTblLoc;
			uint32_t mOptPathTblLoc;
			ISODirEntry rootDir;
			/* root-entry is padded with one byte */
			uint8_t : 8;
			/* Identifier of the volume set of which this volume is a member in d-characters. */
			char volumeSetIdent[128];
			/* The volume publisher in a-characters. If unspecified, all bytes should be 0x20.
			 * For extended publisher information, the first byte should be 0x5F, followed by an
			 * 8.3 format file name. This file must be in the root directory and the filename is
			 * made from d-characters. */
			char publisherIdent[128];
			/* The identifier of the person(s) who prepared the data for this volume. Format as per
			 * Publisher Identifier. */
			char dataPreparerIdent[128];
			/* Identifies how the data are recorded on this volume. Format as per Publisher Identifier. */
			char applicationIdent[128];
			char copyrightFile[38];
			char abstractFile[36];
			char bibliographicFile[37];
			/* dates */
			ISOVolDate created;
			ISOVolDate modified;
			/* After this date and time, the volume should be considered obsolete. If unspecified,
			 * then the information is never considered obsolete. */
			ISOVolDate expires;
			/* Date and time from which the volume should be used. If unspecified, the volume may
			 * be used immediately. */
			ISOVolDate effective;
			/* An 8 bit number specifying the directory records and path table version (always 0x01). */
			uint8_t fileStructureVersion;
		} A_PACKED primary;
	} data;
} A_PACKED;

class ISO9660FileSystem : public fs::FileSystem<fs::OpenFile> {
public:
	class ISO9660BlockCache : public fs::BlockCache {
	public:
		explicit ISO9660BlockCache(ISO9660FileSystem *fs)
			: BlockCache(fs->fd,ISO_BCACHE_SIZE,fs->blockSize()), _fs(fs) {
		}

		virtual bool readBlocks(void *buffer,block_t start,size_t blockCount);
		virtual bool writeBlocks(const void *buffer,size_t start,size_t blockCount);

	private:
		ISO9660FileSystem *_fs;
	};

	explicit ISO9660FileSystem(const char *device);
	virtual ~ISO9660FileSystem() {
		::close(fd);
	}

	size_t blockSize() const {
		return primary.data.primary.logBlkSize.littleEndian;
	}
	size_t blocksToSecs(ulong blks) const {
		return (blks * blockSize()) / ATAPI_SECTOR_SIZE;
	}
	ino_t rootDirId() const {
		return blockSize();
	}

	ino_t open(fs::User *u,const char *path,uint flags,mode_t mode,int fd,fs::OpenFile **file) override;
	void close(fs::OpenFile *file) override;
	int stat(fs::OpenFile *file,struct stat *info) override;
	ssize_t read(fs::OpenFile *file,void *buffer,off_t offset,size_t size) override;
	ssize_t write(fs::OpenFile *file,const void *buffer,off_t offset,size_t size) override;
	int link(fs::User *u,fs::OpenFile *dst,fs::OpenFile *dir,const char *name) override;
	int unlink(fs::User *u,fs::OpenFile *dir,const char *name) override;
	int mkdir(fs::User *u,fs::OpenFile *dir,const char *name,mode_t mode) override;
	int rmdir(fs::User *u,fs::OpenFile *dir,const char *name) override;
	int chmod(fs::User *u,fs::OpenFile *file,mode_t mode) override;
	int chown(fs::User *u,fs::OpenFile *file,uid_t uid,gid_t gid) override;
	int utime(fs::User *u,fs::OpenFile *file,const struct utimbuf *utimes) override;
	void sync() override;
	void print(FILE *f) override;

private:
	static int initPrimaryVol(ISO9660FileSystem *fs,const char *device);
	time_t dirDate2Timestamp(const ISODirDate *ddate);

#if DEBUGGING

	/**
	 * Prints the path-table
	 */
	void printPathTbl();
	/**
	 * Prints an directory-tree
	 */
	void printTree(size_t extLoc,size_t extSize,size_t layer);
	/**
	 * Prints the given volume descriptor
	 */
	void printVolDesc(ISOVolDesc *desc);
	/**
	 * Prints the given date
	 */
	void printVolDate(ISOVolDate *date);

#endif

public:
	/* the fd for the device */
	int fd;

	ISOVolDesc primary;
	int dummy;
	ISO9660DirCache dirCache;
	ISO9660BlockCache blockCache;
};
