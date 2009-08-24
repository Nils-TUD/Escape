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
#include <esc/service.h>
#include <messages.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/ports.h>
#include <esc/heap.h>
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/signals.h>
#include <stdlib.h>
#include <errors.h>
#include "partition.h"

/* port-bases */
#define REG_BASE_PRIMARY			0x1F0
#define REG_BASE_SECONDARY			0x170

#define DRIVE_MASTER				0xA0
#define DRIVE_SLAVE					0xB0

#define COMMAND_IDENTIFY			0xEC
#define COMMAND_READ_SEC			0x20
#define COMMAND_READ_SEC_EXT		0x24
#define COMMAND_WRITE_SEC			0x30
#define COMMAND_WRITE_SEC_EXT		0x34

/* io-ports, offsets from base */
#define REG_DATA					0x0
#define REG_ERROR					0x1
#define REG_FEATURES				0x1
#define REG_SECTOR_COUNT			0x2
#define REG_PART_DISK_SECADDR1		0x3
#define REG_PART_DISK_SECADDR2		0x4
#define REG_PART_DISK_SECADDR3		0x5
#define REG_DRIVE_SELECT			0x6
#define REG_COMMAND					0x7
#define REG_STATUS					0x7
#define REG_CONTROL					0x206

/* drive-identifier */
#define DRIVE_COUNT					4
#define DRIVE_PRIM_MASTER			0
#define DRIVE_PRIM_SLAVE			1
#define DRIVE_SEC_MASTER			2
#define DRIVE_SEC_SLAVE				3

#define BYTES_PER_SECTOR			512

/* Drive is preparing to accept/send data -- wait until this bit clears. If it never
 * clears, do a Software Reset. Technically, when BSY is set, the other bits in the
 * Status byte are meaningless. */
#define CMD_ST_BUSY					(1 << 7)	/* 0x80 */
/* Bit is clear when drive is spun down, or after an error. Set otherwise. */
#define CMD_ST_READY				(1 << 6)	/* 0x40 */
/* Drive Fault Error (does not set ERR!) */
#define CMD_ST_DISK_FAULT			(1 << 5)	/* 0x20 */
/* Overlapped Mode Service Request */
#define CMD_ST_OVERLAPPED_REQ		(1 << 4)	/* 0x10 */
/* Set when the drive has PIO data to transfer, or is ready to accept PIO data. */
#define CMD_ST_DRQ					(1 << 3)	/* 0x08 */
/* Error flag (when set). Send a new command to clear it (or nuke it with a Software Reset). */
#define CMD_ST_ERROR				(1 << 0)	/* 0x01 */

/* Set this to read back the High Order Byte of the last LBA48 value sent to an IO port. */
#define CTRL_HIGH_ORDER_BYTE		(1 << 7)	/* 0x80 */
/* Software Reset -- set this to reset all ATA drives on a bus, if one is misbehaving. */
#define CTRL_SOFTWARE_RESET			(1 << 2)	/* 0x04 */
/* Set this to stop the current device from sending interrupts. */
#define CTRL_INTRPTS_ENABLED		(1 << 1)	/* 0x02 */

typedef struct {
	/* wether the drive exists and we can use it */
	u8 present;
	/* master / slave */
	u8 slaveBit;
	/* primary / secondary */
	u16 basePort;
	/* supports LBA48? */
	u8 lba48;
	/* number of sectors of the drive */
	u64 sectorCount;
	/* the partition-table */
	sPartition partTable[PARTITION_COUNT];
} sATADrive;

typedef struct {
	u32 drive;
	u32 partition;
} sDriver;

static void ata_wait(sATADrive *drive);
static bool ata_isDrivePresent(u8 drive);
static void ata_detectDrives(void);
static void ata_createVFSEntry(sATADrive *drive,sPartition *part,char *name);
static bool ata_readWrite(sATADrive *drive,bool opWrite,u16 *buffer,u64 lba,u16 secCount);
static void ata_selectDrive(sATADrive *drive);
static bool ata_identifyDrive(sATADrive *drive);
static sDriver *getDriver(tServ sid);

/* the drives */
static sATADrive drives[DRIVE_COUNT] = {
	/* primary master */
	{ .present = 0, .slaveBit = 0, .basePort = REG_BASE_PRIMARY },
	/* primary slave */
	{ .present = 0, .slaveBit = 1, .basePort = REG_BASE_PRIMARY },
	/* secondary master */
	{ .present = 0, .slaveBit = 0, .basePort = REG_BASE_SECONDARY },
	/* secondary slave */
	{ .present = 0, .slaveBit = 1, .basePort = REG_BASE_SECONDARY }
};
static u32 servCount = 0;
static tServ services[DRIVE_COUNT * PARTITION_COUNT];
static sDriver driver[DRIVE_COUNT * PARTITION_COUNT];

