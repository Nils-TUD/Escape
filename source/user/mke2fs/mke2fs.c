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

#include <esc/common.h>
#include <esc/cmdargs.h>
#include <esc/endian.h>
#include <esc/time.h>
#include <fs/ext2/ext2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static struct Ext2SuperBlock sb;
static uint32_t blockSize = 1024;
static uint32_t blockCount = 0;
static uint32_t inodeCount = 0;
static uint32_t blockGroups = 0;
static struct Ext2BlockGrp *bgs = NULL;
static const char *volumeLabel = "";
static int fd = -1;
static const char *disk = NULL;
static size_t disksize = 0;

static void log(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	vprintf(fmt,ap);
	fflush(stdout);
	va_end(ap);
}

static void writeToBlock(uint32_t block,const void *data,size_t len,size_t off) {
	if(seek(fd,block * blockSize + off,SEEK_SET) < 0)
		error("Unable to seek to %u",block * blockSize);
	if(write(fd,data,len) != (ssize_t)len)
		error("Unable to write %zu bytes to %u",len,block * blockSize);
}

static uint32_t firstBlockOf(size_t bg) {
	uint32_t iperGroup = le32tocpu(sb.inodesPerGroup);
	uint32_t inoBlocks = (iperGroup * EXT2_REV0_INODE_SIZE + blockSize - 1) / blockSize;
	return le32tocpu(bgs[bg].inodeTable) + inoBlocks;
}

static size_t getDirESize(size_t namelen) {
	size_t tlen = namelen + sizeof(struct Ext2DirEntry);
	if((tlen % EXT2_DIRENTRY_PAD) != 0)
		tlen += EXT2_DIRENTRY_PAD - (tlen % EXT2_DIRENTRY_PAD);
	return tlen;
}

static void initSuperblock(void) {
	if(blockSize == 1024)
		sb.logBlockSize = 0;
	else if(blockSize == 2048)
		sb.logBlockSize = cputole32(1);
	else if(blockSize == 4096)
		sb.logBlockSize = cputole32(2);
	else
		error("Invalid block-size. Valid is 1024, 2048 and 4096.");
	/* fragments = blocks */
	sb.logFragSize = sb.logBlockSize;
	if(disksize % blockSize != 0)
		error("Size of '%s' (%zu) is not a multiple of the block-size (%u)",disk,disksize,blockSize);

	log("Using a block-size of %u\n",blockSize);

	blockCount = disksize / blockSize;
	/* better leave some space unused than having one smaller blockgroup */
	if(blockGroups == 0) {
		blockGroups = blockCount / 8192;
		if(blockGroups == 0)
			blockGroups = 1;
		if(blockGroups * sizeof(struct Ext2BlockGrp) > blockSize)
			blockGroups = blockSize / sizeof(struct Ext2BlockGrp);
	}
	else if(blockGroups * sizeof(struct Ext2BlockGrp) > blockSize)
		error("Too many block-groups. Max. is %u",blockSize / sizeof(struct Ext2BlockGrp));
	blockCount = (blockCount / blockGroups) * blockGroups;

	/* use 5% of the blocks for inodes, by default */
	if(inodeCount == 0)
		inodeCount = ((blockCount / 20) * blockSize) / EXT2_REV0_INODE_SIZE;
	/* ensure that we don't have more in total than the sum of all inodes in block-groups */
	inodeCount = (inodeCount / blockGroups) * blockGroups;

	log("Having %u block-groups, %u blocks, %u inodes in total\n",blockGroups,blockCount,inodeCount);

	sb.blockCount = cputole32(blockCount);
	sb.inodeCount = cputole32(inodeCount);
	sb.suResBlockCount = 0;
	sb.freeBlockCount = 0;
	sb.freeInodeCount = cputole32(inodeCount - (EXT2_REV0_FIRST_INO - 1));
	sb.firstDataBlock = cputole32(blockSize == 1024 ? 1 : 0);
	sb.blocksPerGroup = cputole32(blockCount / blockGroups);
	sb.inodesPerGroup = cputole32(inodeCount / blockGroups);
	sb.fragsPerGroup = sb.blocksPerGroup;
	sb.lastWritetime = cputole32(time(NULL));
	sb.magic = cputole16(EXT2_SUPER_MAGIC);
	sb.state = cputole16(EXT2_VALID_FS);
	sb.minorRevLevel = 0;
	sb.maxMountCount = cputole16(-1);
	sb.lastCheck = cputole32(time(NULL));
	sb.revLevel = cputole32(EXT2_GOOD_OLD_REV);
	sb.firstInode = cputole32(EXT2_REV0_FIRST_INO);
	sb.inodeSize = cputole16(EXT2_REV0_INODE_SIZE);
	for(size_t i = 0; i < ARRAY_SIZE(sb.volumeUid); ++i)
		sb.volumeUid[i] = rand() % 0xFF;
	strnzcpy(sb.volumeName,volumeLabel,sizeof(sb.volumeName));
	sb.lastMountPath[0] = '\0';
}

