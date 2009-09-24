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
#include <esc/proc.h>
#include <esc/ports.h>
#include "drive.h"
#include "ata.h"
#include "atapi.h"

static bool atapi_request(sATADrive *drive,u16 *cmd,u16 *buffer,u32 bufSize);

bool atapi_read(sATADrive *drive,bool opWrite,u16 *buffer,u64 lba,u16 secCount) {
	u8 cmd[] = {SCSI_CMD_READ_SECTORS,0,0,0,0,0,0,0,0,0,0,0};
	/* no writing here ;) */
	if(opWrite)
		return false;
	if(secCount == 0)
		return false;
    cmd[6] = (secCount >> 24) & 0xFF;
    cmd[7] = (secCount >> 16) & 0xFF;
    cmd[8] = (secCount >> 8) & 0xFF;
    cmd[9] = (secCount >> 0) & 0xFF;
    cmd[2] = (lba >> 24) & 0xFF;
    cmd[3] = (lba >> 16) & 0xFF;
    cmd[4] = (lba >> 8) & 0xFF;
    cmd[5] = (lba >> 0) & 0xFF;
	return atapi_request(drive,(u16*)cmd,buffer,secCount * drive->secSize);
}

u32 atapi_getCapacity(sATADrive *drive) {
	u8 resp[8];
	u8 cmd[] = {SCSI_CMD_READ_CAPACITY,0,0,0,0,0,0,0,0,0,0,0};
	bool res = atapi_request(drive,(u16*)cmd,(u16*)resp,8);
	if(!res)
		return 0;
	return (resp[0] << 24) | (resp[1] << 16) | (resp[2] << 8) | (resp[3] << 0);
}

static bool atapi_request(sATADrive *drive,u16 *cmd,u16 *buffer,u32 bufSize) {
	u8 status;
	u32 off,count;
	u16 size;
	u16 basePort = drive->basePort;

	ATA_PR2("Selecting device with port 0x%x",basePort);

	/* select drive */
	ata_unsetIntrpt();
	outByte(basePort + REG_DRIVE_SELECT,drive->slaveBit << 4);
	ata_wait(drive);

	/* reset control-register */
	outByte(basePort + REG_CONTROL,0);

	/* PIO mode */
	outByte(basePort + REG_FEATURES,0x0);
	/* in PIO-mode the drive has to know the size of the buffer */
	outByte(basePort + REG_ADDRESS2,bufSize & 0xFF);
	outByte(basePort + REG_ADDRESS3,bufSize >> 8);
	/* now tell the drive the command */
	outByte(basePort + REG_COMMAND,COMMAND_PACKET);

	ATA_PR2("Waiting while busy");
	/* wait while busy */
	while(inByte(basePort + REG_STATUS) & CMD_ST_BUSY)
		;
	ATA_PR2("Waiting until DRQ or ERROR set");
	/* wait until DRQ or ERROR set */
	while((!(status = inByte(basePort + REG_STATUS)) & CMD_ST_DRQ) && !(status & CMD_ST_ERROR))
		;
	if(status & CMD_ST_ERROR) {
		ATA_PR1("ERROR-bit set");
		return false;
	}

	ATA_PR2("Sending PACKET-command %d",((u8*)cmd)[0]);

	/* send words */
	outWordStr(basePort + REG_DATA,cmd,6);
	/*ata_waitIntrpt();*/
	/* TODO actually we should wait for an interrupt here, but unfortunatly real hardware (my
	 * notebook) doesn't send us any. I don't know why yet :/ */
	/* but it works if we're doing busy-waiting here... */
	ATA_PR2("Waiting while busy");
	while(inByte(basePort + REG_STATUS) & CMD_ST_BUSY)
		;
	ATA_PR2("Waiting until DRQ or ERROR set");
	while((!(status = inByte(basePort + REG_STATUS)) & CMD_ST_DRQ) && !(status & CMD_ST_ERROR))
		;

	ATA_PR2("Reading response-size");

	/* read actual size */
	ata_unsetIntrpt();
	size = ((u16)inByte(basePort + REG_ADDRESS3) << 8) | (u16)inByte(basePort + REG_ADDRESS2);
	count = 0;
	off = 0;
	ATA_PR2("Got %d; starting to read",size);
	while(count < bufSize) {
		u32 end = MIN(size,bufSize - off * sizeof(u16)) / sizeof(u16);
		/* read data */
		inWordStr(basePort + REG_DATA,buffer + off,end);
		off += end;
		count += end * sizeof(u16);
		if(count >= bufSize)
			break;
		ATA_PR2("Waiting until DRQ set");
		/* wait until DRQ set */
		while((inByte(basePort + REG_STATUS) & (CMD_ST_BUSY | CMD_ST_DRQ)) != CMD_ST_DRQ)
			;
	}

	/* TODO: according to http://wiki.osdev.org/ATAPI we should wait for an interrupt here.
	 * But somehow we don't get an interrupt. We get one if we read 256 bytes instead of 2048. I
	 * have no idea why. However, it seems to work everywhere without waiting... */
	/*ata_waitIntrpt();*/
	/* wait while busy or data-available */
	ATA_PR2("Waiting while busy or DRQ set");
	while(inByte(basePort + REG_STATUS) & (CMD_ST_BUSY | CMD_ST_DRQ))
		;
	ATA_PR2("Done");
	return true;
}