static sMsg msg;
static bool gotInterrupt = false;
static void diskIntrptHandler(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	gotInterrupt = true;
}

int main(void) {
	u32 i;
	tServ client;
	tMsgId mid;

	/* request ports */
	if(requestIOPorts(REG_BASE_PRIMARY,8) < 0 || requestIOPorts(REG_BASE_SECONDARY,8) < 0) {
		printe("Unable to request ATA-port %d .. %d or %d .. %d",REG_BASE_PRIMARY,
				REG_BASE_PRIMARY + 7,REG_BASE_SECONDARY,REG_BASE_SECONDARY + 7);
		return EXIT_FAILURE;
	}
	if(requestIOPort(REG_BASE_PRIMARY + REG_CONTROL) < 0 ||
			requestIOPort(REG_BASE_SECONDARY + REG_CONTROL) < 0) {
		printe("Unable to request ATA-port %d or %d",REG_BASE_PRIMARY + REG_CONTROL,
				REG_BASE_SECONDARY + REG_CONTROL);
		return EXIT_FAILURE;
	}

	if(setSigHandler(SIG_INTRPT_ATA1,diskIntrptHandler) < 0 ||
			setSigHandler(SIG_INTRPT_ATA2,diskIntrptHandler) < 0) {
		printe("Unable to announce sig-handler for %d or %d",SIG_INTRPT_ATA1,SIG_INTRPT_ATA2);
		return EXIT_FAILURE;
	}

	ata_detectDrives();

	while(1) {
		tFD fd = getClient(services,servCount,&client);
		if(fd < 0) {
			wait(EV_CLIENT);
		}
		else {
			sDriver *drv = getDriver(client);
			sATADrive *drive = drv == NULL ? NULL : (drives + drv->drive);
			sPartition *part = (drv == NULL || drive == NULL) ? NULL : (drive->partTable + drv->partition);
			if(drv == NULL || drive->present == 0 || part->present == 0) {
				/* should never happen */
				close(fd);
				continue;
			}

			while(receive(fd,&mid,&msg) > 0) {
				switch(mid) {
					case MSG_DRV_OPEN:
						msg.args.arg1 = 0;
						send(fd,MSG_DRV_OPEN_RESP,&msg,sizeof(msg.args));
						break;

					case MSG_DRV_READ: {
						u16 *buffer = NULL;
						u32 offset = msg.args.arg1;
						u32 count = msg.args.arg2;
						msg.args.arg1 = 0;
						if(offset + count <= part->size * BYTES_PER_SECTOR && offset + count > offset) {
							buffer = (u16*)malloc(count);
							if(buffer) {
								if(ata_readWrite(drive,false,buffer,
										offset / BYTES_PER_SECTOR + part->start,count / BYTES_PER_SECTOR)) {
									msg.data.arg1 = count;
								}
							}
						}
						msg.args.arg2 = true;
						send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
						if(buffer) {
							send(fd,MSG_DRV_READ_RESP,buffer,count);
							free(buffer);
						}
					}
					break;

					case MSG_DRV_WRITE: {
						u16 *buffer = NULL;
						u32 offset = msg.args.arg1;
						u32 count = msg.args.arg2;
						msg.args.arg1 = 0;
						if(offset + count <= part->size * BYTES_PER_SECTOR && offset + count > offset) {
							buffer = (u16*)malloc(count);
							if(buffer) {
								receive(fd,&mid,buffer);
								if(ata_readWrite(drive,true,buffer,
										offset / BYTES_PER_SECTOR + part->start,count / BYTES_PER_SECTOR)) {
									msg.args.arg1 = count;
								}
								free(buffer);
							}
						}
						send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
					}
					break;

					case MSG_DRV_IOCTL: {
						msg.data.arg1 = ERR_UNSUPPORTED_OPERATION;
						msg.data.arg2 = 0;
						send(fd,MSG_DRV_IOCTL_RESP,&msg,sizeof(msg.data));
					}
					break;

					case MSG_DRV_CLOSE:
						/* ignore */
						break;
				}
			}

			close(fd);
		}
	}

	/* clean up */
	releaseIOPorts(REG_BASE_PRIMARY,8);
	releaseIOPorts(REG_BASE_SECONDARY,8);
	releaseIOPort(REG_BASE_PRIMARY + REG_CONTROL);
	releaseIOPort(REG_BASE_SECONDARY + REG_CONTROL);
	for(i = 0; i < servCount; i++)
		unregService(services[i]);
	return EXIT_SUCCESS;
}

