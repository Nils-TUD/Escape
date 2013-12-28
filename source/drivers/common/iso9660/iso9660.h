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

#pragma once

#include <esc/common.h>
#include <esc/fsinterface.h>
#include <fs/mount.h>
#include <fs/blockcache.h>
#include <fs/threadpool.h>

#define ATAPI_SECTOR_SIZE			2048
#define ISO_BLK_SIZE(iso)			((iso)->primary.data.primary.logBlkSize.littleEndian)
#define ISO_BLKS_TO_SECS(iso,blks)	((blks) * ISO_BLK_SIZE((iso)) / ATAPI_SECTOR_SIZE)

#define ISO_BCACHE_SIZE				1024
#define ISO_DIRE_CACHE_SIZE			128

#define ISO_VOL_DESC_START			0x10
#define ISO_ROOT_DIR_ID(iso)		ISO_BLK_SIZE(iso)

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

/* 32-bit integer that is stored in little- and bigendian */
typedef struct {
	uint32_t littleEndian;
	uint32_t bigEndian;
} uISOInt32;

/* 16-bit integer that is stored in little- and bigendian */
typedef struct {
	uint16_t littleEndian;
	uint16_t bigEndian;
} uISOInt16;

/* dates how they are stored in directory-entires */
typedef struct {
	uint8_t year;				/* Number of years since 1900. */
	uint8_t month;				/* Month of the year from 1 to 12. */
	uint8_t day;					/* Day of the month from 1 to 31. */
	uint8_t hour;				/* Hour of the day from 0 to 23. */
	uint8_t minute;				/* Minute of the hour from 0 to 59. */
	uint8_t second;				/* Second of the minute from 0 to 59. */
	uint8_t offset;				/* Offset from GMT in 15 minute intervals from -48 (West) to +52 (East). */
} A_PACKED sISODirDate;

/* dates how they are stored in the volume-descriptor */
typedef struct {
	char year[4];			/* Year from 1 to 9999. */
	char month[2];			/* Month from 1 to 12. */
	char day[2];			/* Day from 1 to 31. */
	char hour[2];			/* Hour from 0 to 23. */
	char minute[2];			/* Minute from 0 to 59. */
	char second[2];			/* Second from 0 to 59. */
	char second100ths[2];	/* Hundredths of a second from 0 to 99. */
	uint8_t offset;				/* Offset from GMT in 15 minute intervals from -48 (West) to +52 (East) */
} A_PACKED sISOVolDate;

/* a directory-entry */
typedef struct {
	/* Length of Directory Record. */
	uint8_t length;
	/* Extended Attribute Record length. */
	uint8_t extAttrLen;
	/* Location of extent (LBA) in both-endian format. */
	uISOInt32 extentLoc;
	/* Data length (size of extent) in both-endian format. */
	uISOInt32 extentSize;
	/* Recording date and time (see format below). */
	sISODirDate created;
	/* File flags (see ISO_FILEFL_*). */
	uint8_t flags;
	/*  File unit size for files recorded in interleaved mode, zero otherwise. */
	uint8_t unitSize;
	/* Interleave gap size for files recorded in interleaved mode, zero otherwise. */
	uint8_t gapSize;
	/* Volume sequence number - the volume that this extent is recorded on. */
	uISOInt16 volSeqNo;
	/* Length of file identifier (file name). This terminates with a ';' character followed by
	 * the file ID number in ASCII coded decimal ('1'). */
	uint8_t nameLen;
	/* is padded with a byte so that a directory entry will always start on an even byte number. */
	char name[];
} A_PACKED sISODirEntry;

/* an entry in the path-table */
typedef struct {
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
} A_PACKED sISOPathTblEntry;

