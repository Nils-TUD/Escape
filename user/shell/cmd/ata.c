/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <messages.h>
#include <io.h>
#include <heap.h>
#include <debug.h>
#include <proc.h>
#include <string.h>
#include "ata.h"

/*
 * Sample layout (20MB disk, 1KiB blocks):
 * Block Offset        Length      Description
 * -- block group 0 --
 * byte 0              512 bytes   boot record (if present)
 * byte 512            512 bytes   additional boot record data (if present)
 * byte 1024           1024 bytes  superblock
 * block 2             1 block     block group descriptor table
 * block 3             1 block     block bitmap
 * block 4             1 block     inode bitmap
 * block 5             214 blocks  inode table
 * block 219           7974 blocks data blocks
 * -- block group 1 --
 * block 8193          1 block     superblock backup
 * block 8194          1 block     block group descriptor table backup
 * block 8195          1 block     block bitmap
 * block 8196          1 block     inode bitmap
 * block 8197          214 blocks  inode table
 * block 8408          7974 blocks data blocks
 * -- block group 2 --
 * block 16385         1 block     block bitmap
 * block 16386         1 block     inode bitmap
 * block 16387         214 blocks  inode table
 * block 16601         3879 blocks data blocks
 */

#if 0
static sSuperBlock sb;

static bool readSectors(tFD fd,u8 *buffer,u64 lba,u16 secCount);
static bool readBlocks(tFD fd,u8 *buffer,u32 start,u16 blockCount);
static sInode *resolvePath(tFD fd,sInode *itable,s8 *path);
static void printFile(tFD fd,sInode *inode);
static void printBlockBitmap(u32 blockNo,void *block);
static void printDirectory(void *block);
static void printInodeTable(sInode *table,u32 count);
static void printInode(sInode *inode);
static void printBlockGroupTable(sBlockGroup *table);
static void printBlockGroup(sBlockGroup *group);
static void printSuperBlock(sSuperBlock *super);
#endif

s32 shell_cmdAta(u32 argc,s8 **argv) {
	UNUSED(argc);
	UNUSED(argv);

#if 0
	s32 fd;
	sBlockGroup *bgs;
	sInode *intbl,*in;
	u8 *buffer;

	fd = open("services:/ata",IO_WRITE | IO_READ);
	if(fd < 0) {
		printLastError();
		return 1;
	}

	/* read super-block */
	if(!readSectors(fd,(u8*)&sb,2,1))
		return 1;
	printSuperBlock(&sb);

	/* read block group table */
	bgs = (sBlockGroup*)malloc(sizeof(u8) * BLOCK_SIZE);
	if(bgs == NULL) {
		printLastError();
		return 1;
	}
	if(!readBlocks(fd,(u8*)bgs,2,1))
		return 1;
	printBlockGroupTable(bgs);

	buffer = (u8*)malloc(sizeof(u8) * BLOCK_SIZE);
	readBlocks(fd,buffer,bgs[0].blockBitmap,1);
	printBlockBitmap(0,buffer);
	free(buffer);

	intbl = (sInode*)malloc(sizeof(u8) * BLOCK_SIZE * 2);
	readBlocks(fd,(u8*)intbl,bgs[0].inodeTable,2);
	/*printInodeTable(intbl,BLOCK_SIZE * 2 / sizeof(sInode));*/

	buffer = (u8*)malloc(sizeof(u8) * BLOCK_SIZE);
	readBlocks(fd,buffer,intbl[1].dBlocks[0],1);
	printDirectory(buffer);
	free(buffer);

	in = resolvePath(fd,intbl,(s8*)"/test/file.txt");
	printf("Inode %d:\n",in - intbl);
	printInode(in);
	printf("\n");
	in = resolvePath(fd,intbl,(s8*)"/lost+found");
	printf("Inode %d:\n",in - intbl);
	printInode(in);
	printf("\n");
	in = resolvePath(fd,intbl,(s8*)"/lost+found/");
	printf("Inode %d:\n",in - intbl);
	printInode(in);
	printf("\n");

	/*printFile(fd,intbl + 13);*/

	free(intbl);
	free(bgs);

	close(fd);
#endif
	return 0;
}

#if 0
static sInode *resolvePath(tFD fd,sInode *itable,s8 *path) {
	sInode *inode = NULL;
	s8 *p = path;
	u32 pos;

	if(*p != '/') {
		/* TODO */
		return NULL;
	}

	inode = itable + (EXT2_ROOT_INO - 1);
	/* skip '/' */
	p++;

	pos = strchri(p,'/');
	while(*p) {
		sDirEntry *e = (sDirEntry*)malloc(sizeof(u8) * BLOCK_SIZE);
		if(e == NULL)
			return NULL;

		/* TODO a directory may have more blocks */
		readBlocks(fd,(u8*)e,inode->dBlocks[0],1);
		while(e->inode != 0) {
			if(strncmp(e + 1,p,pos) == 0) {
				p += pos;
				inode = itable + (e->inode - 1);

				/* skip slashes */
				while(*p == '/')
					p++;
				/* "/" at the end is optional */
				if(!*p)
					break;

				/* move to childs of this node */
				pos = strchri(p,'/');
				if((inode->mode & EXT2_S_IFDIR) == 0) {
					free(e);
					return NULL;
				}
				break;
			}

			/* to next dir-entry */
			e = (u8*)e + e->recLen;
		}

		free(e);
	}

	return inode;
}