static void writeBGDescs(void) {
	uint32_t bperGroup = le32tocpu(sb.blocksPerGroup);
	uint32_t iperGroup = le32tocpu(sb.inodesPerGroup);
	uint32_t bbmBlocks = ((bperGroup / 8) + blockSize - 1) / blockSize;
	uint32_t ibmBlocks = ((iperGroup / 8) + blockSize - 1) / blockSize;
	uint32_t inoBlocks = (iperGroup * EXT2_REV0_INODE_SIZE + blockSize - 1) / blockSize;
	uint32_t bno = le32tocpu(sb.firstDataBlock) + 2;
	bgs = calloc(sizeof(struct Ext2BlockGrp),blockGroups);
	if(!bgs)
		error("Unable to allocate memory for block group descriptors");
	for(uint32_t i = 0; i < blockGroups; ++i) {
		uint32_t freeBlocks = bperGroup - (2 + bbmBlocks + ibmBlocks + inoBlocks);
		/* last block is not usable */
		if(i == blockGroups - 1 && blockSize == 1024)
			freeBlocks--;
		bgs[i].freeBlockCount = cputole16(i == 0 ? freeBlocks - 1 : freeBlocks);
		bgs[i].freeInodeCount = cputole16(i == 0 ? iperGroup - (EXT2_REV0_FIRST_INO - 1) : iperGroup);
		bgs[i].blockBitmap = cputole32(bno);
		bgs[i].inodeBitmap = cputole32(bno + bbmBlocks);
		bgs[i].inodeTable = cputole32(bno + bbmBlocks + ibmBlocks);
		bgs[i].usedDirCount = i == 0 ? 1 : 0;

		bno += bperGroup;
	}

	log("Writing block-group descriptors to block-group 0...\n");
	writeToBlock(le32tocpu(sb.firstDataBlock) + 1,bgs,sizeof(*bgs) * blockGroups,0);

	bno = bperGroup + EXT2_BLOGRPTBL_BNO;
	for(size_t bg = 1; bg < blockGroups; ++bg) {
		log("Writing block-group descriptors to block-group %zu...\n",bg);
		writeToBlock(bno,bgs,sizeof(*bgs) * blockGroups,0);
		bno += bperGroup;
	}
}

static void writeINodeBitmap(size_t bg) {
	uint32_t iperGroup = le32tocpu(sb.inodesPerGroup);
	uint8_t *bitmap = calloc(iperGroup / 8,1);
	if(bg == 0) {
		for(uint32_t ino = 0; ino < EXT2_REV0_FIRST_INO - 1; ++ino)
			bitmap[ino / 8] |= 1 << (ino % 8);
	}
	log("Writing inode-bitmap of block-group %zu...\n",bg);
	writeToBlock(le32tocpu(bgs[bg].inodeBitmap),bitmap,iperGroup / 8,0);
	free(bitmap);
}