static void ata_wait(sATADrive *drive) {
	/* FIXME: vmware seems to need a very long wait-time:
	volatile u32 i;
	for(i = 0; i < 1000000; i++);
	*/
	inByte(drive->basePort + REG_STATUS);
	inByte(drive->basePort + REG_STATUS);
	inByte(drive->basePort + REG_STATUS);
	inByte(drive->basePort + REG_STATUS);
}

static bool ata_isDrivePresent(u8 drive) {
	return drive < DRIVE_COUNT && drives[drive].present;
}

static void ata_detectDrives(void) {
	char name[10];
	u32 i,p,s;
	u16 buffer[256];
	s = 0;
	for(i = 0; i < DRIVE_COUNT; i++) {
		/* first, identify the drive */
		if(!ata_identifyDrive(drives + i))
			continue;

		/* if it is present, read the partition-table */
		drives[i].present = 1;
		if(!ata_readWrite(drives + i,false,buffer,0,1)) {
			drives[i].present = 0;
			debugf("Drive %d: Unable to read partition-table!\n",i);
			continue;
		}

		/* copy partitions to mem */
		part_fillPartitions(drives[i].partTable,buffer);
		sprintf(name,"hd%c",'a' + i);
		ata_createVFSEntry(drives + i,NULL,name);

		/* register driver for every partition */
		for(p = 0; p < PARTITION_COUNT; p++) {
			if(drives[i].partTable[p].present) {
				sprintf(name,"hd%c%d",'a' + i,p + 1);
				services[servCount] = regService(name,SERV_DRIVER);
				if(services[servCount] < 0) {
					debugf("Drive %d, Partition %d: Unable to register driver '%s'\n",
							i,p + 1,name);
				}
				else {
					/* we're a block-device, so always data available */
					setDataReadable(services[servCount],true);
					ata_createVFSEntry(drives + i,drives[i].partTable + p,name);
					driver[servCount].drive = i;
					driver[servCount].partition = p;
					servCount++;
				}
			}
		}
	}
}

static sDriver *getDriver(tServ sid) {
	u32 i;
	for(i = 0; i < servCount; i++) {
		if(services[i] == sid)
			return driver + i;
	}
	return NULL;
}

static void ata_createVFSEntry(sATADrive *drive,sPartition *part,char *name) {
	tFile *f;
	char path[21];
	sprintf(path,"/system/devices/%s",name);

	if(createNode(path) < 0) {
		printe("Unable to create '%s'",path);
		return;
	}

	f = fopen(path,"w");
	if(f == NULL) {
		printe("Unable to open '%s'",path);
		return;
	}

	if(part == NULL) {
		fprintf(f,"%-10s%d\n","Sectors:",drive->sectorCount);
		fprintf(f,"%-10s%d\n","LBA48:",drive->lba48);
	}
	else {
		fprintf(f,"%-10s%d\n","Start:",part->start);
		fprintf(f,"%-10s%d\n","Sectors:",part->size);
	}

	fclose(f);
}