typedef struct {
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
			uISOInt32 volSpaceSize;
			/* unused */
			uint64_t : 64;
			uint64_t : 64;
			uint64_t : 64;
			uint64_t : 64;
			/* The size of the set in this logical volume (number of disks). */
			uISOInt16 volSetSize;
			/* The number of this disk in the Volume Set. */
			uISOInt16 volSeqNo;
			/* The size in bytes of a logical block in both-endian format. */
			uISOInt16 logBlkSize;
			/* The size in bytes of the path table */
			uISOInt32 pathTableSize;
			/* LBA location of the (optional) path table for little-endian */
			uint32_t lPathTblLoc;
			uint32_t lOptPathTblLoc;
			/* LBA location of the (optional) path table for big-endian */
			uint32_t mPathTblLoc;
			uint32_t mOptPathTblLoc;
			sISODirEntry rootDir;
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
			sISOVolDate created;
			sISOVolDate modified;
			/* After this date and time, the volume should be considered obsolete. If unspecified,
			 * then the information is never considered obsolete. */
			sISOVolDate expires;
			/* Date and time from which the volume should be used. If unspecified, the volume may
			 * be used immediately. */
			sISOVolDate effective;
			/* An 8 bit number specifying the directory records and path table version (always 0x01). */
			uint8_t fileStructureVersion;
		} A_PACKED primary;
	} data;
} A_PACKED sISOVolDesc;

/* entries in the directory-entry-cache */
typedef struct {
	inode_t id;
	sISODirEntry entry;
} sISOCDirEntry;

/* the iso9660-handle (all information we need when working with it) */
typedef struct {
	/* the file-descs for the device (one for each thread and one for the initial) */
	int drvFds[REQ_THREAD_COUNT + 1];
	sISOVolDesc primary;
	size_t direcNextFree;
	sISOCDirEntry direcache[ISO_DIRE_CACHE_SIZE];
	sBlockCache blockCache;
} sISO9660;

/**
 * Inits the ISO9660-filesystem
 *
 * @param device the device-name (cdrom = find the device yourself)
 * @param usedDev will be set to the used device
 * @param error will be set to the occurred error, if NULL is returned
 * @return the handle
 */
void *iso_init(const char *device,char **usedDev,int *errcode);

/**
 * Deinits the ISO9660-filesystem
 *
 * @param h the handle
 */
void iso_deinit(void *h);

/**
 * Builds an instance of the filesystem and returns it
 * @return the instance or NULL if failed
 */
sFileSystem *iso_getFS(void);

/**
 * Mount-entry for resPath()
 *
 * @param h the ext2-handle
 * @param u the user
 * @param path the path
 * @param flags the flags
 * @param dev should be set to the device-number
 * @param resLastMnt whether mount-points should be resolved if the path is finished
 * @return the inode-number on success
 */
inode_t iso_resPath(void *h,sFSUser *u,const char *path,uint flags,dev_t *dev,bool resLastMnt);

/**
 * Mount-entry for open()
 *
 * @param h the iso9660-handle
 * @param u the user
 * @param ino the inode to open
 * @param flags the open-flags
 * @return the inode on success or < 0
 */
inode_t iso_open(void *h,sFSUser *u,inode_t ino,uint flags);

/**
 * Mount-entry for close()
 *
 * @param h the iso9660-handle
 * @param ino the inode to close
 */
void iso_close(void *h,inode_t ino);

/**
 * Mount-entry for stat()
 *
 * @param h the iso9660-handle
 * @param ino the inode to open
 * @param info the buffer where to write the file-info
 * @return 0 on success
 */
int iso_stat(void *h,inode_t ino,sFileInfo *info);

/**
 * Mount-entry for read()
 *
 * @param h the iso9660-handle
 * @param inodeNo the inode
 * @param buffer the buffer to read from
 * @param offset the offset to read from
 * @param count the number of bytes to read
 * @return number of read bytes on success
 */
ssize_t iso_read(void *h,inode_t inodeNo,void *buffer,off_t offset,size_t count);

/**
 * Builds a timestamp from the given dir-date
 *
 * @param h the iso9660-handle
 * @param ddate the dir-date
 * @return the timestamp
 */
time_t iso_dirDate2Timestamp(sISO9660 *h,const sISODirDate *ddate);

/**
 * Prints statistics and information to the given file
 *
 * @param f the file
 * @param h the handle
 */
void iso_print(FILE *f,void *h);

#if DEBUGGING

/**
 * Prints the path-table
 */
void iso_dbg_printPathTbl(sISO9660 *h);
/**
 * Prints an directory-tree
 */
void iso_dbg_printTree(sISO9660 *h,size_t extLoc,size_t extSize,size_t layer);
/**
 * Prints the given volume descriptor
 */
void iso_dbg_printVolDesc(sISOVolDesc *desc);
/**
 * Prints the given date
 */
void iso_dbg_printVolDate(sISOVolDate *date);

#endif
