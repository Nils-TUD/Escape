/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/service.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/ports.h>
#include <esc/heap.h>
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/signals.h>
#include "partition.h"

/* port-bases */
#define REG_BASE_PRIMARY			0x1F0
#define REG_BASE_SECONDARY			0x170

#define DRIVE_MASTER				0xA0
#define DRIVE_SLAVE					0xB0

#define COMMAND_IDENTIFY			0xEC
#define COMMAND_READ_SEC_EXT		0x24
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

static void ata_printDrives(void);
static void ata_wait(void);
static bool ata_isDrivePresent(u8 drive);
static void ata_detectDrives(void);
static bool ata_readWrite(sATADrive *drive,bool opWrite,u16 *buffer,u64 lba,u16 secCount);
static void ata_selectDrive(sATADrive *drive);
static bool ata_identifyDrive(sATADrive *drive);

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

static bool gotInterrupt = false;
static void diskIntrptHandler(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	gotInterrupt = true;
}

int main(void) {
	tServ id,client;

	/* request ports */
	if(requestIOPorts(REG_BASE_PRIMARY,8) < 0 || requestIOPorts(REG_BASE_SECONDARY,8) < 0) {
		printLastError();
		return 1;
	}

	if(setSigHandler(SIG_INTRPT_ATA1,diskIntrptHandler) < 0 ||
			setSigHandler(SIG_INTRPT_ATA2,diskIntrptHandler) < 0) {
		printLastError();
		return 1;
	}

	ata_detectDrives();
	/*ata_printDrives();*/

	/* reg service */
	id = regService("ata",SERVICE_TYPE_MULTIPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	sMsgHeader header;
	while(1) {
		tFD fd = getClient(&id,1,&client);
		if(fd < 0) {
			wait(EV_CLIENT);
		}
		else {
			while(read(fd,&header,sizeof(sMsgHeader)) > 0) {
				sMsgDataATAReq req;
				/* TODO: better error-handling */
				switch(header.id) {
					case MSG_ATA_READ_REQ: {
						read(fd,&req,sizeof(sMsgDataATAReq));
						if(ata_isDrivePresent(req.drive)) {
							sMsgHeader *res;
							u32 msgLen = sizeof(sMsgHeader) + BYTES_PER_SECTOR * req.secCount;
							res = (sMsgHeader*)malloc(msgLen);
							if(res != NULL) {
								/* TODO the client has to select the partition... */
								u32 partOffset = drives[req.drive].partTable[0].start;
								res->id = MSG_ATA_READ_RESP;
								if(!ata_readWrite(drives + req.drive,false,(u16*)(res + 1),
										req.lba + partOffset,req.secCount)) {
									/* write empty response */
									res->length = 0;
									write(fd,res,sizeof(sMsgHeader));
								}
								else {
									/* write response */
									res->length = BYTES_PER_SECTOR * req.secCount;
									write(fd,res,msgLen);
								}
								free(res);
							}
							else
								printLastError();
						}
					}
					break;

					case MSG_ATA_WRITE_REQ:
						read(fd,&req,header.length);
						if(ata_isDrivePresent(req.drive)) {
							if(!ata_readWrite(drives + req.drive,true,(u16*)(&req + 1),req.lba,req.secCount))
								debugf("Write failed\n");
							/* TODO we should respond something, right? */
						}
						break;
				}
			}

			close(fd);
		}
	}

	/* clean up */
	releaseIOPorts(REG_BASE_PRIMARY,8);
	releaseIOPorts(REG_BASE_SECONDARY,8);
	unregService(id);
	return 0;
}

static void ata_printDrives(void) {
	static const char *names[] = {"Primary Master","Primary Slave","Secondary Master","Secondary Slave"};
	u32 d,p;
	sPartition *part;
	debugf("Drives:\n");
	for(d = 0; d < DRIVE_COUNT; d++) {
		debugf("\t%s (present=%d,sectors=%d)\n",names[d],drives[d].present,drives[d].sectorCount);
		if(drives[d].present) {
			part = drives[d].partTable;
			for(p = 0; p < PARTITION_COUNT; p++) {
				if(part->present) {
					debugf("\t\t%d: start=0x%x, sectors=%d size=%d byte\n",p,part->start,
							part->size,part->size * 512);
				}
				part++;
			}
		}
	}
}

static void ata_wait(void) {
	/*gotInterrupt = false;
	while(!gotInterrupt)
		;*/
	volatile u32 i;
	for(i = 0; i < 100000; i++);
}

static bool ata_isDrivePresent(u8 drive) {
	return drive < DRIVE_COUNT && drives[drive].present;
}

static void ata_detectDrives(void) {
	u32 i;
	u16 buffer[256];
	for(i = 0; i < DRIVE_COUNT; i++) {
		if(ata_identifyDrive(drives + i)) {
			drives[i].present = 1;
			ata_readWrite(drives + i,false,buffer,0,1);
			part_fillPartitions(drives[i].partTable,buffer);
		}
	}
}

static bool ata_readWrite(sATADrive *drive,bool opWrite,u16 *buffer,u64 lba,u16 secCount) {
	u8 status;
	u32 i,x;
	u16 *buf = buffer;
	u16 basePort = drive->basePort;

	outByte(basePort + REG_DRIVE_SELECT,0x40 | (drive->slaveBit << 4));
	ata_wait();

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

	for(i = 0; i < secCount; i++) {
		/* wait until drive is ready */
		do {
			status = inByte(basePort + REG_STATUS);
			if((status & (CMD_ST_BUSY | CMD_ST_DRQ)) == CMD_ST_DRQ)
				break;
			if((status & CMD_ST_ERROR) != 0) {
				debugf("[ata] error\n");
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
	ata_wait();
}

static bool ata_identifyDrive(sATADrive *drive) {
	u8 status;
	u16 data[256];
	u32 i;
	u16 portBase = drive->basePort;

	ata_selectDrive(drive);

	/* disable interrupts */
	outByte(portBase + REG_CONTROL,CTRL_INTRPTS_ENABLED);

	/* check wether the drive exists */
	outByte(portBase + REG_COMMAND,COMMAND_IDENTIFY);
	status = inByte(portBase + REG_STATUS);
	if(status == 0)
		return false;
	else {
		do {
			status = inByte(portBase + REG_STATUS);
			if((status & (CMD_ST_BUSY | CMD_ST_DRQ)) == CMD_ST_DRQ)
				break;
			if((status & CMD_ST_ERROR) != 0)
				return false;
		}
		while(true);

		/* drive ready */
		for(i = 0; i < 256; i++)
			data[i] = inWord(portBase + REG_DATA);

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