static bool ata_readWrite(sATADrive *drive,bool opWrite,u16 *buffer,u64 lba,u16 secCount) {
	u8 status;
	u32 i,x;
	u16 *buf = buffer;
	u16 basePort = drive->basePort;

	if(!drive->lba48) {
		if(lba & 0xFFFFFFFFF0000000LL) {
			debugf("[ata] Trying to read from sector > 2^28-1\n");
			return false;
		}
		if(secCount & 0xFF00) {
			debugf("[ata] Trying to read %u sectors with LBA28\n",secCount);
			return false;
		}

		outByte(basePort + REG_DRIVE_SELECT,0xE0 | (drive->slaveBit << 4) | ((lba >> 24) & 0x0F));
	}
	else
		outByte(basePort + REG_DRIVE_SELECT,0x40 | (drive->slaveBit << 4));

	ata_wait(drive);

	/* reset control-register */
	gotInterrupt = false;
	outByte(basePort + REG_CONTROL,0);

	if(drive->lba48) {
		/* LBA: | LBA6 | LBA5 | LBA4 | LBA3 | LBA2 | LBA1 | */
		/*     48             32            16            0 */
		/* sector-count high-byte */
		outByte(basePort + REG_SECTOR_COUNT,(u8)(secCount >> 8));
		/* LBA4, LBA5 and LBA6 */
		outByte(basePort + REG_PART_DISK_SECADDR1,(u8)(lba >> 24));
		outByte(basePort + REG_PART_DISK_SECADDR2,(u8)(lba >> 32));
		outByte(basePort + REG_PART_DISK_SECADDR3,(u8)(lba >> 40));
		/* sector-count low-byte */
		outByte(basePort + REG_SECTOR_COUNT,(u8)(secCount & 0xFF));
		/* LBA1, LBA2 and LBA3 */
		outByte(basePort + REG_PART_DISK_SECADDR1,(u8)(lba & 0xFF));
		outByte(basePort + REG_PART_DISK_SECADDR2,(u8)(lba >> 8));
		outByte(basePort + REG_PART_DISK_SECADDR3,(u8)(lba >> 16));
		/* send command */
		if(opWrite)
			outByte(basePort + REG_COMMAND,COMMAND_WRITE_SEC_EXT);
		else
			outByte(basePort + REG_COMMAND,COMMAND_READ_SEC_EXT);
	}
	else {
		/* send sector-count */
		outByte(basePort + REG_SECTOR_COUNT,(u8)secCount);
		/* LBA1, LBA2 and LBA3 */
		outByte(basePort + REG_PART_DISK_SECADDR1,(u8)lba);
		outByte(basePort + REG_PART_DISK_SECADDR2,(u8)(lba >> 8));
		outByte(basePort + REG_PART_DISK_SECADDR3,(u8)(lba >> 16));
		/* send command */
		if(opWrite)
			outByte(basePort + REG_COMMAND,COMMAND_WRITE_SEC);
		else
			outByte(basePort + REG_COMMAND,COMMAND_READ_SEC);
	}

	for(i = 0; i < secCount; i++) {
		do {
			/* FIXME: vmware seems to need a ata_wait() here */
			/* wait until drive is ready */
			while(!gotInterrupt)
				wait(EV_NOEVENT);

			status = inByte(basePort + REG_STATUS);
			if((status & (CMD_ST_BUSY | CMD_ST_DRQ)) == CMD_ST_DRQ)
				break;
			if((status & CMD_ST_ERROR) != 0) {
				debugf("[ata] error: %x\n",inByte(basePort + REG_ERROR));
				return false;
			}
		}
		while(true);

		/* now read / write the data */
		if(opWrite) {
			for(x = 0; x < 256; x++)
				outWord(basePort + REG_DATA,*buf++);
		}
		else {
			for(x = 0; x < 256; x++)
				*buf++ = inWord(basePort + REG_DATA);
		}
	}

	return true;
}

static void ata_selectDrive(sATADrive *drive) {
	outByte(drive->basePort + REG_DRIVE_SELECT,0xA0 | (drive->slaveBit << 4));
	ata_wait(drive);
}

static bool ata_identifyDrive(sATADrive *drive) {
	u8 status;
	u16 data[256];
	u32 i;
	u16 basePort = drive->basePort;

	ata_selectDrive(drive);

	/* disable interrupts */
	outByte(basePort + REG_CONTROL,CTRL_INTRPTS_ENABLED);

	/* check wether the drive exists */
	outByte(basePort + REG_COMMAND,COMMAND_IDENTIFY);
	status = inByte(basePort + REG_STATUS);
	if(status == 0)
		return false;
	else {
		do {
			status = inByte(basePort + REG_STATUS);
			if((status & (CMD_ST_BUSY | CMD_ST_DRQ)) == CMD_ST_DRQ)
				break;
			if((status & CMD_ST_ERROR) != 0)
				return false;
		}
		while(true);

		/* drive ready */
		for(i = 0; i < 256; i++)
			data[i] = inWord(basePort + REG_DATA);

		/* check for LBA48 */
		if((data[83] & (1 << 10)) != 0) {
			drive->lba48 = 1;
			drive->sectorCount = ((u64)data[103] << 48) | ((u64)data[102] << 32) |
				((u64)data[101] << 16) | (u64)data[100];
		}
		/* check for LBA28 */
		else {
			drive->lba48 = 0;
			drive->sectorCount = ((u64)data[61] << 16) | (u64)data[60];
			/* LBA28 not supported? */
			if(drive->sectorCount == 0)
				return false;
		}

		return true;
	}
}
