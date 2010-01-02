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
#include <esc/ports.h>
#include <esc/debug.h>
#include <esc/signals.h>
#include <esc/proc.h>
#include "drive.h"
#include "ata.h"

static void ata_intrptHandler(tSig sig,u32 data);

static bool gotInterrupt = false;

void ata_init(void) {
	if(setSigHandler(SIG_INTRPT_ATA1,ata_intrptHandler) < 0 ||
			setSigHandler(SIG_INTRPT_ATA2,ata_intrptHandler) < 0) {
		error("Unable to announce sig-handler for %d or %d",SIG_INTRPT_ATA1,SIG_INTRPT_ATA2);
	}
}

void ata_unsetIntrpt(void) {
	gotInterrupt = false;
}

void ata_waitIntrpt(void) {
	while(!gotInterrupt)
		wait(EV_NOEVENT);
}

void ata_wait(sATADrive *drive) {
	inByte(drive->basePort + REG_STATUS);
	inByte(drive->basePort + REG_STATUS);
	inByte(drive->basePort + REG_STATUS);
	inByte(drive->basePort + REG_STATUS);
}

bool ata_readWrite(sATADrive *drive,bool opWrite,u16 *buffer,u64 lba,u16 secCount) {
	u8 status;
	u32 i;
	u16 *buf = buffer;
	u16 basePort = drive->basePort;

	if(secCount == 0)
		return false;

	if(!drive->info.features.lba48) {
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

	ATA_PR2("Selecting device with port 0x%x",basePort);
	ata_wait(drive);

	ATA_PR2("Resetting control-register");
	ata_unsetIntrpt();
	/* reset control-register */
	outByte(basePort + REG_CONTROL,0);

	if(drive->info.features.lba48) {
		ATA_PR2("LBA48: setting sector-count (%d) and address (%x)",secCount,(u32)(lba & 0xFFFFFFFF));
		/* LBA: | LBA6 | LBA5 | LBA4 | LBA3 | LBA2 | LBA1 | */
		/*     48             32            16            0 */
		/* sector-count high-byte */
		outByte(basePort + REG_SECTOR_COUNT,(u8)(secCount >> 8));
		/* LBA4, LBA5 and LBA6 */
		outByte(basePort + REG_ADDRESS1,(u8)(lba >> 24));
		outByte(basePort + REG_ADDRESS2,(u8)(lba >> 32));
		outByte(basePort + REG_ADDRESS3,(u8)(lba >> 40));
		/* sector-count low-byte */
		outByte(basePort + REG_SECTOR_COUNT,(u8)(secCount & 0xFF));
		/* LBA1, LBA2 and LBA3 */
		outByte(basePort + REG_ADDRESS1,(u8)(lba & 0xFF));
		outByte(basePort + REG_ADDRESS2,(u8)(lba >> 8));
		outByte(basePort + REG_ADDRESS3,(u8)(lba >> 16));
		ATA_PR2("LBA48: Sending command %d",opWrite ? COMMAND_WRITE_SEC_EXT : COMMAND_READ_SEC_EXT);
		/* send command */
		if(opWrite)
			outByte(basePort + REG_COMMAND,COMMAND_WRITE_SEC_EXT);
		else
			outByte(basePort + REG_COMMAND,COMMAND_READ_SEC_EXT);
	}
	else {
		ATA_PR2("LBA28: setting sector-count (%d) and address (%x)",secCount,(u32)(lba & 0xFFFFFFFF));
		/* send sector-count */
		outByte(basePort + REG_SECTOR_COUNT,(u8)secCount);
		/* LBA1, LBA2 and LBA3 */
		outByte(basePort + REG_ADDRESS1,(u8)lba);
		outByte(basePort + REG_ADDRESS2,(u8)(lba >> 8));
		outByte(basePort + REG_ADDRESS3,(u8)(lba >> 16));
		ATA_PR2("LBA28: Sending command %d",opWrite ? COMMAND_WRITE_SEC : COMMAND_READ_SEC);
		/* send command */
		if(opWrite)
			outByte(basePort + REG_COMMAND,COMMAND_WRITE_SEC);
		else
			outByte(basePort + REG_COMMAND,COMMAND_READ_SEC);
	}

	for(i = 0; i < secCount; i++) {
		do {
			if(opWrite) {
				ATA_PR2("Writing: wait until not busy");
				status = inByte(basePort + REG_STATUS);
				while(status & CMD_ST_BUSY) {
					sleep(20);
					status = inByte(basePort + REG_STATUS);
				}
			}
			else {
				ATA_PR2("Reading: wait for interrupt");
				/* FIXME: actually we should wait for an interrupt here. but this doesn't work
				 * in virtualbox. using polling here seems to work fine with all simulators and
				 * with my old notebook */
				/*ata_waitIntrpt();*/
				status = inByte(basePort + REG_STATUS);
			}

			if((status & (CMD_ST_BUSY | CMD_ST_DRQ)) == CMD_ST_DRQ)
				break;
			if((status & CMD_ST_ERROR) != 0) {
				debugf("[ata] error: %x\n",inByte(basePort + REG_ERROR));
				return false;
			}

#if 0
			if((status & (CMD_ST_BUSY | CMD_ST_DRQ)) == 0) {
				/* do a software-reset */
				outByte(basePort + REG_CONTROL,4);
				outByte(basePort + REG_CONTROL,0);
				ata_wait(drive);
				do {
					status = inByte(basePort + REG_STATUS);
				}
				while((status & (CMD_ST_BUSY | CMD_ST_READY)) != CMD_ST_READY);
			}
#endif
		}
		while(true);
		ATA_PR2("Ready, starting read/write");

		/* now read / write the data */
		if(opWrite) {
			outWordStr(basePort + REG_DATA,buf,ATA_SEC_SIZE / sizeof(u16));
			buf += ATA_SEC_SIZE / sizeof(u16);
		}
		else {
			inWordStr(basePort + REG_DATA,buf,ATA_SEC_SIZE / sizeof(u16));
			buf += ATA_SEC_SIZE / sizeof(u16);
		}
		ATA_PR2("Done");
	}

	return true;
}

static void ata_intrptHandler(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	gotInterrupt = true;
}
