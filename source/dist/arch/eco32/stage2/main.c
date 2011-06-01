/**
 * $Id: main.c 52 2009-08-13 12:39:24Z nasmussen $
 * Copyright (C) 2009 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/debug.h>
#include <esc/endian.h>
#include <sys/arch/eco32/boot.h>
#include <sys/task/elf.h>
#include "../../../../drivers/common/fs/ext2/ext2.h"
#include <stdarg.h>

#define PROG_COUNT			4
#define SEC_SIZE			512								/* disk sector size in bytes */
#define BLOCK_SIZE			((size_t)(1024 << le32tocpu(e.superBlock.logBlockSize)))
#define SPB					(BLOCK_SIZE / SEC_SIZE)			/* sectors per block */
#define BLOCKS_TO_SECS(x)	((x) << (le32tocpu(e.superBlock.logBlockSize) + 1))
#define GROUP_COUNT			6								/* no. of block groups to load */
#define PAGE_SIZE			4096

/* hardcoded here; the monitor will choose them later */
#define BOOT_DISK			0
#define START_SECTOR		32		/* part 0 */

typedef struct {
	sExt2SuperBlock superBlock;
	sExt2BlockGrp groups[GROUP_COUNT];
} sExt2Simple;

/**
 * Reads/Writes from/to disk
 */
extern int dskio(int dskno,char cmd,int sct,void *addr,int nscts);
extern int sctcapctl(void);

/* the tasks we should load */
static sLoadProg progs[MAX_PROG_COUNT] = {
	{"/boot/kernel.bin","/boot/kernel.bin",BL_K_ID,0,0},
	{"/sbin/disk","/sbin/disk /system/devices/disk",BL_DISK_ID,0,0},
	{"/sbin/rtc","/sbin/rtc /dev/rtc",BL_RTC_ID,0,0},
	{"/sbin/fs","/sbin/fs /dev/fs /dev/hda1 ext2",BL_FS_ID,0,0},
};
static sBootInfo bootinfo;

static sExt2Simple e;
static sExt2Inode rootIno;
static sExt2Inode inoBuf;
static Elf32_Ehdr eheader;
static Elf32_Phdr pheader;

/* temporary buffer to read a block from disk */
static uint buffer[1024 / sizeof(uint)];
/* the start-address for loading programs; the bootloader needs 1 page for data and 1 stack-page */
static uint loadAddr = 0xC0000000;

static int strncmp(const char *str1,const char *str2,size_t count) {
	ssize_t rem = count;
	while(*str1 && *str2 && rem-- > 0) {
		if(*str1++ != *str2++)
			return str1[-1] < str2[-1] ? -1 : 1;
	}
	if(rem <= 0)
		return 0;
	if(*str1 && !*str2)
		return 1;
	return -1;
}

static void halt(const char *s,...) {
	va_list ap;
	va_start(ap,s);
	debugf("Error: ");
	vdebugf(s,ap);
	va_end(ap);
	while(1);
}

static void printInode(sExt2Inode *inode) {
	size_t i;
	debugf("\tmode=0x%08x\n",le16tocpu(inode->mode));
	debugf("\tuid=%d\n",le16tocpu(inode->uid));
	debugf("\tgid=%d\n",le16tocpu(inode->gid));
	debugf("\tsize=%d\n",le32tocpu(inode->size));
	debugf("\taccesstime=%d\n",le32tocpu(inode->accesstime));
	debugf("\tcreatetime=%d\n",le32tocpu(inode->createtime));
	debugf("\tmodifytime=%d\n",le32tocpu(inode->modifytime));
	debugf("\tdeletetime=%d\n",le32tocpu(inode->deletetime));
	debugf("\tlinkCount=%d\n",le16tocpu(inode->linkCount));
	debugf("\tblocks=%d\n",le32tocpu(inode->blocks));
	debugf("\tflags=0x%08x\n",le32tocpu(inode->flags));
	debugf("\tosd1=0x%08x\n",le32tocpu(inode->osd1));
	for(i = 0; i < EXT2_DIRBLOCK_COUNT; i++)
		debugf("\tblock%d=%d\n",i,le32tocpu(inode->dBlocks[i]));
	debugf("\tsinglyIBlock=%d\n",le32tocpu(inode->singlyIBlock));
	debugf("\tdoublyIBlock=%d\n",le32tocpu(inode->doublyIBlock));
	debugf("\ttriplyIBlock=%d\n",le32tocpu(inode->triplyIBlock));
	debugf("\tgeneration=%d\n",le32tocpu(inode->generation));
	debugf("\tfileACL=%d\n",le32tocpu(inode->fileACL));
	debugf("\tdirACL=%d\n",le32tocpu(inode->dirACL));
	debugf("\tfragAddr=%d\n",le32tocpu(inode->fragAddr));
}

