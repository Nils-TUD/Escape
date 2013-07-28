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
#include <esc/endian.h>
#include <esc/elf.h>
#include <sys/boot.h>
#include "../../../drivers/common/fs/ext2/ext2.h"
#include <string.h>
#include <stdarg.h>

#define PROG_COUNT			4
#define SEC_SIZE			512								/* disk sector size in bytes */
#define BLOCK_SIZE			((size_t)(1024 << le32tocpu(e.superBlock.logBlockSize)))
#define SPB					(BLOCK_SIZE / SEC_SIZE)			/* sectors per block */
#define BLOCKS_TO_SECS(x)	((x) << (le32tocpu(e.superBlock.logBlockSize) + 1))
#define GROUP_COUNT			8								/* no. of block groups to load */
#define PAGE_SIZE			8192

/* hardcoded here; the monitor will choose them later */
#define BOOT_DISK			0
#define START_SECTOR		128		/* part 0 */

typedef struct {
	sExt2SuperBlock superBlock;
	sExt2BlockGrp groups[GROUP_COUNT];
} sExt2Simple;

EXTERN_C void flushRegion(void *addr,size_t count);
/**
 * Reads/Writes from/to disk
 */
EXTERN_C int dskio(long sct,void *addr,long nscts);
EXTERN_C int sctcapctl(void);
EXTERN_C uintptr_t bootload(size_t memSize);

/* the tasks we should load */
static LoadProg progs[MAX_PROG_COUNT] = {
	{"/boot/escape","/boot/escape",BL_K_ID,0,0},
	{"/sbin/disk","/sbin/disk /system/devices/disk",BL_DISK_ID,0,0},
	{"/sbin/rtc","/sbin/rtc /dev/rtc",BL_RTC_ID,0,0},
	{"/sbin/fs","/sbin/fs /dev/fs /dev/hda1 ext2",BL_FS_ID,0,0},
};
static BootInfo bootinfo;

static sExt2Simple e;
static sExt2Inode rootIno;
static sExt2Inode inoBuf;
static Elf64_Ehdr eheader;
static Elf64_Phdr pheader;
static Elf64_Shdr sheader;

/* temporary buffer to read a block from disk */
static ulong buffer[2048 / sizeof(ulong)];
/* the start-address for loading programs; the bootloader needs 1 page for data and 1 stack-page */
static uintptr_t loadAddr = 0x8000000000000000;