static bool readBlocks(tFD fd,u8 *buffer,u32 start,u16 blockCount) {
	return readSectors(fd,buffer,BLOCKS_TO_SECS(start),BLOCKS_TO_SECS(blockCount));
}

static bool readSectors(tFD fd,u8 *buffer,u64 lba,u16 secCount) {
	sMsgATAReq req = {
		.header = {
			.id = MSG_ATA_READ_REQ,
			.length = sizeof(sMsgDataATAReq)
		},
		.data = {
			.drive = 0,
			.lba = lba,
			.secCount = secCount
		}
	};
	sMsgDefHeader res;

	/* send read-request */
	if(write(fd,&req,sizeof(sMsgATAReq)) < 0) {
		printLastError();
		return false;
	}

	/* wait for response */
	do {
		sleep();
	}
	while(read(fd,&res,sizeof(sMsgDefHeader)) <= 0);

	/* read response */
	if(read(fd,buffer,res.length) < 0) {
		printLastError();
		return false;
	}

	return true;
}

static void printFile(tFD fd,sInode *inode) {
	u8 *buffer,*bufWork;
	u32 total = inode->blocks / 2;
	u32 i,limit = MIN(EXT2_DIRBLOCK_COUNT,total);

	buffer = (u8*)malloc(total * BLOCK_SIZE * sizeof(u8));
	if(buffer == NULL)
		return;

	bufWork = buffer;
	for(i = 0; i < limit; i++) {
		/* TODO try to read multiple blocks at once */
		readBlocks(fd,bufWork,inode->dBlocks[i],1);
		bufWork += BLOCK_SIZE;
	}
	/* TODO read indirect blocks */

	dumpBytes(buffer,inode->size);
}

static void printBlockBitmap(u32 blockNo,void *block) {
	u32 i,b;
	u32 limit = MIN(sb.blocksPerGroup,sb.blockCount - (blockNo * sb.blocksPerGroup)) / 8;
	u8 *bitmap = (u8*)block;
	printf("Used blocks: ");
	for(i = 0; i < limit; i++) {
		for(b = 0; b < 8; b++) {
			if((*bitmap & (1 << b)) != 0)
				printf("%d, ",i * 8 + b);
		}
		bitmap++;
	}
	printf("\n");
}

static void printDirectory(void *block) {
	s8 string[256];
	sDirEntry *e = (sDirEntry*)block;
	while(e->inode != 0) {
		memcpy(string,e + 1,e->nameLen);
		*(string + e->nameLen) = '\0';
		printf("inode=%d, name=%s\n",e->inode,string);
		e = (u8*)e + e->recLen;
	}
}

static void printInodeTable(sInode *table,u32 count) {
	u32 i;
	for(i = 0; i < count; i++) {
		printf("Inode %d:\n",i);
		printInode(table);
		printf("\n");
		table++;
	}
}

static void printInode(sInode *inode) {
	u32 i;
	printf("\tmode=0x%08x\n",inode->mode);
	printf("\tuid=%d\n",inode->uid);
	printf("\tgid=%d\n",inode->gid);
	printf("\tsize=%d\n",inode->size);
	printf("\taccesstime=%d\n",inode->accesstime);
	printf("\tcreatetime=%d\n",inode->createtime);
	printf("\tmodifytime=%d\n",inode->modifytime);
	printf("\tdeletetime=%d\n",inode->deletetime);
	printf("\tlinkCount=%d\n",inode->linkCount);
	printf("\tblocks=%d\n",inode->blocks);
	printf("\tflags=0x%08x\n",inode->flags);
	printf("\tosd1=0x%08x\n",inode->osd1);
	for(i = 0; i < EXT2_DIRBLOCK_COUNT; i++)
		printf("\tblock%d=%d\n",i,inode->dBlocks[i]);
	printf("\tsinglyIBlock=%d\n",inode->singlyIBlock);
	printf("\tdoublyIBlock=%d\n",inode->doublyIBlock);
	printf("\ttriplyIBlock=%d\n",inode->triplyIBlock);
	printf("\tgeneration=%d\n",inode->generation);
	printf("\tfileACL=%d\n",inode->fileACL);
	printf("\tdirACL=%d\n",inode->dirACL);
	printf("\tfragAddr=%d\n",inode->fragAddr);
	printf("\tosd2=0x%08x%08x%08x%08x\n",inode->osd2[0],inode->osd2[1],inode->osd2[2],inode->osd2[3]);
}

static void printBlockGroupTable(sBlockGroup *table) {
	u32 i;
	u32 bgCount = (sb.blockCount + sb.blocksPerGroup - 1) / sb.blocksPerGroup;
	for(i = 0; i < bgCount; i++) {
		printf("BlockGroup %d:\n",i);
		printBlockGroup(table);
		printf("\n");
		table++;
	}
}