static int readSectors(void *dst,uint start,size_t secCount) {
	return dskio(BOOT_DISK,'r',START_SECTOR + start,dst,secCount);
}

static int readBlocks(void *dst,tBlockNo start,size_t blockCount) {
	return dskio(BOOT_DISK,'r',START_SECTOR + BLOCKS_TO_SECS(start),dst,BLOCKS_TO_SECS(blockCount));
}

static void readFromDisk(tBlockNo blkno,char *buf,uint offset,uint nbytes) {
	char *dst = offset == 0 && (nbytes % BLOCK_SIZE) == 0 ? buf : buffer;
	if(offset >= BLOCK_SIZE || offset + nbytes > BLOCK_SIZE)
		halt("Offset / nbytes invalid");

	/*debugf("Reading sectors %d .. %d\n",sector + START_SECTOR,sector + START_SECTOR + SPB);*/
	int result = readSectors(dst,blkno * SPB,SPB);
	if(result != 0)
		halt("Load error");

	if(dst != buf)
		memcpy(buf,(char*)buffer + offset,nbytes);
}

static void loadInode(sExt2Inode *ip,tInodeNo inodeno) {
	sExt2BlockGrp *group = e.groups + ((inodeno - 1) / le32tocpu(e.superBlock.inodesPerGroup));
	if(group >= e.groups + GROUP_COUNT)
		halt("Invalid blockgroup of inode %u: %u\n",inodeno,group - e.groups);
	size_t inodesPerBlock = BLOCK_SIZE / sizeof(sExt2Inode);
	size_t noInGroup = (inodeno - 1) % le32tocpu(e.superBlock.inodesPerGroup);
	tBlockNo blockNo = le32tocpu(group->inodeTable) + noInGroup / inodesPerBlock;
	size_t inodeInBlock = (inodeno - 1) % inodesPerBlock;
	readBlocks(buffer,blockNo,1);
	memcpy(ip,(uint8_t*)buffer + inodeInBlock * sizeof(sExt2Inode),sizeof(sExt2Inode));
}

static tInodeNo searchDir(tInodeNo dirIno,sExt2Inode *dir,const char *name,size_t nameLen) {
	size_t size = le32tocpu(dir->size);
	if(size > sizeof(buffer))
		halt("Directory %u larger than %u bytes\n",dirIno,sizeof(buffer));

	readBlocks(buffer,le32tocpu(dir->dBlocks[0]),1);
	ssize_t rem = size;
	sExt2DirEntry *entry = (sExt2DirEntry*)buffer;

	/* search the directory-entries */
	while(rem > 0 && le32tocpu(entry->inode) != 0) {
		/* found a match? */
		if(nameLen == le16tocpu(entry->nameLen) && strncmp(entry->name,name,nameLen) == 0)
			return le32tocpu(entry->inode);

		/* to next dir-entry */
		rem -= le16tocpu(entry->recLen);
		entry = (sExt2DirEntry*)((uintptr_t)entry + le16tocpu(entry->recLen));
	}
	return EXT2_BAD_INO;
}

static tBlockNo getBlock(sExt2Inode *ino,ulong offset) {
	tBlockNo i,block = offset / BLOCK_SIZE;
	size_t blockSize,blocksPerBlock;

	/* direct block */
	if(block < EXT2_DIRBLOCK_COUNT)
		return le32tocpu(ino->dBlocks[block]);

	/* singly indirect */
	block -= EXT2_DIRBLOCK_COUNT;
	blockSize = BLOCK_SIZE;
	blocksPerBlock = blockSize / sizeof(tBlockNo);
	if(block < blocksPerBlock) {
		readBlocks(buffer,le32tocpu(ino->singlyIBlock),1);
		return le32tocpu(*((tBlockNo*)buffer + block));
	}

	/* doubly indirect */
	block -= blocksPerBlock;
	if(block >= blocksPerBlock * blocksPerBlock)
		halt("Block-number %u to high; triply indirect",block + EXT2_DIRBLOCK_COUNT);

	readBlocks(buffer,le32tocpu(ino->doublyIBlock),1);
	i = le32tocpu(*((tBlockNo*)buffer + block / blocksPerBlock));
	readBlocks(buffer,i,1);
	return le32tocpu(*((tBlockNo*)buffer + block % blocksPerBlock));
}