static int mystrncmp(const char *str1,const char *str2,size_t count) {
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

static int readSectors(void *dst,size_t start,size_t secCount) {
	return dskio(START_SECTOR + start,dst,secCount);
}

static int readBlocks(void *dst,block_t start,size_t blockCount) {
	return readSectors(dst,BLOCKS_TO_SECS(start),BLOCKS_TO_SECS(blockCount));
}

static void readFromDisk(block_t blkno,void *buf,size_t offset,size_t nbytes) {
	void *dst = offset == 0 && (nbytes % BLOCK_SIZE) == 0 ? buf : buffer;
	size_t secCount = (offset + nbytes + SEC_SIZE - 1) / SEC_SIZE;
	if(offset >= BLOCK_SIZE || nbytes > BLOCK_SIZE)
		halt("offset invalid");

	/*debugf("Reading sectors %d .. %d\n",START_SECTOR + blkno * SPB,
			START_SECTOR + blkno * SPB + secCount - 1);*/
	int result = readSectors(dst,blkno * SPB,secCount);
	if(result != 0)
		halt("Load error");

	if(dst != buf)
		memcpy(buf,(char*)buffer + offset,nbytes);
}

static void loadInode(sExt2Inode *ip,inode_t inodeno) {
	sExt2BlockGrp *group = e.groups + ((inodeno - 1) / le32tocpu(e.superBlock.inodesPerGroup));
	if(group >= e.groups + GROUP_COUNT)
		halt("Invalid blockgroup of inode %u: %u\n",inodeno,group - e.groups);
	size_t inodesPerBlock = BLOCK_SIZE / sizeof(sExt2Inode);
	size_t noInGroup = (inodeno - 1) % le32tocpu(e.superBlock.inodesPerGroup);
	block_t blockNo = le32tocpu(group->inodeTable) + noInGroup / inodesPerBlock;
	size_t inodeInBlock = (inodeno - 1) % inodesPerBlock;
	readBlocks(buffer,blockNo,1);
	memcpy(ip,(uint8_t*)buffer + inodeInBlock * sizeof(sExt2Inode),sizeof(sExt2Inode));
}

static inode_t searchDir(inode_t dirIno,sExt2Inode *dir,const char *name,size_t nameLen) {
	size_t size = le32tocpu(dir->size);
	if(size > sizeof(buffer))
		halt("Directory %u larger than %u bytes\n",dirIno,sizeof(buffer));

	readBlocks(buffer,le32tocpu(dir->dBlocks[0]),1);
	ssize_t rem = size;
	sExt2DirEntry *entry = (sExt2DirEntry*)buffer;

	/* search the directory-entries */
	while(rem > 0 && le32tocpu(entry->inode) != 0) {
		/* found a match? */
		if(nameLen == le16tocpu(entry->nameLen) && mystrncmp(entry->name,name,nameLen) == 0)
			return le32tocpu(entry->inode);

		/* to next dir-entry */
		rem -= le16tocpu(entry->recLen);
		entry = (sExt2DirEntry*)((uintptr_t)entry + le16tocpu(entry->recLen));
	}
	return EXT2_BAD_INO;
}

static block_t getBlock(sExt2Inode *ino,size_t offset) {
	block_t i,block = offset / BLOCK_SIZE;
	size_t blockSize,blocksPerBlock;

	/* direct block */
	if(block < EXT2_DIRBLOCK_COUNT)
		return le32tocpu(ino->dBlocks[block]);

	/* singly indirect */
	block -= EXT2_DIRBLOCK_COUNT;
	blockSize = BLOCK_SIZE;
	blocksPerBlock = blockSize / sizeof(block_t);
	if(block < blocksPerBlock) {
		readBlocks(buffer,le32tocpu(ino->singlyIBlock),1);
		return le32tocpu(*((block_t*)buffer + block));
	}

	/* doubly indirect */
	block -= blocksPerBlock;
	if(block >= blocksPerBlock * blocksPerBlock)
		halt("Block-number %u to high; triply indirect",block + EXT2_DIRBLOCK_COUNT);

	readBlocks(buffer,le32tocpu(ino->doublyIBlock),1);
	i = le32tocpu(*((block_t*)buffer + block / blocksPerBlock));
	readBlocks(buffer,i,1);
	return le32tocpu(*((block_t*)buffer + block % blocksPerBlock));
}

static inode_t namei(char *path,sExt2Inode *ino) {
	inode_t inodeno;
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

static uintptr_t copyToMem(sExt2Inode *ino,size_t offset,size_t count,uintptr_t dest) {
	block_t blk;
	size_t offInBlk,amount,i = 0;
	while(count > 0) {
		blk = getBlock(ino,offset);

		offInBlk = offset % BLOCK_SIZE;
		amount = MIN(count,BLOCK_SIZE - offInBlk);
		readFromDisk(blk,(void*)dest,offInBlk,amount);

		count -= amount;
		offset += amount;
		dest += amount;
		/* printing a loading-'status' lowers the subjective wait-time ^^ */
		if((i++ % 10) == 0)
			debugChar('.');
	}
	return dest;
}

static int loadKernel(LoadProg *prog,sExt2Inode *ino) {
	size_t j,loadSegNo = 0;
	uint8_t const *datPtr;

	/* read header */
	readFromDisk(le32tocpu(ino->dBlocks[0]),&eheader,0,sizeof(Elf64_Ehdr));

	/* check magic */
	if(eheader.e_ident[0] != ELFMAG[0] ||
		eheader.e_ident[1] != ELFMAG[1] ||
		eheader.e_ident[2] != ELFMAG[2] ||
		eheader.e_ident[3] != ELFMAG[3])
		return 0;

	/* load the LOAD segments. */
	/* TODO it would be better to take care that we don't load a block twice */
	datPtr = (uint8_t const*)(eheader.e_phoff);
	for(j = 0; j < eheader.e_phnum; datPtr += eheader.e_phentsize, j++) {
		/* read pheader */
		block_t block = getBlock(ino,(size_t)datPtr);
		readFromDisk(block,&pheader,(uintptr_t)datPtr & (BLOCK_SIZE - 1),sizeof(Elf64_Phdr));

		if(pheader.p_type == PT_LOAD) {
			/* read into memory */
			copyToMem(ino,pheader.p_offset,pheader.p_filesz,pheader.p_vaddr);
			/* zero the rest */
			memclear((void*)(pheader.p_vaddr + pheader.p_filesz),
					pheader.p_memsz - pheader.p_filesz);
			if(loadSegNo == 0) {
				/* flush to memory and sync caches; we'll use them as instructions */
				flushRegion((void*)pheader.p_vaddr,pheader.p_memsz);
			}
			loadSegNo++;
			if(pheader.p_vaddr + pheader.p_memsz > loadAddr)
				loadAddr = pheader.p_vaddr + pheader.p_memsz;
		}
	}

	prog->start = 0x8000000000000000;
	prog->size = loadAddr - 0x8000000000000000;

	/* to next page for the stack */
	loadAddr = ROUND_UP(loadAddr,PAGE_SIZE);
	bootinfo.kstackBegin = loadAddr;

	/* load the regs-section */
	datPtr = (uint8_t const*)(eheader.e_shoff);
	for(j = 0; j < eheader.e_shnum; datPtr += eheader.e_shentsize, j++) {
		/* read pheader */
		block_t block = getBlock(ino,(size_t)datPtr);
		readFromDisk(block,&sheader,(uintptr_t)datPtr & (BLOCK_SIZE - 1),sizeof(Elf64_Shdr));

		/* the first section with type == PROGBITS and addr != 0 should be .MMIX.reg_contents */
		if(sheader.sh_type == SHT_PROGBITS && sheader.sh_addr < prog->start && sheader.sh_addr != 0) {
			/* give the kernel the bootinfo in $0 */
			*(uint64_t*)loadAddr = (uint64_t)&bootinfo;
			loadAddr += 8;
			/* set rL to 1 */
			*(uint64_t*)loadAddr = 1;
			loadAddr += 8;
			/* append global registers */
			copyToMem(ino,sheader.sh_offset,sheader.sh_size,loadAddr);
			loadAddr += sheader.sh_size;
			/* $255 */
			*(uint64_t*)loadAddr = 0;
			loadAddr += 8;
			/* 12 slots for special registers */
			memclear((void*)loadAddr,12 * 8);
			loadAddr += 12 * 8;
			/* set rG|rA */
			*(uint64_t*)loadAddr = (255 - (sheader.sh_size / 8)) << 56;
			loadAddr += 8;
			bootinfo.kstackEnd = loadAddr - 8;
			break;
		}
	}

	return 1;
}

static int readInProg(LoadProg *prog,sExt2Inode *ino) {
	/* make page-boundary */
	loadAddr = ROUND_PAGE_UP(loadAddr);
	prog->start = loadAddr;
	prog->size = le32tocpu(ino->size);

	/* load file into memory */
	loadAddr = copyToMem(ino,0,le32tocpu(ino->size),loadAddr);
	return 1;
}

EXTERN_C uintptr_t bootload(size_t memSize) {
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
	return bootinfo.kstackEnd;
}
