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

#include <esc/common.h>
#include <stdio.h>

#include "common.h"

class ISO9660FileSystem;

/* a directory-entry */
struct ISODirEntry {
	/* Length of Directory Record. */
	uint8_t length;
	/* Extended Attribute Record length. */
	uint8_t extAttrLen;
	/* Location of extent (LBA) in both-endian format. */
	ISOInt32 extentLoc;
	/* Data length (size of extent) in both-endian format. */
	ISOInt32 extentSize;
	/* Recording date and time (see format below). */
	ISODirDate created;
	/* File flags (see ISO_FILEFL_*). */
	uint8_t flags;
	/*  File unit size for files recorded in interleaved mode, zero otherwise. */
	uint8_t unitSize;
	/* Interleave gap size for files recorded in interleaved mode, zero otherwise. */
	uint8_t gapSize;
	/* Volume sequence number - the volume that this extent is recorded on. */
	ISOInt16 volSeqNo;
	/* Length of file identifier (file name). This terminates with a ';' character followed by
	 * the file ID number in ASCII coded decimal ('1'). */
	uint8_t nameLen;
	/* is padded with a byte so that a directory entry will always start on an even byte number. */
	char name[];
} A_PACKED;

/* entries in the directory-entry-cache */
struct ISOCDirEntry {
	ino_t id;
	ISODirEntry entry;
};

class ISO9660DirCache {
public:
	/**
	 * Inits the dir-entry-cache
	 *
	 * @param h the iso9660 handle
	 */
	explicit ISO9660DirCache(ISO9660FileSystem *h);
	~ISO9660DirCache() {
		delete[] _cache;
	}

	/**
	 * Retrieves the directory-entry with given id (LBA * blockSize + offset in directory)
	 * Note that the entries will NOT contain the name!
	 *
	 * @param id the id
	 * @return the cached directory-entry or NULL if failed
	 */
	const ISOCDirEntry *get(ino_t id);

	/**
	 * Prints information and statistics of the directory-entry-cache to the given file
	 *
	 * @param f the file
	 */
	void print(FILE *f);

private:
	size_t _nextFree;
	ISOCDirEntry *_cache;
	ISO9660FileSystem *_fs;
};