static tInodeNo namei(char *path,sExt2Inode *ino) {
	tInodeNo inodeno;
	char *p = path;
	char *q;
	char name[14];
	int i;

	/* no relative paths */
	if(*path != '/')
		return EXT2_BAD_INO;

	inodeno = EXT2_ROOT_INO;
	memcpy(ino,&rootIno,sizeof(sExt2Inode));

	/* root? */
	if(*path == '/' && *(path + 1) == '\0')
		return EXT2_BAD_INO;

	if(*p == '/')
		++p;

	while(*p) {
		/* read next path name component from input */
		for(i = 0,q = name; *p && *p != '/' && i < 14; ++i)
			*q++ = *p++;
		*q = '\0';
		if(*p == '/')
			++p;

		if((le16tocpu(ino->mode) & EXT2_S_IFDIR) == 0)
			return EXT2_BAD_INO;

		inodeno = searchDir(inodeno,ino,name,q - name);
		/* component not found */
		if(inodeno == 0)
			return EXT2_BAD_INO;

		loadInode(ino,inodeno);
	}
	return inodeno;
}

uint copyToMem(sExt2Inode *ino,uint offset,uint count,uint dest) {
	tBlockNo blk;
	uint offInBlk,amount;
	while(count > 0) {
		blk = getBlock(ino,offset);

		offInBlk = offset % BLOCK_SIZE;
		amount = MIN(count,BLOCK_SIZE - offInBlk);
		readFromDisk(blk,(char*)dest,offInBlk,amount);

		count -= amount;
		offset += amount;
		dest += amount;
		/* printing a loading-'status' lowers the subjective wait-time ^^ */
		debugChar('.');
	}
	return dest;
}

int loadKernel(sLoadProg *prog,sExt2Inode *ino) {
	size_t j,loadSegNo = 0;
	uint8_t const *datPtr;

	/* read header */
	readFromDisk(le32tocpu(ino->dBlocks[0]),(char*)&eheader,0,sizeof(Elf32_Ehdr));

	/* check magic */
	if(eheader.e_ident.chars[0] != ELFMAG[0] ||
		eheader.e_ident.chars[1] != ELFMAG[1] ||
		eheader.e_ident.chars[2] != ELFMAG[2] ||
		eheader.e_ident.chars[3] != ELFMAG[3])
		return 0;

	/* load the LOAD segments. */
	datPtr = (uint8_t const*)(eheader.e_phoff);
	for(j = 0; j < eheader.e_phnum; datPtr += eheader.e_phentsize, j++) {
		/* read pheader */
		tBlockNo block = getBlock(ino,(ulong)datPtr);
		readFromDisk(block,(char*)&pheader,(uint)datPtr & (BLOCK_SIZE - 1),sizeof(Elf32_Phdr));

		if(pheader.p_type == PT_LOAD) {
			/* read into memory */
			copyToMem(ino,pheader.p_offset,pheader.p_filesz,pheader.p_vaddr);
			/* zero the rest */
			memclear((void*)(pheader.p_vaddr + pheader.p_filesz),
					pheader.p_memsz - pheader.p_filesz);
			loadSegNo++;
			if(pheader.p_vaddr + pheader.p_memsz > loadAddr)
				loadAddr = pheader.p_vaddr + pheader.p_memsz;
		}
	}

	prog->start = 0xC0000000;
	prog->size = loadAddr - 0xC0000000;

	/* leave one page for stack */
	loadAddr += (PAGE_SIZE + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	return 1;
}

static int readInProg(sLoadProg *prog,sExt2Inode *ino) {
	/* make page-boundary */
	loadAddr = (loadAddr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	prog->start = loadAddr;
	prog->size = le32tocpu(ino->size);

	/* load file into memory */
	loadAddr = copyToMem(ino,0,le32tocpu(ino->size),loadAddr);
	return 1;
}

sBootInfo *bootload(uint memSize) {
	int i;
	int cap = sctcapctl();
	if(cap == 0)
		halt("Disk not found");

	bootinfo.progCount = 0;
	bootinfo.progs = progs;
	bootinfo.diskSize = cap * SEC_SIZE;
	bootinfo.memSize = memSize;

	/* load superblock */
	readSectors(&(e.superBlock),EXT2_SUPERBLOCK_SECNO,2);

	/* TODO currently required */
	if(BLOCK_SIZE != 1024)
		halt("Invalid block size %d; 1024 expected\n",BLOCK_SIZE);

	/* read blockgroup descriptor table */
	readBlocks(e.groups,le32tocpu(e.superBlock.firstDataBlock) + 1,1);

	/* read root inode */
	loadInode(&rootIno,EXT2_ROOT_INO);

	for(i = 0; i < PROG_COUNT; i++) {
		/* get inode of path */
		if(namei(progs[i].path,&inoBuf) == EXT2_BAD_INO)
			halt("Unable to find '%s'\n",progs[i].path);
		else {
			/* load program */
			debugf("Loading %s",progs[i].path);
			if(progs[i].id == BL_K_ID)
				loadKernel(progs + i,&inoBuf);
			else
				readInProg(progs + i,&inoBuf);
			debugf("\n");
		}
		bootinfo.progCount++;
	}

	return &bootinfo;
}