static void printBlockGroup(sBlockGroup *group) {
	printf("\tblockBitmap=%d\n",group->blockBitmap);
	printf("\tinodeBitmap=%d\n",group->inodeBitmap);
	printf("\tinodeTable=%d\n",group->inodeTable);
	printf("\tfreeBlockCount=%d\n",group->freeBlockCount);
	printf("\tfreeInodeCount=%d\n",group->freeInodeCount);
	printf("\tuserDirCount=%d\n",group->usedDirCount);
}

static void printSuperBlock(sSuperBlock *super) {
	const s8 *states[] = {
		"Valid",
		"Error"
	};
	const s8 *errorStrats[] = {
		"Continue",
		"MountReadonly",
		"Panic"
	};
	const s8 *createOSs[] = {
		"Linux",
		"Hurd",
		"Masix",
		"FreeBSD",
		"Lites"
	};
	const s8 *compatFeatures[] = {
		"DirPrealloc",
		"IMagicInodes",
		"HasJournal",
		"ExtendedAttributes",
		"ResizeInodes",
		"DirectoryIndex"
	};
	const s8 *incompatFeatures[] = {
		"Compression",
		"FileType",
		"Recover",
		"JournalDev",
		"MetaBg"
	};
	const s8 *roFeatures[] = {
		"SparseSuper",
		"LargeFile",
		"BTreeDir"
	};
	u32 i;

	printf("inodeCount=%d\n",super->inodeCount);
	printf("blockCount=%d\n",super->blockCount);
	printf("suResBlockCount=%d\n",super->suResBlockCount);
	printf("freeBlockCount=%d\n",super->freeBlockCount);
	printf("freeInodeCount=%d\n",super->freeInodeCount);
	printf("firstDataBlock=%d\n",super->firstDataBlock);
	printf("logBlockSize=%d\n",super->logBlockSize);
	printf("logFragSize=%d\n",super->logFragSize);
	printf("blocksPerGroup=%d\n",super->blocksPerGroup);
	printf("fragsPerGroup=%d\n",super->fragsPerGroup);
	printf("inodesPerGroup=%d\n",super->inodesPerGroup);
	printf("lastMountTime=%d\n",super->lastMountTime);
	printf("lastWritetime=%d\n",super->lastWritetime);
	printf("mountCount=%d\n",super->mountCount);
	printf("maxMountCount=%d\n",super->maxMountCount);
	printf("magic=0x%x\n",super->magic);
	printf("state=%s\n",states[super->state - 1]);
	printf("errors=%s\n",errorStrats[super->errors - 1]);
	printf("minorRevLevel=%d\n",super->minorRevLevel);
	printf("lastCheck=%d\n",super->lastCheck);
	printf("checkInterval=%d\n",super->checkInterval);
	printf("creatorOS=%s\n",createOSs[super->creatorOS]);
	printf("revLevel=%d\n",super->revLevel);
	printf("defResUid=%d\n",super->defResUid);
	printf("defResGid=%d\n",super->defResGid);
	printf("firstInode=%d\n",super->firstInode);
	printf("inodeSize=%d\n",super->inodeSize);
	printf("blockGroupNo=%d\n",super->blockGroupNo);
	printf("featureCompat=0x%x (",super->featureCompat);
	for(i = 0; i < 6; i++) {
		if(super->featureCompat & (1 << i))
			printf("%s | ",compatFeatures[i]);
	}
	printf(")\n");
	printf("featureInCompat=0x%x (",super->featureInCompat);
	for(i = 0; i < 5; i++) {
		if(super->featureInCompat & (1 << i))
			printf("%s | ",incompatFeatures[i]);
	}
	printf(")\n");
	printf("featureRoCompat=0x%x (",super->featureRoCompat);
	for(i = 0; i < 3; i++) {
		if(super->featureRoCompat & (1 << i))
			printf("%s | ",roFeatures[i]);
	}
	printf(")\n");
	printf("volumeUid=0x");
	for(i = 0; i < 16; i++)
		printf("%02x",super->volumeUid[i]);
	printf("\n");
	printf("volumeName=%s\n",super->volumeName);
	printf("lastMountPath=%s\n",super->lastMountPath);
	printf("algoBitmap=0x%x\n",super->algoBitmap);
	printf("preAllocBlocks=%d\n",super->preAllocBlocks);
	printf("preAllocDirBlocks=%d\n",super->preAllocDirBlocks);
	printf("journalUid=0x");
	for(i = 0; i < 16; i++)
		printf("%02x",super->journalUid[i]);
	printf("\n");
	printf("journalInodeNo=%d\n",super->journalInodeNo);
	printf("journalDev=%d\n",super->journalDev);
	printf("lastOrphan=%d\n",super->lastOrphan);
	printf("hashSeed=0x");
	for(i = 0; i < 4; i++)
		printf("%08x",super->hashSeed[i]);
	printf("\n");
	printf("defHashVersion=0x%x\n",super->defHashVersion);
	printf("defMountOptions=0x%x\n",super->defMountOptions);
	printf("firstMetaBg=%d\n",super->firstMetaBg);
}
#endif