static void writeBlockBitmap(size_t bg) {
	uint32_t bperGroup = le32tocpu(sb.blocksPerGroup);
	uint32_t firstUnused = firstBlockOf(bg) - bg * bperGroup + (1 - le32tocpu(sb.firstDataBlock));
	if(bg == 0)
		firstUnused++;
	sb.freeBlockCount = cputole32(le32tocpu(sb.freeBlockCount) + le16tocpu(bgs[bg].freeBlockCount));

	uint32_t blocks = (bperGroup / 8 + blockSize - 1) / blockSize;
	uint8_t *bitmap = calloc(blocks * blockSize,1);
	for(uint32_t bno = 0; bno < firstUnused - 1; ++bno)
		bitmap[bno / 8] |= 1 << (bno % 8);

	/* last block is not usable */
	if(blockSize == 1024 && bg == blockGroups - 1)
		bitmap[(bperGroup - 1) / 8] |= 1 << ((bperGroup - 1) % 8);

	/* add padding */
	memset(bitmap + bperGroup / 8,0xFF,blocks * blockSize - (bperGroup / 8));

	log("Writing block-bitmap of block-group %zu...\n",bg);
	writeToBlock(le32tocpu(bgs[bg].blockBitmap),bitmap,blocks * blockSize,0);
	free(bitmap);
}

static void writeINodeTable(size_t bg) {
	uint32_t iperGroup = le32tocpu(sb.inodesPerGroup);
	char *inodes = calloc(iperGroup,EXT2_REV0_INODE_SIZE);
	if(bg == 0) {
		struct Ext2Inode *root = (struct Ext2Inode*)(
			inodes + EXT2_REV0_INODE_SIZE * (EXT2_ROOT_INO - 1));
		root->mode = cputole16(EXT2_S_IFDIR | 0755);
		root->uid = 0;
		root->gid = 0;
		root->size = blockSize;
		root->accesstime = 0;
		root->createtime = cputole32(time(NULL));
		root->modifytime = cputole32(time(NULL));
		root->deletetime = 0;
		root->linkCount = cputole16(2);
		/* blocks are counted in 512 byte sectors */
		root->blocks = cputole32(blockSize / 512);
		root->dBlocks[0] = cputole32(firstBlockOf(0));
	}
	log("Writing inode-table of block-group %zu...\n",bg);
	writeToBlock(le32tocpu(bgs[bg].inodeTable),inodes,iperGroup * EXT2_REV0_INODE_SIZE,0);
	free(inodes);
}

static void createRootDir(uint32_t bno) {
	char *entries = calloc(blockSize,1);

	struct Ext2DirEntry *e = (struct Ext2DirEntry*)entries;
	e->inode = cputole32(EXT2_ROOT_INO);
	e->nameLen = cputole16(1);
	memcpy(e->name,".",1);
	e->recLen = cputole16(getDirESize(1));

	e = (struct Ext2DirEntry*)(entries + le16tocpu(e->recLen));
	e->inode = cputole32(EXT2_ROOT_INO);
	e->nameLen = cputole16(2);
	memcpy(e->name,"..",2);
	e->recLen = cputole16(blockSize - getDirESize(1));

	log("Writing root-directory to block %u...\n",bno);
	writeToBlock(bno,entries,blockSize,0);
	free(entries);
}

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-b <blockSize>] [-N <inodes>] [-L <label>] [-G <blockGroups>] <disk>\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char **argv) {
	int res = ca_parse(argc,argv,CA_MAX1_FREE,"b=k N=k L=s G=k",
		&blockSize,&inodeCount,&volumeLabel,&blockGroups);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp() || ca_getFreeCount() == 0)
		usage(argv[0]);

	disk = ca_getFree()[0];
	if(!isblock(disk))
		error("'%s' is neither a block-device nor a regular file",disk);

	fd = open(disk,O_WRONLY);
	if(fd < 0)
		error("Unable to open '%s' for writing",disk);
	disksize = filesize(fd);
	srand(rdtsc());

	log("Writing ext2 filesystem to disk %s (%zu bytes)\n",disk,disksize);

	initSuperblock();
	writeBGDescs();
	createRootDir(firstBlockOf(0));
	for(size_t i = 0; i < blockGroups; ++i) {
		writeINodeBitmap(i);
		writeBlockBitmap(i);
		writeINodeTable(i);
	}

	log("Writing superblock to block-group 0...\n");
	writeToBlock(0,&sb,sizeof(sb),1024);

	uint32_t bno = le32tocpu(sb.blocksPerGroup);
	for(size_t bg = 1; bg < blockGroups; ++bg) {
		log("Writing superblock to block-group %zu...\n",bg);
		writeToBlock(bno,&sb,sizeof(sb),0);
		bno += le32tocpu(sb.blocksPerGroup);
	}

	close(fd);
	return 0;
}
